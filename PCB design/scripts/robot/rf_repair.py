"""Scoped +3V3 reroute out of the DevKit antenna overhang (hub 77_repair_antenna_route method).

The user's outline change (top edge 3840 -> 3900) put the module's radiating element
on-board (y >= module top edge). The +3V3 spine at y=3850 crosses it for x=560..1520.
This module derives the ACTUAL antenna box from the saved geometry, verifies the three
source segments byte-exactly, and produces a 3-segment same-net/layer modification that
dips the crossing into the pin1..module-top band at the class width 0.8 mm
(1.0 mm does not fit between U1B.1 and the antenna; 0.8 mm was the routed-class width,
D-032). Gates in test_rf_repair.py.
"""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'hub'))
import final_ground as FG

ARCHIVE = HERE.parents[1] / 'Robot/WRO_Robot_xt_verified_20261028-1351.epro2'
SEGMENTS = {
    'de82450445d8f812': ((470, 3760), (560, 3850)),   # U1A.1 escape (diagonal into the strip)
    '08b2b30513a78cdc': ((555, 3850), (560, 3850)),   # 5 mil bridge
    '2f5e6710791f7cf7': ((560, 3850), (1765, 3850)),  # 30.6 mm spine in the overhang
}
WIDTH_MIL = 31.4961            # 0.8 mm PWR_3V3 spine class (D-032)
PIN1_TO_MODULE_TOP = 75.2      # 1.91 mm drawing offset, hub test_repair_rf precedent
ANTENNA_LEN = 246              # 6.24 mm radiating element
GAP_ANT = 4.0                  # mil, new path to the actual antenna box (hub gate)
GAP_COPPER = 6.0               # mil, new path to any foreign item (design clearance)
EDGE_MARGIN = 60               # mil beyond each socket column (module width + margin)
RISE_X = 1540                  # past the antenna box right edge before rising back to y=3850


def antenna_box(model):
    """Actual radiating projection: module top edge .. +6.24 mm, module width + margin."""
    from shapely.geometry import box
    u1a1 = next(p for p in model['pads'] if p['des'] == 'U1A' and p['num'] == '1')
    u1b1 = next(p for p in model['pads'] if p['des'] == 'U1B' and p['num'] == '1')
    pin1_y = max(u1a1['center'][1], u1b1['center'][1])
    top = pin1_y + PIN1_TO_MODULE_TOP
    cols = sorted((u1a1['center'][0], u1b1['center'][0]))
    return box(cols[0] - EDGE_MARGIN, top, cols[1] + EDGE_MARGIN, top + ANTENNA_LEN)


def _source_guard(model, rid):
    body = model['records'][('LINE', rid)]['body']
    guard = dict(net=body['netName'], layerId=body['layerId'], width=body['width'],
                 startX=body['startX'], startY=body['startY'],
                 endX=body['endX'], endY=body['endY'])
    expected = SEGMENTS[rid]
    if (guard['net'], guard['layerId']) != ('+3V3', 1):
        raise ValueError(f'{rid}: unexpected net/layer {guard}')
    if (guard['startX'], guard['startY'], guard['endX'], guard['endY']) != (
            expected[0][0], expected[0][1], expected[1][0], expected[1][1]):
        raise ValueError(f'{rid}: source endpoints changed; re-derive the plan')
    return guard


def corridor(model):
    """y window where a WIDTH_MIL track clears U1B.1 (>=6) and the antenna (>=4)."""
    u1b1 = next(p for p in model['pads'] if p['des'] == 'U1B' and p['num'] == '1')
    pad_top = u1b1['shape'].bounds[3]
    ant_bottom = antenna_box(model).bounds[1]
    half = WIDTH_MIL / 2
    lo, hi = pad_top + GAP_COPPER + half, ant_bottom - GAP_ANT - half
    if lo > hi:
        raise ValueError(f'no corridor: pad {pad_top} antenna {ant_bottom} width {WIDTH_MIL}')
    return (lo + hi) / 2


def build_plan(model):
    y = corridor(model)
    path = {
        'de82450445d8f812': ((470, 3760), (560, y)),      # escape diagonal, stops in the band
        '08b2b30513a78cdc': ((RISE_X, y), (1765, 3850)),  # rise back to the spine end
        '2f5e6710791f7cf7': ((560, y), (RISE_X, y)),      # horizontal crossing below the antenna
    }
    modify = {}
    for rid, ((sx, sy), (ex, ey)) in path.items():
        modify[rid] = dict(source=_source_guard(model, rid),
                           startX=sx, startY=round(sy, 4), endX=ex, endY=round(ey, 4),
                           lineWidth=WIDTH_MIL)
    return dict(net='+3V3', layerId=1, widthMil=WIDTH_MIL, modify=modify,
                antennaBox=[round(v, 3) for v in antenna_box(model).bounds])


def applied_items(model, plan):
    from shapely.geometry import LineString
    out = []
    for rid, e in plan['modify'].items():
        a = (e['startX'], e['startY'])
        z = (e['endX'], e['endY'])
        shape = (LineString([a, z]) if a != z else FG.Point(a)).buffer(
            plan['widthMil'] / 2, quad_segs=FG.QS)
        out.append(dict(kind='track', id=rid, net=plan['net'], layers=(plan['layerId'],),
                        shape=shape, label=f"{plan['net']} track {rid}", start=a, end=z,
                        width=plan['widthMil']))
    return out


def main():
    import robot_release as R
    model = R.copper_model(ARCHIVE)
    plan = build_plan(model)
    data = HERE / 'data/release_20261028'
    data.mkdir(parents=True, exist_ok=True)
    out = data / 'rf_plan.json'
    assert not out.exists(), 'refusing to overwrite an existing RF plan'
    out.write_text(json.dumps(plan, indent=2), encoding='utf-8')
    source = (HERE / '55_fix_antenna_route.template.js').read_text(encoding='utf-8')
    writer = HERE / 'data/55_fix_antenna_route.js'
    writer.write_text(source.replace('__PLAN__', json.dumps(plan, separators=(',', ':'))),
                      encoding='utf-8')
    print(json.dumps(dict(planFile=str(out), writer=str(writer), y=round(corridor(model), 3),
                          antenna=plan['antennaBox']), indent=2))


if __name__ == '__main__':
    main()
