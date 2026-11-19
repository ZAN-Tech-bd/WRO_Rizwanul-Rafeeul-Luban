# Station board v0.4 PCB — OFFLINE floor plan (single source of truth for placement). From the pcb-pipeline template
# (attendance rev C engine: inventory parse, CCW rotation, ratsnest, JS generator) with this board's config + checks.
# Reads the footprint inventory (20_pcb_inventory.js output) and applies PLAN = {des: (x, y, rot)}.
# - checks: overlaps, off-board, hole clearance, antenna strip (no parts), METAL 15 mm from the antenna, native-USB lane
#   (low parts only), parts under the raised modules (low parts only), LM386 foam spots (free);
# - renders a PNG preview with pads, nets, holes, zones, module outlines and the ratsnest (MST per net) + its length;
# - writes 21_place_pcb.js (placement + expected pad positions for the in-EDA orientation check).
# Usage: python layout_plan.py [out.png] [--js] [--crop=x0,y0,x1,y1 --scale=1.0]
# Rotation = degrees counter-clockwise (EasyEDA's sense), Y up, mil, origin bottom-left. Spec: STATION-SPEC.md v0.4 §7.
# v0.5 (2026-10-25): C1, C2, D3, R1, TP1–TP6 removed (their spots stay empty), J15 (2nd obstacle port, D-025) added in the
# top-right corner; every other part keeps its v0.4 position. Inventory = the PCB read back after the v0.5 import.
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
INV = os.path.join(HERE, 'data', 'pcb_inventory_v04_import.txt')
NET_FIX = {}
FOOTPRINT_OVERRIDE = {}

W, H = 2000, 1400  # 50.8 x 35.6 mm (v0.4: only the IR module on the board, DHT11 / MQ-5 on JST cables)

# ---- the IR module stands UPRIGHT in its 1x3 socket U2 (user 2026-10-26): keep a clear zone around the socket for its
# body (≈ 14 mm along the pin row, components on one face) — no other part inside it.
IR_X, IR_Y = 1400, 1270
IR_ZONE = (IR_X - 300, IR_Y - 170, IR_X + 300, H)            # ≈ 15 x 8.5 mm, to the top edge
MODULES = [('IR module (upright)', IR_ZONE)]
MODULE_OWN = {'IR module (upright)': {'U2'}}
FOAM = []
EMPTY = (-10, -10, -9, -9)
ANTENNA = USB_LANE = METAL15 = EMPTY          # robot zones: not used on the station

# ---- M3 holes (plated, no net): bottom-left and top-right corners
M3 = (126, 236, 118, True)
HOLES = [('H1', 157, 157) + M3, ('H2', W - 157, H - 157) + M3]
ZONES = []
FLAT = set()
LOW = set()                                   # nothing may sit in the IR module's zone
PADS_ONLY = set()
REACH = set()
METAL = set()
SOCKETS = {'U2'}
POWER = {'5V', '3V3'}

# ---- placement: des -> (x, y, rot); XH ports and the IR socket rot 0
PLAN = {
    # bottom edge (cables): hub link J1, LED stick J2 (pin 1 = 5V on the right)
    'J1': (780, 125, 0), 'J2': (1545, 125, 0),
    # top edge (cables + the upright IR module): DHT11 J3, MQ-5 J4, IR socket U2
    'J3': (290, H - 125, 0), 'J4': (820, H - 145, 0), 'U2': (IR_X, IR_Y, 0),
    # MQ-5 AO divider between J4 and J1
    'R2': (450, 760, 0), 'R3': (450, 600, 0),
    # proximity alarm: D1 / R1 under the IR socket, Q1 + buzzer on the right
    'D1': (1080, 520, 0), 'R1': (1080, 700, 0), 'Q1': (1180, 880, 0), 'BZ1': (1650, 640, 0),
}
PLAN = {k: v for k, v in PLAN.items() if v}


# ---------------------------------------------------------------- inventory + transforms (template engine)
def rot(dx, dy, r):
    r %= 360
    if r == 0:
        return dx, dy
    if r == 90:
        return -dy, dx
    if r == 180:
        return -dx, -dy
    if r == 270:
        return dy, -dx
    raise ValueError(r)


