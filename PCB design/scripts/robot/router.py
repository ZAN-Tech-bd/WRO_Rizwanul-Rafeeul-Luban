# Robot carrier v0.6 PCB (pcb-pipeline template; attendance rev C engine) — OFFLINE 2-layer router (v2; rev B proven, rev C rules added).
# A* maze router on a 5-mil grid over route_model's exact copper map (footprint polygons from the LCSC files).
# - per-net widths wider than spec §7.2 (user request); width levels per cell, the widest that fits wins: main, neck
#   (between socket pins etc.), neck2 (0.4 / 0.35 mm) and per-pad escapes (pad width - 3/8/13 mil, 16/12/8 mil, only
#   within the pad's half-length + 20 mil). A thinner level always costs more (1 / 3 / 6 / 10), so thin copper appears
#   only where nothing wider fits (run 3 had used 8-mil tracks for ~150 mil on VBUS: D-028);
# - low-current branches of power nets (indicators, sense, pull-ups, test points) at a thinner class;
# - top layer preferred so the bottom stays a ground plane; vias only where needed, two in parallel for power;
# - antenna keep-out: rev B let thin signals cross it (DevKit on sockets, D-028); rev C (LP.ANT_STRICT, module on the
#   board) lets nothing in: no tracks, vias or pour of any net (D-032);
# - module body (LP.BODY_PART, between its pad rows): top layer only LP.BODY_NETS (GND pour), no other vias; bottom-layer
#   tracks may cross (spec §7.2).
# Output: data/routes.json (segments + vias per net) -> 40_route.js; preview PNGs.
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

RES, NX, NY = M.RES, M.NX, M.NY
MIL_PER_MM = 39.37

# ---------------------------------------------------------------- rules (mil)
CLR = 10.0            # router clearance between different nets (EDA rule 6 mil: margin for rasterisation)
NECK_CLR = 7.5        # clearance accepted at fine-pitch pins (fine level; EDA rule 6 mil; trim.py guarantees >= 6.5)
EDGE_CLR = 14.0       # copper to board edge (JLC 0.3 mm)
HOLE_CLR = 12.0       # copper to NPTH / mounting hole edge


def mm(v):
    return v * MIL_PER_MM


# class: main width, neck width, via (drill, dia), via cost, parallel vias. Robot spec §7: BAT_SW >= 3 mm in its
# corridor, +5V spine >= 2 mm, VAMP >= 1 mm, signals 0.3-0.4 mm. Power vias 0.5-0.6 mm drill, two in parallel.
CLASSES = {
    'PWR_HI': (mm(3.0), mm(1.5), (mm(0.6), mm(1.2)), 200, 2),    # battery: BAT+, BAT_SW (motors + 5 V buck, ~5 A peak)
    'PWR_5V': (mm(2.0), mm(0.8), (mm(0.5), mm(1.0)), 140, 2),    # +5V spine (DevKit, CAM, Mega, DFPlayer, LM386 at JP1 = 5V)
    'PWR_AMP': (mm(1.2), mm(0.6), (mm(0.5), mm(1.0)), 120, 2),   # LM386 supply chain: AMP_D, VAMP_BAT, VAMP
    'PWR_3V3': (mm(0.8), mm(0.3), (mm(0.4), mm(0.8)), 110, 1),   # 0.3 mm neck fits between socket pins (load <= 0.2 A)
    'SIG': (mm(0.35), mm(0.25), (mm(0.35), mm(0.7)), 45, 1),
    'GND': (mm(0.6), mm(0.35), (mm(0.4), mm(0.8)), 10, 1),
}
PWR_CLASSES = ('PWR_HI', 'PWR_5V', 'PWR_AMP', 'PWR_3V3')   # bottom layer only to cross (keeps the GND plane whole)
PAR_VIA_CLASSES = ('PWR_HI', 'PWR_5V', 'PWR_AMP')          # two vias in parallel
# width levels per cell (widest that fits wins): main, neck, neck2, per-pad fine escapes.
# A thinner level always costs more, so thin copper is used only where nothing wider fits (pad escapes).
NECK_COST, NECK2_COST, FINE_COST = 3.0, 6.0, 10.0
FINE_EXT = 20.0      # fine escape zone of a small pad: its half-length + this (mil)
NET_CLASS = {'BAT+': 'PWR_HI', 'BAT_SW': 'PWR_HI', '+5V': 'PWR_5V', 'AMP_D': 'PWR_AMP', 'VAMP_BAT': 'PWR_AMP',
             'VAMP': 'PWR_AMP', '+3V3': 'PWR_3V3', 'GND': 'GND'}
