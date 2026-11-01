"""Current release audit, pinned to the user's latest edits rather than historical inputs.

No writes to EasyEDA. Records evidence, not an automatic or thermal/mechanical qualification.
"""
from collections import Counter
import json
from pathlib import Path

from shapely.geometry import box
from shapely.strtree import STRtree
from final_ground import connectivity
import repaired_hub as R

BASE=R.HERE.parents[1]/'hub/WRO_HUB_REPAIRED_user_edited_20261027-2358.epro2'
FINAL=R.HERE.parents[1]/'hub/WRO_HUB_REPAIRED_repoured_20261028-0009.epro2'
EVIDENCE=R.DATA/'release_20261028'
MM=.0254


def main():
    EVIDENCE.mkdir(exist_ok=True)
    m=R.copper_model(FINAL);items=m['items'];holes=m['holes']
    preserve=R.compare_repour_records(R.read_archive(BASE),m['docs'],R.TARGET['pcb'])
    ground=R.check_ground_pieces(m,m['fills'])
    assert not ground['foreignOverlaps'] and not ground['clearanceViolations'] and not ground['disconnectedGnd']
    # Explicitly check stored fill net/mode as well as its polygon geometry.
    assert all(r['body']['netName']=='GND' and r['body']['layerId'] in (1,2)
               for (t,i),r in m['records'].items() if t=='FILL')
    rows=R.silk_rows(m,R.json_value(R.DATA/'final_silk_20261028.json'))
    assert not R.silk_style_plan(m['records'])
    assert rows==R.place_silk(rows,m),'Current measured silk bounds need correction'

    tree=STRtree([i['shape'] for i in items]);spacing=[];shorts=[];below_jlc=[]
    for index,a in enumerate(items):
        if a['shape'].is_empty:continue
        for j in tree.query(a['shape'].buffer(.2/MM)):
            j=int(j);b=items[j]
            if j<=index or not set(a['layers'])&set(b['layers']):continue
            if a['net']==b['net'] and a['net']:continue
            if a['label']==b['label']:continue
            gap=a['shape'].distance(b['shape'])*MM
            row=[round(gap,6),a['label'],b['label']]
            spacing.append(row)
            if a['shape'].intersection(b['shape']).area>1e-5:shorts.append(row)
            if gap<.1-1e-5:below_jlc.append(row)
    assert not shorts and not below_jlc,(shorts,below_jlc)

    hole_copper=[];drill_bad=[];hole_gaps=[];drill_gaps=[]
    ht=STRtree([h['shape'] for h in holes])
    for index,h in enumerate(holes):
        kind='via' if h['des']=='via' else 'pth'
        minimum=.2 if kind=='via' else .28
        for j in tree.query(h['shape'].buffer(.4/MM)):
            i=items[int(j)]
            if h['net']==i['net'] and h['net']:continue
            label=h['des']+'.'+h['num']
            if label==i['label']:continue
            gap=h['shape'].distance(i['shape'])*MM
            hole_gaps.append(gap)
            if gap<minimum-1e-4:hole_copper.append([round(gap,6),label,i['label'],minimum])
        for j in ht.query(h['shape'].buffer(.5/MM)):
            j=int(j)
            if j<=index:continue
            other=holes[j];minimum=.2 if kind=='via' and other['des']=='via' else .45
            gap=h['shape'].distance(other['shape'])*MM;drill_gaps.append(gap)
            if gap<minimum-1e-4:drill_bad.append([round(gap,6),h['des']+'.'+h['num'],other['des']+'.'+other['num'],minimum])
    assert not hole_copper and not drill_bad,(hole_copper,drill_bad)

    ring_rows=[]
    for p in m['pads']:
        if p['hole'] is None or not p['native'].get('plated'):continue
        assert p['shape'].buffer(.001).covers(p['hole']),p['des']+'.'+p['num']+' hole outside pad'
        ring=p['shape'].exterior.distance(p['hole'].exterior)*MM
        ring_rows.append([round(ring,6),p['des']+'.'+p['num']])
    assert all(r[0]>=.18-1e-4 for r in ring_rows),sorted(ring_rows)[:5]
    edge=min(i['shape'].distance(m['outline'].exterior)*MM for i in items if not i['shape'].is_empty)
    assert edge>=.2 and all(m['outline'].covers(i['shape']) for i in items if not i['shape'].is_empty)
    antenna=box(1475,3885.196850393701,2595,4075)
    assert not [i['label'] for i in items if i['shape'].intersects(antenna)]
    module=box(500,30,2460,1040)
    r5=[p for p in m['pads'] if p['des']=='R5']
    assert all(not p['shape'].intersects(module) for p in r5)

    netnames=sorted({i['net'] for i in items if i['net']})
    disconnected={}
    for net in netnames:
        if net=='GND':continue
        subset=dict(m,items=[dict(i,net='GND') for i in items if i['net']==net])
        _,bad=connectivity({1:[],2:[]},subset)
        if bad:disconnected[net]=bad
    assert not disconnected,disconnected
    pc=R.json_value(R.DATA/'final_pcb_20261028.json')
    sch=R.json_value(R.DATA/'current_sch_pins_20261028.json')
    erc=R.json_value(R.DATA/'erc_20261028.json')
    pn={c['designator']+'.'+str(p['number']):p['net'] for c in pc['components'] for p in c['pads'] if p['net']}
    sn={q['pin']:q['net'] for q in sch['pinNets']}
    assert pn==sn,'Current schematic pin-wire membership differs from current PCB'
    assert erc['erc'] is True and erc['detailed']==[]
    drc_names=[r['name'] for r in pc['drc']]
    assert all(name=='Netlist Error' for name in drc_names),pc['drc']
    ring_rows.sort();spacing.sort()
    slots=[p for p in m['pads'] if p['des']=='J3']
    assert len(slots)==6 and all(p['native']['hole']['holeType']=='SLOT' for p in slots)
    for p in slots:
        h=p['native']['hole'];assert h['height']*MM>=.5 and h['width']>=2*h['height']
    report=dict(sourceArchive=str(BASE.relative_to(R.HERE.parents[2])),
                releaseArchive=str(FINAL.relative_to(R.HERE.parents[2])),preservation=preserve,ground=ground,
                components=len(m['components']),pads=len(m['pads']),tracks=sum(i['kind']=='track' for i in items),
                vias=sum(i['kind']=='via' for i in items),namedNets=len(netnames),
                currentSchematicPinNetsMatched=len(pn),erc=erc,strictDrc=pc['drc'],
                physicalDrcFindings=0,synchronizationFindings=len(pc['drc']),
                copperMinSpacingMm=spacing[0][0],pairsBelowNominal6Mil=[r for r in spacing if r[0]<.1524],
                copperShorts=shorts,belowJlc1ozSpacing=below_jlc,drillSpacingViolations=drill_bad,
                holeToForeignCopperViolations=hole_copper,minimumPthRing=ring_rows[0],
                copperEdgeClearanceMm=edge,silkVisibleCount=len(rows),silkMeasuredBoundsPass=True,
                signalNetworksDisconnected=disconnected,antennaCopperClear=True,r5OutsideModule=True,
                hardwareGates=['real switch fit/support/contact action','full-load voltage-drop and thermal qualification'],
                releaseStatus='Fabrication review package; synchronization finding and hardware release gates disclosed')
    (EVIDENCE/'audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k not in {'strictDrc','pairsBelowNominal6Mil'}},indent=1))


if __name__=='__main__':main()
