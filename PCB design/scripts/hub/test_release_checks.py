"""Regression tests for the release gate, including harmless IDs and real copper edits."""
import importlib
import unittest


class ReleaseChecksTests(unittest.TestCase):
    def test_gerber_comparison_ignores_unused_apertures_but_not_used_sizes(self):
        module=importlib.import_module('repaired_hub')
        canonical=getattr(module,'canonical_gerber_operations',None)
        self.assertTrue(callable(canonical),'Missing effective Gerber operation comparison')
        old='G04 time old*\n%ADD10C,1.5*%\n%ADD11C,0.5*%\nG54D10*\nG01X100Y200D03*\nM02*\n'
        same='G04 time new*\n%ADD18C,1.5*%\n%ADD19C,0.7*%\nG54D18*\nG01X100Y200D03*\nM02*\n'
        self.assertEqual(canonical(old),canonical(same))
        self.assertNotEqual(canonical(old),canonical(same.replace('1.5','1.6')))

    def test_ground_acceptance_rejects_disconnected_fill_only_island(self):
        from shapely.geometry import Point,box
        module=importlib.import_module('repaired_hub')
        model=dict(items=[dict(net='GND',shape=Point(10,10).buffer(2),layers=(1,2),kind='pad',label='P1.GND')],
                   outline=box(-50,-50,200,200),antenna=box(300,300,400,400))
        pieces=[dict(layer=1,source=[0,0,'L',20,0,20,20,0,20,0,0]),
                dict(layer=2,source=[0,0,'L',20,0,20,20,0,20,0,0]),
                dict(layer=1,source=[100,100,'L',110,100,110,110,100,110,100,100])]
        with self.assertRaises(AssertionError):module.check_ground_pieces(model,pieces)
        report=module.check_ground_pieces(model,pieces[:2])
        self.assertEqual(report['floatingFillPieces'],0)

    def test_slot_matching_rejects_rotation_and_reuse_of_one_actual_slot(self):
        from shapely.geometry import LineString
        module=importlib.import_module('repaired_hub')
        match=getattr(module,'match_slot_geometries',None)
        self.assertTrue(callable(match),'Missing orientation/bijection-aware slot comparison')
        expected=LineString([(0,0),(6,0)]).buffer(.9,quad_segs=64)
        actual=[dict(start=(0,0),end=(6,0),diameter=1.8)]
        self.assertEqual(match([expected],actual),[0])
        with self.assertRaises(AssertionError):
            match([expected],[dict(start=(3,-3),end=(3,3),diameter=1.8)])
        with self.assertRaises(AssertionError):
            match([expected,expected],[actual[0],dict(start=(20,0),end=(26,0),diameter=1.8)])

    def test_excellon_reads_actual_tools_and_plated_slots_not_comments(self):
        module=importlib.import_module('repaired_hub')
        parse=getattr(module,'read_metric_drills',None)
        self.assertTrue(callable(parse),'Missing drill-output inspection')
        text=';TYPE=PLATED\nM48\nMETRIC,LZ,0000.000000\nT01C0.8\nT02C1.8\n%\nT01\nX-1.5Y2\nT02\nX4Y6G85X10Y6\nM30\n'
        result=parse(text)
        self.assertEqual(result['round'],[{'x':-1.5,'y':2.,'diameter':.8}])
        self.assertEqual(result['slots'],[{'start':(4.,6.),'end':(10.,6.),'diameter':1.8}])
        self.assertEqual(result['plated'],True)
        with self.assertRaises(ValueError):parse('INCH\nT01C0.03\n%\nT01\nX1Y2\n')

    def test_track_comparison_allows_only_identity_changes_not_geometry(self):
        module=importlib.import_module('repaired_hub')
        compare=getattr(module,'compare_repour_records',None)
        self.assertTrue(callable(compare),'Missing fail-closed repour preservation gate')
        def archive(rid,x2=100,extra=None):
            records={('LINE',rid):{'body':{'layerId':1,'netName':'A','startX':0,'startY':0,
                                          'endX':x2,'endY':0,'width':10}},
                     ('COMPONENT','c1'):{'body':{'x':1,'y':2}},
                     ('PAD_NET','p1'):{'body':{'padNet':'A'}}}
            if extra:records.update(extra)
            return {'pcb':{'records':records},'device':{'records':{('META','META'):{'body':{'title':'unchanged'}}}}}
        report=compare(archive('old'),archive('new',extra={('FILL','f1'):{'body':{'layerId':1}}}),'pcb')
        self.assertEqual(report['removedTrackIds'],['old'])
        self.assertEqual(report['addedTrackIds'],['new'])
        self.assertEqual(report['addedFillCount'],1)
        with self.assertRaises(AssertionError):compare(archive('old'),archive('new',x2=101),'pcb')
        with self.assertRaises(AssertionError):
            compare(archive('old'),archive('old',extra={('PAD_NET','p1'):{'body':{'padNet':'B'}}}),'pcb')


if __name__=='__main__':unittest.main()
