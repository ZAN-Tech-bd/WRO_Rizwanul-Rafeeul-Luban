# Robot carrier v0.6 PCB — board model for the offline router (pcb-pipeline template, attendance rev C engine).
# Absolute pad copper (exact footprint polygons from the LCSC footprint files, placed per layout_plan.PLAN, nets from
# the EDA inventory), mounting holes, footprint NPTH holes, board edge and keep-out zones, rasterised on a grid.
# Every pad centre is cross-checked against EasyEDA's own pad positions (data/pcb_inventory_placed.json).
import json
import math
import os

import numpy as np
from matplotlib.path import Path

import fp_geometry
import layout_plan as LP

HERE = LP.HERE
RES = 5.0                      # mil per grid cell
W, H = LP.W, LP.H
NX, NY = int(round(W / RES)) + 1, int(round(H / RES)) + 1
TOP, BOT = 0, 1                # router layers -> EasyEDA layers 1 / 2


def rot(x, y, r):
    c, s = math.cos(math.radians(r)), math.sin(math.radians(r))
    return x * c - y * s, x * s + y * c


PENDING = []   # (pad, inventory net, planned net) for every override applied (none on the robot)


INVENTORY = os.environ.get('PCB_INVENTORY', 'pcb_inventory_v06_placed.json')   # 20_pcb_inventory.js output to model


def load_inventory_nets():
    d = json.loads(open(os.path.join(HERE, 'data', INVENTORY), encoding='utf-8').read())
    if isinstance(d, str):
        d = json.loads(d)
    nets, pos = {}, {}
    for line in d['lines']:
        f = line.split('|')
        des = f[0]
        x, y = (float(v) for v in f[2].split(','))
        for p in f[6].split(';'):
            if not p:
                continue
            num, rest = p.split(':', 1)
            net, xy = rest.rsplit('@', 1)
            dx, dy = (float(v) for v in xy.split(','))
            nets[(des, num)] = '' if net == '-' else net
            pos[(des, num)] = (x + dx, y + dy)
    return nets, pos


class Pad:
    # hole = drill diameter, or a slot's length; slot = the slot's width (0 = round), its length runs along the pad
    __slots__ = ('des', 'num', 'net', 'layers', 'poly', 'cx', 'cy', 'w', 'h', 'tht', 'hole', 'slot')

    def __init__(self, **k):
        for a, v in k.items():
            setattr(self, a, v)

    @property
    def minor(self):
        return min(self.w, self.h)

    def __repr__(self):
        return '%s.%s(%s)' % (self.des, self.num, self.net)


# Pads changed on PCB1 itself (not in the library footprint), D-031 (44_xt_pads.js): J1 XT60 pad 5.5 mm / hole 4.0 mm,
# J2-J4 XT30 pad 3.5 mm / hole 2.5 mm. des -> {pad number: shape}, PAD_HOLE des -> drill (mil).
_MM = 1 / 0.0254
PAD_SHAPE = {'J1': {n: ['ELLIPSE', 5.5 * _MM, 5.5 * _MM] for n in ('1', '2')},
             **{d: {n: ['ELLIPSE', 3.5 * _MM, 3.5 * _MM] for n in ('1', '2')} for d in ('J2', 'J3', 'J4')}}
PAD_HOLE = {'J1': 4.0 * _MM, 'J2': 2.5 * _MM, 'J3': 2.5 * _MM, 'J4': 2.5 * _MM}


def build():
    lib, parts_by_lcsc = fp_geometry.load()
    lcsc_of = {d: l for l, ds in parts_by_lcsc.items() for d in ds}
    nets, epos = load_inventory_nets()
    pads, holes, problems = [], [], []
    for des, (px, py, pr) in LP.PLAN.items():
        f = lib[lcsc_of[des]]
        # pad numbers used by several pads (U5.39 = 9 sub-pads of the module's GND pad): EasyEDA's inventory keeps one
        # position per number, so those are checked once, as "the inventory position matches one of the sub-pads"
        multi = {}
        for p in f['pads']:
            multi[p['num']] = multi.get(p['num'], 0) + 1
        multi_pos = {}
        for p in f['pads']:
            ax, ay = rot(p['x'], p['y'], pr)
            cx, cy = px + ax, py + ay
            poly = []
            shape = PAD_SHAPE.get(des, {}).get(p['num'])
            src = fp_geometry._shape_polygon(shape, p['x'], p['y'], p['rot']) if shape else p['poly']
            for qx, qy in src:
                rx, ry = rot(qx, qy, pr)
                poly.append((px + rx, py + ry))
            xs, ys = [q[0] for q in poly], [q[1] for q in poly]
            key = (des, p['num'])
            if key not in nets:
                problems.append('no net info for %s.%s' % key)
            elif multi[p['num']] > 1:
                multi_pos.setdefault(p['num'], []).append((cx, cy))
            else:
                ex, ey = epos[key]
                if abs(ex - cx) > 1.5 or abs(ey - cy) > 1.5:
                    problems.append('pad %s.%s at (%.1f, %.1f), EDA says (%.1f, %.1f)' % (des, p['num'], cx, cy, ex, ey))
            tht = p['layer'] == 12
            pads.append(Pad(des=des, num=p['num'], net=nets.get(key, ''), layers=(TOP, BOT) if tht else (TOP,),
                            poly=poly, cx=cx, cy=cy, w=max(xs) - min(xs), h=max(ys) - min(ys), tht=tht, hole=PAD_HOLE.get(des, p['hole']),
                            slot=p.get('slot', 0.0)))
        for num, pts in multi_pos.items():
            ex, ey = epos[(des, num)]
            if min(max(abs(ex - qx), abs(ey - qy)) for qx, qy in pts) > 1.5:
                problems.append('pad %s.%s (%d sub-pads): none at EDA (%.1f, %.1f)' % (des, num, len(pts), ex, ey))
        for hpos in f['holes']:
            ax, ay = rot(hpos['x'], hpos['y'], pr)
            holes.append({'x': px + ax, 'y': py + ay, 'd': hpos['d'], 'ring': 0.0, 'name': des + ' NPTH'})
    for name, hx, hy, drill, pad, keep, plated in LP.HOLES:
        holes.append({'x': hx, 'y': hy, 'd': drill, 'ring': pad if plated else 0.0, 'name': name})
    return pads, holes, problems