# low-current branches of power nets: (des, pad) -> class used for that branch
AUX = {
    # battery sense divider + LED1 resistor
    'BAT_SW': {('R24', '1'): 'SIG', ('R26', '1'): 'SIG'},
    # +5V: decoupling caps + L298N logic supply at the 0.8 mm class, LED2 resistor at signal width
    '+5V': {('C3', '1'): 'PWR_3V3', ('C4', '1'): 'PWR_3V3', ('C5', '1'): 'PWR_3V3', ('C6', '1'): 'PWR_3V3',
            ('J6', '1'): 'PWR_3V3', ('R27', '1'): 'SIG'},
    # +3V3: pull-ups and the EXP pads at signal width (RC522, sensor ports and their caps keep 0.8 mm)
    '+3V3': {(d, n): 'SIG' for d, n in (('R14', '1'), ('R15', '1'), ('R18', '1'), ('R20', '1'), ('R22', '1'),
                                         ('J14', '1'))},
}
START = {'BAT+': ('J1', '2'), 'BAT_SW': ('J5', '2'), '+5V': ('J4', '2'), 'AMP_D': ('D1', '1'),
         'VAMP_BAT': ('D2', '1'), 'VAMP': ('JP1', '2'), '+3V3': ('U1A', '1')}
POWER_ORDER = ['BAT+', 'BAT_SW', 'AMP_D', 'VAMP_BAT', 'VAMP', '+5V', '+3V3']
LAYER_COST = (1.0, 2.2)      # prefer the top layer: the bottom is the ground plane (signals)
LAYER_COST_PWR = (1.0, 5.0)  # power nets: bottom only to cross (keeps the GND plane whole); 3.2 let VSYS_SW run 50 mm on the bottom (rev C run 3)
ZONE_COST = 4.0
ZONE_ESCAPE = 140.0          # power pads inside the antenna keep-out: escape copper only within this radius (mil)
TURN_COST = {0: 0.0, 1: 0.35, 2: 1.2, 3: 3.0, 4: 6.0}
DIRS = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]


def cls_of(net):
    return NET_CLASS.get(net, 'SIG')


def seg_mask(x1, y1, x2, y2, r):
    """Cells whose centre is within r of the segment (capsule)."""
    i0, i1 = max(0, int((min(x1, x2) - r) / RES) - 1), min(NX - 1, int((max(x1, x2) + r) / RES) + 2)
    j0, j1 = max(0, int((min(y1, y2) - r) / RES) - 1), min(NY - 1, int((max(y1, y2) + r) / RES) + 2)
    m = np.zeros((NY, NX), bool)
    if i1 < i0 or j1 < j0:
        return m
    xx, yy = np.meshgrid(M.GX[i0:i1 + 1], M.GY[j0:j1 + 1])
    dx, dy = x2 - x1, y2 - y1
    L2 = dx * dx + dy * dy
    if L2 == 0:
        d2 = (xx - x1) ** 2 + (yy - y1) ** 2
    else:
        t = np.clip(((xx - x1) * dx + (yy - y1) * dy) / L2, 0, 1)
        d2 = (xx - (x1 + t * dx)) ** 2 + (yy - (y1 + t * dy)) ** 2
    m[j0:j1 + 1, i0:i1 + 1] = d2 <= r * r
    return m


