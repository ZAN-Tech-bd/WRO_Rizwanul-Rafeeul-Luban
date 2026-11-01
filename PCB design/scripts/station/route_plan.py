# Station board v0.4 PCB — HAND-PLANNED routing (offline). 11 THT parts / 12 nets is too small for the grid router
# (templates/pcb/routing/, tied to the attendance board), so each net is a list of waypoint paths here; 90° corners
# get 45° chamfers. Writes data/routes.json (the route_check.py / gen_route_js.py format) and data/route_preview.png.
# Rules (user 2026-10-26): bottom layer = GND fill, as few vias as possible, basic design. Every pad is through-hole,
# so a net that has to cross others runs WHOLLY on the bottom between two THT pads: 0 vias.
#   - bottom: 5V J1.1 -> J2.1 (along the bottom edge, under J2's pins) and MQ J1.5 -> R2.2 (the J1 pin order crosses);
#   - top: everything else; GND has no tracks (THT GND pads join the bottom fill through thermal reliefs).
# Widths: signals 12 mil (0.3 mm), 3V3 16 mil, 5V 24 mil (0.6 mm), 5V J1 -> J2 32 mil (0.8 mm: the LED stick's ≤ 0.5 A).
# Usage: python route_plan.py   then   python route_check.py --clr=10
import json
import math
import os

from PIL import Image, ImageDraw

import layout_plan as LP
import route_model as M

HERE = LP.HERE
TOP, BOT = 0, 1
SIG, V33, V5, V5M = 12.0, 16.0, 24.0, 32.0

# net -> [(layer, width, chamfer, [(x, y), ...]), ...]   (pad centres from route_model / data/pad_geometry.json)
PLAN = {
    # LED data: J1 left end -> J2, in lanes UNDER J1's pins (top), DIN then up and over J2.4 into J2.3
    'DOUT': [(TOP, SIG, 15, [(435.5, 125), (435.5, 22), (1397.4, 22), (1397.4, 125)])],
    'DIN': [(TOP, SIG, 15, [(533.9, 125), (533.9, 46), (1340, 46), (1340, 210), (1495.8, 210), (1495.8, 125)])],
    # PROX: the lane nearest J1's pins, up between J1 and J2 into D1's anode
    'PROX': [(TOP, SIG, 15, [(632.4, 125), (632.4, 70), (1251.3, 70), (1251.3, 520)])],
    # 5V: J1.1 -> J2.1 on the bottom along the edge; J2.1 -> right edge -> U2.3 / Q1.3 (top); U2.3 -> J4.1 along the top edge
    '5V': [(BOT, V5M, 25, [(1124.5, 125), (1124.5, 46), (1692.6, 46), (1692.6, 125)]),
           (TOP, V5, 40, [(1692.6, 125), (1692.6, 230), (1945, 230), (1945, 990), (1330, 990), (1280, 940), (1280, 880)]),
           (TOP, V5, 30, [(1500, 990), (1500, 1270)]),
           (TOP, V5, 30, [(1500, 1270), (1500, 1350), (967.6, 1350), (967.6, 1255)])],
    # DHT11 cable: up the left edge into J3. 3V3 = inner lane (enters J3.1 from below), DHT = outer lane (over J3's pins
    # along the top edge into J3.3)
    '3V3': [(TOP, V33, 30, [(927.6, 125), (927.6, 370), (100, 370), (100, 1180), (191.6, 1180), (191.6, 1275)])],
    'DHT': [(TOP, SIG, 30, [(829.2, 125), (829.2, 330), (60, 330), (60, 1350), (388.4, 1350), (388.4, 1275)])],
    # MQ-5 divider: J1.5 -> R2.2 on the bottom; R3.1 -> R2.2 between the resistor rows (top); MQ_AO J4.3 -> R2.1 (top)
    'MQ': [(BOT, SIG, 0, [(730.8, 125), (730.8, 682), (652.8, 760)]),
           (TOP, SIG, 0, [(247.2, 600), (327.2, 680), (572.8, 680), (652.8, 760)])],
    'MQ_AO': [(TOP, SIG, 30, [(770.8, 1255), (770.8, 1050), (247.2, 1050), (247.2, 760)])],
    # IR module OUT -> R1.1 -> D1.1 (cathode); base resistor -> Q1.B; Q1.C -> buzzer + (under R1, between D1 and R1)
    'IR_OUT': [(TOP, SIG, 30, [(1300, 1270), (1300, 1150), (877.2, 1150), (877.2, 700)]),
               (TOP, SIG, 0, [(877.2, 700), (877.2, 551.5), (908.7, 520)])],
    'Q_BASE': [(TOP, SIG, 0, [(1282.8, 700), (1180, 802.8), (1180, 880)])],
    'BUZ_P': [(TOP, SIG, 25, [(1080, 880), (1080, 615), (1475, 615), (1500.4, 640)])],
}


