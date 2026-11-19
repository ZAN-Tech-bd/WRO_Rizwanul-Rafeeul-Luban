"""Saved-state GND verification: the fills that EasyEDA actually serialized must equal the
validated plan piece-for-piece, and every gate (foreign clearance, edge, antenna margins,
floating pieces, full GND connectivity) must pass on the EXPORTED geometry.

Non-fill geometry is taken from the pre-pour archive (proven byte-identical by the
association_check diffs); fills come from the final export.
"""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'hub'))
import final_ground as FG
import robot_release as R

FINAL = HERE.parents[1] / 'Robot/WRO_Robot_final_pour_20261028-1612.epro2'
PRE_POUR = HERE.parents[1] / 'Robot/WRO_Robot_via_deleted_20261028-1438.epro2'
PLAN = HERE / 'data/release_20261028/ground_plan.json'


def saved_fills(path):
    import gnd_edit
    docs = gnd_edit.read_archive(path)
    fills = {1: [], 2: []}
    for (t, i), r in docs[R.ROBOT]['records'].items():
        if t != 'FILL':
            continue
        b = r['body']
        assert b['netName'] == 'GND' and b['layerId'] in (1, 2), (i, b['netName'], b['layerId'])
        src = b['path'][0] if isinstance(b['path'][0], list) else b['path']
        fills[b['layerId']].append(FG.from_source(src))
    return fills


def main():
    plan = json.loads(PLAN.read_text(encoding='utf-8'))
    want = {1: [], 2: []}
    for p in plan['pieces']:
        want[p['layer']].append(tuple(p['source']))
    fills = saved_fills(FINAL)
    # Exact multiset comparison on the serialized sources.
    docs_src = {1: [], 2: []}
    import gnd_edit
    recs = gnd_edit.read_archive(FINAL)[R.ROBOT]['records']
    for (t, i), r in recs.items():
        if t == 'FILL':
            src = r['body']['path'][0] if isinstance(r['body']['path'][0], list) else r['body']['path']
            docs_src[r['body']['layerId']].append(tuple(src))
    for l in (1, 2):
        w = sorted(tuple(p) for p in want[l])
        g = sorted(docs_src[l])
        assert w == g, (f'layer {l}: saved fills differ from plan',
                        len(w), len(g))
    print(f'saved fills == plan: {sum(len(v) for v in docs_src.values())} pieces '
          f'(L1 {len(docs_src[1])}, L2 {len(docs_src[2])})')

    # Gates on exported geometry.
    from shapely.ops import unary_union
    m = R.copper_model(PRE_POUR)
    bylayer = {l: unary_union(fills[l]) for l in (1, 2)}
    overlaps, bad, minimum = [], [], 1e9
    for i in m['items']:
        if i['net'] == 'GND':
            continue
        for l in i['layers']:
            g = bylayer[l]
            d = g.distance(i['shape'])
            minimum = min(minimum, d)
            if g.intersection(i['shape']).area > 0.001:
                overlaps.append([l, i['label']])
            if d < R.NORMAL - 0.5:
                bad.append([l, i['label'], round(d, 3)])
    assert not overlaps, overlaps[:10]
    assert not bad, bad[:10]
    connected, disconnected = FG.connectivity(fills, m)
    floating = sum(len(fills[l]) - len(connected[l]) for l in (1, 2))
    assert floating == 0, floating
    assert disconnected == [], disconnected
    import rf_repair
    edge = min(g.distance(m['outline'].exterior) for g in bylayer.values())
    strip = min(g.distance(R.ANTENNA) for g in bylayer.values())
    overhang = min(g.distance(rf_repair.antenna_box(m)) for g in bylayer.values())
    assert all(m['outline'].covers(g) for gs in fills.values() for g in gs)
    assert edge >= R.EDGE - 0.2, edge
    assert strip >= R.ANT_MARGIN - 0.2 and overhang >= R.ANT_MARGIN - 0.2, (strip, overhang)
    report = dict(savedFillPieces=sum(len(v) for v in fills.values()),
                  layerCounts={str(l): len(fills[l]) for l in fills},
                  foreignOverlaps=overlaps, clearanceViolations=bad,
                  minForeignGapMil=round(minimum, 3), edgeClearanceMil=round(edge, 3),
                  antennaStripMarginMil=round(strip, 3),
                  antennaOverhangMarginMil=round(overhang, 3),
                  floatingFillPieces=floating, disconnectedGnd=disconnected,
                  allSavedFillsEqualPlan=True)
    out = HERE / 'data/release_20261028/saved_fills_check.json'
    assert not out.exists()
    out.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
