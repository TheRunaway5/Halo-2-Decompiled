#!/usr/bin/env python3
"""Actual font-owner input selection only; no native binary, display or GPU."""
import argparse
import copy
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

import frontend_mcc_rendered_test as rendered

ROOT = Path(__file__).resolve().parents[1]
SELECTED = None


class RenderedInputPins(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.selected = Path(SELECTED).resolve(strict=True)
        cls.profile_path = ROOT / "tools/data/gameplay-font-reference.json"
        cls.profile = json.loads(cls.profile_path.read_text())
        cls.provenance = json.loads((cls.selected / "provenance.json").read_text())
        cls.menu = json.loads((cls.selected / "ui/mcc-menu-assets.json").read_text())

    def setUp(self):
        scratch = os.environ.get("HALO2_TEST_SCRATCH")
        self.assertTrue(scratch, "Use tools/run_test.py storage ownership")
        self.temp = tempfile.TemporaryDirectory(prefix="rendered-input-pins-", dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.root, self.cache = self.base / "port", self.base / "cache"
        self.source = copy.deepcopy(self.provenance)
        self.menu_copy = copy.deepcopy(self.menu)
        profile = self.root / "tools/data/gameplay-font-reference.json"
        profile.parent.mkdir(parents=True)
        shutil.copyfile(self.profile_path, profile)
        # These are identity-only source witnesses. Actual font pixels and the
        # native prompt validator below are genuine; nothing is executed here.
        names = ("tests/frontend_mcc_rendered_test.py", "tests/helpers/mcc_online_ui.py",
                 "tests/frontend_clans_http_test.py", "tests/frontend_parties_http_test.py",
                 "tests/frontend_online_tests.cpp", "src/ui/frontend_painter.cpp", "src/rendering/renderer/renderer.cpp",
                 "resources/frontend/mcc-native-layout.json")
        for name in names:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(("controlled source identity witness: " + name).encode())
        self.fonts = self.cache / "halo2/h2_fonts"
        self.fonts.mkdir(parents=True)
        for name in (*self.profile["mcc_files"], "font_table.txt"):
            shutil.copyfile(self.selected / "halo2/h2_fonts" / name, self.fonts / name)
        layout = self.cache / "presentation/ui/native-layout.json"
        layout.parent.mkdir(parents=True)
        layout.write_bytes(b"controlled layout identity witness; no layout/render acceptance")
        self.publish_metadata()

    def publish_metadata(self):
        (self.cache / "provenance.json").write_text(json.dumps(self.source))
        menu = self.cache / "ui/mcc-menu-assets.json"
        menu.parent.mkdir(parents=True, exist_ok=True)
        menu.write_text(json.dumps(self.menu_copy))

    def select(self):
        return rendered.rendered_input_identities(self.root, self.cache)

    def test_actual_native_font_inputs_need_no_authored_prompt_bank(self):
        self.assertFalse((self.root / "resources/presentation/prompts").exists())
        self.assertFalse((self.cache / "presentation/prompts").exists())
        sources, cache = self.select()
        self.assertIn("tools/data/gameplay-font-reference.json", sources)
        self.assertTrue({"halo2/h2_fonts/" + name for name in (*self.profile["mcc_files"], "font_table.txt")} <= cache.keys())
        self.assertFalse(any("presentation/prompts/" in name for name in (*sources, *cache)))

    def old_prompt_banks(self):
        for bank in (self.root / "resources/presentation/prompts", self.cache / "presentation/prompts"):
            bank.mkdir(parents=True)
            (bank / "manifest.json").write_bytes(b"invalid retained authored manifest\n")
            (bank / "prompt-source.tsv").write_bytes(b"foreign retained binding\n")
        self.source["port_authored_prompts"] = {"custom": "retained independently"}
        self.publish_metadata()

    def test_invalid_retained_authored_banks_are_ignored_and_unchanged(self):
        self.old_prompt_banks()
        files = [*self.root.glob("resources/presentation/prompts/*"), *self.cache.glob("presentation/prompts/*")]
        before = {str(path): (path.read_bytes(), path.stat().st_ino, path.stat().st_mtime_ns) for path in files}
        sources, cache = self.select()
        self.assertFalse(any("presentation/prompts/" in name for name in (*sources, *cache)))
        self.assertEqual(before, {str(path): (path.read_bytes(), path.stat().st_ino, path.stat().st_mtime_ns) for path in files})
        self.assertEqual(json.loads((self.cache / "provenance.json").read_text())["port_authored_prompts"],
                         {"custom": "retained independently"})

    def test_each_native_font_and_selector_table_is_required_without_legacy_fallback(self):
        self.old_prompt_banks()
        for name in (*self.profile["mcc_files"], "font_table.txt"):
            with self.subTest(owner=name):
                path, saved = self.fonts / name, self.fonts / (name + ".absent")
                path.rename(saved)
                try:
                    with self.assertRaises(ValueError):
                        self.select()
                finally:
                    saved.rename(path)

    def test_changed_native_pixels_cannot_be_rebound_by_import_metadata(self):
        name = "conduit-13"
        path = self.fonts / name
        stamp = path.stat()
        raw = bytearray(path.read_bytes())
        raw[-1] ^= 1
        path.write_bytes(raw)
        os.utime(path, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
        witness = rendered.identity(path)
        rebound = {"bytes": witness["bytes"], "sha256": witness["sha256"]}
        self.source["source_files"]["h2_fonts/" + name] = rebound
        self.menu_copy["fonts"]["halo2/h2_fonts/" + name] = rebound
        self.publish_metadata()
        with self.assertRaisesRegex(ValueError, "native source profile"):
            self.select()

    def test_foreign_profile_wrong_table_and_source_provenance_reject(self):
        profile = self.root / "tools/data/gameplay-font-reference.json"
        original = profile.read_bytes()
        profile.write_bytes(original + b" ")
        with self.assertRaisesRegex(ValueError, "font profile"):
            self.select()
        profile.write_bytes(original)
        table = self.fonts / "font_table.txt"
        original = table.read_bytes()
        rows = table.read_text().splitlines()
        rows[1], rows[2] = rows[2], rows[1]
        table.write_text("\n".join(rows) + "\n")
        with self.assertRaisesRegex(ValueError, "selector table"):
            self.select()
        table.write_bytes(original)
        del self.source["source_files"]["h2_fonts/conduit-9"]
        self.publish_metadata()
        with self.assertRaisesRegex(ValueError, "native source profile"):
            self.select()

    def test_linked_font_cannot_borrow_an_external_owner(self):
        path = self.fonts / "conduit-9"
        path.unlink()
        path.symlink_to(self.selected / "halo2/h2_fonts/conduit-9")
        with self.assertRaisesRegex(ValueError, "symbolic-link owner"):
            self.select()

    def test_after_capture_identity_checks_cover_every_native_font_and_profile(self):
        sources, cache = self.select()
        names = [self.fonts / name for name in (*self.profile["mcc_files"], "font_table.txt")]
        names += [self.root / "tools/data/gameplay-font-reference.json", self.cache / "provenance.json",
                  self.cache / "ui/mcc-menu-assets.json"]
        for path in names:
            with self.subTest(owner=path.name):
                original = path.read_bytes()
                stamp = path.stat()
                path.write_bytes(original + b"changed after source selection")
                os.utime(path, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
                self.assertFalse(all(rendered.identity(row["path"]) == row
                                     for row in (*sources.values(), *cache.values())))
                path.write_bytes(original)

    def test_completion_parser_recognizes_native_font_marker_only(self):
        marker = "using selected MCC-only assets and native MCC font prompts"
        for line, expected in ((marker, True),
                               ("using selected MCC-only assets and authored port prompts", False),
                               ("using original menu assets", False)):
            with self.subTest(line=line):
                # Exercise the actual supervisor log parser with an owned CPU
                # text producer. No geometry, capture or GPU claim is emitted.
                row = rendered.run([sys.executable, "-c", "import sys; print(sys.argv[1])", line],
                                   self.base, os.environ.copy(), "completion-marker", 5)
                self.assertEqual(row["exit_code"], 0)
                self.assertEqual(row["mcc_assertions_completed"], expected)
                self.assertIsNone(row["geometry_counters"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mcc-root", type=Path, required=True)
    args, extra = parser.parse_known_args()
    SELECTED = args.mcc_root
    unittest.main(argv=[sys.argv[0], *extra])
