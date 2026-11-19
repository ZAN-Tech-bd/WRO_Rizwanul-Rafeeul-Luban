"""Diff the pour-removed editing archive vs the user's finished edits; report what changed."""
import collections
from pathlib import Path

from gnd_edit import read_archive

ROBOT = '643ce99542452c8f'
HERE = Path(__file__).resolve().parent
BEFORE = HERE.parents[1] / 'Robot/WRO_Robot_editing_no_gnd_20261028-0250.epro2'
AFTER = HERE.parents[1] / 'Robot/WRO_Robot_user_edited_20261028-1059.epro2'


def main():
    a, b = read_archive(BEFORE), read_archive(AFTER)
    assert a.keys() == b.keys()
    classes = collections.Counter()
    detail = []
    for u in a:
        for k in set(a[u]['records']) | set(b[u]['records']):
            x = a[u]['records'].get(k)
            y = b[u]['records'].get(k)
            if x == y:
                continue
            state = 'mod' if x and y else 'del' if x else 'add'
            classes[(u == ROBOT, k[0], state)] += 1
            if k[0] in {'LINE', 'VIA', 'COMPONENT', 'PAD', 'PAD_NET', 'ATTR', 'STRING', 'POLY'}:
                detail.append((u, k, state, x, y))
    print('classes:', {str(k): v for k, v in sorted(classes.items())})
    print('detail records:', len(detail))
    for u, k, state, x, y in detail:
        print(u == ROBOT and 'PCB' or u, k, state)
        body = (y or x)['body']
        if k[0] == 'LINE' and u == ROBOT:
            print('   line:', body.get('netName'), 'L', body.get('layerId'), 'w', body.get('width'),
                  (body.get('startX'), body.get('startY')), (body.get('endX'), body.get('endY')))
        elif k[0] == 'VIA' and u == ROBOT:
            print('   via:', body.get('netName'), (body.get('centerX'), body.get('centerY')),
                  'hole', body.get('holeDiameter'), 'dia', body.get('viaDiameter'))
        elif k[0] == 'COMPONENT' and u == ROBOT:
            print('   comp:', (body.get('x'), body.get('y')), 'rot', body.get('angle'))
        elif k[0] == 'PAD_NET':
            print('   padnet:', body.get('padNet'))
    # footprint pad geometry of XT parts (D-034 open drill issue)
    for u in b:
        recs = b[u]['records']
        heads = {('META', 'META'): None}
        # find component designators for J1..J4 and their footprint uuids
        des = {}
        for (t, i), r in recs.items():
            if t == 'ATTR' and r['body'].get('key') == 'Designator':
                des[r['body']['parentId']] = r['body'].get('value')
        for (t, i), r in recs.items():
            if t == 'COMPONENT' and des.get(i) in {'J1', 'J2', 'J3', 'J4'}:
                print('XT comp', des[i], 'footprint attr:')
                for (t2, i2), r2 in recs.items():
                    if t2 == 'ATTR' and r2['body'].get('parentId') == i and r2['body'].get('key') == 'Footprint':
                        print('   ', r2['body']['value'])
    for u, doc in b.items():
        recs = doc['records']
        if doc['head'].get('docType') != 'FOOTPRINT':
            continue
        pads = [(r['body']['num'], r['body'].get('hole')) for (t, i), r in recs.items() if t == 'PAD']
        if pads and any(h and h.get('width', 0) > 60 for _, h in pads):
            print('footprint', u, 'big-hole pads:', pads)


if __name__ == '__main__':
    main()
