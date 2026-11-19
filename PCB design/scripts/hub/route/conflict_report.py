# Where do routed nets still collide? Grid metric of pathfinder.conflicts() + exact distances from route_check.
# Prints net pairs, conflict locations (clustered) and the exact clearance at each. Usage: python conflict_report.py
import json
import math
import os
from collections import defaultdict

import numpy as np
from scipy import ndimage

import route_check as RC
import route_model as M
import router as R

RES = M.RES


def main():
    routes = json.load(open(os.path.join(M.HERE, 'data', 'routes.json'), encoding='utf-8'))
    viol, edge_v, hole_v, zone_v, unc, vip = RC.check(routes, clr=8.0)
    pairs = defaultdict(list)
    for v in viol:
        d, lay, n1, k1, n2, k2, x, y = v
        pairs[tuple(sorted((n1, n2)))].append((d, lay, k1, k2, x, y))
    print('exact clearance < 8 mil: %d spots in %d net pairs' % (len(viol), len(pairs)))
    for pr, lst in sorted(pairs.items(), key=lambda t: min(v[0] for v in t[1])):
        worst = min(lst)
        print('  %-24s worst %6.1f mil  layer %d  %s/%s  near (%d, %d)  [%d spots]' % (
            ' ~ '.join(pr), worst[0], worst[1], worst[2], worst[3], worst[4], worst[5], len(lst)))
    print('edge', len(edge_v), edge_v[:5])
    print('holes', len(hole_v), hole_v[:5])
    print('zone', len(zone_v), zone_v[:8])
    print('unconnected', len(unc), unc[:20])
    print('via in/at SMD pad', len(vip), vip[:20])


if __name__ == '__main__':
    main()
