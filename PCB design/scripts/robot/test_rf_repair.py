"""RF correction gates for the +3V3 spine under the DevKit antenna (hub test_repair_rf method).

The user's outline change (top edge 3840 -> 3900) brought the module's antenna overhang
onto the board; the +3V3 spine at y=3850 now runs under the radiating element. These tests
hold any repair plan to: exact source guards, kept net/layer/endpoints, class width 0.8 mm,
>= 4 mil to the ACTUAL antenna box, >= 6 mil to every foreign item, and intact +3V3
connectivity after the swap.
"""
import json
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

ARCHIVE = HERE.parents[1] / 'Robot/WRO_Robot_xt_verified_20261028-1351.epro2'
SEGMENTS = ('de82450445d8f812', '08b2b30513a78cdc', '2f5e6710791f7cf7')


def load():
    import robot_release as R
    import rf_repair
    m = R.copper_model(ARCHIVE)
    return rf_repair, m, R


class AntennaFacts(unittest.TestCase):
    def test_actual_antenna_box_derives_from_pin1_and_socket_columns(self):
        _, m, _ = load()
        ant = rf_antenna = None
        import rf_repair
        rf_antenna = rf_repair.antenna_box(m)
        b = rf_antenna.bounds
        self.assertAlmostEqual(b[1], 3840.2, delta=0.51)   # pin1 3765 + 1.91 mm drawing offset
        self.assertLessEqual(b[0], 420)                    # left module edge + margin
        self.assertGreaterEqual(b[2], 1520)                # right module edge + margin
        self.assertGreater(b[3], 4080)                     # 6.24 mm radiating element


class ProblemIsReal(unittest.TestCase):
    def test_current_three_segments_intersect_actual_antenna(self):
        import rf_repair
        _, m, _ = load()
        ant = rf_repair.antenna_box(m)
        hits = [i['id'] for i in m['items']
                if i['kind'] == 'track' and i['shape'].intersects(ant)]
        self.assertEqual(sorted(hits), sorted(SEGMENTS))


class PlanGates(unittest.TestCase):
    def setUp(self):
        self.rf, self.m, self.R = load()
        self.plan = self.rf.build_plan(self.m)
        self.new = self.rf.applied_items(self.m, self.plan)

    def test_plan_modifies_exactly_the_three_segments_with_source_guards(self):
        self.assertEqual(set(self.plan['modify']), set(SEGMENTS))
        for rid, entry in self.plan['modify'].items():
            src = entry['source']
            body = self.m['records'][('LINE', rid)]['body']
            self.assertEqual(src['net'], '+3V3')
            self.assertEqual(src['layerId'], 1)
            for key, value in (('netName', src['net']), ('layerId', src['layerId']),
                               ('width', src['width']),
                               ('startX', src['startX']), ('startY', src['startY']),
                               ('endX', src['endX']), ('endY', src['endY'])):
                self.assertEqual(body[key], value, f'{rid} guard {key} must match saved body')

    def test_chain_endpoints_and_class_width_are_kept(self):
        segs = {e['id']: e for e in self.new}
        self.assertEqual(self.plan['widthMil'], 31.4961)            # 0.8 mm PWR_3V3 spine class
        first = min(segs.values(), key=lambda e: e['start'][0])
        self.assertEqual((first['start'][0], first['start'][1]), (470.0, 3760.0))  # on U1A.1
        last = max(segs.values(), key=lambda e: e['end'][0])
        self.assertEqual((last['end'][0], last['end'][1]), (1765.0, 3850.0))       # into b1c3...
        for e in self.new:
            self.assertEqual(e['net'], '+3V3')
            self.assertEqual(e['layers'], (1,))

    def test_new_path_clears_actual_antenna_by_4_mil(self):
        ant = self.rf.antenna_box(self.m)
        for e in self.new:
            self.assertGreaterEqual(e['shape'].distance(ant), 4.0,
                                    f"{e['id']} too close to the antenna box")

    def test_new_path_clears_every_foreign_item_by_6_mil(self):
        worst = []
        for e in self.new:
            for i in self.m['items']:
                if i['id'] in SEGMENTS and i['kind'] == 'track':
                    continue  # the segments being replaced
                if i['net'] == '+3V3' or not set(i['layers']) & set(e['layers']):
                    continue
                d = e['shape'].distance(i['shape'])
                worst.append((round(d, 2), e['id'], i['label']))
                self.assertGreaterEqual(d, 6.0, f'{e["id"]} vs {i["label"]}: {d:.2f} mil')
        self.assertTrue(worst)  # the check must actually exercise foreign items

    def test_3v3_network_stays_connected_after_the_swap(self):
        import final_ground as FG
        kept = [i for i in self.m['items'] if i['net'] == '+3V3'
                and not (i['kind'] == 'track' and i['id'] in SEGMENTS)]
        sub = dict(self.m, items=kept + [dict(i, net='GND') for i in self.new])
        _, disconnected = FG.connectivity({1: [], 2: []}, sub)
        self.assertEqual(disconnected, [])


if __name__ == '__main__':
    unittest.main()
