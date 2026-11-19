# Robot carrier v0.6 PCB (pcb-pipeline template, attendance rev C engine) — GND stage, in two parts (D-028):
#  fanout(b)  BEFORE routing (called by pathfinder.run): fixed GND copper that every other net routes around
#     1) U1 exposed pad: 5 thermal vias (0.3/0.6 mm, dice pattern) + its GND pins (CE, EN1, VSS, TD) tied straight in;
#        U5 module pad 39 (3 x 3 sub-pads): 6 vias (0.3/0.6 mm) in the gaps, each overlapping two sub-pads (spec §7.2);
#     2) every other SMD GND pad: short stub + via to the bottom plane at the cheapest legal site (prefers sites that
#        leave a routing channel). Done first so no GND pin can be boxed in by tracks (run 4: U3.4 had no via site);
#  stitch(b)  AFTER routing: stitching vias where both layers are free (they bond the top and bottom GND pours), plus a
#     via for any SMD GND pad the fanout could not serve.
# Usage (after pathfinder.py): python gnd_stage.py  -> adds stitching to routes.json["GND"]. Pours: 41_pour.js.
import heapq
import json
import math
import os

import numpy as np
from scipy import ndimage

import layout_plan as LP
import route_model as M
import router as R

RES, NX, NY = M.RES, M.NX, M.NY
mm = R.mm
VIA_GND = (mm(0.4), mm(0.8))
VIA_EP = (mm(0.3), mm(0.6))
STUB_W = mm(0.4)
STITCH_PITCH = 300.0  # robot: THT GND pads already join the layers; 7.6 mm grid bonds the fill islands
REACH = 70            # cells (350 mil): max stub search distance
TIGHT = 15.0          # a via closer than CLR + r + TIGHT to foreign pads costs extra (keeps a routing channel)


def _ep_ties(b):
    """U1: 5 thermal vias in the exposed pad and straight stubs from its GND pins into the pad."""
    segs, vias, notes = [], [], []
    if not LP.EP_PADS:   # robot: 100 % through-hole, no exposed pads to tie
        return segs, vias, notes
    u1 = {p.num: p for p in b.pads if p.des == 'U1'}
    ep = u1['17']
    off = mm(0.45)
    for dx in (-off, off):
        for dy in (-off, off):
            vias.append((ep.cx + dx, ep.cy + dy, VIA_EP[0], VIA_EP[1]))
    vias.append((ep.cx, ep.cy, VIA_EP[0], VIA_EP[1]))  # 5th, centre (spec 7.2: >= 5 vias; 1.7 mm pad)
    # module GND pad (repeated pad number = sub-pads): a via between each pair of neighbours in a row
    subs = [p for p in b.pads if (p.des, p.num) in LP.EP_PADS and p.des != 'U1']
    rows = {}
    for p in subs:
        rows.setdefault(round(p.cy), []).append(p)
    for row in rows.values():
        row.sort(key=lambda p: p.cx)
        for a, c in zip(row, row[1:]):
            vias.append(((a.cx + c.cx) / 2, (a.cy + c.cy) / 2, VIA_EP[0], VIA_EP[1]))
    if subs:
        notes.append('module EP: %d sub-pads, %d vias' % (len(subs), sum(len(r) - 1 for r in rows.values())))
    for pin in ('4', '6', '8', '15'):
        p = u1[pin]
        if p.net != 'GND':
            notes.append('U1.%s is %s, not GND' % (pin, p.net))
            continue
        # straight inward (along the pin axis) into the EP, 4 mil past its edge; a diagonal stub towards the EP centre
        # passed corner pins' neighbours (BAT, PGOOD) at ~6 mil
        vx, vy = ep.cx - p.cx, ep.cy - p.cy
        if abs(vx) >= abs(vy):
            ex, ey = ep.cx - math.copysign(ep.w / 2 - 4, vx), p.cy
        else:
            ex, ey = p.cx, ep.cy - math.copysign(ep.h / 2 - 4, vy)
        segs.append((0, p.cx, p.cy, ex, ey, min(mm(0.25), p.minor)))
    return segs, vias, notes


