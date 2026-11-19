"""READ-ONLY: J1 component pose + pad positions from the saved station export."""
import json
import zipfile
from pathlib import Path

ARCHIVE = Path(__file__).resolve().parents[2] / 'station/station_silkfix_base_20261028.epro2'
PCB = 'fdcb20516f125209'

z = zipfile.ZipFile(ARCHIVE)
assert z.testzip() is None
text = z.read(next(n for n in z.namelist() if n.endswith('.epru'))).decode('utf-8')
dec = json.JSONDecoder()
cur, comp, padnets, outline = None, None, {}, None
for line in text.splitlines():
    left, sep, right = line.partition('||')
    if not sep or not right.startswith('{'):
        continue
    h = json.loads(left)
    b, _ = dec.raw_decode(right)
    if h['type'] == 'DOCHEAD':
        cur = b['uuid']
    elif cur != PCB:
        continue
    if h['type'] == 'COMPONENT':
        comp = b
    elif h['type'] == 'PAD_NET':
        padnets[h['id']] = b.get('padNet')
    elif h['type'] == 'POLY' and b.get('layerId') == 11:
        outline = b['path']

print('COMPONENT J1:', json.dumps(comp, indent=1))
print('OUTLINE:', outline)
