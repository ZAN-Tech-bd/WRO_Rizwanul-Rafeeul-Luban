"""Routing-oriented placement regression tests, using the user's fresh live board."""
import copy
import math
from pathlib import Path
import unittest

import station_placement as model


class StationPlacementTests(unittest.TestCase):
    def setUp(self):
        self.parts, self.outline, self.holes = model.source()

    def test_allowlist_is_exactly_37_unplaced_station_parts(self):
        expected = {f'J{k}' for k in range(10, 18)} | {f'C{k}' for k in range(3, 10)}
        expected |= {f'R{k}' for a, b in [(31, 37), (41, 44), (51, 54), (61, 67)] for k in range(a, b+1)}
        self.assertEqual(set(model.TARGETS), expected)
        self.assertEqual(len(expected), 37)
        for d in expected:
            self.assertGreater(self.parts[d]['x'], 4695, d)
        self.assertFalse({'R4', 'R6', 'R13'} & expected)

    def test_every_fixed_part_is_unchanged(self):
        result = model.placed(self.parts)
        for d in self.parts.keys() - model.TARGETS.keys():
            self.assertEqual(result[d], self.parts[d], d)

    def test_targets_fit_without_new_body_hole_or_edge_collisions(self):
        self.assertEqual(model.check(self.parts, self.outline, self.holes), [])

    def test_caps_have_adc_pad_close_to_matching_mcu_input(self):
        result = model.placed(self.parts)
        for k in range(1, 8):
            net = f'MQ_S{k}_ADC'
            cap = model.net_pad(result[f'C{k+2}'], net)
            mcu = next(model.net_pad(result[d], net) for d in ['U1A', 'U1B']
                       if net in [p[0] for p in result[d]['pads'].values()])
            self.assertLess(math.dist(cap, mcu), 275, net)

    def test_connectors_make_a_two_row_serpentine_chain(self):
        xy = [model.TARGETS[f'J{k}'][:2] for k in range(10, 18)]
        self.assertEqual([p[1] for p in xy], [3150]*4 + [1850]*4)
        self.assertEqual([p[0] for p in xy[:4]], sorted(p[0] for p in xy[:4]))
        self.assertEqual([p[0] for p in xy[4:]], sorted((p[0] for p in xy[4:]), reverse=True))
        self.assertTrue(all(model.TARGETS[f'J{k}'][2] == 90 for k in range(10, 18)))

    def test_led_hop_resistors_stay_close_to_their_own_port_pair(self):
        result = model.placed(self.parts)
        for k in range(1, 8):
            a = model.net_pad(result[f'R{60+k}'], f'S{k}_DOUT')
            b = model.net_pad(result[f'R{60+k}'], f'S{k+1}_DIN')
            pa = model.net_pad(result[f'J{k+9}'], f'S{k}_DOUT')
            pb = model.net_pad(result[f'J{k+10}'], f'S{k+1}_DIN')
            self.assertLess(math.dist(a, pa) + math.dist(b, pb), 1050, f'R{60+k}')

    def test_rotation_preserves_every_pad_net_and_pitch(self):
        result = model.placed(self.parts)
        for d in model.TARGETS:
            old, new = self.parts[d]['pads'], result[d]['pads']
            self.assertEqual({n:p[0] for n,p in old.items()}, {n:p[0] for n,p in new.items()})
            for n in old:
                self.assertAlmostEqual(math.hypot(*old[n][1:]), math.hypot(*new[n][1:]), places=5)

    def test_changed_source_coordinates_are_rejected(self):
        altered = copy.deepcopy(self.parts)
        altered['J10']['x'] = 3500
        with self.assertRaisesRegex(ValueError, 'not unplaced'):
            model.placed(altered)


if __name__ == '__main__':
    unittest.main(verbosity=2)
