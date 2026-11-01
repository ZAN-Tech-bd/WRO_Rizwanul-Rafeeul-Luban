"""Robot-only GND removal for the user's editing interval; no route/library/footprint edits."""
import base64
from collections import Counter
import json
from pathlib import Path
import subprocess
import sys
import zipfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
DATA=HERE/'data/gnd_removal_20261028'
PCB='643ce99542452c8f'
PROJECT='7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864'
BEFORE=HERE.parents[1]/'Robot/WRO_Robot_before_gnd_removal_20261028-0248.epro2'
AFTER=HERE.parents[1]/'Robot/WRO_Robot_editing_no_gnd_20261028-0250.epro2'


def read_archive(path):
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        text=z.read(next(n for n in z.namelist() if n.endswith('.epru'))).decode('utf-8')
    decoder=json.JSONDecoder();docs={};current=None
    for line in text.splitlines():
        left,sep,right=line.partition('||')
        if not sep or not right.startswith('{'):continue
        h=json.loads(left);b,end=decoder.raw_decode(right)
        if h['type']=='DOCHEAD':current={'head':b,'records':{}};docs[b['uuid']]=current
        elif current is not None:
            key=(h['type'],h['id']);assert key not in current['records']
            current['records'][key]={'body':b,'raw':right[:end]}
    assert PCB in docs,'Wrong Robot archive'
    return docs


def verify_records(before,after,target_ids):
    removed=set(before)-set(after);added=set(after)-set(before)
    assert not added,'Unexpected new records'
    assert {i for t,i in removed if t=='FILL'}==target_ids,'Wrong fill identities removed'
    for key in removed:
        b=before[key]['body']
        if key[0]=='FILL':assert b.get('netName')=='GND' and b.get('layerId') in (1,2)
        else:assert key[0]=='ELE_PLACEHOLDER' and b.get('dataType')=='FILL','Non-fill record removed'
    for key in set(before)&set(after):
        assert before[key]['raw']==after[key]['raw'],f'Unexpected modified record {key}'
    assert not any(t=='FILL' and r['body'].get('netName')=='GND' and r['body'].get('layerId') in (1,2)
                   for (t,i),r in after.items()),'GND fill remains'
    return dict(fillsRemoved=len(target_ids),fillAllocatorRecordsRemoved=sum(t=='ELE_PLACEHOLDER' for t,i in removed),
                allSurvivingPcbRecordBodiesByteIdentical=True)


def export():
    assert not AFTER.exists(),'Do not overwrite an editing checkpoint'
    s=subprocess.run(['node',str(ROOT/'tools/easyeda-bridge/eda.mjs'),'exec','-f',str(HERE/'51_export_project.js')],
                     capture_output=True,text=True,check=True).stdout
    result=json.loads(s);result=json.loads(result) if isinstance(result,str) else result
    blob=base64.b64decode(result['b64']);assert len(blob)==result['size']
    AFTER.write_bytes(blob);read_archive(AFTER)
    print(json.dumps(dict(file=str(AFTER),bytes=len(blob))))


def build():
    value=json.loads((DATA/'live_gnd_fills.json').read_text(encoding='utf-8'))
    value=json.loads(value) if isinstance(value,str) else value
    assert value['project']==PROJECT and value['pcb']==PCB
    selected=[q for q in value['fills'] if q['net']=='GND' and q['layer'] in (1,2)]
    native=read_archive(BEFORE)[PCB]['records']
    ids={i for (t,i),r in native.items() if t=='FILL' and r['body'].get('netName')=='GND' and r['body'].get('layerId') in (1,2)}
    assert {q['id'] for q in selected}==ids and len(selected)==len(ids)
    source=(HERE/'53_remove_gnd_fills.template.js').read_text(encoding='utf-8')
    config=dict(project=PROJECT,pcb=PCB,fills=selected)
    (HERE/'53_remove_gnd_fills.js').write_text(source.replace('__CONFIG__',json.dumps(config,separators=(',',':'))),encoding='utf-8')
    print(json.dumps(dict(targetFills=len(selected),top=sum(q['layer']==1 for q in selected),bottom=sum(q['layer']==2 for q in selected))))


def verify():
    a=read_archive(BEFORE);b=read_archive(AFTER);assert a.keys()==b.keys()
    fills=json.loads((DATA/'native_gnd_fills.json').read_text(encoding='utf-8'))['fills']
    report=verify_records(a[PCB]['records'],b[PCB]['records'],{q['id'] for q in fills})
    for doc in a.keys()-{PCB}:
        assert a[doc]['records']==b[doc]['records'],f'Other document changed: {doc}'
    report.update(allOtherProjectDocumentsUnchanged=True,projectDocumentsCompared=len(a),
                  preservedRecordCounts=dict(Counter(t for t,i in b[PCB]['records'])))
    (DATA/'verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=1))


if __name__=='__main__':{'build':build,'export':export,'verify':verify}[sys.argv[1]]()
