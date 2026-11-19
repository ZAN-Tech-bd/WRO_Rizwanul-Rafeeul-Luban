"""Local fabrication-fix geometry and immutable-pad-identity acceptance."""
import unittest

import blocker_fixes as b


class BlockerFixTests(unittest.TestCase):
    def test_q1_preserves_hole_and_pitch_and_passes_absolute_pth_rules(self):
        q=b.PLAN['Q1']
        self.assertEqual(q['holeMm'],.75)
        self.assertGreaterEqual((q['padMm'][0]-q['holeMm'])/2,.18)
        self.assertGreaterEqual(1.27-q['padMm'][0],.1524)

    def test_xt60_uses_approved_holes_without_shrinking_existing_pads(self):
        for d in ['J1','J2','J4']:
            self.assertEqual(b.PLAN[d]['holeMm'],4)
            self.assertGreaterEqual((min(b.PLAN[d]['padMm'])-4)/2,.25)

    def test_xt30_has_fit_clearance_and_annular_margin(self):
        p=b.PLAN['J5']
        self.assertEqual((p['holeMm'],p['padMm']),(2.5,(3.5,3.5)))
        self.assertGreater(p['holeMm']-.08,2.2)
        self.assertGreaterEqual((min(p['padMm'])-p['holeMm'])/2,.25)

    def test_native_plan_changes_no_pad_identity_or_center(self):
        for fp,target in b.footprint_plan().items():
            for p in target['pads']:
                old=p['before'];new=p['after']
                self.assertEqual(old['num'],new['num'])
                self.assertEqual((old['centerX'],old['centerY'],old['padAngle']),
                                 (new['centerX'],new['centerY'],new['padAngle']))
                self.assertEqual(old['plated'],new['plated'])


if __name__=='__main__':unittest.main(verbosity=2)
