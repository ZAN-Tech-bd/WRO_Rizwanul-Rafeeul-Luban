# Robot carrier v0.5 PCB — OFFLINE floor plan (single source of truth for placement). From the pcb-pipeline template
# (attendance rev C engine: inventory parse, CCW rotation, ratsnest, JS generator) with this board's config + checks.
# Reads the footprint inventory (20_pcb_inventory.js output) and applies PLAN = {des: (x, y, rot)}.
# - checks: overlaps, off-board, hole clearance, antenna strip (no parts), METAL 15 mm from the antenna, native-USB lane
#   (low parts only), parts under the raised modules (low parts only), LM386 foam spots (free);
# - renders a PNG preview with pads, nets, holes, zones, module outlines and the ratsnest (MST per net) + its length;
# - writes 21_place_pcb.js (placement + expected pad positions for the in-EDA orientation check).
# Usage: python layout_plan.py [out.png] [--js] [--crop=x0,y0,x1,y1 --scale=1.0]
# Rotation = degrees counter-clockwise (EasyEDA's sense), Y up, mil, origin bottom-left. Spec: WIRING-PLAN.md v0.5 §7.
# v0.5 (2026-10-25): C1, C2, D3, R1, TP1–TP6 removed (their spots stay empty), J15 (2nd obstacle port, D-025) added in the
# top-right corner; every other part keeps its v0.4 position. Inventory = the PCB read back after the v0.5 import.
# v0.6 (2026-10-27): J1-J4 are vertical XT (D-030); the old column coordinates fit the smaller footprints with 0 problems,
# so J1 (rot 180) and J2 / J3 / J4 (rot 90) keep their v0.5 spots. Inventory = PCB1 read back on 2026-10-27.
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
INV = os.path.join(HERE, 'data', 'pcb_inventory_v06_now.txt')  # v0.6: vertical XT footprints (D-030) read back
NET_FIX = {}
FOOTPRINT_OVERRIDE = {}

W, H = 3740, 3840  # 95.0 x 97.5 mm (hard limit 100 x 100; 95 x 90 left no room for the ports, D-019)

# ---- YD-ESP32-S3 DevKit on U1A / U1B (rows 25.40 mm = 1000 mil), pin 1 at the top, DevKit top edge flush with the
# board's top edge so the antenna overhangs it (27.94 x 57.15 mm; pin 1 1.91 mm below its top edge).
DK_X0 = 420                       # DevKit left edge
U1A_X, U1B_X = DK_X0 + 50, DK_X0 + 1050
PIN1_Y = H - 75
DK_CY = PIN1_Y - 1050             # socket centre (22 pins)
DEVKIT = (DK_X0, H - 2250, DK_X0 + 1100, H)
ANT_X0, ANT_X1 = DK_X0 + 196, DK_X0 + 904          # WROOM-1 module (18 mm) centred on the DevKit, antenna above the edge
ANTENNA = (ANT_X0 - 200, H - 200, ANT_X1 + 200, H)   # under the antenna end + 5 mm: no parts / pours / vias / power
METAL15 = (ANT_X0 - 590, H - 590, ANT_X1 + 590, H)   # no XT, electrolytic, LM386 module, JP1, screw within 15 mm
# native USB-C port (console, spec §10) = the LEFT port (the right "COM" port sits next to the CH343P in the YD
# drawing): centre ~7.7 mm from the DevKit's left edge. The DevKit sits ~11 mm up on its sockets, so the plug passes
# ~10 mm above the board: the lane in front of the port (17 mm wide, to the bottom edge) takes only parts <= 5.5 mm.
USB_X = DK_X0 + 303
USB_LANE = (USB_X - 323, 0, USB_X + 323, H - 2250)

# ---- LM386 modules (LC Technology 40.9 x 13.9 mm), lying flat on U3 / U4 (rot 180: pin 1 VCC at +x), hanging down
# from the socket; screw terminal (OUT / GND) at the bottom edge. Socket row 55 mil below the module's top end.
AMP_W, AMP_L = 547, 1610
AMP_R_X, AMP_L_X = 1880, 2490     # U4 = DAC_R (left), U3 = DAC_L (right): no DAC crossing (D-007)
AMP_TOP = 40 + AMP_L              # module top end (far end 40 mil inside the bottom edge)
AMP_SOCK_Y = AMP_TOP - 55
def amp_rect(cx):
    return (cx - AMP_W / 2, 40, cx + AMP_W / 2, AMP_TOP)
FOAM = [(AMP_R_X - 180, 60, AMP_R_X + 180, 360), (AMP_L_X - 180, 60, AMP_L_X + 180, 360)]  # foam pad spots: keep free

