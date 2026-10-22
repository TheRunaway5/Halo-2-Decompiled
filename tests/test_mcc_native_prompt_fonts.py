#!/usr/bin/env python3
"""Actual MCC font-owner admission without a port-authored prompt dependency."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/helpers"))
from mcc_online_ui import native_prompt_fonts

SELECTED = None


class NativeMccPromptFonts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cache_source = Path(SELECTED).resolve(strict=True)
        cls.provenance = json.loads((cls.cache_source / "provenance.json").read_text())
        cls.menu = json.loads((cls.cache_source / "ui/mcc-menu-assets.json").read_text())
        cls.profile_path = ROOT / "tools/data/gameplay-font-reference.json"
        cls.profile = json.loads(cls.profile_path.read_text())

    def setUp(self):
        scratch = os.environ.get("HALO2_TEST_SCRATCH")
        self.assertTrue(scratch, "Run actual MCC font cases through tools/run_test.py")
        self.temp = tempfile.TemporaryDirectory(prefix="native-prompts-", dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.root = self.base / "port"
        profile = self.root / "tools/data/gameplay-font-reference.json"
        profile.parent.mkdir(parents=True)
        shutil.copyfile(self.profile_path, profile)
        self.cache = self.base / "cache"
        fonts = self.cache / "halo2/h2_fonts"
        fonts.mkdir(parents=True)
        for name in (*self.profile["mcc_files"], "font_table.txt"):
            shutil.copyfile(self.cache_source / "halo2/h2_fonts" / name, fonts / name)
        self.source = copy.deepcopy(self.provenance)
        self.fonts = copy.deepcopy(self.menu["fonts"])

    def validate(self):
        native_prompt_fonts(self.root, self.cache, self.source, self.fonts, {})

    def test_actual_native_fonts_need_no_authored_source_or_cache_bank(self):
        self.assertFalse((self.root / "resources/presentation/prompts").exists())
        self.assertFalse((self.cache / "presentation/prompts").exists())
        self.validate()

    def test_old_invalid_authored_owner_is_ignored_without_mutation(self):
        old = self.cache / "presentation/prompts"
        old.mkdir(parents=True)
        (old / "prompt-source.tsv").write_bytes(b"foreign old binding\n")
        (old / "manifest.json").write_bytes(b"not even JSON\n")
        (old / "atlas.dds").write_bytes(b"foreign old pixel owner\n")
        self.source["port_authored_prompts"] = {"custom": True}
        before = {p.name: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in old.iterdir()}
        self.validate()
        self.assertEqual(before, {p.name: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in old.iterdir()})
        self.assertEqual(self.source["port_authored_prompts"], {"custom": True})

    def test_changed_native_pixels_reject_even_with_rewritten_import_rows(self):
        path = self.cache / "halo2/h2_fonts/conduit-13"
        stamp = path.stat()
        raw = bytearray(path.read_bytes())
        raw[-1] ^= 1
        path.write_bytes(raw)
        os.utime(path, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
        replacement = {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}
        self.source["source_files"]["h2_fonts/conduit-13"] = replacement
        self.fonts["halo2/h2_fonts/conduit-13"] = replacement
        with self.assertRaisesRegex(ValueError, "native source profile"):
            self.validate()

    def test_wrong_selector_order_rejects(self):
        table = self.cache / "halo2/h2_fonts/font_table.txt"
        names = table.read_text().splitlines()
        names[4], names[5] = names[5], names[4]
        table.write_text("\n".join(names) + "\n")
        with self.assertRaisesRegex(ValueError, "selector table"):
            self.validate()

    def test_missing_native_font_cannot_fall_back_to_old_prompt_bank(self):
        (self.cache / "halo2/h2_fonts/conduit-9").unlink()
        old = self.cache / "presentation/prompts"
        old.mkdir(parents=True)
        (old / "prompt-source.tsv").write_bytes(b"H2_VISTA_PROMPTS_V1\n.\n")
        with self.assertRaisesRegex(ValueError, "Incomplete MCC UI cache"):
            self.validate()

    def test_foreign_linked_native_font_rejects(self):
        path = self.cache / "halo2/h2_fonts/conduit-9"
        path.unlink()
        path.symlink_to(self.cache_source / "halo2/h2_fonts/conduit-9")
        with self.assertRaisesRegex(ValueError, "symbolic-link owner"):
            self.validate()

    def test_foreign_font_profile_and_malformed_provenance_reject(self):
        profile = self.root / "tools/data/gameplay-font-reference.json"
        original = profile.read_bytes()
        profile.write_bytes(original + b" ")
        with self.assertRaisesRegex(ValueError, "font profile"):
            self.validate()
        profile.write_bytes(original)
        self.source["source_files"]["h2_fonts/conduit-9"] = "foreign owner"
        with self.assertRaisesRegex(ValueError, "native source profile"):
            self.validate()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mcc-root", type=Path, required=True)
    args, extra = parser.parse_known_args()
    SELECTED = args.mcc_root
    unittest.main(argv=[sys.argv[0], *extra])