def _via_site(b, p, f_top, f_bot, placed):
    """Cheapest stub + via for SMD pad p: Dijkstra over top-layer cells from the pad core. f_top / f_bot: distance
    (mil) to foreign copper per layer. Reaching existing GND copper on the bottom layer (a via or a THT GND pad) ends
    the search without a new via. Returns (stub segments, via or None) or None."""
    k = b.nid['GND']
    r = VIA_GND[1] / 2
    w = min(STUB_W, p.minor)
    near_pad = M.disc_mask(p.cx, p.cy, max(p.w, p.h) / 2 + R.FINE_EXT)
    pas = (((f_top >= R.CLR + w / 2) | (near_pad & (f_top >= R.NECK_CLR + w / 2)))
           & (b.edge_dist >= R.EDGE_CLR + w / 2) & (b.hole_dist >= R.HOLE_CLR + w / 2) & (b.zone_dist >= w / 2 + 3))
    jj, ii = np.mgrid[0:NY, 0:NX]
    close = np.zeros((NY, NX), bool)
    for (vx, vy) in placed:
        close |= (ii * RES - vx) ** 2 + (jj * RES - vy) ** 2 < (VIA_GND[1] + 4) ** 2
    nopour = ndimage.binary_dilation(M.zone_mask(LP.POUR_KEEPOUT), iterations=int(r / RES) + 2)
    via_ok = ((f_top >= R.CLR + r) & (f_bot >= R.CLR + r) & (b.edge_dist >= R.EDGE_CLR + r)
              & (b.smd_dist >= r + 4) & (b.tht_dist >= r + 2) & (b.hole_dist >= R.HOLE_CLR + r) & ~nopour & ~close)
    tight = np.minimum(f_top, f_bot) < R.CLR + r + TIGHT
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
        if d > dist.get((j, i), 1e18) or d > REACH:
            continue
        if best is not None and d > best[0]:
            break
        if not padm[j, i]:
            if b.cu[1][j, i] == k:
                best = (d, j, i, True)
                break
            if via_ok[j, i]:
                cost = d + (8.0 if tight[j, i] else 0.0)
                if best is None or cost < best[0]:
                    best = (cost, j, i, False)
        for di, dj in R.DIRS:
            i2, j2 = i + di, j + dj
            if not (0 <= i2 < NX and 0 <= j2 < NY):
                continue
            if not (pas[j2, i2] or padm[j2, i2] or b.cu[0][j2, i2] == k):
                continue
            nd = d + (1.4142 if di and dj else 1.0)
            if nd < dist.get((j2, i2), 1e18):
                dist[(j2, i2)] = nd
                prev[(j2, i2)] = (j, i)
                heapq.heappush(heap, (nd, j2, i2))
    if best is None:
        return None
    _, j, i, reuse = best
    path = [(j, i)]
    while path[-1] in prev:
        path.append(prev[path[-1]])
    path = path[::-1]
    pts = [(i_ * RES, j_ * RES) for j_, i_ in path]
    segs = []
    for a in range(len(pts) - 1):
        s = (0, pts[a][0], pts[a][1], pts[a + 1][0], pts[a + 1][1], w)
        if segs:
            m0 = segs[-1]
            d1 = (m0[3] - m0[1], m0[4] - m0[2])
            d2 = (s[3] - s[1], s[4] - s[2])
            if abs(d1[0] * d2[1] - d1[1] * d2[0]) < 1e-6 and (d1[0] * d2[0] + d1[1] * d2[1]) > 0:
                segs[-1] = (0, m0[1], m0[2], s[3], s[4], w)
                continue
        segs.append(s)
    via = None if reuse else (i * RES, j * RES, VIA_GND[0], VIA_GND[1])
    return segs, via


def fanout(b):
    """Before routing. b: a PFBoard with pads only. Returns {'segs', 'vias', 'fanout': [pad ids], 'notes'} and fixes
    that copper on the board (PFBoard.fix_copper), so every other net routes around it."""
    segs, vias, notes = _ep_ties(b)
    b.fix_copper('GND', segs, vias)
    hard = b.hard_dist('GND')
    done = []
    gnd_smd = [p for p in b.pads if p.net == 'GND' and not p.tht and p.des != 'U1' and (p.des, p.num) not in LP.EP_PADS]
    # tightest pads first (fewest options), so easier pads don't take their sites
    gnd_smd.sort(key=lambda p: float(np.min(hard[0][b.cells[id(p)][:, 0], b.cells[id(p)][:, 1]])))
    for p in gnd_smd:
        res = _via_site(b, p, hard[0], hard[1], [(v[0], v[1]) for v in vias])
        if not res:
            notes.append('fanout: no via site for %s.%s' % (p.des, p.num))
            continue
        st, via = res
        segs += st
        if via:
            vias.append(via)
        done.append('%s.%s' % (p.des, p.num))
        b.fix_copper('GND', st, [via] if via else [])
    notes.append('fanout: %d SMD GND pads, %d vias (incl. the exposed-pad vias)' % (len(done), len(vias)))
    return {'segs': segs, 'vias': vias, 'fanout': done, 'notes': notes}


