"""Bounded native-footprint fabrication corrections; original pad identity and centres are immutable."""
import copy
import json
from pathlib import Path

from test_sb_removal import archive,PCB

HERE=Path(__file__).resolve().parent
DATA=HERE/'data/blocker_fix_20261027'
BEFORE=HERE.parents[1]/'hub/WRO_HUB_before_blocker_fixes_20261027-1836.epro2'
PROJECT='58cf0863ccb9b405a3019aadf223b9df2e72878780f06e4967d8bd8f9eeb441d'
PLAN={'Q1':{'holeMm':.75,'padMm':(1.115,1.6)},'J1':{'holeMm':4.,'padMm':(5.5,5.5)},
      'J2':{'holeMm':4.,'padMm':(5.5,5.5)},'J4':{'holeMm':4.,'padMm':(6.,6.)},
      'J5':{'holeMm':2.5,'padMm':(3.5,3.5)}}


def footprint_plan():
    docs=archive(BEFORE);r=docs[PCB]['records']
    ids={v['body']['parentId']:v['body']['value'] for (t,_),v in r.items() if t=='ATTR' and v['body'].get('key')=='Designator'}
    fps={ids[v['body']['parentId']]:v['body']['value'] for (t,_),v in r.items()
         if t=='ATTR' and v['body'].get('key')=='Footprint' and v['body'].get('parentId') in ids}
    result={}
    for des,settings in PLAN.items():
        fp=fps[des]
        if fp in result:
            result[fp]['designators'].append(des);continue
        rows=[]
        for (kind,pid),record in docs[fp]['records'].items():
            if kind!='PAD':continue
            old=record['body'];new=copy.deepcopy(old)
            if des!='Q1':
                new['hole']['width']=new['hole']['height']=settings['holeMm']/.0254
            new['defaultPad']['width'],new['defaultPad']['height']=[n/.0254 for n in settings['padMm']]
            rows.append({'id':pid,'before':old,'after':new})
        result[fp]={'designators':[des],'pads':rows}
    return result


def build_writer():
    template=(HERE/'67_fix_project_footprints.template.js').read_text(encoding='utf-8')
    plan=footprint_plan()
    (DATA/'footprint_plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
    (HERE/'67_fix_project_footprints.js').write_text(template.replace('__FOOTPRINT_FIX_PLAN__',json.dumps(plan,separators=(',',':'))),encoding='utf-8')


if __name__=='__main__':build_writer()