class Board:
    def __init__(self):
        self.pads, self.holes, problems = M.build()
        assert not problems, problems
        self.nets = sorted(set(p.net for p in self.pads if p.net))
        self.nid = {n: i + 1 for i, n in enumerate(self.nets)}
        self.cu = np.zeros((2, NY, NX), np.int32)          # 0 free, -1 obstacle, k = net id
        self.keep = np.zeros((NY, NX), bool)                # hole keep-outs (both layers)
        self.cells = {}
        for p in self.pads:
            m = M.poly_mask(p.poly)
            if not m.any():
                m[int(round(p.cy / RES)), int(round(p.cx / RES))] = True
            for l in p.layers:
                self.cu[l][m] = self.nid[p.net] if p.net else -1
            self.cells[id(p)] = np.argwhere(m)
        for h in self.holes:
            if h['ring']:
                m = M.disc_mask(h['x'], h['y'], h['ring'] / 2)
                self.cu[0][m] = -1
                self.cu[1][m] = -1
            self.keep |= M.disc_mask(h['x'], h['y'], h['d'] / 2 + HOLE_CLR)
        hd = np.full((NY, NX), 1e9, np.float32)
        jj0, ii0 = np.mgrid[0:NY, 0:NX]
        for h in self.holes:
            if not h['ring']:
                hd = np.minimum(hd, np.hypot(ii0 * RES - h['x'], jj0 * RES - h['y']) - h['d'] / 2)
        self.hole_dist = hd                                  # distance to the nearest NPTH edge (mil)
        smd = np.zeros((NY, NX), bool)
        tht = np.zeros((NY, NX), bool)
        for p in self.pads:
            c = self.cells[id(p)]
            if (p.des, p.num) in LP.EP_PADS:
                continue  # exposed pads (U1, U5 module): thermal vias go inside / at them on purpose
            (tht if p.tht else smd)[c[:, 0], c[:, 1]] = True
        # vias never on/at SMD pads (solder wicking), never overlapping THT pads, whatever the net
        self.smd_dist = ndimage.distance_transform_edt(~smd) * RES - RES * 0.5
        self.tht_dist = ndimage.distance_transform_edt(~tht) * RES - RES * 0.5
        self.ant = M.zone_mask(LP.ANTENNA)
        zx0, zy0, zx1, zy1 = LP.ANTENNA
        dzx = np.maximum(np.maximum(zx0 - ii0 * RES, 0), ii0 * RES - zx1)
        dzy = np.maximum(np.maximum(zy0 - jj0 * RES, 0), jj0 * RES - zy1)
        self.zone_dist = np.hypot(dzx, dzy)                  # 0 inside the antenna keep-out
        # nets whose pads sit inside the antenna keep-out may escape through it (rev B); strict (rev C): none
        self.zone_nets = set() if LP.ANT_STRICT else set(
            p.net for p in self.pads if p.net and LP.ANTENNA[0] <= p.cx <= LP.ANTENNA[2] and LP.ANTENNA[1] <= p.cy <= LP.ANTENNA[3])
        # module body: top layer only LP.BODY_NETS, vias only of those nets (distance to the rectangle, 0 inside)
        self.body_rect = M.module_body(self.pads)
        self.body_dist = M.rect_dist(self.body_rect)
        self.body_nets = set(LP.BODY_NETS)
        # boost block keep-out (foreign nets, both layers): distance to the rectangle, 0 inside
        self.boost_rect = M.boost_rect(self.pads)
        bx0, by0, bx1, by1 = self.boost_rect
        dbx = np.maximum(np.maximum(bx0 - ii0 * RES, 0), ii0 * RES - bx1)
        dby = np.maximum(np.maximum(by0 - jj0 * RES, 0), jj0 * RES - by1)
        self.boost_dist = np.hypot(dbx, dby)
        self.boost_nets = set(LP.BOOST_NETS)
        jj, ii = np.mgrid[0:NY, 0:NX]
        self.edge_dist = np.minimum(np.minimum(ii, NX - 1 - ii), np.minimum(jj, NY - 1 - jj)) * RES
        self.routes = {}
        self._zesc = {}
        self.pads_by_net = {}
        for p in self.pads:
            if p.net:
                self.pads_by_net.setdefault(p.net, []).append(p)

    def zone_escape(self, net, reach=ZONE_ESCAPE):
        """Cells within `reach` mil of this net's own pads that sit in the antenna keep-out (spec §7: no power tracks
        under the antenna end; a power pad there may only escape out of it)."""
        if net not in self._zesc:
            m = np.zeros((NY, NX), bool)
            for p in self.pads_by_net.get(net, []):
                if LP.ANTENNA[0] <= p.cx <= LP.ANTENNA[2] and LP.ANTENNA[1] <= p.cy <= LP.ANTENNA[3]:
                    m |= M.disc_mask(p.cx, p.cy, reach)
            self._zesc[net] = m
        return self._zesc[net]

    def foreign_dist(self, net):
        k = self.nid[net]
        out = []
        for l in (0, 1):
            foreign = (self.cu[l] != 0) & (self.cu[l] != k)
            out.append(ndimage.distance_transform_edt(~foreign) * RES - RES * 0.5)
        return out

    def add_copper(self, net, segs, vias):
        k = self.nid[net]
        for (lay, x1, y1, x2, y2, ww) in segs:
            m = seg_mask(x1, y1, x2, y2, ww / 2)
            self.cu[lay][m & (self.cu[lay] == 0)] = k
        for (vx, vy, drill, dia) in vias:
            m = M.disc_mask(vx, vy, dia / 2)
            for l in (0, 1):
                self.cu[l][m & (self.cu[l] == 0)] = k


