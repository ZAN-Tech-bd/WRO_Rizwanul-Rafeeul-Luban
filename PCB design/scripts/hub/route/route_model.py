# Hub v0.4 PCB — board model for the offline router, built from the LIVE board (data/live.json = ../20e_live_pads.js):
# every component pad with its real shape, rotation, hole and net as EasyEDA reports it (the user hand-placed the hub;
# J3 and the XT parts carry non-library pads). Model frame = EasyEDA + (LP.OX, LP.OY). Rasterised on a grid.
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


PENDING = []   # no net overrides on the hub


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


def build():
    """pads, holes, problems from the live dump. Component-pad holes come in 10-mil units (playbook §7)."""
    L = LP._live()
    pads, holes, problems = [], [], []
    for comp in L['comps']:
        for q in comp['pads']:
            x, y = q['x'] + LP.OX, q['y'] + LP.OY
            shape = q['pad']
            if not shape or shape[0] not in ('RECT', 'ELLIPSE', 'OVAL'):
                problems.append('%s.%s: pad shape %s' % (comp['des'], q['num'], shape))
                continue
            poly = fp_geometry._shape_polygon([shape[0], shape[1], shape[2]], x, y, q['rot'] or 0)
            hole, slot = 0.0, 0.0
            h = q['hole']
            if h and h[0] == 'ROUND':
                hole = float(h[1]) * 10
            elif h and h[0] == 'SLOT':
                slot, hole = sorted((float(h[1]) * 10, float(h[2]) * 10))   # width, length (along the pad's long axis)
            tht = q['layer'] == 12
            xs, ys = [v[0] for v in poly], [v[1] for v in poly]
            pads.append(Pad(des=comp['des'], num=q['num'], net=q['net'], layers=(TOP, BOT) if tht else (TOP,), poly=poly,
                            cx=x, cy=y, w=max(xs) - min(xs), h=max(ys) - min(ys), tht=tht, hole=hole, slot=slot))
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
    print('pads', len(pads), 'holes', len(holes), 'problems', len(problems), problems[:5])
    print('nets', len(set(p.net for p in pads if p.net)), 'board', LP.W, 'x', LP.H, 'OX', LP.OX, 'antenna', LP.ANTENNA)
