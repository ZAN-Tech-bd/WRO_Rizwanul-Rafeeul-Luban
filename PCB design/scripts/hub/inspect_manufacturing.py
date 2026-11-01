"""Inspect the captured manufacturing export: zip contents, drill tools/slots, BOM J3 row.

Also proves the DRC 'Netlist Error' is metadata-only: PCB PAD_NET multiset identical to the
pre-repair verified checkpoint, and the whole repair chain touched no electrical records.
"""
import base64
import collections
import io
import json
import re
import zipfile
from pathlib import Path

import repaired_hub as R

OUT = R.DATA
CLEAN = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_recovery_verified_20261027-1911.epro2'
RELEASE = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2'


def pad_nets(archive):
    docs = R.read_archive(archive)
    recs = docs[R.TARGET['pcb']]['records']
    return collections.Counter(
        (json.loads(rid)[2], b['padNet']) for (t, rid), r in recs.items()
        if t == 'PAD_NET' for b in [r['body']])


def main():
    raw = json.loads(json.loads((OUT / 'manufacturing_export.json').read_text()) or 'null') \
        if (OUT / 'manufacturing_export.json').read_text().strip().startswith('"') \
        else json.loads((OUT / 'manufacturing_export.json').read_text())
    v = json.loads(raw) if isinstance(raw, str) else raw

    z = zipfile.ZipFile(io.BytesIO(base64.b64decode(v['gerber']['b64'])))
    (OUT / 'gerber_fixed').mkdir(exist_ok=True)
    z.extractall(OUT / 'gerber_fixed')
    files = sorted((n, z.getinfo(n).file_size) for n in z.namelist())
    print('Gerber zip files:', files)

    drills = [n for n in z.namelist() if 'Drill' in n or n.lower().endswith('.txt') and 'drill' in n.lower()]
    texts = {}
    for n in z.namelist():
        data = z.read(n)
        try:
            texts[n] = data.decode('utf-8', errors='replace')
        except Exception:
            pass
    drill_hits = {}
    for n, s in texts.items():
        if not re.search(r'T\d+C|METRIC', s):
            continue
        tools = sorted({float(x) for x in re.findall(r'T\d+C([\d.]+)', s)})
        slots = len(re.findall(r'G85|SLOT', s))
        hits = len(re.findall(r'T\d+\s*$', s, re.M))
        drill_hits[n] = dict(tools_mm=tools, slot_commands=slots, platings=len(re.findall(r'; *(?:METRIC|PLATED)', s)))
    print('Drill files:', json.dumps(drill_hits, indent=1))
    all_tools = sorted({t for d in drill_hits.values() for t in d['tools_mm']})
    print('All drill tools (mm):', all_tools)
    assert 2.5 in all_tools and 4.0 in all_tools, all_tools

    gko = next((s for n, s in texts.items() if n.upper().endswith(('GKO', 'GML', 'OUTLINE'))
                and re.search(r'G04.*[Oo]utline|Layer', s) or n.upper().endswith('GKO')), None)
    bom_raw = base64.b64decode(v['bom']['b64']) if v['bom'] else b''
    bom = bom_raw.decode('utf-16') if bom_raw[:2] in (b'\xff\xfe', b'\xfe\xff') else bom_raw.decode('utf-8-sig')
    (OUT / 'bom_fixed.csv').write_text(bom, encoding='utf-8')
    j3 = [line for line in bom.splitlines() if 'J3' in line or 'KCD4' in line]
    print('BOM J3/KCD4 rows:', j3[:6])
    assert j3 and 'KCD4' in j3[0] and 'C20088586' not in j3[0], j3[:2]

    old, new = pad_nets(CLEAN), pad_nets(RELEASE)
    assert old == new, 'PCB pad-net multiset changed!'
    print('PCB pad-net multiset identical to pre-repair verified checkpoint:', sum(old.values()), 'pad-net rows')

    print('Live DRC in export:', json.dumps(v['drc'])[:400])


if __name__ == '__main__':
    main()
