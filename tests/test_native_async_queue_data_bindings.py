#!/usr/bin/env python3
"""Exact async owner/delegation controls, independent of value execution."""
import hashlib
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_async_queue_data_replacements as binding

class AsyncQueueBindingTests(unittest.TestCase):
    def test_whole_abi_reference_and_idempotence(self):
        original=(ROOT/'tests/reference/native_tu15_async_order_reference.hpp').read_text()
        self.assertEqual(binding.digest(original),binding.ABI_REFERENCE_PIN)
        installed=binding.replace_abi(original);self.assertEqual(binding.replace_abi(installed),installed)
        self.assertEqual(binding.replace_abi((ROOT/'src/platform/native_tu15_async_order.hpp').read_text()),installed)
        for changed in(original+'\n',original.replace('0x4ef1bcu','0x4ef1b8u',1),
            installed.replace('site_base);','site_base+1u);',1),installed.replace('std::forward<Compare>(compare)','compare(s)',1)):
            with self.assertRaises(ValueError):binding.replace_abi(changed)

    def test_public_two_owner_round_trip_and_other_bytes(self):
        source=(ROOT/'src/platform/source_async_jobs.cpp').read_text();original=binding.canonical_public(source)
        installed=binding.replace_public(original);self.assertEqual(binding.replace_public(installed),installed)
        self.assertEqual(binding.canonical_public(installed),original)
        self.assertEqual(binding.reference_public(),{name:binding.unique_function(original,name)for name in binding.PUBLIC_PINS})
        # Removing only the two complete target bodies/include restores every
        # unrelated request/list/I/O algorithm byte, not a broad normalization.
        for changed in(original.replace('incoming.priority>other.priority','incoming.priority>=other.priority',1),
            original.replace('source async insertion needs a leased node','source async insertion needs any node',1),
            installed.replace('unlinked(queue,node);','unlinked(queue,node+1u);',1),
            installed.replace('native_async_queue_value_detail::before','native_async_queue_value_detail::insert',1)):
            self.assertNotEqual(changed,original if 'native_async_queue_value_detail'not in changed else installed)
            with self.assertRaises(ValueError):binding.replace_public(changed)

    def test_preserved_public_reference_is_exact(self):
        path=ROOT/'tests/reference/native_tu15_async_queue_value_reference.hpp'
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),binding.VALUE_REFERENCE_PIN)

if __name__=='__main__':unittest.main()
