"""MCC discovery, bounded decoding and transactional preparation boundaries."""
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
import mcc_content as content
import prepare_mcc_gameplay as preparation
import mcc_menu_inventory as menu_inventory
import mcc_loading_assets as loading_assets
import mcc_menu_assets as menu_assets
from export_vista_menu_ui import dds_image
import csv
import hashlib


def cache_bytes():
    data = bytearray(4096)
    def word(at, value):
        struct.pack_into("<I", data, at, value)
    for at, value in ((0, 0x68656164), (4, 13), (8, len(data)), (16, 1024),
                      (32, 1), (36, 2048), (40, 16), (44, 2080),
                      (720, 1024), (1024, 1056), (1032, 1056), (1048, 1)):
        word(at, value)
    data[2048:2058] = b"test\\page\0"
    struct.pack_into("<4I", data, 1056, int.from_bytes(b"bitm", "big"), 0xe1740000, 2304, 256)
    word(2304 + 68, 1)
    word(2304 + 72, 2384)
    return data


def compress_cache(data, raw=False):
    header = bytearray(data[:896])
    struct.pack_into("<H", header, 28, 1)
    block = data[896:] if raw else zlib.compress(data[896:])
    struct.pack_into("<4I", header, 776, 4096, 904, 896, 1)
    size = -len(block) if raw else len(block)
    return header + struct.pack("<ii", size, 904) + block


def source(root):
    halo = root / "halo2"
    for name in ("h2_maps_win64_dx11/textures.dat", "h2_maps_win64_dx11/shared.map",
                 "h2_maps_win64_dx11/mainmenu.map", "h2_fonts/font_table.txt"):
        path = halo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"donor")
    return halo


