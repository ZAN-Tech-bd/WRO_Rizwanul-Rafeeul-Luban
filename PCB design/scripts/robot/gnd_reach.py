# For an isolated SMD GND pad: shortest top-layer GND stub (0.4 mm) from the pad to a via site whose BOTTOM layer is
# part of the main GND fill (pour_fill rules), on the copper that is actually in PCB1. Prints the stub + via.
# Usage: python gnd_reach.py data/<dump>.json PAD [--clr=6.5 --w=10]
import heapq
import json
import sys

import numpy as np
from scipy import ndimage
from shapely.geometry import Point
from shapely.ops import unary_union

import check_eda_copper as CE
import gnd_stage as GS
import pathfinder as PF
import pocket_stitch as PS
import route_model as M
import router as R

RES, NX, NY = M.RES, M.NX, M.NY


def main():
    dump, tag = sys.argv[1], sys.argv[2]
    routes, _ = CE.load(dump)
    pads, holes, _ = M.build()
    regs = PS.fill_regions(routes, pads, holes)
    nodes, comps, main_set = PS.network(regs, pads, routes)
    main_bot = unary_union([nodes[i][1] for i in main_set if nodes[i][0] == 1 and nodes[i][2] == 'fill'])
    b = PF.PFBoard()
    for net, r in routes.items():
        if net in b.nid:
            b.fix_copper(net, [tuple(s) for s in r['segs']], [tuple(v) for v in r['vias']])
    k = b.nid['GND']
    hard = b.hard_dist('GND')
    fd = []
    for l in (0, 1):
        tr = (b.cu[l] != 0) & (b.cu[l] != k) & (b.pad_cu[l] == 0)
        fd.append(np.minimum(hard[l], ndimage.distance_transform_edt(~tr) * RES - RES))
    r = GS.VIA_GND[1] / 2
    # via sites: legal like the fan-out, AND inside the main bottom fill shrunk by the via radius + 2
    inner = main_bot.buffer(-(r + 2))
    jj, ii = np.mgrid[0:NY, 0:NX]
    from shapely import contains_xy
    in_main = contains_xy(inner, ii * RES, jj * RES)
    # also accept: top-layer copper of the main GND network (its fill, GND pads, GND tracks) -> a stub straight to it
    main_top = unary_union([nodes[i][1] for i in main_set if nodes[i][0] in (0, 'both') and nodes[i][2] != 'via'])
    top_hit = contains_xy(main_top.buffer(-2), ii * RES, jj * RES)
    via_ok = ((fd[0] >= R.CLR + r) & (fd[1] >= R.CLR + r) & (b.edge_dist >= R.EDGE_CLR + r) & (b.smd_dist >= r + 4)
              & (b.tht_dist >= r + 2) & (b.hole_dist >= R.HOLE_CLR + r) & in_main)
    des, num = tag.split('.')
    p = next(q for q in b.pads if q.des == des and q.num == num)
    clr = next((float(x.split('=')[1]) for x in sys.argv if x.startswith('--clr=')), R.CLR)
    w = next((float(x.split('=')[1]) for x in sys.argv if x.startswith('--w=')), min(GS.STUB_W, p.minor))
    pas = (fd[0] >= clr + w / 2) & (b.edge_dist >= R.EDGE_CLR + w / 2) & (b.zone_dist >= w / 2 + 3)
    padm = np.zeros((NY, NX), bool)
    c = b.cells[id(p)]
    padm[c[:, 0], c[:, 1]] = True
    dist, prev, heap = {}, {}, []
    for j, i in b.core[id(p)]:
        dist[(j, i)] = 0.0
        heapq.heappush(heap, (0.0, int(j), int(i)))
    best = None
    while heap:
        d, j, i = heapq.heappop(heap)
        if d > dist.get((j, i), 1e18) or d > 200:
            continue
        if not padm[j, i] and (via_ok[j, i] or top_hit[j, i]):
            best = (j, i)
            hit_top = bool(top_hit[j, i])
            break
        for di, dj in R.DIRS:
            i2, j2 = i + di, j + dj
            if 0 <= i2 < NX and 0 <= j2 < NY and (pas[j2, i2] or padm[j2, i2]):
                nd = d + (1.4142 if di and dj else 1.0)
                if nd < dist.get((j2, i2), 1e18):
                    dist[(j2, i2)] = nd
                    prev[(j2, i2)] = (j, i)
                    heapq.heappush(heap, (nd, j2, i2))
    if not best:
        print('%s: no top-layer stub reaches a via site on the main bottom plane within 1000 mil' % tag)
        return
    path = [best]
    while path[-1] in prev:
        path.append(prev[path[-1]])
    pts = [(i_ * RES, j_ * RES) for j_, i_ in path[::-1]]
    # merge straight runs
    segs = []
    for a, bb in zip(pts, pts[1:]):
        if segs and abs((segs[-1][2] - segs[-1][0]) * (bb[1] - a[1]) - (segs[-1][3] - segs[-1][1]) * (bb[0] - a[0])) < 1e-6:
            segs[-1] = (segs[-1][0], segs[-1][1], bb[0], bb[1])
        else:
            segs.append((a[0], a[1], bb[0], bb[1]))
    L = sum(((s[2] - s[0]) ** 2 + (s[3] - s[1]) ** 2) ** 0.5 for s in segs)
    end = 'joins main top GND copper at (%.0f, %.0f)' % pts[-1] if hit_top else 'via at (%.0f, %.0f) 0.4/0.8 mm' % pts[-1]
    print('%s: top stub %.0f mil (%.1f mil wide), %d segments %s -> %s' % (
        tag, L, w, len(segs), [tuple(round(v) for v in s) for s in segs], end))
    out = {'segs': [[0, s[0], s[1], s[2], s[3], w] for s in segs], 'vias': [] if hit_top else [[pts[-1][0], pts[-1][1], GS.VIA_GND[0], GS.VIA_GND[1]]]}
    json.dump(out, open(dump.replace('.json', '_reach_%s.json' % tag.replace('.', '_')), 'w'))


if __name__ == '__main__':
    main()
