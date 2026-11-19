"""Current saved-board facts: XT identities/pads, antenna zone, outline, inventory, net widths."""
import collections
import json
import re
from pathlib import Path

import gnd_edit
import layout_plan as LP

HERE = Path(__file__).resolve().parent
ARCHIVE = HERE.parents[1] / 'Robot/WRO_Robot_user_edited_20261028-1059.epro2'
ROBOT = '643ce99542452c8f'


def main():
    print('ANTENNA zone (layout_plan):', LP.ANTENNA, 'strict:', getattr(LP, 'ANT_STRICT', None))
    print('BODY_PART:', getattr(LP, 'BODY_PART', None), 'BODY_NETS:', getattr(LP, 'BODY_NETS', None))
    docs = gnd_edit.read_archive(ARCHIVE)
    recs = docs[ROBOT]['records']
    attrs = collections.defaultdict(dict)
    for (t, i), r in recs.items():
        if t == 'ATTR' and r['body'].get('parentId'):
            attrs[r['body']['parentId']][r['body']['key']] = r['body']['value']
    comps = {i: r['body'] for (t, i), r in recs.items() if t == 'COMPONENT'}
    print('components:', len(comps))
    for cid, body in comps.items():
        des = attrs[cid].get('Designator')
        if des in {'J1', 'J2', 'J3', 'J4', 'J5'}:
            print(des, 'pos', (body['x'], body['y']), 'rot', body['angle'],
                  'Name:', attrs[cid].get('Name'), 'MPN:', attrs[cid].get('Manufacturer Part'),
                  'FP:', attrs[cid].get('Footprint'))
    for fp in {attrs[c].get('Footprint') for c in comps
               if attrs[c].get('Designator') in {'J1', 'J2', 'J3', 'J4'} and attrs[c].get('Footprint')}:
        pads = [(r['body']['num'], r['body']['defaultPad'], r['body'].get('hole'))
                for (t, i), r in docs[fp]['records'].items() if t == 'PAD']
        print('footprint', fp, 'pads:')
        for num, dp, hole in pads:
            print('   pad', num, dp, hole)
    nets = collections.Counter()
    widths = collections.defaultdict(collections.Counter)
    for (t, i), r in recs.items():
        if t == 'LINE' and r['body'].get('layerId') in (1, 2):
            b = r['body']
            nets[b['netName']] += 1
            widths[b['netName']][b['width']] += 1
    print('named nets:', len(nets))
    mm = lambda mil: round(mil * 0.0254, 3)
    for net in sorted(widths):
        levels = ', '.join(f'{w}mil({mm(w)}mm)x{n}' for w, n in sorted(widths[net].items()))
        print(f'  {net}: {levels}')
    via_sizes = collections.Counter()
    for (t, i), r in recs.items():
        if t == 'VIA':
            b = r['body']
            via_sizes[(b['holeDiameter'], b['viaDiameter'])] += 1
    print('via sizes:', {f'{h}/{d}': n for (h, d), n in via_sizes.items()})
    outline = [r['body'] for (t, i), r in recs.items() if t == 'POLY' and r['body'].get('layerId') == 11]
    print('outline records:', len(outline))
    for o in outline:
        nums = [v for v in o['path'] if not isinstance(v, str)]
        xs, ys = nums[0::2], nums[1::2]
        print('   bounds', min(xs), min(ys), max(xs), max(ys))
    pads_free = [(r['body'].get('netName'), r['body']['centerX'], r['body']['centerY'],
                  r['body'].get('hole', {})) for (t, i), r in recs.items() if t == 'PAD']
    print('free pads:', pads_free)


if __name__ == '__main__':
    main()
