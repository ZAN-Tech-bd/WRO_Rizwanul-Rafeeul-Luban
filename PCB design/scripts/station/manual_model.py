"""Fail-closed geometry adapter for a live snapshot, never the historical placement plan."""
import math

from shapely import affinity
from shapely.geometry import Point, Polygon, box


def ring(source):
    if not source or isinstance(source[0], list):
        raise ValueError('One straight outline ring required')
    if any(isinstance(v, str) and v != 'L' for v in source):
        raise ValueError('Curved/unknown polygon commands need a separate geometry model')
    numbers = [v for v in source if not isinstance(v, str)]
    if len(numbers) % 2:
        raise ValueError('Invalid coordinate pairs')
    return list(zip(numbers[::2], numbers[1::2]))


def build_model(snapshot):
    import route_model as M

    s = snapshot
    if s['project'] != 'station_pcb' or s['pours'] or s['regions']:
        raise ValueError('Wrong project or unmodelled native pours/keep-outs')
    if any(p['layer'] in (1, 2) for p in s['polylines']):
        raise ValueError('Copper polylines need explicit modelling')
    outlines = [p for p in s['polylines'] if p['layer'] == 11]
    if len(outlines) != 1:
        raise ValueError('Exactly one rectangular board outline required')
    board = Polygon(ring(outlines[0]['source']))
    if not board.is_valid or board.bounds[:2] != (0, 0) or not board.equals(box(*board.bounds)):
        raise ValueError('Nonrectangular/non-origin outline: do not substitute a bounding box')
    owners = {p['id']: (c['des'], p['num']) for c in s['components'] for p in c['pads']}
    absolute = {p['id']: p for p in s['pads']}
    if len(absolute) != len(s['pads']) or not set(owners).issubset(absolute):
        raise ValueError('Component pads missing or duplicated in absolute pad API')
    pads, holes = [], []
    for p in s['pads']:
        sh = p['shape']
        if not sh or sh[0] not in ('ELLIPSE', 'RECT') or sh[1] <= 0 or sh[2] <= 0:
            raise ValueError('Unmodelled pad shape: %r' % sh)
        if sh[0] == 'RECT' and len(sh) > 3 and sh[3]:
            raise ValueError('Rounded rectangle needs explicit modelling')
        if p['layer'] not in (1, 2, 12):
            raise ValueError('Unknown pad layer')
        if sh[0] == 'ELLIPSE':
            g = affinity.scale(Point(0, 0).buffer(1, quad_segs=32), sh[1] / 2, sh[2] / 2)
        else:
            g = box(-sh[1] / 2, -sh[2] / 2, sh[1] / 2, sh[2] / 2)
        g = affinity.translate(affinity.rotate(g, p['rotation'], origin=(0, 0)), p['x'], p['y'])
        h = p['hole']
        if h and (h[0] != 'ROUND' or len(h) != 2):
            raise ValueError('Use absolute round drill state, not the component-proxy drill')
        drill = float(h[1]) if h else 0.0
        if p['id'] not in owners:
            if not p['num'].startswith('H') or p['net'] or sh[0] != 'ELLIPSE' or sh[1] != sh[2] or not drill:
                raise ValueError('Unmodelled standalone pad')
            holes.append(dict(name=p['num'], x=p['x'], y=p['y'], d=drill,
                              ring=sh[1] if p['plated'] else 0.0))
            continue
        tht = p['layer'] == 12
        if tht and (not p['plated'] or not drill):
            raise ValueError('Unexpected nonplated component pad')
        des, num = owners[p['id']]
        x0, y0, x1, y1 = g.bounds
        pads.append(M.Pad(des=des, num=num, net=p['net'], layers=(0, 1) if tht else (p['layer'] - 1,),
                          poly=list(g.exterior.coords), cx=p['x'], cy=p['y'], w=x1-x0, h=y1-y0,
                          tht=tht, hole=drill, slot=0.0))
    return pads, holes, board


def configure(snapshot):
    """Use existing check/fill algorithms with live geometry, without modifying historical scripts."""
    import layout_plan as LP
    import route_model as M
    pads, holes, board = build_model(snapshot)
    LP.W, LP.H = M.W, M.H = board.bounds[2:]
    LP.PLAN = {c['des']: (c['x'], c['y'], c['rotation']) for c in snapshot['components']}
    M.build = lambda: (pads, holes, [])
    return pads, holes, board
