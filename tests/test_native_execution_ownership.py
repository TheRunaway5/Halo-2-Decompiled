#!/usr/bin/env python3
"""Ownership resolution must not reinterpret counters or accept open dispatch."""
from pathlib import Path
import sys,unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from audit_native_execution_ownership import selector,forwarder_kind,handoffs,historical_source,sha,effect_exports,EFFECT

class OwnershipTests(unittest.TestCase):
    def test_export_selector_requires_complete_exclusive_control_flow(self):
        body='void invoke(){if(entry==0x10u)first(m);else if(entry==0x20u)second(m);else throw std::invalid_argument("unknown");}'
        self.assertEqual(selector(body),{16:'first',32:'second'})
        for changed in(body.replace('if(entry==0x10u)','entry^=1u;if(entry==0x10u)'),body.replace('else if','if'),body.replace('0x20u','0x10u')):
            with self.assertRaises(ValueError):selector(changed)
    def test_local_callback_forwarding_is_separate_from_real_owner_calls(self):
        body='void routine(){const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};instruction(0x10u);call(m,a,o,s,0x20u,0x30u);}'
        self.assertEqual(forwarder_kind(body,['s.instruction(pc)']),'local_unchanged_pc_callback_lambda')
        self.assertEqual(handoffs(body)['literal_effect_required_owner_calls'],[{'instruction':'0x20','target':'0x30'}])
        self.assertFalse(handoffs(body)['execution_of_candidates_proved'])
        changed=body.replace('s.instruction(pc)','s.instruction(pc+1u)')
        self.assertEqual(forwarder_kind(changed,['s.instruction(pc+1u)']),'dynamic_callback_or_pc_mapping_requires_owner_audit')
    def test_frozen_whole_source_hash_is_mandatory(self):
        self.assertEqual(historical_source('old',sha('old')),('old',[]))
        self.assertEqual(historical_source('changed',sha('old')),(None,[]))
    def test_real_fixed_image_export_owner_fails_closed_on_mutation(self):
        source=(ROOT/EFFECT).read_text();entries,_=effect_exports(source)
        self.assertEqual(entries['marker_transform_product'],['145560'])
        self.assertEqual(entries['allocate_effect_datum'],['16e6a0'])
        with self.assertRaises(ValueError):effect_exports(source.replace('0x145560u','0x145564u',1))

if __name__=='__main__':unittest.main()
