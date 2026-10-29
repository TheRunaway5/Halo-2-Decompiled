import ctypes
import json
from pathlib import Path
import struct
import unittest

from multiplayer.master_server import matchmaking_ratings as port


class OriginalRatingsNativeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture=json.loads((Path(__file__).resolve().parents[3]/
            'tests/fixtures/matchmaking_ratings_original.json').read_text())

    def test_original_raw_effective_and_advertisement_through_real_abi(self):
        self.assertEqual(self.fixture['original_sha256'],
            '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d')
        self.assertEqual(len(self.fixture['rosters']),300)
        for case in self.fixture['rosters']:
            with self.subTest(case=case):
                limits=self.fixture['maps'][case['map']]
                actual=port.raw_statistics(case['raw'],case['flags'])
                self.assertEqual([actual.average,actual.maximum,actual.minimum,actual.rated_count],case['statistics'])
                self.assertEqual(port.adjusted_ranks(case['raw'],case['groups'],case['catalog_index'],limits),
                    tuple(case['adjusted']))
                self.assertEqual([actual.average,actual.maximum,actual.minimum,
                    port.advertised_minimum(actual.minimum,actual.maximum,limits)],case['advertisement'])

    def test_original_queries_use_the_actual_supplied_map(self):
        for case in self.fixture['searches']:
            limits=self.fixture['maps'][case['map']]
            for query in case['queries']:
                self.assertEqual(port.rank_window(case['minimum'],case['maximum'],query['radius'],limits),
                    (case['initialization'][3],query['minimum'],query['maximum']))
        for case in self.fixture['defaults']:
            self.assertEqual(port.rank_limits(),tuple(case['table']))
        for limits in self.fixture['maps'].values():
            header=bytearray(0x614)
            struct.pack_into('<i',header,0,4)
            header[0x51c:0x59c]=bytes(limits)
            self.assertEqual(port.rank_limits(bytes(header)),tuple(limits))

    def test_invalid_ffi_operands_leave_output_untouched(self):
        lib=port.native()
        raw=(ctypes.c_int32*16)(*[0]*16);flags=(ctypes.c_uint32*16)(*[0]*16)
        output=(ctypes.c_int32*4)(777,777,777,777)
        self.assertFalse(lib.h2_matchmaking_raw_ratings(17,raw,flags,output,4))
        self.assertEqual(tuple(output),(777,)*4)
        raw[0]=128
        self.assertFalse(lib.h2_matchmaking_raw_ratings(1,raw,flags,output,4))
        self.assertEqual(tuple(output),(777,)*4)
        table=(ctypes.c_uint8*128)(*[0]*128)
        self.assertFalse(lib.h2_matchmaking_rank_window(0,0,0,table,128,output,3))
        self.assertEqual(tuple(output),(777,)*4)
        with self.assertRaises(ValueError):port.rank_window(1<<40,1<<40)
        with self.assertRaises(ValueError):port.advertised_minimum(True,0)
