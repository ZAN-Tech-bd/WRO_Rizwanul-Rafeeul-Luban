"""Ground review: native geometry, isolation policy, and serialized-fill acceptance tests."""
import unittest
from shapely.geometry import Point

import final_ground as g


class FinalGroundTests(unittest.TestCase):
    def test_source_negatives_are_foreign_to_gnd(self):
        for net in ['BAT-','DC-','BAT+','DC+','V12']:
            self.assertGreaterEqual(g.clearance(net,Point(0,2000)),40)

    def test_left_side_other_nets_receive_extra_clearance(self):
        self.assertGreaterEqual(g.clearance('+5V',Point(0,700)),40)
        self.assertGreaterEqual(g.clearance('LEDR_A',Point(400,1200)),40)
        self.assertGreaterEqual(g.clearance('DHT_S1',Point(3500,3000)),12)

    def test_native_pad_geometry_matches_live_getters(self):
        model=g.load_model()
        self.assertEqual(len(model['components']),77)
        self.assertEqual(len(model['pads']),274)
        self.assertEqual(model['padNetMismatches'],[])

    def test_j3_slot_axes_and_sizes_follow_native_rotation(self):
        model=g.load_model()
        pads=[p for p in model['pads'] if p['des']=='J3']
        self.assertEqual(len(pads),6)
        for p in pads:
            self.assertAlmostEqual(p['hole'].bounds[2]-p['hole'].bounds[0],7.8/.0254,places=3)
            self.assertAlmostEqual(p['hole'].bounds[3]-p['hole'].bounds[1],1.8/.0254,places=3)

    def test_generated_serialized_pieces_are_hole_free(self):
        pieces=g.load_plan()['pieces']
        self.assertGreater(len(pieces),0)
        for p in pieces:
            poly=g.from_source(p['source'])
            self.assertTrue(poly.is_valid)
            self.assertEqual(len(poly.interiors),0)
            self.assertGreater(poly.area,1)

    def test_planned_copper_has_no_foreign_overlap(self):
        r=g.verify_plan()
        self.assertEqual(r['foreignOverlaps'],[])
        self.assertEqual(r['clearanceViolations'],[])
        self.assertEqual(r['disconnectedGnd'],[])


if __name__=='__main__':unittest.main(verbosity=2)
