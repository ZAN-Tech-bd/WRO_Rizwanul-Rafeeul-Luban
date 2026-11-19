# Local re-route INSIDE a box, preserving everything else (rev D, D-044: J1 moved 50 mil toward the edge).
# Unlike partial_reroute.py (which keeps its box OUT and routes signal widths around it), this tool:
#   1. removes, for each NET, every track (both layers) and via that touches BOX, then prunes what is left dangling;
#   2. reconnects the net's islands (kept copper + pads at their current route_model positions) with the PathFinder A*
#      using the net's OWN class (router.cls_of: widths, neck/fine ladder near small pads, class via) — routes may run
#      inside the box;
#   all copper of every other net stays fixed and exact (it is an obstacle with the router's clearances).
# Extra fixed edits can be applied first with --shift (move given segments by dx,dy, e.g. stubs that belong to a
# moved part). Writes <dump>_lr.json (dump format) and prints what changed per net.
# --noprune keeps the cut ends instead of pruning dangling copper back to the next pad / via (that pruning can remove a
# long single-chain net far outside the box); the router then bridges the gap, and dangling stubs left over afterwards
# are pruned.
# --keepout=L:x0,y0,x1,y1[;...] adds areas the new copper must avoid on layer L (e.g. the D-039 charger corner, bottom
# layer GND only: 1:2400,880,2960,1360). check_eda_copper.py does not know those zones: always pass them here.
# Usage: python local_reroute.py data/<dump>.json NET[,NET] --box=x0,y0,x1,y1 [--vgap=10] [--clr=7] [--noprune]
#        [--keepout=L:x0,y0,x1,y1[;...]]
#        [--shift=NET:L:x1,y1,x2,y2:dx,dy[;...]]   (L = 0 top / 1 bottom, the segment to move, exact within 0.6 mil)
import json
import math
import sys

import numpy as np
from scipy import ndimage
from shapely.geometry import LineString, Point, Polygon, box

import check_eda_copper as CE
import pathfinder as PF
import route_model as M
import router as R
from partial_reroute import prune, touches

RES, NX, NY = M.RES, M.NX, M.NY


