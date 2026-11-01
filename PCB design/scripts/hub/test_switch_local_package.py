"""Offline J3-only custom-device patch: no unsafe library API and no geometry edits."""
import tempfile
import unittest
from pathlib import Path
import repaired_hub as R

SOURCE=Path(__file__).resolve().parents[2]/'hub/WRO_HUB_REPAIRED_ground_fixed_20261027-2028.epro2'


class LocalDeviceTests(unittest.TestCase):
    def test_custom_device_removes_catalogue_binding_without_touching_other_devices(self):
        fn=getattr(R,'switch_metadata_package',None)
        self.assertTrue(callable(fn),'Missing J3-only offline package repair')
        with tempfile.TemporaryDirectory() as folder:
            dest=Path(folder)/'local.epro2';report=fn(SOURCE,dest)
            old=R.read_archive(SOURCE);new=R.read_archive(dest)
            device=report['deviceUuid'];meta=new[device]['records'][('META','META')]['body']
            self.assertEqual(meta['title'],'KCD4-203-B')
            self.assertEqual(meta['source'],'')
            self.assertEqual(meta['attributes']['Footprint'],R.TARGET['footprint'])
            self.assertEqual(meta['attributes']['Supplier Part'],'')
            changed=[]
            for doc in old:
                self.assertEqual(set(old[doc]['records']),set(new[doc]['records']))
                for key,a in old[doc]['records'].items():
                    b=new[doc]['records'][key]
                    if a['raw']==b['raw']:continue
                    changed.append((doc,key))
                    if doc==device:
                        self.assertEqual(key,('META','META'));continue
                    self.assertIn(doc,(R.TARGET['pcb'],R.TARGET['page']))
                    self.assertEqual(key[0],'ATTR')
                    self.assertIn(a['body']['parentId'],report['j3InstanceIds'])
                    self.assertEqual({k:v for k,v in a['body'].items() if k!='value'},
                                     {k:v for k,v in b['body'].items() if k!='value'})
            self.assertGreater(len(changed),10)


if __name__=='__main__':unittest.main()
