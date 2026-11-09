#!/usr/bin/env python3
import hashlib
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_async_promotion_data_replacements as binding
class PromotionBindings(unittest.TestCase):
    def test_original_abi_and_exact_delegation(self):
        original=(ROOT/'tests/reference/native_tu15_async_promotion_reference.hpp').read_text()
        self.assertEqual(binding.digest(original),binding.ABI_REFERENCE_PIN)
        installed=binding.replace_abi(original);self.assertEqual(binding.replace_abi(installed),installed)
        for changed in(original+'\n',original.replace('g[6]&255u','g[6]&127u',1),installed.replace('std::forward<Insert>(insert)','insert(state)',1)):
            self.assertNotIn(changed,(original,installed))
            with self.assertRaises(ValueError):binding.replace_abi(changed)
    def test_public_pinned_round_trip_and_mutations(self):
        source=(ROOT/'src/platform/source_async_jobs.cpp').read_text();original=binding.canonical_public(source)
        installed=binding.replace_public(original);self.assertEqual(binding.replace_public(installed),installed)
        self.assertEqual(binding.canonical_public(installed),original)
        self.assertEqual(binding.reference_public(),binding.ORIGINAL_PUBLIC)
        for changed in(original.replace('queue.nodes[node].job!=job','queue.nodes[node].job==job',1),
            original.replace('queue.nodes[node].job=source_async_none;','queue.nodes[node].job=0;',1),
            installed.replace('chain(queue,queue.work_head);});','chain(queue,queue.free_head);});',1)):
            self.assertNotIn(changed,(original,installed))
            with self.assertRaises(ValueError):binding.replace_public(changed)
    def test_reference_control_is_immutable(self):
        path=ROOT/'tests/reference/native_tu15_async_promotion_value_reference.hpp'
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),binding.VALUE_REFERENCE_PIN)
if __name__=='__main__':unittest.main()
