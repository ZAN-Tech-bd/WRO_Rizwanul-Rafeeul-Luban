# Hub v0.4 PCB, live board — after pathfinder.py: exact trim, GND stitching and the exact check, done on the MERGED
# copper (the user's fixed tracks + the new routes) so nothing is checked in isolation. data/routes.json keeps only the
# NEW copper (what gen_route_add.py writes); the user's tracks are never modified: a trim that would touch one aborts.
# Usage: python finish.py      (idempotent: drops earlier stitching vias before re-stitching)
import json
import os

import check_eda_copper as CE
import gnd_stage as GS
import pathfinder as PF
import route_check as RC
import route_model as M
import trim as TR

SRC = os.path.join(M.HERE, 'data', 'routes.json')


def main():
    new = json.load(open(SRC, encoding='utf-8'))
    g = new.get('GND')
    if g and 'stitch' in g:
        g['vias'] = g['vias'][:len(g['vias']) - g['stitch']]
        g.pop('stitch')
    user, _ = CE.load(PF.USER_DUMP)
    merged, nuser = {}, {}
    for net in set(new) | set(user):
        u, n = user.get(net, {'segs': [], 'vias': []}), new.get(net, {'segs': [], 'vias': []})
        merged[net] = {'segs': [tuple(s) for s in u['segs']] + [tuple(s) for s in n['segs']],
                       'vias': [tuple(v) for v in u['vias']] + [tuple(v) for v in n['vias']]}
        for k in ('fanout', 'stitch'):
            if k in n:
                merged[net][k] = n[k]
        nuser[net] = (len(u['segs']), len(u['vias']))
    merged, trimmed, failed = TR.run(merged)
    bad = [t for t in trimmed if t[1] < nuser[t[0]][0]]
    assert not bad, 'trim would change user tracks: %s' % bad[:5]
    print('trimmed %d new segments; cannot trim: %d %s' % (len(trimmed), len(failed), failed[:10]))
    merged, notes = GS.run(merged)
    print('\n'.join(notes))
    out = {}
    for net, r in merged.items():
        su, vu = nuser.get(net, (0, 0))
        out[net] = {'segs': [list(s) for s in r['segs'][su:]], 'vias': [list(v) for v in r['vias'][vu:]]}
        for k in ('fanout', 'stitch'):
            if k in r:
                out[net][k] = r[k]
    json.dump(out, open(SRC, 'w', encoding='utf-8'))
    json.dump({n: {'segs': [list(s) for s in r['segs']], 'vias': [list(v) for v in r['vias']]} for n, r in merged.items()},
              open(os.path.join(M.HERE, 'data', 'routes_merged.json'), 'w', encoding='utf-8'))
    viol, edge_v, hole_v, zone_v, unc, vip = RC.check(merged)
    print('merged check: clearance %d, edge %d, holes %d, antenna %d, unconnected %d %s' % (
        len(viol), len(edge_v), len(hole_v), len(zone_v), len(unc), unc[:20]))
    for v in sorted(viol)[:20]:
        print('  ', v)
    print('new copper: %d segments, %d vias' % (sum(len(r['segs']) for r in out.values()),
                                                 sum(len(r['vias']) for r in out.values())))


if __name__ == '__main__':
    main()
