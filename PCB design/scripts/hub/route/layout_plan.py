# Hub v0.4 PCB — routing config for the LIVE board (2026-10-27). Not a floor planner: the user hand-placed the hub, so
# the parts stay where they are. This module only carries the board frame and the zones the router engine
# (router.py / pathfinder.py / gnd_stage.py / pour_fill.py / check_eda_copper.py, copied from scripts/robot) imports
# as `layout_plan`. Geometry comes from EasyEDA (data/live.json = ../20e_live_pads.js).
#
# Board frame: EasyEDA's outline runs x -675..4695, y 0..3975 mil. The grid router needs x >= 0, so the model is
# shifted by OX: model x = EasyEDA x + OX. Everything that reads EasyEDA adds OX; everything that writes subtracts it.
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
LIVE = os.path.join(HERE, 'data', 'live.json')


def _live():
    d = json.loads(open(LIVE, encoding='utf-8').read())
    return json.loads(d) if isinstance(d, str) else d


_L = _live()
_ol = [v for v in _L['outline'][0] if not isinstance(v, str)]
_xs, _ys = _ol[0::2], _ol[1::2]
OX, OY = -min(_xs), -min(_ys)          # 675, 0
W, H = max(_xs) + OX, max(_ys) + OY    # 5370 x 3975 mil (136.4 x 101.0 mm)

# ---- DevKit (U1A / U1B sockets, pin 1 at the top): YD-ESP32-S3, WROOM-1 antenna at the DevKit's top end. Pin 1 sits
# 75 mil below the DevKit's top edge; the module's antenna end + 5 mm is a keep-out for pours, vias and power tracks
# (spec §7 robot rule, attendance D-028); thin signals may cross (DevKit on sockets, ~11 mm up).
_u1a = next(c for c in _L['comps'] if c['des'] == 'U1A')
_p1 = next(p for p in _u1a['pads'] if p['num'] == '1')
DK_X0 = _p1['x'] + OX - 50
DK_TOP = _p1['y'] + OY + 75
ANT_X0, ANT_X1 = DK_X0 + 196, DK_X0 + 904
ANTENNA = (ANT_X0 - 200, DK_TOP - 250, ANT_X1 + 200, H)   # antenna end (~6 mm) + 5 mm, up to the board edge
ANT_STRICT = False
POUR_KEEPOUT = ANTENNA

# ---- mounting holes: the 4 free "MH" pads (M3, plated, no net)
HOLES = []
for i, p in enumerate(_L['free']):
    if p['num'] == 'MH':
        HOLES.append(('H%d' % (i + 1), p['x'] + OX, p['y'] + OY, float(p['hole'][1]), float(p['pad'][1]), 0, True))

EP_PADS = set()                    # 100 % through-hole
BODY_PART, BODY_NETS = None, ()
BOOST_PARTS, BOOST_NETS = (), ()   # the 10 A buck is off-board
NO_VIA_BODIES = {}                 # XT / VH / XH housings are plastic; the SD module and DevKit sit on headers
