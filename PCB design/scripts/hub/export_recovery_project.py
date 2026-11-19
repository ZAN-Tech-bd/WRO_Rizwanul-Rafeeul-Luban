"""Export only the isolated recovery working copy, with archive integrity checking."""
import base64
import json
from pathlib import Path
import subprocess
import sys
import zipfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]


def export(tag):
    path=HERE.parents[1]/'hub'/f'WRO_HUB_REPAIRED_{tag}.epro2'
    if path.exists():raise FileExistsError(path)
    result=subprocess.run(['node',str(ROOT/'tools/easyeda-bridge/eda.mjs'),'exec','-f',
                           str(HERE/'69_export_recovery_project.js')],capture_output=True,text=True,check=True).stdout
    data=json.loads(json.loads(result));blob=base64.b64decode(data['b64'])
    assert len(blob)==data['size']
    path.write_bytes(blob)
    with zipfile.ZipFile(path) as z:assert z.testzip() is None
    print(json.dumps({'file':str(path),'bytes':len(blob)}))
    return path


if __name__=='__main__':export(sys.argv[1])