# ---------------------------------------------------------------- rasterisation helpers
GX = np.arange(NX) * RES
GY = np.arange(NY) * RES


def poly_mask(poly, grow=0.0):
    """Boolean mask (NY, NX) of the cells whose centre lies inside the polygon grown by `grow` mil."""
    xs, ys = [q[0] for q in poly], [q[1] for q in poly]
    i0, i1 = max(0, int((min(xs) - grow) / RES) - 1), min(NX - 1, int((max(xs) + grow) / RES) + 2)
    j0, j1 = max(0, int((min(ys) - grow) / RES) - 1), min(NY - 1, int((max(ys) + grow) / RES) + 2)
    m = np.zeros((NY, NX), bool)
    if i1 < i0 or j1 < j0:
        return m
    xx, yy = np.meshgrid(GX[i0:i1 + 1], GY[j0:j1 + 1])
    pts = np.column_stack([xx.ravel(), yy.ravel()])
    path = Path(poly + [poly[0]])
    inside = path.contains_points(pts, radius=2 * grow if grow else 0.0).reshape(xx.shape)
    m[j0:j1 + 1, i0:i1 + 1] = inside
    return m


def disc_mask(cx, cy, r):
    m = np.zeros((NY, NX), bool)
    i0, i1 = max(0, int((cx - r) / RES) - 1), min(NX - 1, int((cx + r) / RES) + 2)
    j0, j1 = max(0, int((cy - r) / RES) - 1), min(NY - 1, int((cy + r) / RES) + 2)
    if i1 < i0 or j1 < j0:
        return m
    xx, yy = np.meshgrid(GX[i0:i1 + 1], GY[j0:j1 + 1])
    m[j0:j1 + 1, i0:i1 + 1] = (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r
    return m


def boost_rect(pads):
    """Boost block keep-out rectangle: pads of LP.BOOST_PARTS + LP.BOOST_MARGIN (far off-board when there is none)."""
    if not LP.BOOST_PARTS:
        return (-1e5, -1e5, -1e5 + 1, -1e5 + 1)
    xs = [q[0] for p in pads if p.des in LP.BOOST_PARTS for q in p.poly]
    ys = [q[1] for p in pads if p.des in LP.BOOST_PARTS for q in p.poly]
    m = LP.BOOST_MARGIN
    return (min(xs) - m, min(ys) - m, max(xs) + m, max(ys) + m)


def module_body(pads):
    """Area between the module's pad rows (LP.BODY_PART), LP.BODY_MARGIN inside the pad ends, up to the antenna
    keep-out: top layer = LP.BODY_NETS only (spec §7.2). Rows found from the pads' positions relative to the centre.
    Far off-board when the board has no such rule (LP.BODY_PART None)."""
    if not LP.BODY_PART:
        return (-1e5, -1e5, -1e5 + 1, -1e5 + 1)
    mp = [p for p in pads if p.des == LP.BODY_PART and (p.des, p.num) not in LP.EP_PADS]
    cx = sum(p.cx for p in mp) / len(mp)
    left = max(max(q[0] for q in p.poly) for p in mp if p.cx < cx - 300)
    right = min(min(q[0] for q in p.poly) for p in mp if p.cx > cx + 300)
    bottom = max(max(q[1] for q in p.poly) for p in mp if abs(p.cx - cx) <= 300)
    m = LP.BODY_MARGIN
    return (left + m, bottom + m, right - m, LP.ANTENNA[1])


def rect_dist(rect):
    """Distance (mil) from every cell centre to the rectangle, 0 inside."""
    x0, y0, x1, y1 = rect
    jj, ii = np.mgrid[0:NY, 0:NX]
    dx = np.maximum(np.maximum(x0 - ii * RES, 0), ii * RES - x1)
    dy = np.maximum(np.maximum(y0 - jj * RES, 0), jj * RES - y1)
    return np.hypot(dx, dy)


def zone_mask(rect):
    x0, y0, x1, y1 = rect
    m = np.zeros((NY, NX), bool)
    m[max(0, int(math.ceil(y0 / RES))):int(y1 / RES) + 1, max(0, int(math.ceil(x0 / RES))):int(x1 / RES) + 1] = True
    return m


if __name__ == '__main__':
    pads, holes, problems = build()
    print('pads', len(pads), 'holes', len(holes), 'problems', len(problems))
    for p in problems[:20]:
        print('  ', p)
    nets = sorted(set(p.net for p in pads if p.net))
    print('nets', len(nets))
    print('net overrides:', len(PENDING))
