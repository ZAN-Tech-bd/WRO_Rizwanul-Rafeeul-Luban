"""Report the user's manual edits: record classes, geometry, and safety gates before re-pour."""
import collections
import json

import repaired_hub as R

BEFORE = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_user_editing_no_gnd_20261027-2323.epro2'
AFTER = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_user_edited_20261027-2358.epro2'
PCB = R.TARGET['pcb']


def pad_nets(docs):
    recs = docs[PCB]['records']
    return collections.Counter(
        (json.loads(rid)[2], b['padNet']) for (t, rid), r in recs.items()
        if t == 'PAD_NET' for b in [r['body']])


def main():
    base, new = R.read_archive(BEFORE), R.read_archive(AFTER)
    classes = collections.Counter()
    details = []
    for k in set(base[PCB]['records']) | set(new[PCB]['records']):
        a, b = base[PCB]['records'].get(k), new[PCB]['records'].get(k)
        if a == b:
            continue
        classes[k[0] + ('-' if a and b else '_removed' if a else '_added')] += 1
        if k[0] in {'LINE', 'VIA', 'COMPONENT', 'PAD', 'POLY', 'ARC', 'STRING', 'ATTR', 'HOLE'}:
            details.append([k[0], k[1], 'mod' if a and b else 'del' if a else 'add'])
    print('edit classes:', dict(classes))
    print('changed records:', len(details))
    for d in details[:40]:
        print('  ', d)
        if d[0] == 'LINE' and d[2] in {'mod', 'add'}:
            b = new[PCB]['records'][('LINE', d[1])]['body']
            print('     line:', b.get('netName'), 'L', b.get('layerId'), 'w', b.get('width'),
                  (b.get('startX'), b.get('startY')), (b.get('endX'), b.get('endY')))
        elif d[0] == 'COMPONENT' and d[2] in {'mod', 'add'}:
            b = new[PCB]['records'][('COMPONENT', d[1])]['body']
            attrs = {r['body']['key']: r['body']['value'] for (t, _), r in new[PCB]['records'].items()
                     if t == 'ATTR' and r['body'].get('parentId') == d[1]}
            print('     comp:', attrs.get('Designator'), (b.get('x'), b.get('y')), 'rot', b.get('angle'))
    assert not [d for d in details if d[0] in {'PAD', 'HOLE'}], 'pad/hole records changed!'
    old_nets, new_nets = pad_nets(base), pad_nets(new)
    assert old_nets == new_nets, 'pad-net multiset changed!'
    print('pad-net multiset unchanged:', sum(new_nets.values()), 'rows')

    m = R.copper_model(AFTER)
    assert len(m['fills']) == 0, 'unexpected fills in edited state'
    # foreign copper geometry gates on the edited state
    from shapely.geometry import box
    ant = box(1475, 3885.196850393701, 2595, 4075)
    hits = [i['label'] for i in m['items'] if i['shape'].intersects(ant)]
    print('antenna projection copper:', hits or 'clear')
    its, tree = m['items'], None
    from shapely.strtree import STRtree
    tree = STRtree([i['shape'] for i in its])
    close = [(round(a['shape'].distance(its[int(j)]['shape']), 2), a['label'], its[int(j)]['label'])
             for i, a in enumerate(its) for j in tree.query(a['shape'].buffer(6))
             if j > i and a['net'] != its[int(j)]['net'] and set(a['layers']) & set(its[int(j)]['layers'])
             and a['shape'].distance(its[int(j)]['shape']) < 5.99]
    print('copper pairs <6 mil:', len(close), sorted(close)[:6])
    r5 = [p for p in m['pads'] if p['des'] == 'R5']
    mod = box(500, 30, 2460, 1040)
    print('R5 still outside module:', all(not mod.intersects(p['shape']) for p in r5))


if __name__ == '__main__':
    main()
