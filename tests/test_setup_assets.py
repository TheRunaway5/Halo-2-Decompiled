"""Source-selection and archive boundaries; real-source checks are separate."""
import io
import json
import shutil
import subprocess
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import setup_assets as assets
ROOT = Path(__file__).resolve().parents[1]


def vista_cache(mask=0, unused=False):
    data = bytearray(4096)
    def word(at, value):
        struct.pack_into("<I", data, at, value)
    word(0, 0x68656164)
    word(4, 8)
    word(8, len(data))
    word(16, 2048)
    word(32, 2048 + mask)
    data[300:300 + len(assets.VISTA_BUILD)] = assets.VISTA_BUILD.encode()
    word(2044, 0x666f6f74)
    word(2048 + 8, 32)
    word(2048 + 24, 2 if unused else 1)
    word(716, 2 if unused else 1)
    word(720, 2200)
    word(724, 16)
    word(728, 2240)
    data[2200:2213] = b"ui\\test\\page\0"
    struct.pack_into("<IIII", data, 2080, int.from_bytes(b"bitm", "big"), 42, mask + 2300, 116)
    if unused:
        struct.pack_into("<IIII", data, 2096, 0, 0xffffffff, 0, 0)
        word(2244, 0xffffffff)
    return data


def archive_bytes(rows):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w") as archive:
        for name, value in rows:
            archive.writestr(name, value)
    return stream.getvalue()


class SourceSelectorStartupTests(unittest.TestCase):
    def test_missing_tk_names_the_interpreter_and_mcc_path(self):
        from setup_assets_ui import run
        with patch.dict(sys.modules, {"tkinter": None}):
            with self.assertRaises(RuntimeError) as failure:
                run(ROOT)
        message = str(failure.exception)
        self.assertIn(sys.executable, message)
        self.assertIn(str(failure.exception.__cause__), message)
        self.assertIn('--mcc-source "/path/to/Halo The Master Chief Collection"', message)
        self.assertNotIn("--xbox-source", message)
        self.assertNotIn("--vista-source", message)