class Levels:
    """Per-cell passability for one connection. specs = [(width, clearance, cost, zone mask or None), ...];
    lev[l][j, i] = index of the widest spec that fits at the cell (-1 = none). Widths in self.w, costs in self.cost."""

    def __init__(self, b, net, specs, fd, zone_ok=None):
        order = sorted(range(len(specs)), key=lambda k: specs[k][0])  # narrowest first: wider ones overwrite
        self.w = tuple(sp[0] for sp in specs)
        self.cost = tuple(sp[2] for sp in specs)
        self.lev = []
        # antenna keep-out: escape nets always; thin signal tracks allowed at ZONE_COST (no power, no vias, no pour)
        okzone = (net in b.zone_nets) if zone_ok is None else zone_ok
        if LP.ANT_STRICT:
            okzone = False
        hd = b.hole_dist
        for l in (0, 1):
            lev = np.full((NY, NX), -1, np.int8)
            for k in order:
                w, clr, cost, zone = specs[k]
                fit = (fd[l] >= clr + w / 2) & (b.edge_dist >= EDGE_CLR + w / 2) & (hd >= HOLE_CLR + w / 2)
                if zone is not None:
                    fit &= zone
                if not okzone:  # whole trace copper (not just the centreline) stays out of the antenna keep-out
                    fit &= b.zone_dist >= w / 2 + 3
                elif cls_of(net) != 'SIG':  # power with a pad in the strip (U1A.1 +3V3): only a short escape there
                    fit &= (b.zone_dist >= w / 2 + 3) | b.zone_escape(net)
                if net not in b.boost_nets:  # nor out of the boost block
                    fit &= b.boost_dist >= w / 2 + 3
                if l == 0 and net not in b.body_nets:  # top layer under the module body: GND only
                    fit &= b.body_dist >= w / 2 + 3
                lev[fit] = k
            self.lev.append(lev)


def width_specs(net, pc, npads):
    """Width levels for one connection of `net` routed at class `pc` (AUX branches use a thinner class)."""
    cls = cls_of(net)
    w_main, w_neck = CLASSES[pc][0], CLASSES[pc][1]
    w_main = min(w_main, CLASSES[cls][0])
    w_neck = min(w_neck, w_main)
    specs = [(w_main, CLR, 1.0, None), (w_neck, CLR, NECK_COST, None)]
    w_neck2 = max(CLASSES['SIG'][0], w_neck / 2)
    if w_neck2 < w_neck - 1:
        specs.append((w_neck2, CLR, NECK2_COST, None))
    # per-pad escape ladder (the grid model loses ~2.5 mil, so the pad width - 3 may not fit beside close neighbours):
    # the widest width that fits wins; zones of the same width are merged into one level
    fine = {}
    for q in npads:
        if q.minor < w_main + 1:
            zone = M.disc_mask(q.cx, q.cy, max(q.w, q.h) / 2 + FINE_EXT)
            for fw in {q.minor - 3.0, q.minor - 8.0, q.minor - 13.0, 16.0, 12.0, 8.0}:
                fw = round(max(6.0, min(fw, w_neck)), 1)
                if fw < w_neck:
                    fine[fw] = fine[fw] | zone if fw in fine else zone
    for fw, zone in sorted(fine.items(), reverse=True):
        specs.append((fw, NECK_CLR, FINE_COST, zone))
    return specs


