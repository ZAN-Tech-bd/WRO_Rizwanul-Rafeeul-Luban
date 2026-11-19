# Exact geometric check of data/routes.json (independent of the routing grid):
# segment-segment / segment-pad / via-anything clearances between different nets, copper to board edge and holes,
# antenna-zone intrusions (every net when LP.ANT_STRICT), module-body intrusions (top layer, non-GND; vias of any
# non-GND net), and pad connectivity (every pad of a routed net touched by that net's copper).
# Usage: python route_check.py [routes.json] [--clr=6]
import json
import math
import os
import sys
from collections import defaultdict

import layout_plan as LP
import route_model as M
import router as RT

CELL = 100.0  # spatial hash bucket (mil)
HOLE2HOLE = 11.9  # mil, EasyEDA JLC 2-layer rule 0.3 mm


def seg_seg(a, b):
    """Minimum distance between segments a=(x1,y1,x2,y2) and b."""
    def pt_seg(px, py, x1, y1, x2, y2):
        dx, dy = x2 - x1, y2 - y1
        L2 = dx * dx + dy * dy
        t = 0 if L2 == 0 else max(0, min(1, ((px - x1) * dx + (py - y1) * dy) / L2))
        return math.hypot(px - x1 - t * dx, py - y1 - t * dy)

    def inter(p1, p2, p3, p4):
        def cr(o, a, b):
            return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
        d1, d2 = cr(p3, p4, p1), cr(p3, p4, p2)
        d3, d4 = cr(p1, p2, p3), cr(p1, p2, p4)
        return (d1 * d2 < 0) and (d3 * d4 < 0)
    p1, p2, p3, p4 = (a[0], a[1]), (a[2], a[3]), (b[0], b[1]), (b[2], b[3])
    if inter(p1, p2, p3, p4):
        return 0.0
    return min(pt_seg(*p1, *b), pt_seg(*p2, *b), pt_seg(*p3, *a), pt_seg(*p4, *a))


def poly_edges(poly):
    return [(poly[i][0], poly[i][1], poly[(i + 1) % len(poly)][0], poly[(i + 1) % len(poly)][1]) for i in range(len(poly))]


def point_in_poly(px, py, poly):
    inside = False
    n = len(poly)
    for i in range(n):
        x1, y1 = poly[i]
        x2, y2 = poly[(i + 1) % n]
        if (y1 > py) != (y2 > py) and px < (x2 - x1) * (py - y1) / (y2 - y1 + 1e-12) + x1:
            inside = not inside
    return inside


def seg_poly_dist(s, poly):
    if point_in_poly(s[0], s[1], poly) or point_in_poly(s[2], s[3], poly):
        return 0.0
    return min(seg_seg(s, e) for e in poly_edges(poly))


