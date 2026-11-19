"""H-004 scope, resistor geometry, signal proximity and silk clearance tests."""
import math
import unittest

import station_cleanup as m


class StationCleanupTests(unittest.TestCase):
    def test_exact_three_resistors_and_no_other_component_moves(self):
        self.assertEqual(set(m.POSES), {'R4','R6','R13'})
        old,new=m.parts(),m.final_parts()
        for d in old:
            if d not in m.POSES:
                self.assertEqual(old[d],new[d],d)

    def test_three_resistors_fit_clear_of_all_other_bodies_and_holes(self):
        self.assertEqual(m.body_problems(),[])

    def test_r6_is_close_to_the_led_driver_output(self):
        p=m.final_parts()
        self.assertLess(math.dist(m.net_pad(p['R6'],'LED_BUF'),m.net_pad(p['U2'],'LED_BUF')),125)

    def test_r13_is_close_to_the_mcu_estop_input(self):
        p=m.final_parts()
        self.assertLess(math.dist(m.net_pad(p['R13'],'ESTOP_IO'),m.net_pad(p['U1B'],'ESTOP_IO')),175)

    def test_complete_station_labels_and_title_are_relocated_by_identity(self):
        plan=m.text_plan()
        self.assertEqual(len(plan),73)  # 8 station names + 64 pin labels + title.
        self.assertEqual(sum(x['kind']=='station' for x in plan.values()),8)
        self.assertEqual(sum(x['kind']=='pin' for x in plan.values()),64)
        self.assertEqual(sum(x['kind']=='title' for x in plan.values()),1)
        for x in plan.values():
            self.assertLess(x['x'],4695)
            self.assertGreater(x['x'],-675)
            self.assertEqual(x['rotation'],0)
            if x['kind']=='pin':
                self.assertEqual((x['fontSize'],x['lineWidth']),(32,6))

    def test_all_relocated_silk_clears_pads_edges_and_other_text(self):
        self.assertEqual(m.silk_problems(),[])


if __name__=='__main__':
    unittest.main(verbosity=2)