class SetupTests(unittest.TestCase):
    @unittest.skipUnless((ROOT / "Halo 2 Vista installer/maps/shared.map").is_file(), "owned Vista donor unavailable")
    def test_all_vista_crosshair_pages(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            x = assets.Cache(ROOT / "assets/disc/maps/shared.map")
            v = assets.VistaUiCache(ROOT / "Halo 2 Vista installer/maps/shared.map")
            try:
                original = assets.banks(x, assets.VISTA_CROSSHAIR_PREFIX)
                sampled = assets.banks(v, assets.VISTA_CROSSHAIR_PREFIX)
                report = assets.export_pack(x, v, output, "hud.tsv", assets.VISTA_CROSSHAIR_PREFIX)
                self.assertEqual((report["banks"], report["frames"]), (4, 98))
                self.assertEqual(report["retained_original"], {})
                for line in (output / "hud.tsv").read_text().splitlines()[1:]:
                    name, frame, count, width, height, relative = line.split("\t")
                    a, b = original[name][0][int(frame)], sampled[name][0][int(frame)]
                    self.assertEqual((int(width), int(height)), (x.u16(a + 4), x.u16(a + 6)))
                    w, h, fmt, mips = (v.u16(b + n) for n in (4, 6, 12, 20))
                    raw = assets.expand_resource(assets.resource(v, b), sum(r["bytes"] for r in assets.mip_shapes(w, h, mips, fmt)))
                    expected, _ = assets.dds_image(w, h, mips, fmt, raw)
                    self.assertEqual((output / relative).read_bytes(), expected)
                # A different extent must not be admitted by the seven exact
                # source exceptions, or silently fall back during setup.
                name = assets.VISTA_CROSSHAIR_PREFIX + "hud_reticles"
                page = sampled[name][0][3]
                u16 = v.u16
                with patch.object(v, "u16", side_effect=lambda at: 92 if at == page + 6 else u16(at)):
                    with self.assertRaisesRegex(assets.FormatError, "Incompatible Vista crosshair"):
                        assets.export_pack(x, v, output, "bad.tsv", assets.VISTA_CROSSHAIR_PREFIX)
                # Every original campaign/multiplayer cache uses the same
                # crosshair page order, sprite metadata and logical extents.
                for path in sorted((ROOT / "assets/disc/maps").glob("*.map")):
                    cache = assets.Cache(path)
                    try:
                        for name, bank in assets.banks(cache, assets.VISTA_CROSSHAIR_PREFIX).items():
                            self.assertEqual(bank[1], original[name][1], str(path))
                            self.assertEqual([(cache.u16(p + 4), cache.u16(p + 6)) for p in bank[0]],
                                             [(x.u16(p + 4), x.u16(p + 6)) for p in original[name][0]], str(path))
                    finally:
                        cache.close()
            finally:
                x.close()
                v.close()

    @unittest.skipUnless((ROOT / "Halo 2 Vista installer/maps/shared.map").is_file(), "owned Vista donor unavailable")
    def test_complete_scoreboard_artwork(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            x = assets.Cache(ROOT / "assets/disc/maps/shared.map")
            v = assets.VistaUiCache(ROOT / "Halo 2 Vista installer/maps/shared.map")
            try:
                report = assets.export_scoreboard_artwork(x, v, output)
                self.assertEqual(report["banks"], 4)
                self.assertEqual(report["frames"], 98)
                self.assertEqual(report["retained_original"], {})
                rows = (output / "scoreboard.tsv").read_text().splitlines()[1:]
                self.assertEqual(len(rows), 98)
                self.assertTrue(all((output / row.split("\t")[-1]).is_file() for row in rows))
                import json
                style = json.loads((output / "scoreboard.json").read_text())
                self.assertEqual(style["schema"], "H2UI_SCOREBOARD_V1")
                self.assertEqual(len(style["palette"]), 18)
                self.assertEqual(len(style["team_palette"]), 9)
            finally:
                x.close()
                v.close()

    def test_original_shield_artwork_relative_alignment(self):
        root = Path(__file__).resolve().parents[1]
        xbox = root / "assets/disc/maps/shared.map"
        vista = root / "Halo 2 Vista installer/maps/shared.map"
        if not xbox.is_file() or not vista.is_file():
            self.skipTest("original Xbox and Vista shared caches unavailable")
        with tempfile.TemporaryDirectory() as directory:
            x, v = assets.Cache(xbox), assets.VistaUiCache(vista)
            try:
                output = Path(directory)
                report = assets.export_hud_alignment(x, v, output,
                    ["ui\\hud\\bitmaps\\new_hud\\meters\\shield_meter"])
                rows = [line.split("\t") for line in (output / "hud-alignment.tsv").read_text().splitlines()[1:]]
                self.assertEqual({int(row[1]): tuple(map(float, row[2:])) for row in rows},
                    {0: (0, .046875), 1: (.00390625, .03125), 4: (0, 0), 5: (0, 0)})
                self.assertEqual(len(report["evidence"]), 6)
                self.assertTrue(all(row["density"] == 2 for row in report["evidence"]))
            finally:
                x.close()
                v.close()

    def test_committed_import_retains_match_announcer_cache_owners(self):
        # Exercise setup's published registry, not only archive extraction:
        # normal solo launch resolves its Vista response bank from this root.
        rows = [("maps/mainmenu.map", b"menu"), ("maps/shared.map", b"audio"),
                ("maps/lockout.map", b"lockout responses"),
                ("maps/ascension.map", b"ascension responses"),
                ("maps/fonts/conduit-13", b"font"), ("setup.exe", b"excluded"),
                ("loading.bin", b"loading images"), ("halo2.exe", b"loading audio")]
        for archive in (False, True):
            with self.subTest(archive=archive), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                source = root / "installer"
                source.mkdir()
                for name, data in rows:
                    path = source / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(data)
                if archive:
                    source = root / "installer.zip"
                    source.write_bytes(archive_bytes([
                        ("h2pc_data.bin", archive_bytes(rows))]))
                xbox = root / "xbox"
                (xbox / "maps").mkdir(parents=True)
                for name in assets.XBOX_MAPS:
                    (xbox / "maps" / (name + ".map")).write_bytes(b"xbox")
                (xbox / "default.xbe").write_bytes(b"xbe")
                # Bitmap decoding and Xbox validation have independent tests.
                # Leave the actual extraction, copy, provenance and registry
                # commit in place to catch discarded source cache owners.
                def loading(donor, output, progress):
                    self.assertEqual((donor / "loading.bin").read_bytes(), b"loading images")
                    self.assertEqual((donor / "halo2.exe").read_bytes(), b"loading audio")
                    output.mkdir()
                    (output / "loading-textures.tsv").write_text("prepared loading bank")
                    (output / "loading-tone.xbox-adpcm").write_bytes(b"prepared ambience")
                    return {"complete": True}
                with patch.object(assets, "validate_xbox"), \
                     patch.object(assets, "Cache"), \
                     patch.object(assets, "VistaUiCache"), \
                     patch.object(assets, "prepare_loading", side_effect=loading), \
                     patch.object(assets, "export_pack", return_value={"banks": 0}):
                    report = assets.setup_reference(root, source, xbox, lambda _: None)
                registry = dict(line.split("\t") for line in
                    (root / "assets/asset-sources.tsv").read_text().splitlines()[1:])
                installed = Path(registry["vista"])
                for name, data in rows:
                    if not name.startswith("maps/"):
                        continue
                    self.assertTrue((installed / name).is_file(),
                                    f"published import dropped match dependency {name}")
                    self.assertEqual((installed / name).read_bytes(), data)
                    self.assertEqual(report["vista"]["files"][name]["sha256"],
                                     assets.file_sha(installed / name))
                self.assertFalse((installed / "setup.exe").exists())
                self.assertTrue(report["vista"]["loading"]["complete"])
                for name in ("loading-textures.tsv", "loading-tone.xbox-adpcm"):
                    self.assertIn("loading/" + name, report["vista"]["files"])
                    self.assertTrue((installed / "loading" / name).is_file())

    def test_loading_repair_extracts_only_loading_inputs_and_preserves_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            vista = root / "assets/imported/original/vista"
            vista.mkdir(parents=True)
            (vista / "pack.tsv").write_bytes(b"existing menu")
            (vista / "loading").mkdir()
            (vista / "loading/keep.txt").write_text("previous incomplete export")
            registry = root / "assets/asset-sources.tsv"
            registry.write_text(f"{assets.SCHEMA}\nxbox\tunchanged-xbox\nvista\t{vista}\n")
            before = registry.read_bytes()
            source = root / "installer.zip"
            source.write_bytes(archive_bytes([("h2pc_data.bin", archive_bytes([
                ("maps/mainmenu.map", b"do not copy"), ("maps/shared.map", b"do not copy"),
                ("loading.bin", b"images"), ("halo2.exe", b"audio"), ("setup.exe", b"do not run")]))]))
            provenance = vista.parent / "provenance.json"
            provenance.write_text(json.dumps({"vista": {"source": {"path": str(source)},
                                                        "files": {"pack.tsv": {"existing": True}}}}))
            def loading(donor, output, progress):
                self.assertEqual({p.name for p in donor.iterdir()}, {"loading.bin", "halo2.exe"})
                output.mkdir()
                (output / "loading-textures.tsv").write_text("complete")
                return {"complete": True}
            with patch.object(assets, "prepare_loading", side_effect=loading):
                assets.repair_loading(root, lambda _: None)
            self.assertEqual(registry.read_bytes(), before)
            self.assertEqual((vista / "pack.tsv").read_bytes(), b"existing menu")
            self.assertEqual((vista / "loading/loading-textures.tsv").read_text(), "complete")
            report = json.loads(provenance.read_text())
            self.assertIn("loading/loading-textures.tsv", report["vista"]["files"])
            self.assertTrue(report["vista"]["files"]["pack.tsv"]["existing"])
            backups = list(vista.glob("loading.previous-*"))
            self.assertEqual(len(backups), 1)
            self.assertTrue((backups[0] / "keep.txt").is_file())

    @unittest.skipUnless(shutil.which("7z") or shutil.which("7zz"), "7-Zip not installed")
    def test_solid_archive_extracts_exact_allowlist_in_one_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            for name in ("maps/mainmenu.map", "maps/shared.map", "maps/fonts/conduit-13",
                         "maps/lockout.map", "setup.exe"):
                path = source / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(name.encode())
            archive = root / "installer.bin"
            subprocess.run([shutil.which("7z") or shutil.which("7zz"), "a", "-t7z", "-ms=on",
                            str(archive), "."], cwd=source, check=True, capture_output=True)
            assets.extract_vista(archive, root / "output")
            self.assertEqual({p.relative_to(root / "output").as_posix()
                              for p in (root / "output").rglob("*") if p.is_file()},
                             {"maps/mainmenu.map", "maps/shared.map", "maps/fonts/conduit-13"})

    def test_nested_installer_extracts_only_ui_inputs(self):
        rows = [("maps/mainmenu.map", b"menu"), ("maps/shared.map", b"shared"),
                ("maps/fonts/conduit-13", b"font"), ("maps/lockout.map", b"world"),
                ("shaders/world.spv", b"shader"), ("h2pc_setup.exe", b"never execute")]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "installer.zip"
            source.write_bytes(archive_bytes([("h2pc_data.bin", archive_bytes(rows)),
                                             ("h2pc_setup.exe", b"never execute")]))
            before = assets.file_sha(source)
            result = assets.extract_vista(source, root / "output", lambda _: None)
            self.assertEqual(result["kind"], "installer-zip")
            self.assertEqual(before, assets.file_sha(source))
            selected = {p.relative_to(root / "output").as_posix() for p in (root / "output").rglob("*") if p.is_file()}
            self.assertEqual(selected, {"maps/mainmenu.map", "maps/shared.map", "maps/fonts/conduit-13"})
            self.assertEqual((root / "output/maps/fonts/conduit-13").read_bytes(), b"font")

    def test_traversal_and_symlinks_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for i, name in enumerate(("../escape", "/absolute", "maps\\fonts\\escape")):
                source = root / f"bad-{i}.zip"
                source.write_bytes(archive_bytes([(name, b"bad")]))
                with self.assertRaises(assets.FormatError):
                    assets.extract_vista(source, root / f"output-{i}")
            source = root / "link.zip"
            with zipfile.ZipFile(source, "w") as archive:
                info = zipfile.ZipInfo("maps/fonts/conduit-13")
                info.external_attr = 0xa1ff << 16
                archive.writestr(info, "../../escape")
            with self.assertRaises(assets.FormatError):
                assets.extract_vista(source, root / "link-output")
            self.assertFalse((root.parent / "escape").exists())

    def test_cache_shared_null_slots_and_metadata_bases(self):
        with tempfile.TemporaryDirectory() as directory:
            for mask in (0, 0x1400000):
                path = Path(directory) / f"cache-{mask}.map"
                path.write_bytes(vista_cache(mask, unused=True))
                cache = assets.VistaUiCache(path)
                try:
                    self.assertEqual(list(cache.tags), [42])
                    self.assertEqual(cache.tag(42, "bitm")[1], 2300)
                finally:
                    cache.close()
                data = bytearray(path.read_bytes())
                struct.pack_into("<I", data, 8, len(data) + 1)
                path.write_bytes(data)
                with self.assertRaises(assets.FormatError):
                    assets.VistaUiCache(path)

    def test_failed_import_keeps_prior_selection_and_cleans_staging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "assets").mkdir()
            registry = root / "assets/asset-sources.tsv"
            registry.write_text("previous working sources\n")
            source = root / "invalid.zip"
            source.write_bytes(archive_bytes([("maps/mainmenu.map", b"incomplete")]))
            with self.assertRaises(assets.FormatError):
                assets.setup_reference(root, source, root / "missing-xbox", lambda _: None)
            self.assertEqual(registry.read_text(), "previous working sources\n")
            self.assertEqual(list((root / "assets/imported").iterdir()), [])

    def test_menu_only_vista_source_cannot_replace_complete_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "assets").mkdir()
            registry = root / "assets/asset-sources.tsv"
            registry.write_text("previous complete sources\n")
            source = root / "menu-only.zip"
            source.write_bytes(archive_bytes([
                ("maps/mainmenu.map", b"menu"), ("maps/shared.map", b"shared"),
                ("maps/fonts/conduit-13", b"font")]))
            with self.assertRaisesRegex(assets.FormatError, "lockout.map"):
                assets.setup_reference(root, source, root / "unused-xbox", lambda _: None)
            self.assertEqual(registry.read_text(), "previous complete sources\n")
            self.assertEqual(list((root / "assets/imported").iterdir()), [])


if __name__ == "__main__":
    unittest.main()
