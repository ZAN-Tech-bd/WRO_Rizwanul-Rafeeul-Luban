"""Regression tests for the real switch geometry, not a generic KCD4 variant."""
import importlib.util
from pathlib import Path
import unittest


class RockerGeometryTests(unittest.TestCase):
    def model(self):
        path = Path(__file__).with_name('rocker_model.py')
        self.assertTrue(path.exists(), 'The tested direct-mount geometry model must exist')
        spec = importlib.util.spec_from_file_location('rocker_model', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_exact_original_terminal_pattern_and_net_map(self):
        m = self.model()
        self.assertEqual(m.CONTACTS, {
            '1': ('BAT+', -10.5, 5.5), '2': ('V12', 0.0, 5.5), '3': ('DC+', 10.5, 5.5),
            '4': ('BAT-', -10.5, -5.5), '5': ('GND', 0.0, -5.5), '6': ('DC-', 10.5, -5.5),
        })

    def test_conservative_finished_slot_contains_all_blade_corners(self):
        m = self.model()
        for dx in (-0.2, 0.0, 0.2):
            for dy in (-0.2, 0.0, 0.2):
                self.assertTrue(m.blade_fits(7.7, 1.7, 6.4, 0.83, dx, dy))
        self.assertFalse(m.blade_fits(6.6, 1.1, 6.3, 0.8))

    def test_pad_and_slot_axes_match_the_end_view(self):
        m = self.model()
        self.assertEqual(m.SLOT_MM, (1.8, 7.8))
        self.assertEqual(m.PAD_MM, (3.0, 9.0))
        self.assertEqual(m.FLANGE_MM, (34.5, 25.5))
        self.assertEqual(m.COURTYARD_MM, (36.5, 27.5))

    def test_emitted_footprint_has_six_plated_slots_and_no_old_connector_graphics(self):
        m = self.model()
        records = m.footprint_records('test-footprint')
        pads = [b for a, b in records if a['type'] == 'PAD']
        self.assertEqual(len(pads), 6)
        self.assertEqual({p['num'] for p in pads}, set(m.CONTACTS))
        for p in pads:
            net, x, y = m.CONTACTS[p['num']]
            self.assertAlmostEqual(p['centerX'] * 0.0254, x)
            self.assertAlmostEqual(p['centerY'] * 0.0254, y)
            self.assertTrue(p['plated'])
            self.assertEqual(p['hole']['holeType'], 'SLOT')
            self.assertEqual(p['defaultPad']['padType'], 'OVAL')
            # Native record width is SLOT LENGTH, height is diameter; rotate the hole 90 degrees.
            self.assertAlmostEqual(p['hole']['width'] * 0.0254, 7.8)
            self.assertAlmostEqual(p['hole']['height'] * 0.0254, 1.8)
            self.assertEqual(p['relativeAngle'], 90)
            self.assertEqual(p['netName'], '')
        text = m.encode_records(records)
        self.assertNotIn('B6P-VH', text)
        self.assertNotIn('d12aebc8', text)

    def test_slot_ring_and_between_pole_clearance(self):
        m = self.model()
        self.assertAlmostEqual((m.PAD_MM[0] - m.SLOT_MM[0]) / 2, 0.6)
        self.assertAlmostEqual((m.PAD_MM[1] - m.SLOT_MM[1]) / 2, 0.6)
        self.assertGreaterEqual(11 - m.PAD_MM[1], 2.0)

    def test_as_built_placement_rotates_the_pattern_but_not_the_pin_numbers(self):
        """As built 2026-10-26 (H-001 retry 2): J3 at (-60, 1495) mil, rotation -90 (x, y -> y, -x)."""
        m = self.model()
        plan = m.live_plan()
        self.assertEqual(plan['centerMil'], [-60, 1495])
        self.assertEqual(plan['rotation'], -90)
        for p in plan['pads']:
            net, x, y = m.CONTACTS[p['number']]
            self.assertEqual(p['net'], net)
            self.assertAlmostEqual((p['x'] + 60) * 0.0254, y)
            self.assertAlmostEqual((p['y'] - 1495) * 0.0254, -x)
        pin1 = next(p for p in plan['pads'] if p['number'] == '1')
        self.assertAlmostEqual(pin1['x'], -60 + 216.535, places=2)   # 20b inventory: 1:BAT+@216.5,413.4
        self.assertAlmostEqual(pin1['y'], 1495 + 413.386, places=2)

    def test_as_built_outline_is_widened_left_and_only_listed_parts_moved(self):
        m = self.model()
        plan = m.live_plan()
        points = plan['outlineMil']
        self.assertEqual(points[0], points[-1])
        self.assertEqual(sorted({tuple(p) for p in points}), [(-620, 0), (-620, 3900), (4600, 0), (4600, 3900)])
        self.assertEqual(plan['moveOtherComponents'], {'J4': [0, -195], 'J5': [0, -125], 'LED3': [450, 0]})
        flange = plan['flangeBoundsMil']
        self.assertGreater(flange[0], -620)    # flange inside the new left edge

if __name__ == '__main__':
    unittest.main()