def main():
    dump, nets = sys.argv[1], sys.argv[2].split(',')
    x0, y0, x1, y1 = (float(v) for v in next(a.split('=')[1] for a in sys.argv if a.startswith('--box=')).split(','))
    vgap = next((float(a.split('=')[1]) for a in sys.argv if a.startswith('--vgap=')), 10.0)
    clr = next((float(a.split('=')[1]) for a in sys.argv if a.startswith('--clr=')), None)
    if clr:
        R.CLR = PF.CLR = clr
    shifts = next((a.split('=', 1)[1] for a in sys.argv if a.startswith('--shift=')), '')
    B = box(x0, y0, x1, y1)
    routes, _ = CE.load(dump)
    for spec in [s for s in shifts.split(';') if s]:
        net, L, seg, d = spec.split(':')
        L = int(L)
        sx1, sy1, sx2, sy2 = (float(v) for v in seg.split(','))
        dx, dy = (float(v) for v in d.split(','))
        hit = [s for s in routes[net]['segs'] if s[0] == L and (
            (abs(s[1] - sx1) < 0.6 and abs(s[2] - sy1) < 0.6 and abs(s[3] - sx2) < 0.6 and abs(s[4] - sy2) < 0.6) or
            (abs(s[1] - sx2) < 0.6 and abs(s[2] - sy2) < 0.6 and abs(s[3] - sx1) < 0.6 and abs(s[4] - sy1) < 0.6))]
        assert len(hit) == 1, ('shift: segment not found exactly once', spec, len(hit))
        s = hit[0]
        routes[net]['segs'].remove(s)
        routes[net]['segs'].append((s[0], s[1] + dx, s[2] + dy, s[3] + dx, s[4] + dy, s[5]))
        print('shifted %s L%d (%.1f,%.1f)-(%.1f,%.1f) by (%g, %g)' % (net, L, s[1], s[2], s[3], s[4], dx, dy))
    b = PF.PFBoard()
    for net, r in routes.items():
        if net in b.nid and net not in nets:
            b.fix_copper(net, [tuple(s) for s in r['segs']], [tuple(v) for v in r['vias']])
    for spec in [s for s in next((a.split('=', 1)[1] for a in sys.argv if a.startswith('--keepout=')), '').split(';') if s]:
        kl, kr = spec.split(':')
        kl = int(kl)
        kx0, ky0, kx1, ky1 = (float(v) for v in kr.split(','))
        i0, i1 = max(0, int(kx0 / RES) - PF.WIN), min(NX, int(kx1 / RES) + PF.WIN + 2)
        j0, j1 = max(0, int(ky0 / RES) - PF.WIN), min(NY, int(ky1 / RES) + PF.WIN + 2)
        X, Y = np.meshgrid(np.arange(i0, i1) * RES, np.arange(j0, j1) * RES)
        dk = np.hypot(np.maximum(np.maximum(kx0 - X, 0), X - kx1), np.maximum(np.maximum(ky0 - Y, 0), Y - ky1)).astype(np.float32)
        b.fields.append(((kl,), -1, (j0, j1, i0, i1), dk, False, False))   # same obstacle field as partial_reroute's box
        print('keep-out on layer %d: %s' % (kl, kr))
    kept = {}
    for net in nets:   # phase 1: cut every net out of the box (both layers) and prune; kept copper becomes fixed
        r = routes[net]
        segs0, vias0 = list(r['segs']), list(r['vias'])
        segs = [s for s in segs0 if not LineString([(s[1], s[2]), (s[3], s[4])]).buffer(s[5] / 2).intersects(B)]
        vias = [v for v in vias0 if not Point(v[0], v[1]).buffer(v[3] / 2).intersects(B)]
        if '--noprune' not in sys.argv:
            segs, vias = prune(segs, vias, [Polygon(p.poly) for p in b.pads_by_net.get(net, [])])
        kept[net] = (segs, vias, (len(segs0) - len(segs), len(vias0) - len(vias)))
        b.fix_copper(net, [tuple(x) for x in segs], [tuple(x) for x in vias])
    failed = []
    for net in nets:   # phase 2: reconnect each net's islands with its own class (routes may run inside the box)
        segs, vias, removed = kept[net]
        npads = b.pads_by_net.get(net, [])
        items = [('pad', p) for p in npads] + [('seg', s) for s in segs] + [('via', v) for v in vias]
        geo = []
        for kind, it in items:
            if kind == 'pad':
                geo.append((it.layers, Polygon(it.poly)))
            elif kind == 'seg':
                geo.append(((it[0],), LineString([(it[1], it[2]), (it[3], it[4])]).buffer(it[5] / 2)))
            else:
                geo.append(((0, 1), Point(it[0], it[1]).buffer(it[3] / 2)))
        par = list(range(len(items)))

        def f(i):
            while par[i] != i:
                par[i] = par[par[i]]
                i = par[i]
            return i
        for a in range(len(items)):
            for c in range(a + 1, len(items)):
                if set(geo[a][0]) & set(geo[c][0]) and touches(geo[a][1], geo[c][1]):
                    par[f(a)] = f(c)
        groups = {}
        for a in range(len(items)):
            groups.setdefault(f(a), []).append(a)
        groups = list(groups.values())
        cls = R.cls_of(net)
        hard = b.hard_dist(net)
        lv = R.Levels(b, net, R.width_specs(net, cls, npads), hard, zone_ok=False)
        _, _, via, via_cost, _ = R.CLASSES[cls]
        rv = via[1] / 2
        via_hard = ((hard[0] >= PF.CLR + rv) & (hard[1] >= PF.CLR + rv) & (b.edge_dist >= R.EDGE_CLR + rv)
                    & (b.hole_dist >= R.HOLE_CLR + rv) & (b.smd_dist >= rv + vgap) & (b.tht_dist >= rv + vgap)
                    & ~ndimage.binary_dilation(b.ant, iterations=int(rv / RES) + 2) & (b.body_dist >= rv + 3))
        via_hard &= b.boost_dist >= rv + 3
        new_segs, new_vias = [], []

        def cells(group):
            m = np.zeros((2, NY, NX), bool)
            for a in group:
                kind, it = items[a]
                if kind == 'pad':
                    c = b.core[id(it)]
                    for l in it.layers:
                        m[l][c[:, 0], c[:, 1]] = True
                elif kind == 'seg':
                    m[it[0]] |= R.seg_mask(it[1], it[2], it[3], it[4], max(it[5] / 2 - 2, 1))
                else:
                    dm = M.disc_mask(it[0], it[1], max(it[3] / 2 - 2, 1))
                    m[0] |= dm
                    m[1] |= dm
            return m
        while len(groups) > 1:
            src_m = cells(groups[0])
            tgt = np.zeros((2, NY, NX), bool)
            for g in groups[1:]:
                tgt |= cells(g)
            src = [(int(l), int(j), int(i)) for l, j, i in np.argwhere(src_m)]
            conf = [[np.zeros((NY, NX), bool) for _ in lv.w] for _l in (0, 1)]
            path = PF.astar_pf(b, src, tgt, lv.lev, conf, via_hard, np.zeros((NY, NX), bool), via_cost * 3, 1.0,
                               R.LAYER_COST, lv.cost)
            if not path:
                print('%s: could not reconnect %d islands' % (net, len(groups)))
                failed.append(net)
                break
            sg, vs = R.path_to_copper(path, lv)
            new_segs += sg
            new_vias += [(vx, vy, via[0], via[1]) for vx, vy in vs]
            end = path[-1]
            hit = next(gi for gi, g in enumerate(groups[1:], 1) if cells(g)[end[0], end[1], end[2]])
            groups[0] = groups[0] + groups[hit]
            del groups[hit]
            b.fix_copper(net, sg, [(vx, vy, via[0], via[1]) for vx, vy in vs])
        allsegs, allvias = segs + new_segs, vias + new_vias
        if '--noprune' in sys.argv:   # the router may join an island away from its cut end: drop what dangles now
            n0 = len(allsegs)
            allsegs, allvias = prune(allsegs, allvias, [Polygon(p.poly) for p in npads])
            if len(allsegs) != n0:
                print('%s: pruned %d dangling stubs after routing' % (net, n0 - len(allsegs)))
        routes[net] = {'segs': allsegs, 'vias': allvias}
        print('%-8s class %-7s removed %d tracks + %d vias in/at the box; added %d tracks + %d vias (widths %s)' % (
            net, cls, removed[0], removed[1], len(new_segs), len(new_vias), sorted(set(round(s[5], 1) for s in new_segs))))
    d = {n: {'segs': [[1 if s[0] == 0 else 2] + list(s[1:6]) + ['lr'] for s in r['segs']],
             'vias': [list(v[:4]) + ['lr'] for v in r['vias']]} for n, r in routes.items()}
    out = dump.replace('.json', '_lr.json')
    json.dump(d, open(out, 'w', encoding='utf-8'))
    print('->', out, '| failed:', failed or 'none')


if __name__ == '__main__':
    main()
