# Hub v0.4 PCB — offline floor plan (user 2026-10-26: "re-place the components, make the board shorter, too much empty
# space bottom-left"). Source of truth for the hub placement from now on (replaces the hand-written targets of the
# superseded 21_place_pcb.js). Same idea as robot/layout_plan.py, smaller:
#   - part sizes / pad offsets / nets from data/pcb_inventory_unplaced.txt (20b_pcb_inventory.js), at the rotation they
#     had there; a plan rotation is applied on top (EasyEDA: + = counter-clockwise, y up);
#   - checks: overlap (bbox + GAP), off-board, M3 holes, DevKit footprint + USB lane + SD module (LOW parts only under
#     them), antenna strip (no part within the DevKit's top edge band), DevKit rows 1000 mil apart;
#   - PNG preview with ratsnest (data/layout_<name>.png); --js writes 21b_place_pcb.js (placement + outline + holes).
# Usage: python layout_plan.py [old|new] [--js]
import json
import math
import os
import re
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, 'data')
GAP = 20          # mil between part bboxes
HOLE_R = 118 + 40  # M3 pad radius + clearance

# ---------------------------------------------------------------- parts (inventory at its own rotation)
INV = {}
for line in open(os.path.join(DATA, 'pcb_inventory_unplaced.txt'), encoding='utf8'):
    d, _fp, _xy, rot, _layer, box, pads = line.rstrip('\n').split('|')
    m = re.match(r'([\d.]+)x([\d.]+)\(([-\d.]+),([-\d.]+)\)', box)
    w, h, cx, cy = map(float, m.groups())
    P = {}
    for p in pads.split(';'):
        mm = re.match(r'([^:]+):(.*)@([-\d.]+),([-\d.]+)$', p)
        P[mm.group(1)] = (mm.group(2), float(mm.group(3)), float(mm.group(4)))
    INV[d] = dict(rot=round(float(rot)) % 360, w=w, h=h, cx=cx, cy=cy, pads=P)

# height classes: LOW may sit under the DevKit / SD module / in the USB lane (<= 5.5 mm)
def is_low(d):
    return bool(re.match(r'^(R\d+|C[1-9]|D1)$', d)) and d != 'C2'

METAL = {'J1', 'J2', 'J3', 'J4', 'J5', 'BZ1', 'C2'}

def rot_pt(x, y, a):
    a = math.radians(a)
    return x * math.cos(a) - y * math.sin(a), x * math.sin(a) + y * math.cos(a)

def geom(d, x, y, rot):
    """bbox (x0, y0, x1, y1) and pads {n: (net, px, py)} of part d placed at (x, y) with absolute rotation rot."""
    p = INV[d]
    da = (rot - p['rot']) % 360
    cx, cy = rot_pt(p['cx'], p['cy'], da)
    w, h = (p['w'], p['h']) if da in (0, 180) else (p['h'], p['w'])
    box = (x + cx - w / 2, y + cy - h / 2, x + cx + w / 2, y + cy + h / 2)
    pads = {n: (net, x + rot_pt(px, py, da)[0], y + rot_pt(px, py, da)[1]) for n, (net, px, py) in p['pads'].items()}
    return box, pads

