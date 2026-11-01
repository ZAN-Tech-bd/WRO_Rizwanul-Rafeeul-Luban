"""Compare the probe export (after batch 3) vs the user_edited archive: which tracks changed?"""
import collections

import repaired_hub as R

A = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_user_edited_20261027-2358.epro2'
B = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_probe_after_batch3_20261028-0004.epro2'
pc = R.TARGET['pcb']
a, b = R.read_archive(A), R.read_archive(B)
cnt = collections.Counter()
for k in set(a[pc]['records']) | set(b[pc]['records']):
    x, y = a[pc]['records'].get(k), b[pc]['records'].get(k)
    if x == y:
        continue
    cnt[k[0] + ('_mod' if x and y else '_del' if x else '_add')] += 1
    if k[0] == 'LINE':
        src = (y or x)['body']
        print('LINE', k[1], 'mod' if x and y else 'del' if x else 'add', src.get('netName'),
              (src.get('startX'), src.get('startY'), src.get('endX'), src.get('endY')), src.get('width'))
        if x and y:
            print('   arch:', (x['body']['startX'], x['body']['startY'], x['body']['endX'], x['body']['endY'], x['body']['width']))
    elif k[0] in {'VIA', 'COMPONENT', 'PAD', 'HOLE'}:
        print(k[0], k[1])
    else:
        print('other:', k[0], k[1])
print('classes:', dict(cnt))