def parse_inventory(path):
    parts = {}
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if not line:
            continue
        f = line.split('|')
        des = f[0]
        w, rest = f[5].split('x', 1)
        h, c = rest.split('(')
        cx, cy = c.rstrip(')').split(',')
        r0 = int(round(float(f[3]))) % 360
        w, h, cx, cy = float(w), float(h), float(cx), float(cy)
        cx, cy = rot(cx, cy, -r0 % 360)
        if r0 % 180:
            w, h = h, w
        pads = []
        for p in f[6].split(';'):
            if not p:
                continue
            num, rest = p.split(':', 1)
            net, xy = rest.rsplit('@', 1)
            dx, dy = (float(v) for v in xy.split(','))
            dx, dy = rot(dx, dy, -r0 % 360)
            net = NET_FIX.get((des, num), net)
            pads.append((num, '' if net == '-' else net, dx, dy))
        parts[des] = {'w': w, 'h': h, 'cx': cx, 'cy': cy, 'pads': pads, 'rot0': r0, 'fp': f[1]}
    return parts


def shapes(des, part, x, y, r):
    def rect(w, h, cx, cy):
        cx, cy = rot(cx, cy, r)
        if r % 180:
            w, h = h, w
        return (x + cx - w / 2, y + cy - h / 2, x + cx + w / 2, y + cy + h / 2)
    return [rect(part['w'], part['h'], part['cx'], part['cy'])]


def pads_abs(part, x, y, r):
    out = []
    for num, net, dx, dy in part['pads']:
        px, py = rot(dx, dy, r)
        out.append((num, net, x + px, y + py))
    return out


def overlap(a, b, m=0):
    return min(a[2], b[2]) - max(a[0], b[0]) > -m and min(a[3], b[3]) - max(a[1], b[1]) > -m


def rect_circle_dist(rc, cx, cy):
    dx = max(rc[0] - cx, 0, cx - rc[2])
    dy = max(rc[1] - cy, 0, cy - rc[3])
    return math.hypot(dx, dy)


# ---------------------------------------------------------------- checks (robot rules)
def check(parts):
    rep = {'missing': sorted(set(parts) - set(PLAN)), 'unknown': sorted(set(PLAN) - set(parts)),
           'overlaps': [], 'offBoard': [], 'nearHoles': [], 'antenna': [], 'usbLane': [], 'metal15': [],
           'underModule': [], 'foam': [], 'modulesOverlap': []}
    S = {d: shapes(d, parts[d], *PLAN[d]) for d in PLAN if d in parts}
    ds = sorted(S)
    for i in range(len(ds)):
        for j in range(i + 1, len(ds)):
            if any(overlap(a, b, 0) for a in S[ds[i]] for b in S[ds[j]]):
                rep['overlaps'].append(ds[i] + '~' + ds[j])
    over = {'J1': 15, 'J2': 15, 'J3': 15, 'J4': 15}   # XT fronts may reach the edge
    for d in ds:
        for rc in S[d]:
            o = over.get(d, 0)
            if rc[0] < -o or rc[1] < 0 or rc[2] > W + o or rc[3] > H:
                rep['offBoard'].append(d)
                break
        for name, hx, hy, drill, pad, keep, plated in HOLES:
            need = max(pad / 2 + 8, keep + 8)
            if any(rect_circle_dist(rc, hx, hy) < need for rc in S[d]):
                rep['nearHoles'].append(d + '@' + name)
        if d not in ('U1A', 'U1B') and any(overlap(rc, ANTENNA) for rc in S[d]):
            rep['antenna'].append(d)
        if d not in LOW | PADS_ONLY and any(overlap(rc, USB_LANE) for rc in S[d]):
            rep['usbLane'].append(d)
        if d in METAL and any(overlap(rc, METAL15) for rc in S[d]):
            rep['metal15'].append(d)
        for mname, m in MODULES:
            if d in MODULE_OWN[mname]:
                continue
            ok = FLAT if mname.startswith('LM386') else LOW   # LM386 modules: <= 3 mm (spec §7)
            if (d not in ok or d in REACH) and any(overlap(rc, m) for rc in S[d]):
                rep['underModule'].append(d + '@' + mname)
        if any(overlap(rc, f) for rc in S[d] for f in FOAM):
            rep['foam'].append(d)
    # the raised modules themselves: no overlap with each other, the antenna rule for the LM386 modules, on the board
    for i in range(len(MODULES)):
        for j in range(i + 1, len(MODULES)):
            if overlap(MODULES[i][1], MODULES[j][1]):
                rep['modulesOverlap'].append(MODULES[i][0] + '~' + MODULES[j][0])
        if MODULES[i][0].startswith('LM386') and overlap(MODULES[i][1], METAL15):
            rep['metal15'].append(MODULES[i][0])
        if MODULES[i][0].startswith(('LM386', 'DF')) and overlap(MODULES[i][1], USB_LANE):
            rep['usbLane'].append(MODULES[i][0])
    rep['overlaps'] = sorted(set(rep['overlaps']))
    return rep, S