# ---------------------------------------------------------------- layouts
def old_layout():
    """the superseded 21_place_pcb.js targets (for comparison)."""
    W, H = 5200, 3900
    L = {
        'J1': (330, 3150, 0), 'J2': (330, 2400, 0), 'J4': (330, 1650, 0), 'J5': (290, 900, 0), 'J3': (760, 2780, 270),
        'LED1': (450, 3650, 0), 'LED2': (720, 3650, 0), 'LED3': (750, 350, 0), 'R1': (1180, 3420, 90), 'R2': (1310, 3420, 90),
        'R3': (750, 160, 0), 'J21': (720, 2040, 90), 'J22': (720, 1610, 90), 'J23': (720, 1180, 90), 'J24': (720, 750, 90),
        'U1A': (1050, 2775, 270), 'U1B': (2050, 2775, 270),
        'J20': (2410, 3640, 0), 'J18': (2905, 3640, 0), 'R13': (2410, 3360, 0), 'R15': (2905, 3360, 0), 'R16': (2905, 3230, 0),
        'BZ1': (2400, 2800, 0), 'Q1': (2880, 2960, 0), 'R11': (2905, 2800, 0), 'R12': (2905, 2670, 0), 'D1': (2905, 2540, 0),
        'C2': (2905, 2250, 0), 'J25': (2460, 1920, 0), 'J26': (2880, 1920, 0), 'U6': (2650, 1720, 0), 'U2': (2650, 1300, 0),
        'C1': (2450, 1590, 0), 'R4': (2410, 960, 0), 'R5': (2890, 960, 0), 'R6': (2410, 830, 0), 'R7': (2890, 830, 0),
        'R8': (2410, 700, 0), 'R9': (2890, 700, 0), 'R10': (2410, 570, 0),
    }
    COLA, COLB, ROWS = 3690, 4666, [860, 1727, 2593, 3440]
    CELL = {1: (COLA, 0), 2: (COLA, 1), 3: (COLA, 2), 4: (COLA, 3), 5: (COLB, 3), 6: (COLB, 2), 7: (COLB, 1), 8: (COLB, 0)}
    for k in range(1, 9):
        cx, cy = CELL[k][0], ROWS[CELL[k][1]]
        L['J%d' % (9 + k)] = (cx, cy, 0)
        if k > 7:
            continue
        L['R%d' % (30 + k)] = (cx + 240, cy - 220, 0)
        L['C%d' % (2 + k)] = (cx - 340, cy - 350, 0)
        L['R%d' % (60 + k)] = (cx + 60, cy - 350, 0)
        if k % 2:
            n = (k + 1) // 2
            L['R%d' % (40 + n)] = (cx - 240, cy - 480, 0)
            L['R%d' % (50 + n)] = (cx + 240, cy - 480, 0)
            L['SB%d' % k] = (cx - 360, cy - 610, 0)
        else:
            L['SB%d' % k] = (cx - 360, cy - 480, 0)
    return W, H, L, dict(devkit=(1050, 2050), sd=(2650, 1720, 0))

def station_cells(L, COLA, COLB, ROWS):
    """2 x 4 identical station cells (as v0.3): S1..S4 up column A, S5..S8 down column B (= LED chain order)."""
    CELL = {1: (COLA, 0), 2: (COLA, 1), 3: (COLA, 2), 4: (COLA, 3), 5: (COLB, 3), 6: (COLB, 2), 7: (COLB, 1), 8: (COLB, 0)}
    for k in range(1, 9):
        cx, cy = CELL[k][0], ROWS[CELL[k][1]]
        L['J%d' % (9 + k)] = (cx, cy, 0)
        if k > 7:
            continue
        L['R%d' % (30 + k)] = (cx + 240, cy - 220, 0)      # DHT series 1 k
        L['C%d' % (2 + k)] = (cx - 340, cy - 350, 0)       # MQ filter 100 nF
        L['R%d' % (60 + k)] = (cx + 60, cy - 350, 0)       # LED hop 220 R
        if k % 2:
            n = (k + 1) // 2
            L['R%d' % (40 + n)] = (cx - 240, cy - 480, 0)  # zone pull-up
            L['R%d' % (50 + n)] = (cx + 240, cy - 480, 0)  # zone series
            L['SB%d' % k] = (cx + 360, cy - 610, 0)   # v0.4b: right side, clear of the next cell's label
        else:
            L['SB%d' % k] = (cx - 360, cy - 490, 0)

def new_layout():
    """2026-10-26: power column on the left edge (vertical XT, J3), DevKit, USB lane filled with LOW resistors, middle
    column (ports, buzzer, SD module over U2 + R4-R10), station block unchanged."""
    W, H = 4600, 3900
    L = {
        # left power column: battery, DC, switch harness, 12 V -> buck (cables plug in from above)
        'J1': (230, 2996, 90), 'J2': (230, 2306, 90), 'J3': (230, 1446, 270), 'J4': (230, 640, 90),
        # DevKit: rows 1000 mil apart, pin 1 75 mil below the top edge (module top flush, antenna over the edge)
        'U1A': (560, 2775, 270), 'U1B': (1560, 2775, 270),
        # USB lane (LOW only): LED resistors, buzzer driver, E-stop and I2C resistors
        'R15': (790, 1560, 0), 'R16': (790, 1430, 0), 'R1': (790, 1300, 0), 'R2': (790, 1170, 0), 'R3': (790, 1040, 0),
        'R13': (1330, 1560, 0), 'R11': (1330, 1430, 0), 'R12': (1330, 1300, 0), 'D1': (1330, 1170, 0),
        # middle column, top -> bottom
        'J20': (1880, 3740, 0), 'J18': (2370, 3740, 0),
        'Q1': (1800, 3430, 0), 'C2': (1860, 3140, 0), 'BZ1': (2390, 3300, 0),   # metal 15 mm from the antenna
        'J21': (1890, 2830, 0), 'J22': (2400, 2830, 0), 'J23': (1890, 2540, 0), 'J24': (2400, 2540, 0),
        'J25': (1830, 2230, 0), 'J26': (2180, 2230, 0), 'LED3': (2500, 2230, 0),
        'J5': (1905, 1900, 0), 'LED1': (2280, 1900, 0), 'LED2': (2530, 1900, 0),
        # SD header; the module lies flat over U2 + C1 + R4-R10 and its card end overhangs the bottom edge (~5 mm)
        'U6': (2150, 1560, 0), 'C1': (1830, 1380, 0), 'U2': (2150, 1030, 0),
        'R4': (1910, 640, 0), 'R5': (2390, 640, 0), 'R6': (1910, 510, 0), 'R7': (2390, 510, 0),
        'R8': (1910, 380, 0), 'R9': (2390, 380, 0), 'R10': (1910, 250, 0),
    }
    station_cells(L, 3130, 4090, [860, 1700, 2540, 3380])
    return W, H, L, dict(devkit=(560, 1560), sd=(2150, 1560, 0))