def astar(b, src_cells, tgt, lev, via_mask, via_cost, max_expand=2_500_000, layer_cost=LAYER_COST, lvl_cost=None):
    tdist = [ndimage.distance_transform_edt(~tgt[l]) for l in (0, 1)]
    hmap = (np.minimum(tdist[0], tdist[1] + via_cost), np.minimum(tdist[1], tdist[0] + via_cost))
    g = np.full((2, NY, NX), np.inf, np.float32)
    came = np.full((2, NY, NX), -1, np.int64)
    cdir = np.full((2, NY, NX), -1, np.int8)
    heap = []
    for (l, j, i) in src_cells:
        if g[l, j, i] > 0:
            g[l, j, i] = 0
            heapq.heappush(heap, (hmap[l][j, i], 0.0, l, j, i))
    zone = b.ant
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
        for d, (di, dj) in enumerate(DIRS):
            i2, j2 = i + di, j + dj
            if not (0 <= i2 < NX and 0 <= j2 < NY):
                continue
            lv = L[j2, i2]
            if lv < 0 and not tgt[l, j2, i2]:
                continue
            step = (1.4142 if (di and dj) else 1.0) * layer_cost[l] * (lvl_cost[lv] if lv >= 0 else 1.0)
            if zone[j2, i2]:
                step *= ZONE_COST
            if d_in >= 0:
                t = abs(d - d_in)
                step += TURN_COST[min(t, 8 - t)]
            ng = gc + step
            if ng < g[l, j2, i2]:
                g[l, j2, i2] = ng
                came[l, j2, i2] = (l * NY + j) * NX + i
                cdir[l, j2, i2] = d
                heapq.heappush(heap, (ng + hmap[l][j2, i2], ng, l, j2, i2))
        if via_mask[j, i]:
            l2 = 1 - l
            ng = gc + via_cost
            if ng < g[l2, j, i] and (lev[l2][j, i] >= 0 or tgt[l2, j, i]):
                g[l2, j, i] = ng
                came[l2, j, i] = (l * NY + j) * NX + i
                cdir[l2, j, i] = -1
                heapq.heappush(heap, (ng + hmap[l2][j, i], ng, l2, j, i))
    return None


def path_to_copper(path, lv_obj):
    """Grid path -> segments [(layer, x1, y1, x2, y2, width)] (merged while direction + width stay the same), vias."""
    segs, vias = [], []

    def wcell(c):
        lv = lv_obj.lev[c[0]][c[1], c[2]]
        return lv_obj.w[lv] if lv >= 0 else min(lv_obj.w)

    runs, run = [], [path[0]]
    for c in path[1:]:
        if c[0] != run[-1][0]:
            runs.append(run)
            vias.append((c[2] * RES, c[1] * RES))
            run = [c]
        else:
            run.append(c)
    runs.append(run)
    for run in runs:
        if len(run) < 2:
            continue
        ws = [wcell(c) for c in run]
        start = 0
        for k in range(1, len(run)):
            if k < len(run) - 1:
                d1 = (run[k][2] - run[k - 1][2], run[k][1] - run[k - 1][1])
                d2 = (run[k + 1][2] - run[k][2], run[k + 1][1] - run[k][1])
                if d1 == d2 and ws[k + 1] == ws[k] == ws[start]:
                    continue
            w = min(ws[start:k + 1])
            segs.append((run[0][0], run[start][2] * RES, run[start][1] * RES, run[k][2] * RES, run[k][1] * RES, w))
            start = k
    return segs, vias


