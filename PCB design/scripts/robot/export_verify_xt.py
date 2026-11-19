"""Export the Robot project and verify the XT footprint pad/hole corrections in the saved file."""
import base64
import json
from pathlib import Path
import subprocess
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'hub'))
from inspect_export import documents

HERE = Path(__file__).resolve().parent
OUT = HERE.parents[1] / 'Robot/WRO_Robot_xt_fixed_20261028-1200.epro2'
PLAN = {'eac10ca20e6778ba': (157.48, None), '9e78c3c3ca74ad42': (98.425, 137.795),
        '9197bb2e6c38da76': (98.425, 137.795)}


def main():
    assert not OUT.exists(), OUT
    s = subprocess.run(['node', 'tools/easyeda-bridge/eda.mjs', 'exec', '-f',
                        str(HERE / '51_export_project.js')],
                       capture_output=True, text=True, check=True).stdout
    v = json.loads(s)
    v = json.loads(v) if isinstance(v, str) else v
    blob = base64.b64decode(v['b64'])
    assert len(blob) == v['size']
    OUT.write_bytes(blob)
    assert zipfile.ZipFile(OUT).testzip() is None
    docs = documents(OUT)
    ok = True
    for h, rs in docs:
        if h['uuid'] not in PLAN:
            continue
        hole, pad = PLAN[h['uuid']]
        for a, b in rs:
            if a['type'] != 'PAD':
                continue
            hw, pw = b['hole']['width'], b['defaultPad']['width']
            good = abs(hw - hole) < 0.01 and (pad is None or abs(pw - pad) < 0.01)
            ok &= good
            print(h['uuid'], 'pad', b['num'], 'hole', hw, 'pad', pw, 'OK' if good else 'MISMATCH')
    print('bytes', len(blob))
    print('ALL XT PADS CORRECT' if ok else 'FIX FAILED')
    assert ok


if __name__ == '__main__':
    main()
