"""KCD4-203-B drawing-based prototype, millimetres except serialized EDA records.

Original drawing and user-confirmed dimensions: WIRING-PLAN section 2.4.
The maximum blade rectangle is checked against rounded, not rectangular, slot ends.
"""
from copy import deepcopy
import json
from math import hypot
from pathlib import Path
import zipfile

HERE = Path(__file__).resolve().parent
BACKUP = HERE.parent.parent / 'hub/WRO_HUB_before_direct_rocker_20261026.epro2'
MM = 1 / 0.0254
NAME = 'KCD4-203-B_DIRECT_P10.5_R11_SLOT7.8x1.8'
CONTACTS = {
    '1': ('BAT+', -10.5, 5.5), '2': ('V12', 0.0, 5.5), '3': ('DC+', 10.5, 5.5),
    '4': ('BAT-', -10.5, -5.5), '5': ('GND', 0.0, -5.5), '6': ('DC-', 10.5, -5.5),
}
SLOT_MM = (1.8, 7.8)  # x / y; the broad blade direction is across the poles
PAD_MM = (3.0, 9.0)
FLANGE_MM = (34.5, 25.5)
COURTYARD_MM = (36.5, 27.5)


def blade_fits(length, width, blade_width, blade_thickness, dx=0.0, dy=0.0):
    """Rectangle-in-capsule test; dx is along the slot, dy across it."""
    if width <= 0 or length < width:
        return False
    radius = width / 2
    straight_half = (length - width) / 2
    return all(
        hypot(max(abs(dx + sx * blade_width / 2) - straight_half, 0),
              abs(dy + sy * blade_thickness / 2)) <= radius
        for sx in (-1, 1) for sy in (-1, 1)
    )


def read_records(source):
    decoder = json.JSONDecoder()
    records = []
    for line in source.splitlines():
        a, separator, b = line.partition('||')
        if separator and b.startswith('{'):
            records.append((json.loads(a), decoder.raw_decode(b)[0]))
    return records


def backup_records():
    with zipfile.ZipFile(BACKUP) as archive:
        return read_records(archive.read('WRO_HUB.epru').decode('utf-8'))


def footprint_records(uuid):
    """Emit native record-format footprint using the verified archive's layer/pad schema."""
    all_records = backup_records()
    first = next(i for i, (a, b) in enumerate(all_records)
                 if a['type'] == 'DOCHEAD' and b.get('uuid') == 'ae8f5762b637161e')
    last = next(i for i in range(first + 1, len(all_records))
                if all_records[i][0]['type'] == 'DOCHEAD')
    old = all_records[first:last]
    records = [({'type': 'DOCHEAD'}, {
        'docType': 'FOOTPRINT', 'uuid': uuid, 'version': '1', 'editVersion': '3.2.209',
    })]
    for a, b in old:
        if a['type'] in ('LAYER', 'CANVAS'):
            a, b = deepcopy(a), deepcopy(b)
            if a['type'] == 'LAYER' and b['layerType'] in ('MECHANICAL', 'TOP_ASSEMBLY'):
                b.update(use=True, show=True)
            records.append((a, b))
    template = next(b for a, b in old if a['type'] == 'PAD')
    for number, (_, x, y) in CONTACTS.items():
        pad = deepcopy(template)
        pad.update(num=number, centerX=x * MM, centerY=y * MM, padAngle=0,
                   padOffsetX=0, padOffsetY=0, relativeAngle=90, plated=True,
                   netName='', specialPad=[], zIndex=0)
        # Native SLOT width = overall length, height = diameter. Rotate 90 degrees for the blade axis.
        pad['hole'] = dict(holeType='SLOT', width=SLOT_MM[1] * MM, height=SLOT_MM[0] * MM)
        pad['defaultPad'] = dict(padType='OVAL', width=PAD_MM[0] * MM, height=PAD_MM[1] * MM)
        records.append((dict(type='PAD', id='p' + number), pad))
    for key, value in [('Name', NAME), ('Designator', 'J?')]:
        attr = deepcopy(next(b for a, b in old if a['type'] == 'ATTR' and b.get('key') == key))
        attr.update(value=value, x=0, y=(14 if key == 'Designator' else -14) * MM,
                    angle=0, valueVisible=key == 'Designator', parentId='', fontSize=45)
        records.append((dict(type='ATTR', id='a' + key), attr))
    for tag, layer, (w, h) in [('flange', 3, FLANGE_MM), ('body', 9, (26.0, 21.8)),
                                ('courtyard', 14, COURTYARD_MM)]:
        x, y = w * MM / 2, h * MM / 2
        records.append((dict(type='POLY', id=tag), dict(
            groupId=0, netName='', layerId=layer, width=0.2 * MM,
            path=[-x, -y, 'L', x, -y, x, y, -x, y, -x, -y],
            locked=False, zIndex=0, polyType='NORMAL')))
    return records


def encode_records(records):
    return ''.join(json.dumps(a, separators=(',', ':')) + '||' +
                   json.dumps(b, separators=(',', ':')) + '|\n' for a, b in records)


def live_plan():
    """As built 2026-10-26 (H-001 retry 2, 53_rocker_local_moves.js): J3 rotated -90 in the left power column, left
    board edge -300 -> -620 mil (rectangular, user: "a bit more width"), only the overlapping parts moved.
    Supersedes the earlier (-720, 1450) + outline-tab plan."""
    cx, cy = -60, 1495
    return {
        'centerMil': [cx, cy],
        'rotation': -90,
        'pads': [dict(number=n, net=net, x=cx + y * MM, y=cy - x * MM,
                      padMm=[9.0, 3.0]) for n, (net, x, y) in CONTACTS.items()],
        'flangeBoundsMil': [cx - 25.5 * MM / 2, cy - 34.5 * MM / 2,
                            cx + 25.5 * MM / 2, cy + 34.5 * MM / 2],
        'courtyardBoundsMil': [cx - 27.5 * MM / 2, cy - 36.5 * MM / 2,
                               cx + 27.5 * MM / 2, cy + 36.5 * MM / 2],
        'outlineMil': [[-620, 0], [-620, 3900], [4600, 3900], [4600, 0], [-620, 0]],
        'moveOtherComponents': {'J4': [0, -195], 'J5': [0, -125], 'LED3': [450, 0]},
    }


if __name__ == '__main__':
    out = HERE / 'data/rocker_footprint_source_20261026.efoo'
    out.write_text(encode_records(footprint_records('rocker-prototype')), encoding='utf-8')
    (HERE / 'data/rocker_live_plan_20261026.json').write_text(json.dumps(live_plan(), indent=2), encoding='utf-8')
    print('Generated', out)
    print('Worst-case slot fit:', blade_fits(7.7, 1.7, 6.4, 0.83, 0.2, 0.2))