LAYOUTS = {'old': old_layout, 'new': new_layout}

# ---------------------------------------------------------------- zones derived from DevKit / SD positions
def zones(W, H, Z):
    xa, xb = Z['devkit']
    top = H  # DevKit PCB top edge flush with the board edge, antenna overhangs
    dk = (xa - 56, top - 2250, xb + 56, top)                                  # 27.94 mm x 57.15 mm module
    lane = (xa - 56, Z.get('lane_bottom', 0), xb + 56, top - 2250)            # USB-C plug path below the DevKit
    out = {'DEVKIT': dk, 'USB_LANE': lane}
    if 'sd' in Z:
        sx, sy, srot = Z['sd']
        # 46 x 24 mm module, header at its top end, body hanging down (-y) for rot 0
        out['SD_MODULE'] = (sx - 472, sy - 1811 + 60, sx + 472, sy + 60) if srot == 0 else None
    return out

def check(W, H, L, Z):
    probs = []
    boxes = {d: geom(d, *L[d])[0] for d in L}
    missing = sorted(set(INV) - set(L))
    if missing:
        probs.append('not placed: ' + ' '.join(missing))
    ds = sorted(boxes)
    for i, a in enumerate(ds):
        A = boxes[a]
        if A[0] < 0 or A[1] < 0 or A[2] > W or A[3] > H:
            if not a.startswith('U1'):
                probs.append('%s off-board %s' % (a, [round(v) for v in A]))
        for hx, hy in holes(W, H):
            nx, ny = min(max(hx, A[0]), A[2]), min(max(hy, A[1]), A[3])
            if math.hypot(nx - hx, ny - hy) < HOLE_R:
                probs.append('%s on hole (%d,%d)' % (a, hx, hy))
        for b in ds[i + 1:]:
            B = boxes[b]
            if min(A[2], B[2]) - max(A[0], B[0]) > -GAP and min(A[3], B[3]) - max(A[1], B[1]) > -GAP:
                probs.append('overlap %s / %s' % (a, b))
    zs = zones(W, H, Z)
    for zn, zr in zs.items():
        if not zr:
            continue
        for d, B in boxes.items():
            if d.startswith('U1') or (zn == 'SD_MODULE' and d == 'U6') or is_low(d) or (zn == 'SD_MODULE' and d == 'U2'):
                continue
            if min(B[2], zr[2]) - max(B[0], zr[0]) > 0 and min(B[3], zr[3]) - max(B[1], zr[1]) > 0:
                probs.append('%s (not LOW) inside %s' % (d, zn))
    # METAL15 (robot rule): XT, VH, buzzer, electrolytic >= 15 mm from the antenna (over the top edge, DevKit width)
    ant = (zs['DEVKIT'][0], H, zs['DEVKIT'][2], H + 246)
    for d in METAL:
        B = boxes[d]
        dx = max(ant[0] - B[2], B[0] - ant[2], 0); dy = max(ant[1] - B[3], B[1] - ant[3], 0)
        if math.hypot(dx, dy) < 590:
            probs.append('%s only %.1f mm from the antenna (METAL15)' % (d, math.hypot(dx, dy) * 0.0254))
    xa, xb = Z['devkit']
    if abs((L['U1B'][0] - L['U1A'][0]) - 1000) > 0.5 or L['U1A'][1] != L['U1B'][1]:
        probs.append('DevKit rows not 1000 mil apart / level')
    return probs