def parallel_vias(b, net, vias, segs, via, fd):
    """Second via beside each power via (perpendicular to the top-layer trace), if it fits."""
    out = []
    r = via[1] / 2
    for (vx, vy) in vias:
        out.append((vx, vy))
        # direction of the top segment touching this via
        dx, dy = 1.0, 0.0
        for (lay, x1, y1, x2, y2, w) in segs:
            if lay == 0 and ((abs(x1 - vx) < 1 and abs(y1 - vy) < 1) or (abs(x2 - vx) < 1 and abs(y2 - vy) < 1)):
                dx, dy = x2 - x1, y2 - y1
                n = math.hypot(dx, dy) or 1.0
                dx, dy = dx / n, dy / n
                break
        for sgn in (1, -1):
            px, py = vx - sgn * dy * via[1] * 1.05, vy + sgn * dx * via[1] * 1.05
            i, j = int(round(px / RES)), int(round(py / RES))
            if 0 <= i < NX and 0 <= j < NY and fd[0][j, i] >= CLR + r and fd[1][j, i] >= CLR + r \
                    and b.edge_dist[j, i] >= EDGE_CLR + r and b.hole_dist[j, i] >= HOLE_CLR + r and not b.ant[j, i] \
                    and b.smd_dist[j, i] >= r + 4 and b.tht_dist[j, i] >= r + 2 \
                    and (net in b.body_nets or b.body_dist[j, i] >= r + 3):
                out.append((px, py))
                break
    return out


def route_net(b, net, report, verbose=True):
    npads = b.pads_by_net.get(net, [])
    if len(npads) < 2:
        return
    cls = cls_of(net)
    k = b.nid[net]
    aux = AUX.get(net, {})
    start = START.get(net)
    main = [p for p in npads if (p.des, p.num) not in aux]
    auxp = [p for p in npads if (p.des, p.num) in aux]
    if start:
        main.sort(key=lambda p: (p.des, p.num) != start)
    tree = np.zeros((2, NY, NX), bool)
    for l in (0, 1):
        tree[l] = (b.cu[l] == k) & False
    first = main[0]
    for l in first.layers:
        c = b.cells[id(first)]
        tree[l][c[:, 0], c[:, 1]] = True
    todo = [(p, cls) for p in main[1:]] + [(p, aux[(p.des, p.num)]) for p in auxp]
    segs_all, vias_all = [], []
    t0 = time.time()
    while todo:
        tc = np.argwhere(tree[0] | tree[1])
        cand = [t for t in todo if t[1] == cls] or todo
        best = min(cand, key=lambda t: np.min((tc[:, 1] * RES - t[0].cx) ** 2 + (tc[:, 0] * RES - t[0].cy) ** 2))
        todo.remove(best)
        p, pc = best
        w_main, w_neck, via, via_cost, nvia = CLASSES[pc]
        w_main = min(w_main, CLASSES[cls][0])
        fd = b.foreign_dist(net)
        lv = Levels(b, net, width_specs(net, pc, npads), fd)
        r = via[1] / 2
        vmask = ((fd[0] >= CLR + r) & (fd[1] >= CLR + r) & (b.edge_dist >= EDGE_CLR + r) & (b.hole_dist >= HOLE_CLR + r)
                 & (b.smd_dist >= r + 4) & (b.tht_dist >= r + 2)
                 & ~ndimage.binary_dilation(b.ant, iterations=int(r / RES) + 2))
        if net not in b.body_nets:
            vmask &= b.body_dist >= r + 3
        src = [(l, int(c[0]), int(c[1])) for l in p.layers for c in b.cells[id(p)]]
        path = astar(b, src, tree, lv.lev, vmask, via_cost,
                     layer_cost=LAYER_COST_PWR if pc in PWR_CLASSES else LAYER_COST, lvl_cost=lv.cost)
        if not path:
            report['failed'].append('%s: %s.%s' % (net, p.des, p.num))
            if verbose:
                print('  FAIL', net, p.des, p.num)
            continue
        segs, vias = path_to_copper(path, lv)
        if pc in PAR_VIA_CLASSES and vias:
            vias = parallel_vias(b, net, vias, segs, via, fd)
        vv = [(vx, vy, via[0], via[1]) for vx, vy in vias]
        b.add_copper(net, segs, vv)
        for (lay, x1, y1, x2, y2, ww) in segs:
            tree[lay] |= seg_mask(x1, y1, x2, y2, ww / 2) & (b.cu[lay] == k)
        for (vx, vy, d1, d2) in vv:
            m = M.disc_mask(vx, vy, d2 / 2)
            tree[0] |= m
            tree[1] |= m
        for l in p.layers:
            c = b.cells[id(p)]
            tree[l][c[:, 0], c[:, 1]] = True
        segs_all += segs
        vias_all += vv
    b.routes.setdefault(net, {'segs': [], 'vias': []})
    b.routes[net]['segs'] += segs_all
    b.routes[net]['vias'] += vias_all
    report['routed'].append(net)
    if verbose:
        print('%-10s %-7s segs %3d vias %2d  (%.1fs)' % (net, cls, len(segs_all), len(vias_all), time.time() - t0))


