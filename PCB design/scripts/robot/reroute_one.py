# Re-route single nets on top of the copper that is actually in PCB1 (every other net fixed, exact clearances),
# optionally keeping the new route out of rectangles on one layer (e.g. to leave a GND pocket open).
# Usage: python reroute_one.py data/<dump>.json NET[,NET] [--avoid=L:x0,y0,x1,y1;...] [--clr=7] [--iters=15]  -> <dump>_rr.json (dump with
#        the nets replaced)
import json
import sys

import numpy as np

import check_eda_copper as CE
import pathfinder as PF
import route_model as M
import router as R


def main():
    dump, nets = sys.argv[1], sys.argv[2].split(',')
    avoid = []
    for a in sys.argv[3:]:
        if a.startswith('--avoid='):
            for part in a.split('=', 1)[1].split(';'):
                L, r = part.split(':')
                avoid.append((int(L), [float(v) for v in r.split(',')]))
    clr = next((float(x.split('=')[1]) for x in sys.argv if x.startswith('--clr=')), None)
    if clr:  # tighter than the router default (10 mil) where a re-route must squeeze between fixed copper
        R.CLR = PF.CLR = clr
    routes, _ = CE.load(dump)
    b = PF.PFBoard()
    for net, r in routes.items():
        if net in b.nid and net not in nets:
            b.fix_copper(net, [tuple(s) for s in r['segs']], [tuple(v) for v in r['vias']])
    # avoid rectangles: an exact distance field owned by "no net" on that layer (an obstacle for every net)
    for L, (x0, y0, x1, y1) in avoid:
        RES, NX, NY = M.RES, M.NX, M.NY
        i0, i1 = max(0, int(x0 / RES) - PF.WIN), min(NX, int(x1 / RES) + PF.WIN + 2)
        j0, j1 = max(0, int(y0 / RES) - PF.WIN), min(NY, int(y1 / RES) + PF.WIN + 2)
        X, Y = np.meshgrid(np.arange(i0, i1) * RES, np.arange(j0, j1) * RES)
        dx = np.maximum(np.maximum(x0 - X, 0), X - x1)
        dy = np.maximum(np.maximum(y0 - Y, 0), Y - y1)
        b.fields.append(((L,), -1, (j0, j1, i0, i1), np.hypot(dx, dy).astype(np.float32), False, False))
    log = []
    ok = True
    iters = next((int(x.split('=')[1]) for x in sys.argv if x.startswith('--iters=')), 1)
    pres, todo = 0.6, list(nets)
    for it in range(iters):   # negotiated congestion among the re-routed nets (pathfinder.run without the GND stage)
        ok = True
        for net in todo:
            ok &= PF.route_net_pf(b, net, pres, log)
        bad, cnets = PF.conflicts(b)
        print('iter %2d: conflicts %4d cells in %s' % (it, int(bad.sum()), sorted(cnets)), flush=True)
        if not cnets:
            break
        b.hist[bad] += 1.0
        pres *= 1.7
        todo = [n for n in nets if n in cnets]
    print('\n'.join(l for l in log if l.startswith('HARD')) or 'routed: %s' % nets)
    d = json.load(open(dump, encoding='utf-8'))
    d = json.loads(d) if isinstance(d, str) else d
    for net in nets:
        r = b.routes[net]
        d[net] = {'segs': [[1 if s[0] == 0 else 2] + list(s[1:6]) + ['rr'] for s in r['segs']],
                  'vias': [list(v[:4]) + ['rr'] for v in r['vias']]}
        print('%s: %d segments, %d vias' % (net, len(r['segs']), len(r['vias'])))
    out = dump.replace('.json', '_rr.json')
    json.dump(d, open(out, 'w', encoding='utf-8'))
    print('ok' if ok else 'FAILED', '->', out)


if __name__ == '__main__':
    main()