def holes(W, H):
    return [(170, 170), (W - 170, 170), (170, H - 170), (W - 170, H - 170)]

# ---------------------------------------------------------------- preview
def render(name, W, H, L, Z):
    S = 0.2
    M = 60
    img = Image.new('RGB', (int(W * S) + 2 * M, int(H * S) + 2 * M + 30), 'white')
    g = ImageDraw.Draw(img)
    X = lambda x: M + x * S
    Y = lambda y: M + (H - y) * S
    try:
        f = ImageFont.truetype('arial.ttf', 11)
    except OSError:
        f = ImageFont.load_default()
    g.rectangle([X(0), Y(H), X(W), Y(0)], outline='black', width=2)
    for zn, zr in zones(W, H, Z).items():
        if zr:
            col = {'DEVKIT': (220, 235, 255), 'USB_LANE': (255, 245, 200), 'SD_MODULE': (225, 255, 225)}[zn]
            g.rectangle([X(zr[0]), Y(zr[3]), X(zr[2]), Y(zr[1])], fill=col, outline=(150, 150, 150))
            g.text((X(zr[0]) + 3, Y(zr[3]) + 3), zn, fill=(90, 90, 90), font=f)
    for hx, hy in holes(W, H):
        g.ellipse([X(hx - 118), Y(hy + 118), X(hx + 118), Y(hy - 118)], outline='black')
    # ratsnest: nearest-neighbour chain per net (GND / +5V / +3V3 skipped: they become fills / spines)
    nets = {}
    for d in L:
        for n, (net, px, py) in geom(d, *L[d])[1].items():
            if net not in ('-', 'GND', '+5V', '+3V3'):
                nets.setdefault(net, []).append((px, py))
    for net, pts in nets.items():
        done, rest = [pts[0]], pts[1:]
        while rest:
            a, b = min(((a, b) for a in done for b in rest), key=lambda ab: math.dist(*ab))
            g.line([X(a[0]), Y(a[1]), X(b[0]), Y(b[1])], fill=(230, 120, 120), width=1)
            done.append(b); rest.remove(b)
    for d in L:
        B, pads = geom(d, *L[d])
        col = (60, 110, 200) if is_low(d) else (40, 40, 40)
        g.rectangle([X(B[0]), Y(B[3]), X(B[2]), Y(B[1])], outline=col, width=1)
        for n, (net, px, py) in pads.items():
            r = 2
            g.ellipse([X(px) - r, Y(py) - r, X(px) + r, Y(py) + r], fill=(200, 140, 0) if n == '1' else (120, 120, 120))
        g.text((X((B[0] + B[2]) / 2) - 8, Y((B[1] + B[3]) / 2) - 6), d, fill=col, font=f)
    g.text((M, int(H * S) + M + 8), '%s: %d x %d mil = %.1f x %.1f mm' % (name, W, H, W * 0.0254, H * 0.0254), fill='black', font=f)
    out = os.path.join(DATA, 'layout_%s.png' % name)
    img.save(out)
    return out

if __name__ == '__main__':
    name = next((a for a in sys.argv[1:] if not a.startswith('--')), 'new')
    W, H, L, Z = LAYOUTS[name]()
    probs = check(W, H, L, Z)
    print('%s: %d x %d mil (%.1f x %.1f mm), %d parts, %d problems' % (name, W, H, W * 0.0254, H * 0.0254, len(L), len(probs)))
    for p in probs:
        print('  ' + p)
    print(render(name, W, H, L, Z))
    if '--js' in sys.argv:
        if probs:
            sys.exit('not writing 21b_place_pcb.js: fix the problems first')
        plan = [[d, L[d][0], L[d][1], L[d][2], {n: [round(px, 2), round(py, 2)] for n, (_net, px, py) in geom(d, *L[d])[1].items()}]
                for d in sorted(L)]
        tpl = open(os.path.join(HERE, '21b_place_pcb.template.js'), encoding='utf8').read()
        js = tpl.replace('/*W*/', str(W)).replace('/*H*/', str(H)).replace('/*PLAN*/', json.dumps(plan, separators=(',', ':')))
        open(os.path.join(HERE, '21b_place_pcb.js'), 'w', encoding='utf8', newline='\n').write(js)
        print('wrote 21b_place_pcb.js (%d parts)' % len(plan))
