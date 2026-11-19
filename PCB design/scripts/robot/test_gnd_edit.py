"""Acceptance regression: pour removal must reject changes to copper, nets or other fills."""
import importlib
import importlib.util
import unittest


class GroundRemovalTests(unittest.TestCase):
    def test_only_selected_fills_and_fill_allocator_can_be_removed(self):
        self.assertIsNotNone(importlib.util.find_spec('gnd_edit'),'Missing Robot pour-removal verifier')
        verify=importlib.import_module('gnd_edit').verify_records
        rec=lambda body:{'body':body,'raw':str(body)}
        base={('FILL','g1'):rec(dict(netName='GND',layerId=1)),
              ('FILL','power'):rec(dict(netName='+5V',layerId=1)),
              ('LINE','trace'):rec(dict(netName='A',width=10)),
              ('PAD_NET','pad1'):rec(dict(padNet='GND')),
              ('ELE_PLACEHOLDER','f'):rec(dict(dataType='FILL',max=4)),
              ('ELE_PLACEHOLDER','l'):rec(dict(dataType='LINE',max=5))}
        final={k:v for k,v in base.items() if k not in {('FILL','g1'),('ELE_PLACEHOLDER','f')}}
        self.assertEqual(verify(base,final,{'g1'})['fillsRemoved'],1)
        copper_change=dict(final);copper_change[('LINE','trace')]=rec(dict(netName='B',width=10))
        with self.assertRaises(AssertionError):verify(base,copper_change,{'g1'})
        with self.assertRaises(AssertionError):verify(base,{k:v for k,v in final.items() if k!=('FILL','power')},{'g1'})
        with self.assertRaises(AssertionError):verify(base,{k:v for k,v in final.items() if k!=('PAD_NET','pad1')},{'g1'})


if __name__=='__main__':unittest.main()
