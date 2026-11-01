# Exact post-pass on data/routes.json: narrow any track segment whose copper comes closer than TARGET to another
# net's copper (tracks, vias, pads), an NPTH/mounting hole or the board edge, or (non-escape nets) the antenna keep-out.
# Grid routing can lose ~3 mil on diagonals / at polygon pad edges; trimming fixes those spots without re-routing.
# Segments that would have to go below MIN_W, and vias, are reported for re-routing instead.
# Usage: python trim.py [--target=6.5]
import json
import math
import os
import sys
from collections import defaultdict

import layout_plan as LP
import route_check as RC
import route_model as M
import router as RT

TARGET = 6.5      # mil, EasyEDA/JLC rule is 6
MIN_W = 6.0       # mil, never narrower (JLC min 5)
EDGE_T, HOLE_T = 12.5, 8.5
CELL = 100.0


def run(routes, target=TARGET):
    pads, holes, _ = M.build()
    zone_nets = set(p.net for p in pads if p.net and LP.ANTENNA[0] <= p.cx <= LP.ANTENNA[2] and LP.ANTENNA[1] <= p.cy <= LP.ANTENNA[3])
    zx0, zy0, zx1, zy1 = LP.ANTENNA
    zrect = [(zx0, zy0), (zx1, zy0), (zx1, zy1), (zx0, zy1)]
    bx0, by0, bx1, by1 = M.module_body(pads)
    brect = [(bx0, by0), (bx1, by0), (bx1, by1), (bx0, by1)]
    # obstacles per layer: (net, kind, geom, halfwidth, ref)
    obs = []
    for net, r in routes.items():
        for si, (lay, x1, y1, x2, y2, w) in enumerate(r['segs']):
            obs.append((lay, net, 'seg', (x1, y1, x2, y2), w / 2, (net, si)))
        for (vx, vy, d1, d2) in r['vias']:
            for lay in (0, 1):
                obs.append((lay, net, 'via', (vx, vy, vx, vy), d2 / 2, None))
    for p in pads:
        for lay in p.layers:
            obs.append((lay, p.net or '-', 'pad', p.poly, 0.0, None))
    grid = defaultdict(list)
    for idx, o in enumerate(obs):
        g = o[3]
        if o[2] == 'pad':
            xs, ys = [q[0] for q in g], [q[1] for q in g]
            bx = (min(xs), min(ys), max(xs), max(ys))
        else:
            bx = (min(g[0], g[2]) - o[4], min(g[1], g[3]) - o[4], max(g[0], g[2]) + o[4], max(g[1], g[3]) + o[4])
        for gx in range(int(bx[0] // CELL), int(bx[2] // CELL) + 1):
            for gy in range(int(bx[1] // CELL), int(bx[3] // CELL) + 1):
                grid[(o[0], gx, gy)].append(idx)
    trimmed, failed = [], []
    for net, r in routes.items():
        newsegs = []
        for si, (lay, x1, y1, x2, y2, w) in enumerate(r['segs']):
            s = (x1, y1, x2, y2)
            # clearance from the centreline to every foreign thing nearby (minus its own half-width)
            need = []
            cand = set()
            for gx in range(int((min(x1, x2) - w) // CELL) - 1, int((max(x1, x2) + w) // CELL) + 2):
                for gy in range(int((min(y1, y2) - w) // CELL) - 1, int((max(y1, y2) + w) // CELL) + 2):
                    cand.update(grid.get((lay, gx, gy), ()))
            cl = 1e9
            for idx in cand:
                o = obs[idx]
                if o[1] == net:
                    continue
                if o[2] == 'pad':
                    d = RC.seg_poly_dist(s, o[3])
                else:
                    d = RC.seg_seg(s, o[3]) - o[4]
                cl = min(cl, d - target)
            for h in holes:
                rr = max(h['d'], h['ring']) / 2
                cl = min(cl, RC.seg_seg(s, (h['x'], h['y'], h['x'], h['y'])) - rr - HOLE_T)
            edge = min(min(x1, x2), min(y1, y2), M.W - max(x1, x2), M.H - max(y1, y2)) - EDGE_T
            cl = min(cl, edge)
            if LP.ANT_STRICT or (net not in zone_nets and RT.cls_of(net) != 'SIG'):
                cl = min(cl, RC.seg_poly_dist(s, zrect) - 3.0)
            if lay == 0 and net not in LP.BODY_NETS:
                cl = min(cl, RC.seg_poly_dist(s, brect) - 3.0)
            # cl = room for the half-width
            if w / 2 > cl:
                nw = math.floor(2 * cl * 10) / 10
                if nw >= MIN_W:
                    trimmed.append((net, si, round(w, 1), nw))
                    w = nw
                else:
                    failed.append((net, si, round(w, 1), round(2 * cl, 1), round((x1 + x2) / 2), round((y1 + y2) / 2), lay))
            newsegs.append((lay, x1, y1, x2, y2, w))
        r['segs'] = newsegs
    return routes, trimmed, failed


if __name__ == '__main__':
    tgt = next((float(a.split('=')[1]) for a in sys.argv if a.startswith('--target=')), TARGET)
    src = os.path.join(M.HERE, 'data', 'routes.json')
    routes = json.load(open(src, encoding='utf-8'))
    routes, trimmed, failed = run(routes, tgt)
    json.dump(routes, open(src, 'w', encoding='utf-8'))
    print('trimmed %d segments' % len(trimmed))
    for t in trimmed[:30]:
        print('  ', t)
    print('cannot trim (re-route needed): %d' % len(failed))
    for f in failed[:30]:
        print('  ', f)