class DiscoveryTests(unittest.TestCase):
    def test_external_steam_library_manifest_and_nonstandard_folder(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            steam = root / "home/.local/share/Steam"
            (steam / "steamapps").mkdir(parents=True)
            library = root / "external games"
            expected = source(library / "steamapps/common/custom MCC folder")
            (steam / "steamapps/libraryfolders.vdf").write_text('"1" { "path" "' + str(library) + '" }')
            (library / "steamapps/appmanifest_976730.acf").write_text('"installdir" "custom MCC folder"')
            found = content.discover(home=root / "home", environment={})
            self.assertEqual(found, expected)
            for candidate in (expected, expected.parent, expected / "h2_maps_win64_dx11"):
                self.assertEqual(content.directory(candidate), expected)

    def test_explicit_environment_source_precedes_steam_and_missing_reports_path_option(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            expected = source(root / "installed")
            self.assertEqual(content.discover(home=root, environment={"HALO2_MCC_ROOT": str(expected)}), expected)
            with self.assertRaisesRegex(FormatError, "--mcc-root"):
                content.discover(home=root, environment={})


class CacheTests(unittest.TestCase):
    def test_both_chunk_encodings_preserve_decoded_metadata(self):
        data = cache_bytes()
        for raw in (False, True):
            with self.subTest(raw=raw):
                result = content.decode_cache(compress_cache(data, raw))
                self.assertEqual(result[896:], data[896:])
                self.assertEqual(len(result), len(data))

    def test_adjacent_raw_zlib_raw_chunks_retain_both_boundary_bytes(self):
        data = cache_bytes()
        data[896:898] = b"\x78\x9c"  # The signed size selects raw, even here.
        pieces = (data[896:1025], data[1025:3073], data[3073:])
        encoded = bytearray(data[:896]) + bytearray(24)
        struct.pack_into("<H", encoded, 28, 1)
        struct.pack_into("<4I", encoded, 776, 4096, 920, 896, 3)
        for index, piece in enumerate(pieces):
            block = zlib.compress(piece) if index == 1 else piece
            size = len(block) if index == 1 else -len(block)
            struct.pack_into("<ii", encoded, 896 + index * 8, size, len(encoded))
            encoded.extend(block)
        self.assertEqual(content.decode_cache(encoded)[896:], data[896:])
        with self.assertRaises(FormatError):
            content.decode_cache(encoded[:-1])

    def test_directory_full_salt_names_and_bounds(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "test.map"
            path.write_bytes(compress_cache(cache_bytes()))
            cache = content.MccCache(path)
            tag, at = cache.tag(0xe1740000, "bitm")
            self.assertEqual((tag["name"], at, cache.block(at + 68, 168)), ("test\\page", 2304, [2384]))
            with self.assertRaises(FormatError):
                cache.tag(0xe1750000, "bitm")
            cache.close()
            for at, value in ((1060, 0xe1740001), (2080, 17), (1064, 5000), (1048, 65537)):
                invalid = cache_bytes()
                struct.pack_into("<I", invalid, at, value)
                path.write_bytes(invalid)
                with self.subTest(offset=at), self.assertRaises(FormatError):
                    content.MccCache(path)

    def test_compression_rejects_bombs_truncation_and_trailing_bytes(self):
        compressed = compress_cache(cache_bytes())
        invalid = bytearray(compressed)
        struct.pack_into("<I", invalid, 776, 16)
        cases = [invalid, compressed[:-1], b"daeh", compressed[:896]]
        trailing = bytearray(compressed + b"junk")
        struct.pack_into("<i", trailing, 896, len(compressed) - 904 + 4)
        cases.append(trailing)
        for data in cases:
            with self.subTest(size=len(data)), self.assertRaises(FormatError):
                content.decode_cache(data)

    def test_mixed_texture_segments_and_extent_rejection(self):
        first, second = b"original bump pixels", b"second authored mip"
        encoded = zlib.compress(first)
        data = struct.pack("<3i", 2, len(encoded), -len(second)) + encoded + second
        stream = io.BytesIO(b"padding!" + data)
        self.assertEqual(content.texture_resource(stream, 0x80000008, len(data)), first + second)
        for size in (0, 7, len(data) - 1, len(data) + 1):
            with self.subTest(size=size), self.assertRaises(FormatError):
                content.texture_resource(stream, 8, size)

    def test_menu_inventory_reads_copied_pixels_and_case_preserved_fonts(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            maps = root / "h2_maps_win64_dx11"
            maps.mkdir()
            data = cache_bytes()
            frame = 2384
            struct.pack_into("<HHBBHHH", data, frame+4, 1, 1, 1, 0, 0, 11, 0)
            pixels = b"\x12\x34\x56\xff"
            packed = struct.pack("<ii", 1, -len(pixels)) + pixels
            struct.pack_into("<I", data, frame+56, 8)
            struct.pack_into("<I", data, frame+80, len(packed))
            (maps / "mainmenu.map").write_bytes(compress_cache(data))
            (maps / "textures.dat").write_bytes(b"padding!" + packed)
            fonts = root / "h2_fonts"
            fonts.mkdir()
            for name in (*menu_inventory.PRIMARY_FONTS, "japanese_font_table.txt"):
                (fonts / name).write_bytes(name.encode())
            inventory = root / "inventory.tsv"
            fields = ("status", "mcc_cache", "mcc_name", "mcc_frame", "width", "height", "dds_sha256", "vista_name", "vista_frame")
            digest = hashlib.sha256(dds_image(1, 1, 0, 11, pixels)[0]).hexdigest()
            with inventory.open("w", newline="") as out:
                writer = csv.DictWriter(out, fields, delimiter="\t")
                writer.writeheader()
                writer.writerow(dict(status="different-pixels", mcc_cache="mainmenu.map", mcc_name="test\\page", mcc_frame=0,
                    width=1, height=1, dds_sha256=digest, vista_name="test\\page", vista_frame=0))
                writer.writerow(dict(status="missing", vista_name="server\\icon", vista_frame=3))
            result = menu_inventory.verify(root, inventory)
            self.assertEqual((result["banks"], result["frame_owners"], result["unique_dds_pages"]), (1, 1, 1))
            self.assertEqual(result["comparisons"], {"different-pixels": 1})
            self.assertIn("h2_fonts/MSLCD-14", result["fonts"])
            self.assertIn("h2_fonts/japanese_font_table.txt", result["fonts"])
            self.assertEqual(result["unavailable_in_mcc"][0]["frame"], 3)
            (maps / "textures.dat").write_bytes(b"padding!" + packed[:-1] + b"\x7f")
            with self.assertRaisesRegex(FormatError, "DDS differs"):
                menu_inventory.verify(root, inventory)
            (maps / "textures.dat").write_bytes(b"padding!" + packed)
            (fonts / "MSLCD-14").rename(fonts / "mslcd-14")
            with self.assertRaisesRegex(FormatError, "MSLCD-14"):
                menu_inventory.verify(root, inventory)

    def test_modified_cache_is_v13_reopens_with_unchanged_pointers(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "test.map"
            path.write_bytes(compress_cache(cache_bytes()))
            cache = content.MccCache(path)
            before = bytes(cache.data)
            struct.pack_into("<I", cache.data, 2500, 0x3f800000)
            preparation.save_changed_cache(cache)
            result = content.MccCache(path)
            self.assertEqual(result.u32(4), 13)
            self.assertEqual(result.tags, cache.tags)
            self.assertEqual(result.data[896:2500], before[896:2500])
            self.assertEqual(result.u32(2500), 0x3f800000)
            self.assertEqual(result.data[2504:], before[2504:])


class TransactionTests(unittest.TestCase):
    def test_loading_preparation_uses_mcc_and_authored_files_without_vista_or_xbox(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            project, pack = root / "port", root / "cache"
            payload = b"\x12\x34\x56\xff"
            resource = pack / "bitmap-resources/pixels.bin"
            resource.parent.mkdir(parents=True)
            resource.write_bytes(payload)
            digest = hashlib.sha256(payload).hexdigest()
            frame = dict(dimensions=[1, 1, 1], type=0, partitions=["one", None, None], additional_mips=0,
                         format=11, record_hex="source-record")
            (pack / "bitmap-resources.json").write_text(json.dumps(dict(
                banks=[dict(map="mainmenu.map", tag="globals\\loading_screen", datum="e0000001", frames=[frame])],
                resources=dict(one=dict(path="bitmap-resources/pixels.bin", sha256=digest)))))
            custom = project / loading_assets.CUSTOM
            custom.mkdir(parents=True)
            vector = b'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1 1"><circle r="1"/></svg>'
            (custom / "ring.svg").write_bytes(vector)
            (custom / "profile.json").write_text(json.dumps(dict(schema="H2_AUTHORED_LOADING_ASSETS_V1", runtime_admitted=False,
                files={"ring.svg": dict(bytes=len(vector), sha256=hashlib.sha256(vector).hexdigest(), origin="authored-vector")})))
            result = loading_assets.prepare(project, pack)
            self.assertEqual(result["frames"], 1)
            self.assertFalse(result["vista_executable_bank_imported"])
            self.assertFalse(result["external_vista_or_xbox_required"])
            self.assertEqual((pack / "loading/custom/ring.svg").read_bytes(), vector)
            target = pack / result["banks"][0]["frames"][0]["path"]
            self.assertEqual(target.read_bytes(), dds_image(1, 1, 0, 11, payload)[0])
            self.assertFalse((pack / "loading/loading-tone.xbox-adpcm").exists())
            (custom / "ring.svg").write_bytes(b"changed custom asset")
            with self.assertRaisesRegex(FormatError, "Authored loading asset changed"):
                loading_assets.prepare(project, pack)

    def test_copy_excludes_programs_and_shader_cache_preserves_audio_fonts_variants(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor = source(root / "source")
            for name in ("halo2.dll", "reference/default.xbe", "reference/halo2.iso", "prebuild/shaders_2_cache/program.sdc", "preload/paks/data.pck",
                         "game_variants/slayer.bin", "h2_maps_win64_dx11/sounds_en.dat",
                         "tags/objects/weapons/rifle/custom/custom.weapon",
                         "data/objects/weapons/rifle/custom/bitmaps/custom.tif"):
                path = donor / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(name.encode())
            (donor/"tags/empty/nested").mkdir(parents=True)
            files, omitted = preparation.source_files(donor)
            output = root / "copy"
            records = preparation.copy_verified(donor, output, files, lambda _: None)
            self.assertEqual(omitted, ["halo2.dll", "prebuild/shaders_2_cache/program.sdc", "reference/default.xbe", "reference/halo2.iso"])
            self.assertIn("h2_maps_win64_dx11/sounds_en.dat", records)
            self.assertIn("game_variants/slayer.bin", records)
            self.assertIn("preload/paks/data.pck", records)
            self.assertIn("tags/objects/weapons/rifle/custom/custom.weapon",records)
            self.assertIn("data/objects/weapons/rifle/custom/bitmaps/custom.tif",records)
            self.assertTrue((output/"tags/empty/nested").is_dir())
            self.assertFalse((output/"prebuild/shaders_2_cache").exists())
            for relative in records:
                self.assertEqual((donor / relative).read_bytes(), (output / relative).read_bytes())

    def test_conversion_failure_removes_stage_and_preserves_active_sources(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor = source(root / "donor")
            project = root / "port"
            (project / "assets").mkdir(parents=True)
            registry = project / "assets/asset-sources.tsv"
            registry.write_bytes(b"previous working source")
            with patch.object(preparation, "prepare_maps", side_effect=FormatError("unsupported graph")):
                with self.assertRaisesRegex(FormatError, "unsupported graph"):
                    preparation.prepare(project, donor, lambda _: None)
            self.assertEqual(registry.read_bytes(), b"previous working source")
            self.assertEqual(list((project / "assets/imported").iterdir()), [])
            self.assertEqual((donor / "h2_maps_win64_dx11/shared.map").read_bytes(), b"donor")

    def test_success_publishes_inactive_hashed_cache_without_iso_or_registry(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor = source(root / "donor")
            project = root / "port"
            with patch.object(preparation, "prepare_maps", return_value={"maps": 43}), patch.object(menu_assets, "prepare", return_value=dict(complete=True, frame_owners=648, unique_dds_pages=561, fonts={})), patch.object(loading_assets, "prepare", return_value=dict(complete=True, vista_executable_bank_imported=False)), patch.object(menu_inventory, "verify",
                    return_value=dict(complete=True, banks=287, frame_owners=648, unique_dds_pages=561, fonts={})):
                final, report = preparation.prepare(project, donor, lambda _: None)
            self.assertFalse(report["runtime_active"])
            self.assertFalse(report["full_tu15_parity"])
            self.assertEqual(json.loads((final / "provenance.json").read_text()), report)
            self.assertFalse((project / "assets/asset-sources.tsv").exists())
            self.assertFalse(list((project / "assets/imported").glob(".staging-*")))

    def test_symlink_and_source_changes_are_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor = source(root / "donor")
            (donor / "redirect").symlink_to(root)
            with self.assertRaisesRegex(FormatError, "Symlink"):
                preparation.source_files(donor)
            (donor / "redirect").unlink()
            files, _ = preparation.source_files(donor)
            (donor / files[0][0]).write_bytes(b"updated by Steam")
            with self.assertRaisesRegex(FormatError, "source changed"):
                preparation.copy_verified(donor, root / "copy", files, lambda _: None)

    def test_steam_update_after_copy_prevents_publication(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor = source(root / "donor")
            project = root / "port"
            def update(*_):
                (donor / "h2_maps_win64_dx11/new.map").write_bytes(b"new update")
                return {}
            with patch.object(preparation, "prepare_maps", side_effect=update), patch.object(menu_assets, "prepare", return_value=dict(complete=True, frame_owners=648, unique_dds_pages=561, fonts={})), patch.object(loading_assets, "prepare", return_value=dict(complete=True, vista_executable_bank_imported=False)), patch.object(menu_inventory, "verify",
                    return_value=dict(complete=True, banks=287, frame_owners=648, unique_dds_pages=561, fonts={})):
                with self.assertRaisesRegex(FormatError, "installation changed"):
                    preparation.prepare(project, donor, lambda _: None)
            self.assertEqual(list((project / "assets/imported").iterdir()), [])

    def test_source_output_overlap_and_insufficient_space_rejected_before_copy(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor = source(root / "donor")
            with self.assertRaisesRegex(FormatError, "must be separate"):
                preparation.prepare(donor, donor, lambda _: None)
            with patch.object(preparation.shutil, "disk_usage", return_value=shutil_usage(0)):
                with self.assertRaisesRegex(FormatError, "free space"):
                    preparation.prepare(root / "port", donor, lambda _: None)
            self.assertEqual(list((root / "port/assets/imported").iterdir()), [])


def shutil_usage(free):
    from collections import namedtuple
    return namedtuple("usage", "total used free")(100, 100 - free, free)


if __name__ == "__main__":
    unittest.main()
