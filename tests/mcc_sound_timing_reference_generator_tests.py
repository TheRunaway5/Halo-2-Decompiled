#!/usr/bin/env python3
import copy
import importlib.util
import json
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('timing_generator',ROOT/'tools/generate_mcc_sound_timing_reference.py')
gen=importlib.util.module_from_spec(spec);spec.loader.exec_module(gen)
class PolicyGuards(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.facts=json.loads(gen.FACTS.read_text())
 def reject(self,change):
  facts=copy.deepcopy(self.facts);change(facts)
  with self.assertRaises((AssertionError,ValueError,struct_error())):gen.generate(facts)
 def test_exact_source_and_exclusions(self):
  self.assertEqual(gen.generate(self.facts),gen.OUTPUT.read_text());self.assertEqual(sum(k['admitted']for k in self.facts['keys']),24621)
 def test_mutations(self):
  changes=[lambda d:d.update(schema='foreign'),lambda d:d['owners'].pop(),lambda d:d['keys'].pop(),lambda d:d.update(source_reference71_sha256='0'*64),lambda d:d['keys'][0].update(key_sha256='0'*64),lambda d:d['keys'][0].update(payload_sha256='ff'),lambda d:d['keys'][0]['original'].__setitem__(0,3),lambda d:d['keys'][0]['original'].__setitem__(1,3),lambda d:d['keys'][0]['original'].__setitem__(2,5),lambda d:d['keys'][0]['original'].__setitem__(3,0),lambda d:d['keys'][0]['original'].__setitem__(4,32768),lambda d:d['keys'].__setitem__(1,copy.deepcopy(d['keys'][0])),lambda d:d['owners'].__setitem__(1,copy.deepcopy(d['owners'][0])),lambda d:d['owners'][0]['source_maps'][0].update(sha256='bad'),lambda d:d['keys'][0].update(admitted=False)]
  for n,change in enumerate(changes):
   with self.subTest(mutation=n):self.reject(change)
 def test_all_three_pitch_omissions_are_mandatory(self):
  for index,row in enumerate(self.facts['keys']):
   if not row['admitted']:
    with self.subTest(key=row['key_sha256']):self.reject(lambda d:d['keys'][index].update(admitted=True))
def struct_error():
 import struct
 return struct.error
if __name__=='__main__':unittest.main()