def ratsnest(parts):
    nets = {}
    for d, (x, y, r) in PLAN.items():
        if d not in parts:
            continue
        for num, net, px, py in pads_abs(parts[d], x, y, r):
            if net:
                nets.setdefault(net, []).append((d + '.' + num, px, py))
    edges, total = {}, {}
    for net, pts in nets.items():
        if len(pts) < 2:
            continue
        # Prim MST
        inside = {0}
        e = []
        best = {i: (math.hypot(pts[i][1] - pts[0][1], pts[i][2] - pts[0][2]), 0) for i in range(1, len(pts))}
        while best:
            i = min(best, key=lambda k: best[k][0])
            dist, j = best.pop(i)
            e.append((pts[j], pts[i], dist))
            inside.add(i)
            for k in best:
                dk = math.hypot(pts[k][1] - pts[i][1], pts[k][2] - pts[i][2])
                if dk < best[k][0]:
                    best[k] = (dk, i)
        edges[net] = e
        total[net] = sum(x[2] for x in e)
    return edges, total


# ---------------------------------------------------------------- render
ZONE_COL = {'ANTENNA': ((255, 215, 215), (210, 60, 60)), 'USB LANE': ((215, 235, 255), (60, 120, 210)),
            'METAL 15 mm': (None, (230, 140, 140)), 'FOAM': ((235, 235, 200), (160, 160, 90))}


def render(parts, S, edges, out_png, rep, scale=0.30, crop=None):
    s = scale
    m = 40
    img = Image.new('RGB', (int(W * s) + 2 * m, int(H * s) + 2 * m), 'white')
    g = ImageDraw.Draw(img)
    try:
        font = ImageFont.truetype('arial.ttf', max(10, int(30 * s)))
        big = ImageFont.truetype('arial.ttf', 12)
    except OSError:
        font = big = ImageFont.load_default()
    X = lambda v: m + v * s
    Y = lambda v: m + (H - v) * s
    R = lambda rc: [X(rc[0]), Y(rc[3]), X(rc[2]), Y(rc[1])]
    g.rectangle([X(0), Y(H), X(W), Y(0)], outline='black', width=2)
    for zname, z in ZONES:
        fill, line = ZONE_COL.get(zname, (None, (120, 120, 120)))
        g.rectangle(R(z), fill=fill, outline=line)
        g.text((X(z[0]) + 3, Y(z[3]) + 3), zname, fill=line, font=font)
    for mname, mo in MODULES:
        g.rectangle(R(mo), outline=(60, 110, 220), width=2)
        g.text((X(mo[0]) + 4, Y(mo[1]) - 14), mname, fill=(60, 110, 220), font=font)
    for name, hx, hy, drill, pad, keep, plated in HOLES:
        g.ellipse([X(hx - pad / 2), Y(hy + pad / 2), X(hx + pad / 2), Y(hy - pad / 2)], outline='black', fill=(230, 230, 230))
        g.ellipse([X(hx - drill / 2), Y(hy + drill / 2), X(hx + drill / 2), Y(hy - drill / 2)], fill='black')
        g.text((X(hx) + 8, Y(hy) - 6), name, fill='black', font=font)
    bad = set()
    for k in ('overlaps', 'offBoard', 'nearHoles', 'antenna', 'usbLane', 'metal15', 'underModule', 'foam'):
        for item in rep[k]:
            for d in item.split('@')[0].split('~'):
                bad.add(d)
    for d, rcs in S.items():
        for rc in rcs:
            g.rectangle(R(rc), outline=(220, 0, 0) if d in bad else (120, 120, 120), width=2 if d in bad else 1)
        rc = rcs[0]
        g.text((X((rc[0] + rc[2]) / 2) - 3 * len(d), Y((rc[1] + rc[3]) / 2) - 5), d, fill=(0, 0, 0), font=font)
    for net, e in edges.items():
        col = (190, 190, 190) if net == 'GND' else ((230, 120, 0) if net in POWER else (0, 150, 220))
        for a, b, _ in e:
            g.line([X(a[1]), Y(a[2]), X(b[1]), Y(b[2])], fill=col, width=1)
    for d, (x, y, r) in PLAN.items():
        if d not in parts:
            continue
        for num, net, px, py in pads_abs(parts[d], x, y, r):
            col = (0, 0, 0) if net == 'GND' else ((230, 90, 0) if net in POWER else ((0, 110, 200) if net else (170, 170, 170)))
            g.rectangle([X(px) - 2, Y(py) - 2, X(px) + 2, Y(py) + 2], fill=col)
            if num == '1' and len(parts[d]['pads']) > 1:
                g.rectangle([X(px) - 3, Y(py) - 3, X(px) + 3, Y(py) + 3], outline=(255, 0, 255))
    g.text((m, 6), 'WRO station board v0.4 - %d x %d mil (%.1f x %.1f mm)  orange = power, blue = signal, grey = GND, '
           'magenta box = pad 1, blue outlines = raised modules' % (W, H, W * 0.0254, H * 0.0254), fill='black', font=big)
    if crop:
        x0, y0, x1, y1 = crop
        img = img.crop((int(X(x0)), int(Y(y1)), int(X(x1)), int(Y(y0))))
    img.save(out_png)


