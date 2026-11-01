"""Acceptance gate: exactly the 37 allowlisted component poses may change in a full project export.

Run: python verify_station_placement.py AFTER.epro2 AFTER_INVENTORY.json AFTER_LIVE.json
"""
import collections
import json
import math
from pathlib import Path
import sys

from live_space import parse_inventory, board
from station_placement import BACKUP, DATA, TARGETS, source, placed, check
from test_sb_removal import archive, live, PCB


def verify(after_path, inventory_path, live_path):
    old, new = archive(BACKUP), archive(after_path)
    assert old.keys() == new.keys(), 'Project documents changed'
    before, after = old[PCB]['records'], new[PCB]['records']
    assert set(before) == set(after), 'Record added/deleted'
    des = {v['body']['parentId']:v['body']['value'] for (t,_),v in before.items()
           if t == 'ATTR' and v['body'].get('key') == 'Designator'}
    ids = {cid:d for cid,d in des.items() if d in TARGETS}
    changed_components, changed_attrs = [], []
    for key, record in before.items():
        kind, rid = key
        b, a = record['body'], after[key]['body']
        if kind == 'COMPONENT' and rid in ids:
            d = ids[rid]
            x,y,angle = TARGETS[d]
            assert (a['x'], a['y']) == (x,y), f'{d}: target not reached'
            assert a['angle'] % 360 == angle % 360, f'{d}: angle wrong'
            assert {k:v for k,v in a.items() if k not in {'x','y','angle'}} == {
                k:v for k,v in b.items() if k not in {'x','y','angle'}}, f'{d}: other component properties changed'
            if a != b:
                changed_components.append(d)
        elif kind == 'ATTR' and b.get('parentId') in ids:
            assert {k:v for k,v in a.items() if k not in {'x','y','angle'}} == {
                k:v for k,v in b.items() if k not in {'x','y','angle'}}, f'{key}: non-geometric attribute change'
            if a != b:
                changed_attrs.append(rid)
        else:
            assert after[key]['raw'] == record['raw'], f'Unapproved PCB change: {key}'
    assert set(changed_components) == set(TARGETS), 'Not exactly 37 component pose changes'
    for uuid in old.keys()-{PCB}:
        assert new[uuid]['records'] == old[uuid]['records'], f'Other project document changed: {uuid}'
    original, outline, holes = source()
    assert board(after_path) == (outline,holes), 'Outline or holes changed'
    planned = placed(original)
    actual = parse_inventory(Path(inventory_path))
    for d,p in original.items():
        if d not in TARGETS:
            assert actual[d] == p, f'Frozen {d} inventory changed'
        else:
            q = actual[d]
            assert (q['x'],q['y']) == TARGETS[d][:2]
            assert all(abs(q[k]-planned[d][k]) <= .2 for k in ['w','h','cx','cy']), f'{d}: actual body bounds differ from plan'
            for number,(net,dx,dy) in planned[d]['pads'].items():
                n,xx,yy = q['pads'][number]
                assert n == net and math.dist((dx,dy),(xx,yy)) <= .2, f'{d}.{number}: pad/net transform wrong'
    assert check(original,outline,holes) == [], 'Offline placement failed'
    old_live, new_live = live(DATA/'before_live.json'), live(live_path)
    assert len(new_live['components']) == len(old_live['components']) == 77
    assert new_live['nets'] == old_live['nets'], 'Nets changed'
    assert new_live['strings'] == old_live['strings'], 'Free silk changed'
    def leaves(x):
        if isinstance(x,list):
            return [q for p in x for q in leaves(p)]
        if isinstance(x,dict) and 'list' in x:
            return leaves(x['list'])
        return [x] if isinstance(x,dict) and 'errorType' in x else []
    def signature(e):
        return json.dumps({k:e.get(k) for k in ['errorType','errorObjType','ruleName','net','obj1','obj2','objs']},sort_keys=True)
    bc = collections.Counter(signature(e) for e in leaves(old_live['drc']))
    ac = collections.Counter(signature(e) for e in leaves(new_live['drc']))
    assert not ac-bc, f'New DRC findings: {ac-bc}'
    report = {'moved':sorted(changed_components),'movedCount':len(changed_components),'fixedCount':40,
              'ownedAttributesTransformed':len(changed_attrs),'copperAndOtherPcbRecordsUnchanged':True,
              'allOtherDocumentsUnchanged':True,'newDrcFindings':0,
              'drcBefore':[(x['name'],x['count']) for x in old_live['drc']],
              'drcAfter':[(x['name'],x['count']) for x in new_live['drc']]}
    (DATA/'verification.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=1))


if __name__ == '__main__':
    verify(Path(sys.argv[1]),Path(sys.argv[2]),Path(sys.argv[3]))
