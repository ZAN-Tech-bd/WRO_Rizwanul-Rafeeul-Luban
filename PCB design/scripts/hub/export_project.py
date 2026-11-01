"""Run 51_export_project.js and save the EasyEDA project archive as hub/WRO_HUB_<tag>.epro2 (zip integrity checked).

Run: python export_project.py <tag>      e.g. python export_project.py before_rocker_v2_20261026
"""
import base64
import json
from pathlib import Path
import subprocess
import sys
import zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
HUB = HERE.parent.parent / 'hub'


def export(tag):
    out = HUB / f'WRO_HUB_{tag}.epro2'
    if out.exists():
        sys.exit(f'{out.name} exists: pick a new tag')
    raw = subprocess.run(['node', str(ROOT / 'tools/easyeda-bridge/eda.mjs'), 'exec', '-f',
                          str(HERE / '51_export_project.js')], capture_output=True, text=True, check=True).stdout
    data = json.loads(json.loads(raw))
    if 'b64' not in data:
        sys.exit(f'export failed: {data}')
    blob = base64.b64decode(data['b64'])
    assert len(blob) == data['size'], 'size mismatch'
    out.write_bytes(blob)
    with zipfile.ZipFile(out) as z:
        bad = z.testzip()
        names = z.namelist()
    if bad:
        sys.exit(f'corrupt entry {bad}')
    print(json.dumps({'file': str(out), 'bytes': len(blob), 'entries': names}))
    return out


if __name__ == '__main__':
    export(sys.argv[1])