# ---------------------------------------------------------------- JS generator
def emit_js(parts, path):
    rows = []
    for d in sorted(PLAN):
        if d not in parts:
            continue
        x, y, r = PLAN[d]
        pa = pads_abs(parts[d], x, y, r)
        # two pads farthest apart pin down position + rotation
        if len(pa) >= 2:
            best = max(((a, b) for a in pa for b in pa), key=lambda t: math.hypot(t[0][2] - t[1][2], t[0][3] - t[1][3]))
            chk = [[best[0][0], round(best[0][2], 2), round(best[0][3], 2)], [best[1][0], round(best[1][2], 2), round(best[1][3], 2)]]
        else:
            chk = [[pa[0][0], round(pa[0][2], 2), round(pa[0][3], 2)]]
        rows.append("  ['%s', %s, %s, %d, %s]," % (d, round(x, 2), round(y, 2), r, str(chk).replace("'", '"')))
    holes = ',\n  '.join("['%s', %s, %s, %s, %s, %s]" % (n, round(hx, 1), round(hy, 1), dr, pd, 'true' if pl else 'false')
                          for n, hx, hy, dr, pd, _, pl in HOLES)
    js = open(os.path.join(HERE, '21_place_pcb.template.js'), encoding='utf-8').read()
    js = js.replace('/*W*/', str(W)).replace('/*H*/', str(H)).replace('/*PLAN*/', '\n'.join(rows)).replace('/*HOLES*/', holes)
    open(path, 'w', encoding='utf-8').write(js)


if __name__ == '__main__':
    parts = parse_inventory(INV)
    rep, S = check(parts)
    edges, total = ratsnest(parts)
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    out = args[0] if args else os.path.join(HERE, 'data', 'layout_preview.png')
    crop = next((tuple(float(v) for v in a[7:].split(',')) for a in sys.argv if a.startswith('--crop=')), None)
    scale = next((float(a[8:]) for a in sys.argv if a.startswith('--scale=')), 0.30)
    render(parts, S, edges, out, rep, scale, crop)
    problems = 0
    for k, v in rep.items():
        if k not in ('missing',):
            problems += len(v)
        print(k, len(v), v if len(v) < 60 else v[:60])
    big = sorted(total.items(), key=lambda t: -t[1])
    print('ratsnest total (no GND) %.0f mil; GND %.0f; top: %s' % (
        sum(v for n, v in total.items() if n != 'GND'), total.get('GND', 0),
        ', '.join('%s %.0f' % t for t in big[:12])))
    print('PROBLEMS (excluding missing):', problems)
    if '--js' in sys.argv:
        if rep['missing'] or problems:
            print('NOT writing 21_place_pcb.js: fix the problems / place every part first')
        else:
            emit_js(parts, os.path.join(HERE, '21_place_pcb.js'))
            print('wrote 21_place_pcb.js')
