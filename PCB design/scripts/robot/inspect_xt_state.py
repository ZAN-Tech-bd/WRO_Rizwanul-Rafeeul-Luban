"""Inspect the captured saved-state probe: XT pad/hole state and footprint bindings."""
import base64
import json
from pathlib import Path
import subprocess
import zipfile

OUT = Path(__file__).resolve().parents[1] / 'Robot/WRO_Robot_state_probe_20261028-1142.epro2'
ROBOT = '643ce99542452c8f'
FOOTPRINTS = {'eac10ca20e6778ba', '9e78c3c3ca74ad42', '9197bb2e6c38da76'}


def pad_id(header_id):
    return json.loads(header_id)[-1] if header_id.startswith('[') else header_id


def export():
    assert not OUT.exists(), OUT
    s = subprocess.run(['node', 'tools/easyeda-bridge/eda.mjs', 'exec', '-f',
                        str(Path(__file__).resolve().parent / '51_export_project.js')],
                       capture_output=True, text=True, check=True).stdout
    v = json.loads(s)
    v = json.loads(v) if isinstance(v, str) else v
    blob = base64.b64decode(v['b64'])
    OUT.write_bytes(blob)
    assert zipfile.ZipFile(OUT).testzip() is None


def inspect():
    with zipfile.ZipFile(OUT) as z:
        assert z.testzip() is None
        text = z.read(next(n for n in z.namelist() if n.endswith('.epru'))).decode('utf-8')
    docs = {}
    decoder = json.JSONDecoder()
    current = None
    for line in text.splitlines():
        left, sep, right = line.partition('||')
        if not sep or not right.startswith('{'):
            continue
        h = json.loads(left)
        b, _ = decoder.raw_decode(right)
        if h['type'] == 'DOCHEAD':
            current = b.get('uuid')
            docs.setdefault(current, [])
        elif current:
            docs[current].append((pad_id(h['id']), h['type'], b))
    for uuid in FOOTPRINTS:
        pads = [(b['num'], b['hole']['width'], b['defaultPad']['width'])
                for _, t, b in docs.get(uuid, []) if t == 'PAD']
        print('footprint', uuid, 'pads:', pads)
    pcb = docs[ROBOT]
    des = {b['parentId']: b['value'] for _, t, b in pcb if t == 'ATTR' and b.get('key') == 'Designator'}
    for _, t, b in pcb:
        if t == 'ATTR' and b.get('key') == 'Footprint':
            d = des.get(b['parentId'])
            if d in {'J1', 'J2', 'J3', 'J4'}:
                uuid = b['value']
                print('binding', d, '->', uuid, 'KNOWN' if uuid in FOOTPRINTS else '*** REBOUND TO NEW/OTHER DOC ***')
    print('probe:', OUT)


if __name__ == '__main__':
    inspect()
