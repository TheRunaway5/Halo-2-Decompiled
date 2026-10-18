#!/usr/bin/env python3
import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("stored_time_generator", ROOT / "tools/generate_mcc_sound_stored_time_reference.py")
GEN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GEN)


class StoredTimeGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = json.loads((ROOT / "tools/mcc-sound-stored-time-reference-profiles.json").read_text())

    def test_exact_profile(self):
        self.assertEqual(len(self.policy["values"]), 7993)
        self.assertEqual(len(self.policy["owners"]), 41)
        self.assertEqual(len(self.policy["payloads"]), 24624)
        self.assertEqual(GEN.generate(self.policy), (ROOT / "src/mcc_sound_stored_time_reference_profiles.inc").read_text())

    def rejects(self, mutate):
        policy = copy.deepcopy(self.policy)
        mutate(policy)
        with self.assertRaises((ValueError, KeyError)):
            GEN.generate(policy)

    def test_primary_header_guards(self):
        self.rejects(lambda p: p["values"][0].update(original_ms=12345))
        self.rejects(lambda p: p["values"][0]["original_reference"].update(root_hex="00" * 20))
        self.rejects(lambda p: p["values"][0]["original_reference"].update(root_sha256="00" * 32))
        self.rejects(lambda p: p["values"][0]["original_reference"].update(ms=12345))
        self.rejects(lambda p: p["values"][0]["original_reference"].update(full_salt=0xffffffff))

    def test_named_values_guards(self):
        self.rejects(lambda p: p["values"].append(p["values"][0]))
        self.rejects(lambda p: p["values"][0].update(path="custom"))
        self.rejects(lambda p: p["values"][0].update(key_sha256="00" * 32))
        self.rejects(lambda p: p["values"][0].update(original_ms=-1))

    def test_payload_and_owner_guards(self):
        self.rejects(lambda p: p["payloads"].append(p["payloads"][0]))
        self.rejects(lambda p: p["payloads"][0].update(permutation="custom"))
        self.rejects(lambda p: p["payloads"][0].update(payload_sha256="00"))
        self.rejects(lambda p: p["owners"].append(p["owners"][0]))
        self.rejects(lambda p: p["owners"][0].update(metadata_sha256="00"))
        self.rejects(lambda p: p.update(schema="custom"))


if __name__ == "__main__":
    unittest.main()
