"""Generate the Robot final-ground plan + guarded fill writer from the CURRENT saved archive.

Uses robot_release.ground_plan/serialize/verify_plan (gated by test_ground_plan.py).
The writer (57_final_ground.template.js) is source-locked to this exact archive's
components/pads/tracks/vias; any drift aborts before a write.
"""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import robot_release as R

ARCHIVE = HERE.parents[1] / 'Robot/WRO_Robot_via_deleted_20261028-1438.epro2'
DATA = HERE / 'data/release_20261028'


def snapshot(model):
    lines, vias, components = [], [], []
    for (t, rid), r in model['records'].items():
        b = r['body']
        if t == 'LINE' and b.get('layerId') in (1, 2):
            lines.append(dict(id=rid, net=b['netName'], layer=b['layerId'],
                              x1=b['startX'], y1=b['startY'], x2=b['endX'], y2=b['endY'],
                              width=b['width']))
        elif t == 'VIA':
            vias.append(dict(id=rid, net=b['netName'], x=b['centerX'], y=b['centerY'],
                             hole=b['holeDiameter'], diameter=b['viaDiameter']))
    for cid, c in model['components'].items():
        a = model['attrs'][cid]
        pads = sorted((dict(number=p['num'], net=p['net'],
                            x=round(p['center'][0] - c['x'], 4),
                            y=round(p['center'][1] - c['y'], 4))
                       for p in model['pads'] if p['id'].startswith(cid)),
                      key=lambda p: str(p['number']))
        components.append(dict(designator=a['Designator'], x=c['x'], y=c['y'],
                               rotation=c['angle'], footprint=a['Footprint'], pads=pads))
    components.sort(key=lambda c: c['designator'])
    lines.sort(key=lambda l: l['id'])
    vias.sort(key=lambda v: v['id'])
    return dict(components=components, lines=lines, vias=vias)


def main():
    model = R.copper_model(ARCHIVE)
    fills = R.ground_plan(model)
    pieces = R.serialize(fills)
    report = R.verify_plan(model, pieces)
    DATA.mkdir(parents=True, exist_ok=True)
    plan_path = DATA / 'ground_plan.json'
    assert not plan_path.exists(), 'refusing to overwrite an existing ground plan'
    plan_path.write_text(json.dumps(
        dict(sourceArchive=str(ARCHIVE), normalClearanceMil=R.NORMAL, edgeMil=R.EDGE,
             antennaMarginMil=R.ANT_MARGIN, minNeckMil=R.MIN_NECK, pieces=pieces,
             verify=report), separators=(',', ':')), encoding='utf-8')
    config = dict(project=R.PROJECT, pcb=R.ROBOT, pieces=pieces, **snapshot(model))
    source = (HERE / '57_final_ground.template.js').read_text(encoding='utf-8')
    writer = HERE / 'data/57_final_ground.js'
    writer.write_text(source.replace('__CONFIG__', json.dumps(config, separators=(',', ':'))),
                      encoding='utf-8')
    print(json.dumps(dict(pieces=len(pieces), layerCounts=report['layerCounts'], writer=str(writer),
                          minForeignGapMil=report['minForeignGapMil'],
                          edgeMil=report['edgeClearanceMil'],
                          stripMil=report['antennaStripMarginMil'],
                          overhangMil=report['antennaOverhangMarginMil']), indent=2))


if __name__ == '__main__':
    main()
