#!/usr/bin/env python3
"""Bounded original Vista cache directory regression coverage."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_rendering import FormatError
from export_vista_menu_ui import VistaCache


def fixture(path,bias=0,*,bad_pointer=False,bad_datum=False):
    data=bytearray(8192)
    def word(at,value):struct.pack_into('<I',data,at,value)
    word(0,0x68656164);word(4,8);word(8,len(data));word(2044,0x666f6f74)
    word(16,4096);word(32,bias)
    build=b'11081.07.04.30.0934.main';data[300:300+len(build)]=build
    word(4096+8,32);word(4096+24,2);word(0x2cc,2)
    name=b'ui\\test\\bounded\0';data[2048:2048+len(name)]=name
    word(0x2d0,2048);word(0x2d4,len(name));word(0x2d8,2200)
    word(2200,0);word(2204,0xffffffff)
    word(4128,int.from_bytes(b'bitm','big'))
    word(4132,0x12340001 if bad_datum else 0x12340000)
    word(4136,bias-4096+(9000 if bad_pointer else 6000));word(4140,8)
    word(4148,0xffffffff)
    data[6000:6008]=b'UIBOUNDS'
    path.write_bytes(data)


class VistaDirectoryTests(unittest.TestCase):
    def test_mainmenu_and_multiplayer_have_distinct_metadata_mappings(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'cache.map'
            for bias in (0,0x1400000):
                fixture(path,bias)
                cache=VistaCache(path)
                try:
                    _,at=cache.tag(0x12340000,'bitm')
                    self.assertEqual(at,6000)
                    self.assertEqual(bytes(cache.data[at:at+8]),b'UIBOUNDS')
                    self.assertEqual(len(cache.tags),1) # vacant filename was not read
                finally:cache.close()

    def test_metadata_bounds_remain_enforced_with_multiplayer_bias(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'cache.map';fixture(path,0x1400000,bad_pointer=True)
            with self.assertRaises(FormatError):VistaCache(path)

    def test_datum_must_match_its_original_directory_slot(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'cache.map';fixture(path,bad_datum=True)
            with self.assertRaises(FormatError):VistaCache(path)


if __name__=='__main__':unittest.main()