# ---- DFPlayer Mini (20 x 20 mm) on U2A (bottom row, DF 1-8, pin 1 left) / U2B (top row, DF 16..9, pin 1 left),
# 1 mm right of the DevKit so its UART / BUSY pins face the S3 and the VAMP block gets the space on its right
DF_ROW = 708.66                   # 18.00 mm (LCSC footprint); caliper the module (spec §8.1)
DF_CX = DK_X0 + 1100 + 40 + 394
DF_Y0 = AMP_TOP + 190             # U2A row
DF = (DF_CX - 394, DF_Y0 - 39, DF_CX + 394, DF_Y0 + DF_ROW + 39)

MODULES = [('DevKit (8.5 mm up)', DEVKIT), ('DFPlayer', DF), ('LM386 R (DAC_R)', amp_rect(AMP_R_X)),
           ('LM386 L (DAC_L)', amp_rect(AMP_L_X))]
MODULE_OWN = {'DevKit (8.5 mm up)': {'U1A', 'U1B'}, 'DFPlayer': {'U2A', 'U2B'}, 'LM386 R (DAC_R)': {'U4'},
              'LM386 L (DAC_L)': {'U3'}}

# ---- mounting holes (M3, plated, 4 mm from the corners) + zip-tie anchors (2.5 mm NPTH pairs, 5 mm apart):
# name, x, y, drill, pad, keep-out radius for parts, plated
M3 = (126, 236, 118, True)
ZT = (98.4, 98.4, 60, False)
HOLES = [
    ('H1', 157, 157) + M3, ('H2', W - 157, 157) + M3, ('H3', 157, H - 157) + M3, ('H4', W - 157, H - 157) + M3,
]

ZONES = [('ANTENNA', ANTENNA), ('USB LANE', USB_LANE), ('METAL 15 mm', METAL15)] + [('FOAM', f) for f in FOAM]
# height classes (spec §7): FLAT <= 3 mm may sit under the LM386 modules; LOW <= 5.5 mm under the DevKit / DFPlayer
# (>= 9 mm clear) and in the USB lane. Test points, J14, JP1 and the switches must stay reachable (never under a module).
FLAT = {'R%d' % i for i in range(2, 28)} | {'D1', 'D2'}                  # 0.25 W resistors (2.3 mm), DO-41 (2.7 mm)
LOW = FLAT | {'C4', 'C6', 'C8', 'C10', 'C11', 'C12', 'C13', 'C14'}       # FG28 ceramics 5.5 mm
PADS_ONLY = {'J14'}                                                       # not fitted: allowed in the USB lane
REACH = PADS_ONLY | {'JP1', 'SW1', 'SW2'}
METAL = {'J1', 'J2', 'J3', 'J4', 'C3', 'C5', 'C7', 'C9', 'JP1'}  # + the LM386 modules (fixed position)
SOCKETS = {'U1A', 'U1B', 'U2A', 'U2B', 'U3', 'U4'}
POWER = {'BAT+', 'BAT_SW', '+5V', '+3V3', 'VAMP', 'VAMP_BAT', 'AMP_D'}
# ---- routing constants (router.py / gnd_stage.py / pour_fill.py / check_eda_copper.py, pcb-pipeline P5 step 9)
EP_PADS = set()              # no exposed pads: 100 % through-hole board (D-013)
ANT_STRICT = False           # DevKit on sockets (antenna ~11 mm up): thin signals may cross the strip (attendance D-028);
                             # no pour, vias or power tracks there (spec §7)
POUR_KEEPOUT = ANTENNA       # no GND fill under the antenna end + 5 mm
BODY_PART, BODY_NETS = None, ()        # no on-board module body rule (the DevKit is socketed)
BOOST_PARTS, BOOST_NETS = (), ()       # no switching regulator on the board (bucks are off-board)
NO_VIA_BODIES = {}           # no connector with a metal body (XT / XH / VH housings are plastic)

# power column (right edge, mating faces out), top -> bottom: battery, motor-buck feed, 5 V-buck feed, 5 V in, C1
COL_X_XT30 = W - 461.5            # J2 / J3 (XT30PW-F, rot 90): body x 3131..W
J1_Y = H - 595                    # XT60 body top 126 mil below H4's centre
J2_Y = J1_Y - 310 - 60 - 271
J3_Y = J2_Y - 271 - 60 - 271
J4_Y = J3_Y - 271 - 60 - 267

