# Robot carrier v0.6 PCB (pcb-pipeline template; attendance rev C engine) — negotiated-congestion router (PathFinder) on top of router.py.
# Hard obstacles: pads of other nets, holes, board edge, antenna zone (every net when LP.ANT_STRICT), the module body
# (top layer, non-GND nets). Soft obstacles: other nets'
# traces/vias — allowed at a growing price (present factor x (1 + history)). Each pass re-routes the nets still in
# conflict until no two nets' copper comes closer than the clearance.
# Usage: python pathfinder.py [--iters=15]   -> data/routes.json, data/route_preview.png, data/pf_log.txt
import heapq
import json
import math
import os
import sys
import time

import numpy as np
from scipy import ndimage

import layout_plan as LP
import route_model as M
import router as R

RES, NX, NY = M.RES, M.NX, M.NY
CLR, NECK_CLR, EDGE_CLR = R.CLR, R.NECK_CLR, R.EDGE_CLR
DISC1 = ndimage.generate_binary_structure(2, 1)
WIN = 24          # cells (120 mil) around each pad for its exact distance field (> widest clearance need ~50 mil)
BIG = 1.0e4


def _pt_seg(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    L2 = dx * dx + dy * dy
    t = np.clip(((px - x1) * dx + (py - y1) * dy) / L2, 0, 1) if L2 else 0
    return np.hypot(px - x1 - t * dx, py - y1 - t * dy)


class PFBoard(R.Board):
    def __init__(self):
        super().__init__()
        self.pad_cu = self.cu.copy()                    # pads + obstacles only (fixed)
        self.tr = {}                                    # net -> (2, NY, NX) bool trace/via copper
        self.hist = np.zeros((2, NY, NX), np.float32)
        # pad cores: routes start/end only >= 3.5 mil inside a pad's edge, so the final step into a pad is a width-
        # checked approach, not a diagonal into its corner (run 4: VBUS at J1.B8 4.9 mil, BST_SW at U3.6 5.8 mil)
        self.core = {}
        for p in self.pads:
            c = self.cells[id(p)]
            x, y = c[:, 1] * RES, c[:, 0] * RES
            poly = p.poly
            d = np.full(len(c), np.inf)
            for k in range(len(poly)):
                (x1, y1), (x2, y2) = poly[k], poly[(k + 1) % len(poly)]
                d = np.minimum(d, _pt_seg(x, y, x1, y1, x2, y2))
            keep = d >= min(3.5, p.minor / 2 - 0.5)
            if not keep.any():
                keep = d >= d.max() - 1e-6
            if LP.ANT_STRICT:  # no route starts at the antenna keep-out (U5.1 / U5.38 reach 3 mil into it)
                ok = keep & (self.zone_dist[c[:, 0], c[:, 1]] >= 12.0)
                if ok.any():
                    keep = ok
            self.core[id(p)] = c[keep]
        # exact distance fields: distance from each cell centre to each pad polygon (0 inside), in a window around the
        # pad; and to each plated mounting-hole ring. Rasterised EDT distances were up to ~5 mil optimistic at pad
        # corners (run 4/5: VBUS passed J1.B8's corner at 4.8 mil while the grid said 11.6).
        self.fields = []   # (layers, net id or -1, (j0, j1, i0, i1), dist window, is_smd)
        for p in self.pads:
            xs, ys = [q[0] for q in p.poly], [q[1] for q in p.poly]
            i0, i1 = max(0, int(min(xs) / RES) - WIN), min(NX, int(max(xs) / RES) + WIN + 2)
            j0, j1 = max(0, int(min(ys) / RES) - WIN), min(NY, int(max(ys) / RES) + WIN + 2)
            X, Y = np.meshgrid(np.arange(i0, i1) * RES, np.arange(j0, j1) * RES)
            d = np.full(X.shape, np.inf)
            poly = p.poly
            for k in range(len(poly)):
                (x1, y1), (x2, y2) = poly[k], poly[(k + 1) % len(poly)]
                d = np.minimum(d, _pt_seg(X, Y, x1, y1, x2, y2))
            c = self.cells[id(p)]
            d[c[:, 0] - j0, c[:, 1] - i0] = 0.0
            self.fields.append((p.layers, self.nid[p.net] if p.net else -1, (j0, j1, i0, i1), d.astype(np.float32),
                                (not p.tht) and (p.des, p.num) not in LP.EP_PADS, p.tht))
        for h in self.holes:
            if h['ring']:
                r = h['ring'] / 2
                i0, i1 = max(0, int((h['x'] - r) / RES) - WIN), min(NX, int((h['x'] + r) / RES) + WIN + 2)
                j0, j1 = max(0, int((h['y'] - r) / RES) - WIN), min(NY, int((h['y'] + r) / RES) + WIN + 2)
                X, Y = np.meshgrid(np.arange(i0, i1) * RES, np.arange(j0, j1) * RES)
                d = np.maximum(np.hypot(X - h['x'], Y - h['y']) - r, 0).astype(np.float32)
                self.fields.append(((0, 1), -1, (j0, j1, i0, i1), d, False, False))
        # exact via keep-off maps (any net): distance to the nearest SMD pad (exposed pads excluded) / THT pad
        for attr, idx in (('smd_dist', 4), ('tht_dist', 5)):
            m = np.full((NY, NX), BIG, np.float32)
            for f in self.fields:
                if f[idx]:
                    j0, j1, i0, i1 = f[2]
                    np.minimum(m[j0:j1, i0:i1], f[3], out=m[j0:j1, i0:i1])
            setattr(self, attr, m)

    def fix_copper(self, net, segs, vias):
        """Fixed copper of `net` (the GND fanout): an obstacle for every other net, like a pad (exact field)."""
        k = self.nid[net]
        for (lay, x1, y1, x2, y2, w) in segs:
            hw = w / 2
            i0, i1 = max(0, int((min(x1, x2) - hw) / RES) - WIN), min(NX, int((max(x1, x2) + hw) / RES) + WIN + 2)
            j0, j1 = max(0, int((min(y1, y2) - hw) / RES) - WIN), min(NY, int((max(y1, y2) + hw) / RES) + WIN + 2)
            X, Y = np.meshgrid(np.arange(i0, i1) * RES, np.arange(j0, j1) * RES)
            d = np.maximum(_pt_seg(X, Y, x1, y1, x2, y2) - hw, 0).astype(np.float32)
            self.fields.append(((lay,), k, (j0, j1, i0, i1), d, False, False))
            m = R.seg_mask(x1, y1, x2, y2, hw)
            for arr in (self.cu, self.pad_cu):
                arr[lay][m & (arr[lay] == 0)] = k
        for (vx, vy, d1, d2) in vias:
            r = d2 / 2
            i0, i1 = max(0, int((vx - r) / RES) - WIN), min(NX, int((vx + r) / RES) + WIN + 2)
            j0, j1 = max(0, int((vy - r) / RES) - WIN), min(NY, int((vy + r) / RES) + WIN + 2)
            X, Y = np.meshgrid(np.arange(i0, i1) * RES, np.arange(j0, j1) * RES)
            d = np.maximum(np.hypot(X - vx, Y - vy) - r, 0).astype(np.float32)
            self.fields.append(((0, 1), k, (j0, j1, i0, i1), d, False, False))
            m = M.disc_mask(vx, vy, r)
            for arr in (self.cu, self.pad_cu):
                for l in (0, 1):
                    arr[l][m & (arr[l] == 0)] = k

    def hard_dist(self, net):
        """Exact distance (mil) from each cell centre to the nearest pad of another net / no-net pad / hole ring."""
        k = self.nid[net]
        out = []
        for l in (0, 1):
            d = np.full((NY, NX), BIG, np.float32)
            for (lays, nid, (j0, j1, i0, i1), loc, _, _) in self.fields:
                if l in lays and nid != k:
                    np.minimum(d[j0:j1, i0:i1], loc, out=d[j0:j1, i0:i1])
            out.append(d)
        return out

    def soft_dist(self, net):
        other = np.zeros((2, NY, NX), bool)
        for n, m in self.tr.items():
            if n != net:
                other |= m
        return [ndimage.distance_transform_edt(~other[l]) * RES - RES * 0.5 for l in (0, 1)]


def astar_pf(b, src, tgt, lev, conf, via_hard, via_conf, via_cost, pres, layer_cost, lvl_cost, max_expand=2_500_000):
    tdist = [ndimage.distance_transform_edt(~tgt[l]) for l in (0, 1)]
    hmap = (np.minimum(tdist[0], tdist[1] + via_cost), np.minimum(tdist[1], tdist[0] + via_cost))
    g = np.full((2, NY, NX), np.inf, np.float32)
    came = np.full((2, NY, NX), -1, np.int64)
    cdir = np.full((2, NY, NX), -1, np.int8)
    heap = []
    for (l, j, i) in src:
        if g[l, j, i] > 0:
            g[l, j, i] = 0
            heapq.heappush(heap, (hmap[l][j, i], 0.0, l, j, i))
    zone = b.ant
    hist = b.hist
    n = 0
    while heap:
        f, gc, l, j, i = heapq.heappop(heap)
        if gc > g[l, j, i]:
            continue
        if tgt[l, j, i]:
            path = [(l, j, i)]
            idx = came[l, j, i]
            while idx >= 0:
                l2, rem = divmod(int(idx), NY * NX)
                j2, i2 = divmod(rem, NX)
                path.append((l2, j2, i2))
                idx = came[l2, j2, i2]
            return path[::-1]
        n += 1
        if n > max_expand:
            return None
        d_in = cdir[l, j, i]
        L = lev[l]
        C = conf[l]
        H = hist[l]
        for d, (di, dj) in enumerate(R.DIRS):
            i2, j2 = i + di, j + dj
            if not (0 <= i2 < NX and 0 <= j2 < NY):
                continue
            lv = L[j2, i2]
            if lv < 0 and not tgt[l, j2, i2]:
                continue
            base = (1.4142 if (di and dj) else 1.0) * layer_cost[l]
            step = base * (lvl_cost[lv] if lv >= 0 else 1.0)
            if lv >= 0 and C[lv][j2, i2]:
                step += base * pres * (1.0 + H[j2, i2])
            elif H[j2, i2] > 0:
                step += base * 0.2 * H[j2, i2]
            if zone[j2, i2]:
                step *= R.ZONE_COST
            if d_in >= 0:
                t = abs(d - d_in)
                step += R.TURN_COST[min(t, 8 - t)]
            ng = gc + step
            if ng < g[l, j2, i2]:
                g[l, j2, i2] = ng
                came[l, j2, i2] = (l * NY + j) * NX + i
                cdir[l, j2, i2] = d
                heapq.heappush(heap, (ng + hmap[l][j2, i2], ng, l, j2, i2))
        if via_hard[j, i]:
            l2 = 1 - l
            ng = gc + via_cost + (via_cost * 0.5 * pres * (1.0 + hist[0, j, i] + hist[1, j, i]) if via_conf[j, i] else 0)
            if ng < g[l2, j, i] and (lev[l2][j, i] >= 0 or tgt[l2, j, i]):
                g[l2, j, i] = ng
                came[l2, j, i] = (l * NY + j) * NX + i
                cdir[l2, j, i] = -1
                heapq.heappush(heap, (ng + hmap[l2][j, i], ng, l2, j, i))
    return None


def route_net_pf(b, net, pres, log):
    """(Re)route one net against hard pads + soft other-net copper. Replaces b.routes[net] / b.tr[net]."""
    npads = b.pads_by_net.get(net, [])
    if len(npads) < 2:
        return True
    cls = R.cls_of(net)
    aux = R.AUX.get(net, {})
    start = R.START.get(net)
    main = [p for p in npads if (p.des, p.num) not in aux]
    auxp = [p for p in npads if (p.des, p.num) in aux]
    if start:
        main.sort(key=lambda p: (p.des, p.num) != start)
    b.tr[net] = np.zeros((2, NY, NX), bool)
    b.routes[net] = {'segs': [], 'vias': []}
    hard = b.hard_dist(net)
    soft = b.soft_dist(net)
    tree = np.zeros((2, NY, NX), bool)       # this net's routed copper so far
    tcore = np.zeros((2, NY, NX), bool)      # cores of the pads already in the tree
    first = main[0]
    for l in first.layers:
        c = b.core[id(first)]
        tcore[l][c[:, 0], c[:, 1]] = True
    todo = [(p, cls) for p in main[1:]] + [(p, aux[(p.des, p.num)]) for p in auxp]
    ok_all = True
    while todo:
        tc = np.argwhere(tree[0] | tree[1] | tcore[0] | tcore[1])
        cand = [t for t in todo if t[1] == cls] or todo
        best = min(cand, key=lambda t: np.min((tc[:, 1] * RES - t[0].cx) ** 2 + (tc[:, 0] * RES - t[0].cy) ** 2))
        todo.remove(best)
        p, pc = best
        _, _, via, via_cost, nvia = R.CLASSES[pc]
        lv = R.Levels(b, net, R.width_specs(net, pc, npads), hard,
                      zone_ok=(not LP.ANT_STRICT) and ((net in b.zone_nets) or cls == 'SIG'))
        conf = [[soft[l] < CLR + w / 2 for w in lv.w] for l in (0, 1)]
        r = via[1] / 2
        via_hard = ((hard[0] >= CLR + r) & (hard[1] >= CLR + r) & (b.edge_dist >= EDGE_CLR + r) & (b.hole_dist >= R.HOLE_CLR + r)
                    & (b.smd_dist >= r + 4) & (b.tht_dist >= r + 2)
                    & ~ndimage.binary_dilation(b.ant, iterations=int(r / RES) + 2))
        if net not in b.boost_nets:
            via_hard &= b.boost_dist >= r + 3
        if net not in b.body_nets:  # no signal / power vias under the module body
            via_hard &= b.body_dist >= r + 3
        via_conf = (soft[0] < CLR + r) | (soft[1] < CLR + r)
        src = [(l, int(c[0]), int(c[1])) for l in p.layers for c in b.core[id(p)]]
        lc = R.LAYER_COST_PWR if pc in R.PWR_CLASSES else R.LAYER_COST
        # targets: pad cores of the tree, and tree copper only where this connection's copper fits (lev >= 0)
        tgt = np.stack([tcore[l] | (tree[l] & (lv.lev[l] >= 0)) for l in (0, 1)])
        path = astar_pf(b, src, tgt, lv.lev, conf, via_hard, via_conf, via_cost, pres, lc, lv.cost)
        if not path:
            log.append('HARD FAIL %s %s.%s' % (net, p.des, p.num))
            ok_all = False
            continue
        segs, vias = R.path_to_copper(path, lv)
        if pc in R.PAR_VIA_CLASSES and vias:
            vias = R.parallel_vias(b, net, vias, segs, via, hard)
        vv = [(vx, vy, via[0], via[1]) for vx, vy in vias]
        for (lay, x1, y1, x2, y2, ww) in segs:
            m = R.seg_mask(x1, y1, x2, y2, ww / 2)
            b.tr[net][lay] |= m
            tree[lay] |= m
        for (vx, vy, d1, d2) in vv:
            m = M.disc_mask(vx, vy, d2 / 2)
            b.tr[net][0] |= m
            b.tr[net][1] |= m
            tree[0] |= m
            tree[1] |= m
        for l in p.layers:
            c = b.core[id(p)]
            tcore[l][c[:, 0], c[:, 1]] = True
        b.routes[net]['segs'] += segs
        b.routes[net]['vias'] += vv
    return ok_all


def conflicts(b):
    """Cells where two nets' trace copper (each grown by ~CLR/2) overlap, plus trace copper on foreign pads."""
    cnt = np.zeros((2, NY, NX), np.int16)
    claims = {}
    for n, m in b.tr.items():
        c = np.stack([ndimage.binary_dilation(m[l], DISC1, iterations=1) for l in (0, 1)])
        claims[n] = c
        cnt += c
    bad = cnt >= 2
    nets = set(n for n, c in claims.items() if (c & bad).any())
    return bad, nets


def run(iters=15, init=None, reroute=None, pres=0.6):
    """init: routes dict to warm-start from (kept for nets not in `reroute`); reroute: nets to rip up first."""
    import gnd_stage as GS
    b = PFBoard()
    log = []
    order = [n for _, nets in R.full_order(b) for n in nets]
    t0 = time.time()
    todo = list(order)
    # GND fanout first: fixed copper every net routes around (reused from a warm start when it has one)
    g = (init or {}).get('GND')
    if g and 'fanout' in g:
        g = {'segs': [tuple(x) for x in g['segs']], 'vias': [tuple(x) for x in g['vias']], 'fanout': g['fanout']}
        if 'stitch' in init['GND']:
            g['vias'] = g['vias'][:len(g['vias']) - init['GND']['stitch']]
        b.fix_copper('GND', g['segs'], g['vias'])
    else:
        g = GS.fanout(b)
        for n in g.pop('notes'):
            print(n)
            log.append(n)
    b.routes['GND'] = g
    if init:
        for net, r in init.items():
            if net == 'GND' or net not in b.nid:
                continue
            m = np.zeros((2, NY, NX), bool)
            for (lay, x1, y1, x2, y2, ww) in r['segs']:
                m[lay] |= R.seg_mask(x1, y1, x2, y2, ww / 2)
            for (vx, vy, d1, d2) in r['vias']:
                dm = M.disc_mask(vx, vy, d2 / 2)
                m[0] |= dm
                m[1] |= dm
            b.tr[net] = m
            b.routes[net] = {'segs': [tuple(x) for x in r['segs']], 'vias': [tuple(x) for x in r['vias']]}
        todo = [n for n in order if n in (reroute or set())]
    for it in range(iters):
        for net in todo:
            route_net_pf(b, net, pres, log)
        bad, cnets = conflicts(b)
        ncells = int(bad.sum())
        msg = 'iter %2d: routed %2d nets, conflicts %5d cells in %2d nets (pres %.2f) %4.0fs' % (
            it, len(todo), ncells, len(cnets), pres, time.time() - t0)
        print(msg)
        log.append(msg)
        if not cnets:
            break
        b.hist[bad] += 1.0
        pres *= 1.7
        todo = [n for n in order if n in cnets]
    return b, log


if __name__ == '__main__':
    iters = next((int(a.split('=')[1]) for a in sys.argv if a.startswith('--iters=')), 15)
    warm = next((a.split('=', 1)[1] for a in sys.argv if a.startswith('--warm=')), None)
    if warm:
        init = json.load(open(warm, encoding='utf-8'))
        rr = set(next(a.split('=', 1)[1] for a in sys.argv if a.startswith('--reroute=')).split(','))
        pres0 = next((float(a.split('=')[1]) for a in sys.argv if a.startswith('--pres=')), 20.0)
        b, log = run(iters, init=init, reroute=rr, pres=pres0)
    else:
        b, log = run(iters)
    # --out=<file.json>: write there (+ <file>_preview.png, <file>_log.txt) instead of routes.json, so two runs can't
    # overwrite each other's results
    out = next((a.split('=', 1)[1] for a in sys.argv if a.startswith('--out=')), None)
    base = os.path.splitext(out)[0] if out else None
    R.save(b, out or os.path.join(M.HERE, 'data', 'routes.json'))
    R.render(b, base + '_preview.png' if out else os.path.join(M.HERE, 'data', 'route_preview.png'), 0.4)
    open(base + '_log.txt' if out else os.path.join(M.HERE, 'data', 'pf_log.txt'), 'w').write('\n'.join(log))
    print('\n'.join(l for l in log if l.startswith('HARD')))