def check(routes, clr=6.0, edge=12.0, hole_clr=8.0):
    pads, holes, _ = M.build()
    items = []  # (layer, net, kind, geom, halfwidth)
    for net, r in routes.items():
        for (lay, x1, y1, x2, y2, w) in r['segs']:
            items.append((lay, net, 'seg', (x1, y1, x2, y2), w / 2))
        for (vx, vy, drill, dia) in r['vias']:
            for lay in (0, 1):
                items.append((lay, net, 'via', (vx, vy, vx, vy), dia / 2))
    for p in pads:
        for lay in p.layers:
            items.append((lay, p.net or '-', 'pad', p.poly, 0.0, p))
    grid = defaultdict(list)
    for idx, it in enumerate(items):
        g = it[3]
        if it[2] == 'pad':
            xs, ys = [q[0] for q in g], [q[1] for q in g]
            bx = (min(xs), min(ys), max(xs), max(ys))
        else:
            bx = (min(g[0], g[2]) - it[4], min(g[1], g[3]) - it[4], max(g[0], g[2]) + it[4], max(g[1], g[3]) + it[4])
        for gx in range(int(bx[0] // CELL), int(bx[2] // CELL) + 1):
            for gy in range(int(bx[1] // CELL), int(bx[3] // CELL) + 1):
                grid[(it[0], gx, gy)].append(idx)
    viol, seen = [], set()
    for key, idxs in grid.items():
        for a_i in range(len(idxs)):
            for b_i in range(a_i + 1, len(idxs)):
                a, b = items[idxs[a_i]], items[idxs[b_i]]
                if a[1] == b[1] or (a[2] == 'pad' and b[2] == 'pad'):
                    continue
                pair = (min(idxs[a_i], idxs[b_i]), max(idxs[a_i], idxs[b_i]))
                if pair in seen:
                    continue
                seen.add(pair)
                if a[2] == 'pad' or b[2] == 'pad':
                    pad, other = (a, b) if a[2] == 'pad' else (b, a)
                    d = seg_poly_dist(other[3], pad[3]) - other[4]
                else:
                    d = seg_seg(a[3], b[3]) - a[4] - b[4]
                if d < clr - 0.01:
                    viol.append((round(d, 1), a[0], a[1], a[2], b[1], b[2],
                                 round((a[3][0] + a[3][2]) / 2 if a[2] != 'pad' else a[3][0][0], 0),
                                 round((a[3][1] + a[3][3]) / 2 if a[2] != 'pad' else a[3][0][1], 0)))
    edge_v, hole_v, zone_v, boost_v, body_v = [], [], [], [], []
    brect = M.boost_rect(pads)
    bx0_, by0_, bx1_, by1_ = M.module_body(pads)
    body = [(bx0_, by0_), (bx1_, by0_), (bx1_, by1_), (bx0_, by1_)]
    zone_nets = set(p.net for p in pads if p.net and LP.ANTENNA[0] <= p.cx <= LP.ANTENNA[2] and LP.ANTENNA[1] <= p.cy <= LP.ANTENNA[3])
    zx0, zy0, zx1, zy1 = LP.ANTENNA
    for it in items:
        if it[2] == 'pad':
            continue
        x1, y1, x2, y2 = it[3]
        hw = it[4]
        m = min(x1, x2) - hw, min(y1, y2) - hw, M.W - max(x1, x2) - hw, M.H - max(y1, y2) - hw
        if min(m) < edge:
            edge_v.append((it[1], it[2], round(min(m), 1), round(x1), round(y1)))
        for h in holes:
            r = max(h['d'], h['ring']) / 2
            d = seg_seg(it[3], (h['x'], h['y'], h['x'], h['y'])) - hw - r
            if d < hole_clr:
                hole_v.append((it[1], it[2], h['name'], round(d, 1)))
        if LP.ANT_STRICT or (it[1] not in zone_nets and (RT.cls_of(it[1]) != 'SIG' or it[2] == 'via')):
            rect = [(zx0, zy0), (zx1, zy0), (zx1, zy1), (zx0, zy1)]
            if seg_poly_dist(it[3], rect) < hw:
                zone_v.append((it[1], it[2], it[0], round(x1), round(y1)))
        if it[1] not in LP.BODY_NETS and (it[0] == 0 or it[2] == 'via') and seg_poly_dist(it[3], body) < hw:
            body_v.append((it[1], it[2], it[0], round(x1), round(y1)))
        if it[1] not in LP.BOOST_NETS:
            bx0, by0, bx1, by1 = brect
            if seg_poly_dist(it[3], [(bx0, by0), (bx1, by0), (bx1, by1), (bx0, by1)]) < hw:
                boost_v.append((it[1], it[2], it[0], round(x1), round(y1)))
    # via-in-pad: any via overlapping an SMD pad (own net too: solder wicks into the via) -- U1's EP vias are intended
    via_in_pad = []
    for net, r in routes.items():
        for (vx, vy, drill, dia) in r['vias']:
            for p in pads:
                if p.tht or (p.des, p.num) in LP.EP_PADS:
                    continue
                if abs(p.cx - vx) > 200 or abs(p.cy - vy) > 200:
                    continue
                d = seg_poly_dist((vx, vy, vx, vy), p.poly) - dia / 2
                if d < 3.0:
                    via_in_pad.append((net, round(vx), round(vy), '%s.%s' % (p.des, p.num), round(d, 1)))
    # hole-to-hole (DRC "hole2Hole" 0.3 mm = 11.8 mil): every via drill vs every other drill (vias, THT pads, holes)
    # hub: a slotted pad hole (J3 rocker, 1.8 x 7.8 mm) is a stadium along the pad's long axis, not a 7.8 mm circle:
    # (x1, y1, x2, y2) centre line + slot radius
    def _drill(p):
        if not p.slot:
            return (p.cx, p.cy, p.cx, p.cy, p.hole / 2)
        k = (p.hole - p.slot) / 2
        ux, uy = (1.0, 0.0) if p.w >= p.h else (0.0, 1.0)
        return (p.cx - ux * k, p.cy - uy * k, p.cx + ux * k, p.cy + uy * k, p.slot / 2)
    drills = [(vx, vy, vx, vy, d1 / 2, 'via ' + net) for net, r in routes.items() for (vx, vy, d1, d2) in r['vias']]
    nvia = len(drills)
    drills += [_drill(p) + ('%s.%s' % (p.des, p.num),) for p in pads if p.tht and p.hole]
    drills += [(h['x'], h['y'], h['x'], h['y'], h['d'] / 2, h['name']) for h in holes]
    hole2hole = []
    for a_i in range(nvia):
        ax, ay, _, _, ar, an = drills[a_i]
        for b_i in range(len(drills)):
            if b_i == a_i or (b_i < nvia and b_i < a_i):
                continue
            bx1, by1, bx2, by2, br, bn = drills[b_i]
            if abs(ax - (bx1 + bx2) / 2) > 360 or abs(ay - (by1 + by2) / 2) > 360:
                continue
            g = seg_seg((ax, ay, ax, ay), (bx1, by1, bx2, by2)) - ar - br
            if g < HOLE2HOLE:
                hole2hole.append((an, bn, round(ax), round(ay), round(g, 1)))
    # connectivity: every pad of a routed net touched by its own copper
    unconnected = []
    by_net = defaultdict(list)
    for it in items:
        if it[2] != 'pad':
            by_net[(it[1], it[0])].append(it)
    for p in pads:
        if not p.net or p.net not in routes or p.net == 'GND':
            continue
        touched = False
        for lay in p.layers:
            for it in by_net.get((p.net, lay), []):
                if seg_poly_dist(it[3], p.poly) <= it[4] + 0.5:
                    touched = True
                    break
            if touched:
                break
        if not touched:
            unconnected.append('%s.%s(%s)' % (p.des, p.num, p.net))
    check.hole2hole = hole2hole
    check.boost = boost_v
    check.body = body_v
    # width review: on power nets, copper thinner than 0.35 mm is only allowed as a pad escape (near one of its pads)
    thin_power = []
    for net, r in routes.items():
        if RT.cls_of(net) not in ('PWR_HI', 'PWR_5V', 'PWR_3V3'):
            continue
        npads = [p for p in pads if p.net == net]
        for (lay, x1, y1, x2, y2, w) in r['segs']:
            if w >= 13.7:
                continue
            far = []
            for (px, py) in ((x1, y1), (x2, y2)):
                far.append(all(math.hypot(px - q.cx, py - q.cy) > max(q.w, q.h) / 2 + 25 for q in npads))
            if any(far):
                thin_power.append((net, round(w, 1), round(math.hypot(x2 - x1, y2 - y1)), round(x1), round(y1), lay))
    check.thin_power = thin_power
    return viol, edge_v, hole_v, zone_v, unconnected, via_in_pad


if __name__ == '__main__':
    path = next((a for a in sys.argv[1:] if not a.startswith('--')), os.path.join(M.HERE, 'data', 'routes.json'))
    clr = next((float(a.split('=')[1]) for a in sys.argv if a.startswith('--clr=')), 6.0)
    routes = json.load(open(path, encoding='utf-8'))
    viol, edge_v, hole_v, zone_v, unc, vip = check(routes, clr)
    print('clearance violations (<%.1f mil): %d' % (clr, len(viol)))
    for v in sorted(viol)[:40]:
        print('  ', v)
    print('edge:', len(edge_v), edge_v[:10])
    print('holes:', len(hole_v), hole_v[:10])
    print('antenna zone:', len(zone_v), zone_v[:10])
    print('unconnected pads:', len(unc), unc[:40])
    print('via in/at SMD pad:', len(vip), vip[:20])
    print('hole to hole < 0.3 mm:', len(check.hole2hole), check.hole2hole[:20])
    print('boost block intrusions (foreign nets):', len(check.boost), check.boost[:20])
    print('module body intrusions (top non-GND / non-GND vias):', len(check.body), check.body[:20])
    print('thin power copper (< 0.35 mm) away from pads:', len(check.thin_power), check.thin_power[:20])