def stitch(b, gnd):
    """After routing. b: PFBoard with the fanout fixed and all routed copper added. gnd: routes['GND']."""
    k = b.nid['GND']
    segs, vias = [tuple(s) for s in gnd['segs']], [tuple(v) for v in gnd['vias']]
    notes = []
    hard = b.hard_dist('GND')

    def fd():
        # exact distance to other nets' pads / hole rings, and (conservatively, -RES) to other nets' routed copper
        out = []
        for l in (0, 1):
            tr = (b.cu[l] != 0) & (b.cu[l] != k) & (b.pad_cu[l] == 0)
            td = ndimage.distance_transform_edt(~tr) * RES - RES
            out.append(np.minimum(hard[l], td))
        return out

    served = set(gnd.get('fanout', []))
    for p in b.pads:  # leftover SMD GND pads (the fanout could not serve them)
        if p.net != 'GND' or p.tht or p.des == 'U1' or (p.des, p.num) in LP.EP_PADS or ('%s.%s' % (p.des, p.num)) in served:
            continue
        f = fd()
        res = _via_site(b, p, f[0], f[1], [(v[0], v[1]) for v in vias])
        if not res:
            notes.append('no via site for %s.%s (top pour only)' % (p.des, p.num))
            continue
        st, via = res
        segs += st
        if via:
            vias.append(via)
        b.add_copper('GND', st, [via] if via else [])
        served.add('%s.%s' % (p.des, p.num))
        notes.append('late via for %s.%s' % (p.des, p.num))
    f = fd()
    r = VIA_GND[1] / 2
    nopour = M.zone_mask(LP.POUR_KEEPOUT)
    ok = ((f[0] >= R.CLR + r + 15) & (f[1] >= R.CLR + r + 15) & (b.edge_dist >= 60) & (b.hole_dist >= R.HOLE_CLR + r + 20)
          & (b.smd_dist >= r + 15) & (b.tht_dist >= r + 15)
          & ~ndimage.binary_dilation(nopour, iterations=int((r + 40) / RES)))
    placed = [(v[0], v[1]) for v in vias]
    n_st = 0
    for y in np.arange(80, M.H - 60, STITCH_PITCH):
        for x in np.arange(80, M.W - 60, STITCH_PITCH):
            best = None
            j0, i0 = int(y / RES), int(x / RES)
            rr = int(STITCH_PITCH / 2 / RES)
            for dj in range(-rr, rr + 1, 2):
                for di in range(-rr, rr + 1, 2):
                    j, i = j0 + dj, i0 + di
                    if 0 <= j < NY and 0 <= i < NX and ok[j, i]:
                        d = dj * dj + di * di
                        if best is None or d < best[0]:
                            best = (d, i * RES, j * RES)
            if not best:
                continue
            px, py = best[1], best[2]
            if any(math.hypot(px - qx, py - qy) < 70 for qx, qy in placed):
                continue
            placed.append((px, py))
            vias.append((px, py, VIA_GND[0], VIA_GND[1]))
            n_st += 1
    notes.append('stitching vias: %d' % n_st)
    out = {'segs': [[round(v, 2) if isinstance(v, float) else v for v in s] for s in segs],
           'vias': [[round(v, 2) for v in vv] for vv in vias], 'fanout': sorted(served), 'stitch': n_st}
    return out, notes


def run(routes):
    import pathfinder as PF
    b = PF.PFBoard()
    gnd = routes.get('GND')
    if not gnd or 'fanout' not in gnd:
        gnd = fanout(b)   # legacy: routes made before the fanout existed (pads only on the board at this point)
        print('\n'.join(gnd.pop('notes')))
    else:
        b.fix_copper('GND', [tuple(s) for s in gnd['segs']], [tuple(v) for v in gnd['vias']])
    for net, r in routes.items():
        if net != 'GND':
            b.add_copper(net, [tuple(s) for s in r['segs']], [tuple(v) for v in r['vias']])
    out, notes = stitch(b, gnd)
    routes['GND'] = out
    return routes, notes


if __name__ == '__main__':
    src = os.path.join(M.HERE, 'data', 'routes.json')
    routes = json.load(open(src, encoding='utf-8'))
    g = routes.get('GND')
    if g and 'stitch' in g:
        # already stitched: drop the stitching vias (the last g['stitch'] vias), keep the fanout -> idempotent re-run
        g['vias'] = g['vias'][:len(g['vias']) - g['stitch']]
        g.pop('stitch')
    routes, notes = run(routes)
    json.dump(routes, open(src, 'w', encoding='utf-8'))
    print('\n'.join(notes))
    print('GND segs %d vias %d' % (len(routes['GND']['segs']), len(routes['GND']['vias'])))