# ---- placement: des -> (x, y, rot) ----
PLAN = {
    # DevKit sockets (pin 1 at the antenna end = top)
    'U1A': (U1A_X, DK_CY, 270), 'U1B': (U1B_X, DK_CY, 270),
    # LM386 sockets + DFPlayer sockets
    'U4': (AMP_R_X, AMP_SOCK_Y, 180), 'U3': (AMP_L_X, AMP_SOCK_Y, 180),
    'U2A': (DF_CX, DF_Y0, 0), 'U2B': (DF_CX, DF_Y0 + DF_ROW, 0),

    # ---- left edge (cables leave left): RFID (pin 1 3V3 at the top), RFID supply caps, test points, EXP pads, L298N
    'J7': (150, H - 2590, 270), 'C7': (150, H - 2060, 90), 'C8': (150, H - 1810, 90), 'J14': (150, H - 1440, 90),
    'J6': (150, H - 735, 90),
    # RFID series resistors in the USB lane (low), IO pad up toward U1A.16..18
    'R5': (520, H - 2540, 270), 'R6': (640, H - 2540, 270), 'R4': (760, H - 2540, 270),

    # ---- under the DevKit (low): pull-downs and series resistors next to their U1A pin; VBAT divider at U1B.4
    'R2': (U1A_X + 310, PIN1_Y - 300, 0),      # ENA pull-down at U1A.4
    'R3': (U1A_X + 310, PIN1_Y - 800, 0),      # ENB pull-down at U1A.9
    'R10': (U1A_X + 310, PIN1_Y - 1000, 0),    # CAM TX at U1A.11
    'R7': (U1A_X + 310, PIN1_Y - 1900, 0),     # DF RX at U1A.20
    'R25': (U1B_X - 120, PIN1_Y - 503, 270), 'R24': (U1B_X - 240, PIN1_Y - 503, 90), 'C11': (U1B_X - 230, PIN1_Y - 205, 180),
    # button filter caps at U1B.18 / U1B.17
    'C13': (U1B_X - 120, PIN1_Y - 1815, 270), 'C14': (U1B_X - 120, PIN1_Y - 1485, 90),
    # ---- under the DFPlayer (low): BUSY / TX series resistors + its 100 nF
    'R9': (DF_CX - 344, DF_Y0 + 355, 270), 'C4': (DF_CX - 224, DF_Y0 + 210, 90), 'R8': (DF_CX - 84, DF_Y0 + 355, 90),

    # ---- bottom-left (right of the USB lane, below the +5V spine): MUTE / spare buttons + panel ports, Mega, C3
    'SW1': (1191, 220, 90), 'J12': (1475, 220, 90), 'SW2': (1191, 600, 90), 'J13': (1475, 600, 90),
    'J9': (1320, 920, 180), 'C3': (1220, 1255, 0),
    # under the LM386 R module (flat): button RC (row A), Mega UART (row B)
    'R21': (1660, 608, 90), 'R20': (1780, 608, 90), 'R23': (1900, 608, 90), 'R22': (2020, 608, 90),
    'R11': (1660, 1100, 270), 'R12': (1780, 1100, 90), 'R13': (1900, 1100, 270),

    # ---- right edge power column
    'J1': (W - 523, J1_Y, 180), 'J2': (COL_X_XT30, J2_Y, 90), 'J3': (COL_X_XT30, J3_Y, 90),
    'J4': (W - 326, J4_Y, 90),
    # ---- VAMP block: D1/D2 beside J3.1 (BAT_SW), JP1 (BAT | AMP | 5V), C9 1000 uF, C10; rocker port J5
    'D1': (3050, J3_Y - 300, 90), 'D2': (2840, J3_Y - 300, 270),
    'JP1': (2420, J3_Y + 310, 90), 'C9': (2790, J3_Y + 420, 0),
    'C10': (DF_CX + 336, J3_Y + 298, 90),   # under the DFPlayer's right edge (low), next to JP1.2
    'J5': (2850, J3_Y + 868, 180),
    # ---- bottom-right (between the LM386 L module and the column): CAM port + caps, indicator LEDs
    'J8': (2980, 170, 0), 'C5': (2960, 480, 0), 'C6': (2960, 720, 0),
    'LED2': (2900, 940, 0), 'LED1': (2900, 1210, 0),
    'R27': (2500, 940, 0), 'R26': (2500, 1210, 0),   # under the LM386 L module (flat)

    # ---- top edge right of the DevKit (>= 15 mm from the antenna): OBST + I2C ports, staircase from U1B.5 / U1B.8-9
    'J11': (2137, H - 105, 0), 'J10': (2700, H - 141, 0),
    'J15': (3225, H - 105, 0),                 # v0.5: 2nd obstacle port (D-025), free corner between J10 and H4, above J1
    'R19': (1790, PIN1_Y - 400, 180), 'C12': (1690, PIN1_Y - 600, 0), 'R18': (2310, PIN1_Y - 405, 90),
    'R16': (1790, PIN1_Y - 800, 180), 'R17': (2264, PIN1_Y - 700, 180),
    # v0.5: R15 / R14 55 mil lower than v0.4 so J10's pin names fit under its pads (the old legend's spot is J15's now)
    'R15': (2552, PIN1_Y - 470, 90), 'R14': (2680, PIN1_Y - 470, 90),
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
    g.text((m, 6), 'WRO robot carrier v0.4 - %d x %d mil (%.1f x %.1f mm)  orange = power, blue = signal, grey = GND, '
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