def chamfer(pts, c):
    """45° chamfer of every right-angle corner: cut c mil (at most half of either leg) off both legs."""
    if c <= 0 or len(pts) < 3:
        return list(pts)
    out = [pts[0]]
    for i in range(1, len(pts) - 1):
        (ax, ay), (bx, by), (cx, cy) = out[-1], pts[i], pts[i + 1]
        u = (bx - ax, by - ay)
        v = (cx - bx, cy - by)
        lu, lv = math.hypot(*u), math.hypot(*v)
        if lu < 1e-6 or lv < 1e-6 or abs(u[0] * v[0] + u[1] * v[1]) > 1e-6 * lu * lv:
            out.append((bx, by))
            continue
        k = min(c, lu / 2, lv / 2)
        out.append((bx - u[0] / lu * k, by - u[1] / lu * k))
        out.append((bx + v[0] / lv * k, by + v[1] / lv * k))
    out.append(pts[-1])
    return out


def build():
    routes = {}
    for net, paths in PLAN.items():
        segs = []
        for layer, w, c, pts in paths:
            q = chamfer(pts, c)
            for (x1, y1), (x2, y2) in zip(q, q[1:]):
                segs.append([layer, round(x1, 2), round(y1, 2), round(x2, 2), round(y2, 2), w])
        routes[net] = {'segs': segs, 'vias': []}
    return routes


def render(routes, path, s=0.45):
    pads, holes, _ = M.build()
    m = 20
    img = Image.new('RGB', (int(LP.W * s) + 2 * m, int(LP.H * s) + 2 * m), (16, 16, 16))
    g = ImageDraw.Draw(img, 'RGBA')
    X = lambda v: m + v * s
    Y = lambda v: m + (LP.H - v) * s
    g.rectangle([X(0), Y(LP.H), X(LP.W), Y(0)], outline=(220, 220, 220))
    for h in holes:
        r = max(h['d'], h['ring']) / 2 * s
        g.ellipse([X(h['x']) - r, Y(h['y']) - r, X(h['x']) + r, Y(h['y']) + r], outline=(200, 200, 200))
    for layer, col in ((BOT, (60, 120, 240, 200)), (TOP, (220, 60, 50, 220))):
        for net, r in routes.items():
            for (L, x1, y1, x2, y2, w) in r['segs']:
                if L == layer:
                    g.line([X(x1), Y(y1), X(x2), Y(y2)], fill=col, width=max(1, int(w * s)))
    for p in pads:
        g.polygon([(X(x), Y(y)) for x, y in p.poly], fill=(150, 150, 150) if p.net == 'GND' else (230, 190, 60))
        g.text((X(p.cx) - 6, Y(p.cy) - 18), '%s.%s' % (p.des, p.num), fill=(200, 200, 200))
    img.save(path)
    print('preview', path)


if __name__ == '__main__':
    routes = build()
    out = os.path.join(HERE, 'data', 'routes.json')
    json.dump(routes, open(out, 'w'), indent=0)
    n = sum(len(r['segs']) for r in routes.values())
    bot = sum(math.hypot(s[3] - s[1], s[4] - s[2]) for r in routes.values() for s in r['segs'] if s[0] == BOT)
    print('wrote %s: %d nets, %d segments, 0 vias; bottom-layer track length %.0f mil' % (out, len(routes), n, bot))
    render(routes, os.path.join(HERE, 'data', 'route_preview.png'))