# ---------------------------------------------------------------- render + save
def render(b, out_png, scale=0.5, crop=None, layers=(0, 1)):
    from PIL import Image, ImageDraw
    s, m = scale, 30
    img = Image.new('RGB', (int(M.W * s) + 2 * m, int(M.H * s) + 2 * m), (10, 30, 15))
    g = ImageDraw.Draw(img, 'RGBA')
    X = lambda v: m + v * s
    Y = lambda v: m + (M.H - v) * s
    g.rectangle([X(0), Y(M.H), X(M.W), Y(0)], outline=(220, 220, 220), width=2)
    a = LP.ANTENNA
    g.rectangle([X(a[0]), Y(a[3]), X(a[2]), Y(a[1])], outline=(255, 120, 120), width=1)
    for h in b.holes:
        r = max(h['d'], h['ring']) / 2
        g.ellipse([X(h['x'] - r), Y(h['y'] + r), X(h['x'] + r), Y(h['y'] - r)], outline=(200, 200, 200), fill=(0, 0, 0))
    col = {0: (230, 60, 50, 200), 1: (60, 120, 255, 170)}
    for l in sorted(layers, reverse=True):
        for net, r in b.routes.items():
            for (lay, x1, y1, x2, y2, w) in r['segs']:
                if lay != l:
                    continue
                g.line([X(x1), Y(y1), X(x2), Y(y2)], fill=col[l], width=max(1, int(w * s)))
                rr = w * s / 2
                for (px, py) in ((x1, y1), (x2, y2)):
                    g.ellipse([X(px) - rr, Y(py) - rr, X(px) + rr, Y(py) + rr], fill=col[l])
    for p in b.pads:
        pts = [(X(x), Y(y)) for x, y in p.poly]
        g.polygon(pts, fill=(200, 170, 60, 255) if p.net != 'GND' else (120, 120, 120, 255))
    for net, r in b.routes.items():
        for (vx, vy, drill, dia) in r['vias']:
            rr = dia * s / 2
            g.ellipse([X(vx) - rr, Y(vy) - rr, X(vx) + rr, Y(vy) + rr], fill=(230, 230, 230))
            rr = drill * s / 2
            g.ellipse([X(vx) - rr, Y(vy) - rr, X(vx) + rr, Y(vy) + rr], fill=(20, 20, 20))
    if crop:
        x0, y0, x1, y1 = crop
        img = img.crop((int(X(x0)), int(Y(y1)), int(X(x1)), int(Y(y0))))
    img.save(out_png)


def save(b, path):
    out = {}
    for n, r in b.routes.items():
        out[n] = {'segs': [[round(v, 2) if isinstance(v, float) else v for v in s] for s in r['segs']],
                  'vias': [[round(v, 2) for v in v] for v in r['vias']]}
        for extra in ('fanout', 'stitch'):   # GND fanout bookkeeping (gnd_stage.py)
            if extra in r:
                out[n][extra] = r[extra]
    json.dump(out, open(path, 'w', encoding='utf-8'))


