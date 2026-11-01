"""Verify R5's saved move, then regenerate the GND plan for the moved geometry."""
import collections
import json

import repaired_hub as R

BASE = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_before_r5_20261027-2234.epro2'
NEW = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_r5_moved_20261027-2234.epro2'


def main():
    m = R.copper_model(NEW)
    r5 = [p for p in m['pads'] if p['des'] == 'R5']
    from shapely.geometry import box
    p1 = next(p for p in r5 if p['num'] == '1')
    p2 = next(p for p in r5 if p['num'] == '2')
    # pad1 touches the same-net track (connection); pad2 sits in the GND plane area
    touching = [i['label'] for i in m['items'] if i['net'] == 'SERVO_IN' and i['kind'] == 'track'
                and i['shape'].intersects(p1['shape'])]
    print('pad1 same-net overlap:', touching)
    assert touching, 'pad1 not connected to SERVO_IN copper'
    foreign = [i for i in m['items'] if i['net'] != 'SERVO_IN' and i['net'] != 'GND']
    for name, p in (('pad1', p1), ('pad2', p2)):
        bad = [(round(p['shape'].buffer(6).distance(i['shape']), 2), i['label'])
               for i in foreign if set((1, 2)) & set(i['layers'])
               and p['shape'].buffer(6).intersects(i['shape'])]
        assert not bad, (name, bad[:6])
    print('R5 pads clear foreign (non-GND) copper by >=6 mil')

    # export diff: only COMPONENT R5, its Designator ATTR, and the deleted stub LINE
    base, new = R.read_archive(BASE), R.read_archive(NEW)
    changed, removed, added = [], [], []
    for doc in base:
        for k in set(base[doc]['records']) | set(new[doc]['records']):
            a, b = base[doc]['records'].get(k), new[doc]['records'].get(k)
            if a == b:
                continue
            if a and b:
                changed.append((doc == R.TARGET['pcb'], k))
            elif a:
                removed.append((doc == R.TARGET['pcb'], k))
            else:
                added.append((doc == R.TARGET['pcb'], k))
    pcb_changed = [k for pcb, k in changed if pcb]
    assert all(k[0] in {'COMPONENT', 'ATTR', 'ELE_PLACEHOLDER'} for k in pcb_changed), pcb_changed
    assert len(removed) == 1 and removed[0][1][0] == 'LINE' and removed[0][1][1] == '39b1d74c0d7108f7', removed
    assert not added, added
    print('diff: changed', [str(k) for k in pcb_changed], 'removed', [str(k[1]) for k in removed], 'added 0')

    plan, report = R.regenerate_ground(NEW)
    print(json.dumps({k: v for k, v in report.items() if k != 'foreignOverlaps'}))
    assert not report['foreignOverlaps'] and not report['clearanceViolations'] and not report['disconnectedGnd']
    print('ground plan regenerated for moved R5:', plan['pieces'].__len__(), 'pieces')


if __name__ == '__main__':
    main()
