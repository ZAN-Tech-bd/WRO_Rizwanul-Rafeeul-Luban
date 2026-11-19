"""READ-ONLY: compare the station's current copper vs the 2026-10-26 reviewed snapshot."""
import json
import sys
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SNAP = HERE / 'data/snapshot_after_gnd_20261026.json'
ARCHIVE = HERE.parents[1] / 'station/station_silk_fixed_20261028.epro2'
PCB = 'fdcb20516f125209'


def main():
    snap = json.loads(SNAP.read_text(encoding='utf-8-sig'))
    if isinstance(snap, str):
        snap = json.loads(snap)
    z = zipfile.ZipFile(ARCHIVE)
    text = z.read(next(n for n in z.namelist() if n.endswith('.epru'))).decode('utf-8')
    dec = json.JSONDecoder()
    cur, tracks, fills, vias, padnets = None, [], [], [], {}
    for line in text.splitlines():
        left, sep, right = line.partition('||')
        if not sep or not right.startswith('{'):
            continue
        h = json.loads(left)
        b, _ = dec.raw_decode(right)
        if h['type'] == 'DOCHEAD':
            cur = b['uuid']
        elif cur != PCB:
            continue
        if h['type'] == 'LINE':
            tracks.append((b.get('netName'), b.get('layerId'), b.get('startX'), b.get('startY'),
                           b.get('endX'), b.get('endY'), b.get('width')))
        elif h['type'] == 'FILL':
            fills.append((b.get('layerId'), tuple(json.dumps(b.get('path')))))
        elif h['type'] == 'VIA':
            vias.append((b.get('netName'), b.get('centerX'), b.get('centerY'),
                         b.get('holeDiameter'), b.get('viaDiameter')))
    snap_tracks = sorted((t.get('net'), t.get('layer'), round(t.get('x1', 0), 2), round(t.get('y1', 0), 2),
                          round(t.get('x2', 0), 2), round(t.get('y2', 0), 2), round(t.get('width', 0), 3))
                         for t in snap.get('tracks', []) if isinstance(t, dict))
    print('snapshot tracks:', len(snap_tracks), 'archive LINE records:', len(tracks))
    print('archive fills:', len(fills), ' snapshot fills:', len(snap.get('fills', [])))
    print('archive vias:', len(vias), ' snapshot vias:', len(snap.get('vias', [])))
    if snap_tracks:
        print('snapshot track sample:', snap_tracks[0])
    print('archive track sample:', tracks[0] if tracks else None)


if __name__ == '__main__':
    main()
