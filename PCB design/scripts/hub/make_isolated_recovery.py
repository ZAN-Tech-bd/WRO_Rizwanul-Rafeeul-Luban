"""Create a new-identity recovery archive from the known-good checkpoint, never from the damaged worker.

Only document/library UUID references and the archive title change. Primitive IDs, coordinates, pad nets,
routes and all device properties remain exact after reverse-mapping the new UUIDs. No original file is edited.
"""
import json
from pathlib import Path
import re
import uuid
import zipfile

HERE=Path(__file__).resolve().parent
DATA=HERE/'data/blocker_fix_20261027'
SOURCE=HERE.parents[1]/'hub/WRO_HUB_after_footprint_fixes_20261027-1844.epro2'
OUTPUT=HERE.parents[1]/'hub/WRO_HUB_REPAIR_ISOLATED_20261027.epro2'
TITLE='WRO_HUB_REPAIR_ISOLATED_20261027'


def main():
    assert not OUTPUT.exists(),'Do not overwrite an existing recovery artifact'
    with zipfile.ZipFile(SOURCE) as z:
        assert z.testzip() is None
        files={n:z.read(n) for n in z.namelist()}
    native=next(n for n in files if n.endswith('.epru'))
    lines=files[native].decode('utf-8').splitlines(keepends=True)
    decoder=json.JSONDecoder();parsed=[];ids=[]
    for line in lines:
        left,sep,right=line.partition('||')
        if sep and right.startswith('{'):
            body,end=decoder.raw_decode(right);header=json.loads(left)
            parsed.append((header,body,right[end:]))
            if header['type']=='DOCHEAD' and re.fullmatch(r'[0-9a-f]{16}(?:_[0-9a-f]{16})?',body['uuid']):ids.append(body['uuid'])
        else:parsed.append(line)
    mapping={old:uuid.uuid4().hex[:16] for old in ids}
    assert len(set(mapping.values()))==len(mapping)
    inverse={v:k for k,v in mapping.items()}
    def remap(value,table):
        if isinstance(value,str):
            if value in table:return table[value]
            # Some source fields encode a document reference and library UUID separated by '|'.
            if '|' in value:return '|'.join(table.get(v,v) for v in value.split('|'))
            return value
        if isinstance(value,list):return [remap(v,table) for v in value]
        if isinstance(value,dict):return {table.get(k,k):remap(v,table) for k,v in value.items()}
        return value
    rebuilt=[];count=0
    for entry in parsed:
        if isinstance(entry,str):rebuilt.append(entry);continue
        header,body,suffix=entry
        h,b=remap(header,mapping),remap(body,mapping)
        assert remap(h,inverse)==header and remap(b,inverse)==body,'Non-identity data changed'
        rebuilt.append(json.dumps(h,ensure_ascii=False,separators=(',',':'))+'||'+json.dumps(b,ensure_ascii=False,separators=(',',':'))+suffix)
        count+=1
    files[native]=''.join(rebuilt).encode('utf-8')
    meta=json.loads(files['project2.json']);meta['title']=TITLE
    files['project2.json']=json.dumps(meta,ensure_ascii=False,indent=2).encode('utf-8')
    newfiles={}
    for name,blob in files.items():
        if name==native:name=TITLE+'.epru'
        if name.startswith('IMAGE/') and name.endswith('.webp'):
            stem=Path(name).stem
            if stem in mapping:name='IMAGE/'+mapping[stem]+'.webp'
        newfiles[name]=blob
    with zipfile.ZipFile(OUTPUT,'w',zipfile.ZIP_DEFLATED) as z:
        for name,blob in newfiles.items():z.writestr(name,blob)
    with zipfile.ZipFile(OUTPUT) as z:assert z.testzip() is None
    report={'source':str(SOURCE),'output':str(OUTPUT),'title':TITLE,'mapping':mapping,
            'recordsVerifiedByReverseMapping':count,'allGeometryNetsAndDevicePropertiesEquivalent':True}
    (DATA/'isolated_recovery_manifest.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k!='mapping'},indent=1))


if __name__=='__main__':main()
