"""Silk regression gates: catch undersized text or hidden/unrelated edits."""
import importlib
import unittest
from pathlib import Path


class SilkStyleTests(unittest.TestCase):
    def setUp(self):
        self.module = importlib.import_module('repaired_hub')

    def test_visible_silk_is_enlarged_without_shrinking_existing_style(self):
        fn = getattr(self.module, 'silk_style_plan', None)
        self.assertTrue(callable(fn), 'Missing safe silk style planner')
        records = {
            ('STRING', 'small'): {'body': dict(layerId=3, text='GND', fontSize=24, strokeWidth=5)},
            ('STRING', 'large'): {'body': dict(layerId=4, text='TITLE', fontSize=60, strokeWidth=5)},
            ('ATTR', 'des'): {'body': dict(layerId=3, key='Designator', value='R1',
                                         valueVisible=True, fontSize=28, strokeWidth=8)},
            ('ATTR', 'hidden'): {'body': dict(layerId=3, key='Name', value='secret name',
                                            valueVisible=False, fontSize=20, strokeWidth=4)},
            ('STRING', 'doc'): {'body': dict(layerId=48, text='body', fontSize=20, strokeWidth=4)},
        }
        plan = fn(records)
        self.assertEqual(plan, [
            dict(kind='STRING', id='small', text='GND', fontSize=40, lineWidth=6),
            dict(kind='STRING', id='large', text='TITLE', fontSize=60, lineWidth=6),
            dict(kind='ATTR', id='des', text='R1', fontSize=40, lineWidth=8),
        ])

    def test_native_model_uses_corrected_footprints_not_stale_live_inputs(self):
        fn = getattr(self.module, 'copper_model', None)
        self.assertTrue(callable(fn), 'Missing current-project native copper model')
        path = Path(__file__).resolve().parents[2]/'hub/WRO_HUB_REPAIRED_silk_styled_20261027-2015.epro2'
        model = fn(path)
        self.assertEqual(len(model['components']), 77)
        self.assertEqual(len(model['pads']), 274)
        q1 = next(p for p in model['pads'] if p['des']=='Q1' and p['num']=='1')
        self.assertAlmostEqual(min(q1['native']['defaultPad']['width'],q1['native']['defaultPad']['height'])*.0254,
                               1.115, places=5)
        j5 = next(p for p in model['pads'] if p['des']=='J5' and p['num']=='1')
        self.assertAlmostEqual(j5['native']['hole']['width']*.0254,2.5,places=5)
        self.assertEqual(len(model['fills']),2360)
        self.assertEqual(model['outline'].bounds,(-675,0,4695,3975))

    def test_text_clearance_rejects_pad_collision_and_keeps_valid_text_fixed(self):
        fn=getattr(self.module,'place_silk',None)
        self.assertTrue(callable(fn),'Missing bounded silk placement')
        from shapely.geometry import box
        rows=[dict(id='a',text='R1',kind='ATTR',x=50,y=50,layer=3,
                   box=dict(minX=35,minY=40,maxX=65,maxY=60)),
              dict(id='b',text='GND',kind='STRING',x=150,y=150,layer=3,
                   box=dict(minX=130,minY=140,maxX=170,maxY=160))]
        model=dict(pads=[dict(shape=box(40,40,60,60),layers=(1,2))],outline=box(0,0,200,200))
        placed=fn(rows,model)
        a,b=placed
        self.assertGreaterEqual(box(a['box']['minX'],a['box']['minY'],a['box']['maxX'],a['box']['maxY']).distance(model['pads'][0]['shape']),6)
        self.assertEqual(b,rows[1])


if __name__ == '__main__':
    unittest.main()
