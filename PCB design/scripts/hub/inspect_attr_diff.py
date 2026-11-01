"""Show exactly which fields changed in the 25 schematic ATTR records."""
import collections

import repaired_hub as R

base = R.read_archive('PCB design/hub/WRO_HUB_REPAIRED_switch_instance_fixed_20261027-1914.epro2')
final = R.read_archive('PCB design/hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2')
page = R.TARGET['page']
j3 = 'f26ea52ce64d9e40'

changed = [k for k in base[page]['records']
           if base[page]['records'][k] != final[page]['records'][k] and k[0] == 'ATTR']
owners = collections.Counter(base[page]['records'][k]['body'].get('parentId') for k in changed)
print('Owners of changed ATTR records:', owners)

nonj3 = [k for k in changed if base[page]['records'][k]['body'].get('parentId') != j3]
print('Non-J3 changes:', nonj3)

for k in changed[:25]:
    a = base[page]['records'][k]['body']
    b = final[page]['records'][k]['body']
    diff = {f: [a.get(f), b.get(f)] for f in set(a) | set(b) if a.get(f) != b.get(f)}
    print(k, a.get('key'), diff)
