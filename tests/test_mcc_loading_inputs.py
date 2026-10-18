"""MCC resource-role metadata never manufactures a complete loading bank."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
from mcc_loading_assets import DRAW_INPUTS, presentation_inputs


class LoadingInputTests(unittest.TestCase):
    def banks(self):
        banks = {}
        for slot, role, tag, ordinal in DRAW_INPUTS:
            bank = banks.setdefault(tag, dict(cache="mainmenu.map", tag=tag, datum="e1000001", frames=[]))
            bank["frames"].append(dict(frame=ordinal, path=f"loading/mcc/{slot}.dds",
                width=512, height=512, dds_sha256=str(slot), source_record_hex="owner-record"))
        return list(banks.values())

    def test_five_inputs_preserve_exact_owners_without_claiming_vista_bank(self):
        result = presentation_inputs(self.banks())
        self.assertEqual([r["texture_slot"] for r in result["inputs"]], [0, 1, 3, 4, 5])
        self.assertEqual(result["missing_bitmap_inputs"], [])
        self.assertEqual(result["separate_consumers"][0]["texture_slot"], 11)
        self.assertEqual(result["separate_consumers"][1]["role"], "loading_ambience")
        self.assertFalse(result["readiness_signal"])
        self.assertFalse(result["exact_vista_bank"])
        for record in result["inputs"]:
            self.assertEqual(record["source_record_hex"], "owner-record")
            self.assertEqual(record["datum"], "e1000001")

    def test_missing_input_is_described_without_fabrication_or_whole_game_gate(self):
        banks = self.banks()
        banks[2]["frames"].pop()
        result = presentation_inputs(banks)
        self.assertEqual([r["texture_slot"] for r in result["inputs"]], [0, 1, 3, 5])
        self.assertEqual(result["missing_bitmap_inputs"][0]["texture_slot"], 4)

    def test_ambiguous_bitmap_owners_and_frames_reject(self):
        banks = self.banks()
        with self.assertRaisesRegex(FormatError, "Duplicate MCC loading bitmap owner"):
            presentation_inputs(banks + [banks[0]])
        banks[0]["frames"].append(banks[0]["frames"][0])
        with self.assertRaisesRegex(FormatError, "Duplicate MCC loading bitmap frame"):
            presentation_inputs(banks)


if __name__ == "__main__":
    unittest.main()