FINE_ICS = ()                                  # no fine-pitch ICs (100 % THT)
MODULE_PARTS = ('U1A', 'U1B', 'U2A', 'U2B')   # DevKit + DFPlayer sockets: their signals claim the pin escapes first


def full_order(b):
    """ANT, fine-pitch IC escapes, local power, module signals (claim their pin escapes), long power, other signals."""
    def span(n):
        ps = b.pads_by_net[n]
        return (max(p.cx for p in ps) - min(p.cx for p in ps)) + (max(p.cy for p in ps) - min(p.cy for p in ps))
    sig = [n for n in b.nets if n not in POWER_ORDER and n != 'GND' and len(b.pads_by_net[n]) >= 2]
    ic = sorted([n for n in sig if any(p.des in FINE_ICS for p in b.pads_by_net[n])], key=span)
    dk = sorted([n for n in sig if n not in ic and any(p.des in MODULE_PARTS for p in b.pads_by_net[n])], key=span)
    rest = sorted([n for n in sig if n not in ic and n not in dk], key=span, reverse=True)
    long_pwr = ['+5V', '+3V3']
    local_pwr = [n for n in POWER_ORDER if n not in long_pwr]
    return [('ic', ic), ('local power', local_pwr), ('module', dk), ('long power', long_pwr), ('rest', rest)]


if __name__ == '__main__':
    b = Board()
    rep = {'routed': [], 'failed': []}
    t0 = time.time()
    for name, nets in full_order(b):
        for net in nets:
            route_net(b, net, rep, verbose=False)
        print('%-12s %3d nets done %4.0fs  failed so far %s' % (name, len(nets), time.time() - t0, rep['failed']))
    save(b, os.path.join(M.HERE, 'data', 'routes.json'))
    render(b, os.path.join(M.HERE, 'data', 'route_preview.png'), 0.4)


def render_labeled(routes, out_png, layer, scale=0.4, min_len=150):
    """Debug view: one layer, every net's segments coloured per net and labelled on its longest segment."""
    from PIL import Image, ImageDraw, ImageFont
    import colorsys
    b = Board()
    s, m = scale, 30
    img = Image.new('RGB', (int(M.W * s) + 2 * m, int(M.H * s) + 2 * m), (10, 30, 15))
    g = ImageDraw.Draw(img, 'RGBA')
    X = lambda v: m + v * s
    Y = lambda v: m + (M.H - v) * s
    try:
        font = ImageFont.truetype('arialbd.ttf', 11)
    except OSError:
        font = ImageFont.load_default()
    g.rectangle([X(0), Y(M.H), X(M.W), Y(0)], outline=(220, 220, 220), width=2)
    for p in b.pads:
        if layer in p.layers:
            g.polygon([(X(x), Y(y)) for x, y in p.poly], fill=(90, 80, 40, 255))
    for idx, (net, r) in enumerate(sorted(routes.items())):
        hue = (idx * 0.137) % 1.0
        c = tuple(int(255 * v) for v in colorsys.hsv_to_rgb(hue, 0.85, 1.0)) + (220,)
        best = None
        for (lay, x1, y1, x2, y2, w) in r['segs']:
            if lay != layer:
                continue
            g.line([X(x1), Y(y1), X(x2), Y(y2)], fill=c, width=max(1, int(w * s)))
            L = math.hypot(x2 - x1, y2 - y1)
            if best is None or L > best[0]:
                best = (L, (x1 + x2) / 2, (y1 + y2) / 2)
        for (vx, vy, d1, d2) in r['vias']:
            rr = d2 * s / 2
            g.ellipse([X(vx) - rr, Y(vy) - rr, X(vx) + rr, Y(vy) + rr], outline=(255, 255, 255), fill=c)
        if best and best[0] >= min_len:
            g.text((X(best[1]) + 3, Y(best[2]) - 6), net, fill=(255, 255, 255), font=font)
    img.save(out_png)
