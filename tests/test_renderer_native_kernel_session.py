#!/usr/bin/env python3
"""Owned refresh preflight: borrowed reference refusal must be atomic."""
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from renderer_native_kernel_session import prepare_refresh


class RefreshTests(unittest.TestCase):
    def test_preflight(self):
        with tempfile.TemporaryDirectory(dir=os.environ['HALO2_TEST_SCRATCH'])as owner:
            owner=Path(owner);root=owner/'root';source=owner/'source'
            for directory in (root,source):directory.mkdir();(directory/'src').mkdir();(directory/'tests').mkdir()
            (root/'src/one.cpp').write_text('new');(source/'src/one.cpp').write_text('old')
            reference=root/'tests/fixtures';reference.mkdir();(reference/'gold.json').write_text('reference')
            (source/'tests/fixtures').symlink_to(reference,target_is_directory=True)
            for names in (['src/one.cpp','tests/fixtures/gold.json'],['src/one.cpp','missing.cpp'],
                ['src/one.cpp','../escape.cpp'],['src/one.cpp','/absolute.cpp'],['src/one.cpp','src/one.cpp'],[],['.']):
                with self.assertRaises((ValueError,OSError)):prepare_refresh(root,source,names)
                self.assertEqual((source/'src/one.cpp').read_text(),'old')
                self.assertEqual((reference/'gold.json').read_text(),'reference')
            # The same owner remains usable after every rejected plan.
            plan=prepare_refresh(root,source,['src/one.cpp']);self.assertEqual(len(plan),1)
            name,destination,data,before=plan[0]
            self.assertEqual(name,'src/one.cpp');self.assertEqual(destination,source/'src/one.cpp');self.assertEqual(data,b'new')
            self.assertEqual(len(before),64);self.assertEqual(destination.read_text(),'old')
            destination.write_bytes(data);self.assertEqual(destination.read_text(),'new')
            (root/'src/two.cpp').write_text('added')
            self.assertIsNone(prepare_refresh(root,source,['src/two.cpp'])[0][3])


if __name__=='__main__':unittest.main()
