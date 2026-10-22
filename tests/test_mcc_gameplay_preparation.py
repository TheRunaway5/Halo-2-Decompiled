"""MCC discovery, bounded decoding and transactional preparation boundaries."""
import io
import json
import os
from pathlib import Path
import selectors
import signal
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_rendering import FormatError
import mcc_content as content
import prepare_mcc_gameplay as preparation
import mcc_menu_inventory as menu_inventory
import mcc_loading_assets as loading_assets
import mcc_loading_presentation as loading_presentation
import mcc_menu_assets as menu_assets
from export_vista_menu_ui import dds_image
import csv
import hashlib
from test_mcc_movies import bundle_fixture as movie_fixture
from test_mcc_ui_layout import layout_fixture
from test_mcc_loading_presentation import v3_summary_fixture
from native_variant_test_support import variant_fixture
from native_variant_assets import LAYOUTS, MANIFEST
from test_controller_prompt_assets import controller_prompt_fixture


def bundle_fixture(root):
    layout_fixture(root)
    variant_fixture(root)
    controller_prompt_fixture(root)


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
        path.write_bytes(b"daeh\x0d\0\0\0donor" if name.endswith("mainmenu.map") else b"donor")
    return halo


def import_transaction_worker(project, donor, phase):
    """Run real copy/conversion/presentation fixtures behind a signal barrier."""
    import mcc_multiplayer_catalog as catalog
    def barrier(stage):
        print(json.dumps(dict(stage=str(stage))), flush=True)
        while True:
            # Always return to Python if SIGTERM lands just before the C wait.
            time.sleep(0.01)

    original_open = Path.open
    original_mkdir = Path.mkdir

    def make_directory(path, *args, **kwargs):
        result = original_mkdir(path, *args, **kwargs)
        if phase == "stage-created" and path.name.startswith(".staging-mcc-"):
            barrier(path)
        return result

    def progress(message):
        if ((phase == "conversion" and message.startswith("Reading MCC Classic tags")) or
                (phase == "after-presentation" and message.startswith("Preserving the selected existing native shader"))):
            barrier(next((project / "assets/imported").glob(".staging-mcc-*")))
        if phase == "published" and message.startswith("Prepared MCC cache: "):
            barrier(Path(message.removeprefix("Prepared MCC cache: ")))

    class CopyStream:
        def __init__(self, stream, stage):
            self.stream, self.stage = stream, stage

        def __enter__(self):
            return self

        def __exit__(self, *args):
            return self.stream.__exit__(*args)

        def write(self, data):
            size = self.stream.write(data)
            self.stream.flush()
            barrier(self.stage)
            return size

    def open_file(path, mode="r", *args, **kwargs):
        stream = original_open(path, mode, *args, **kwargs)
        if phase == "copy" and mode == "xb":
            stage = next((p for p in path.parents if p.name.startswith(".staging-mcc-")), None)
            if stage is not None:
                return CopyStream(stream, stage)
        return stream

    previous = signal.getsignal(signal.SIGTERM)
    # This lifecycle fixture has no multiplayer catalog destinations. Keep its
    # required catalog set empty while executing the real planner/applicator;
    # catalog content qualification has its own tests and actual MCC receipts.
    with patch.dict(catalog.TARGETS, {}, clear=True), \
         patch.object(Path, "open", open_file), patch.object(Path, "mkdir", make_directory), \
         patch.object(menu_inventory, "verify", return_value=dict(complete=True, banks=0,
                      frame_owners=0, unique_dds_pages=0, fonts={})), \
         patch.object(menu_assets, "prepare", return_value=dict(complete=True,
                      frame_owners=0, unique_dds_pages=0, fonts={})), \
         patch.object(loading_assets, "prepare", return_value=dict(complete=True,
                      vista_executable_bank_imported=False)), \
         patch.object(loading_presentation, "preflight_v3"), \
         patch.object(loading_presentation, "prepare_v3", side_effect=lambda root, cache, source, **kwargs: v3_summary_fixture(cache)):
        final, report = preparation.prepare(project, donor, progress)
    print(json.dumps(dict(final=str(final), report=report,
                          handler_restored=signal.getsignal(signal.SIGTERM) == previous)), flush=True)


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
    def setUp(self):
        # These transaction fixtures replace map conversion and use tiny
        # non-map bytes. Damage-response content has its own real-map tests.
        inventory = patch.object(preparation, 'inventory_player_screen_flashes',
            return_value=dict(complete=True, summary=dict(synthetic_transaction_fixture=True)))
        inventory.start()
        self.addCleanup(inventory.stop)

    @unittest.skipUnless(os.name == "posix", "POSIX SIGTERM cancellation")
    def test_sigterm_during_copy_removes_stage_preserves_inputs_and_allows_retry(self):
        self.check_sigterm_transaction("copy")

    @unittest.skipUnless(os.name == "posix", "POSIX SIGTERM cancellation")
    def test_sigterm_during_conversion_removes_stage_and_allows_retry(self):
        self.check_sigterm_transaction("conversion")

    @unittest.skipUnless(os.name == "posix", "POSIX SIGTERM cancellation")
    def test_sigterm_after_presentation_helpers_still_removes_stage(self):
        self.check_sigterm_transaction("after-presentation")

    @unittest.skipUnless(os.name == "posix", "POSIX SIGTERM cancellation")
    def test_sigterm_at_stage_creation_is_covered_by_cleanup(self):
        self.check_sigterm_transaction("stage-created")

    @unittest.skipUnless(os.name == "posix", "POSIX SIGTERM cancellation")
    def test_sigterm_after_publication_preserves_complete_unselected_cache(self):
        self.check_sigterm_transaction("published")

    def check_sigterm_transaction(self, phase):
        with tempfile.TemporaryDirectory(prefix="halo2-import-sigterm-") as temp:
            root = Path(temp)
            donor, project = source(root / "donor"), root / "port"
            bundle_fixture(project)
            # Tiny v13 scenario caches exercise the actual converter without
            # depending on reference banks or the live MCC installation.
            data = cache_bytes()
            for at, value in ((32, 2), (40, 64), (44, 2112), (1036, 0xe1740000),
                              (1040, 0xe1740001), (1048, 2)):
                struct.pack_into("<I", data, at, value)
            struct.pack_into("<4I", data, 1056, int.from_bytes(b"scnr", "big"), 0xe1740000, 2304, 256)
            struct.pack_into("<4I", data, 1072, int.from_bytes(b"matg", "big"), 0xe1740001, 2560, 512)
            data[2048:2074] = b"test\\page\0globals\\globals\0"
            struct.pack_into("<2I", data, 2112, 0, 10)
            struct.pack_into("<2I", data, 2560 + 0x178, 1, 2944)
            for name in ("mainmenu.map", "shared.map"):
                (donor / "h2_maps_win64_dx11" / name).write_bytes(data)
            prior = project / "assets/imported/mcc-prior"
            prior.mkdir(parents=True)
            (prior / "keep.bin").write_bytes(b"previous complete import")
            (project / "assets/mcc-source.tsv").write_text("H2_MCC_ASSET_SOURCE_V1\ncache\tassets/imported/mcc-prior\n")
            (project / "assets/asset-sources.tsv").write_bytes(b"previous working selection")
            protected = [p for base in (donor, project / "resources", prior)
                         for p in base.rglob("*") if p.is_file()]
            protected += list((project / "assets").glob("*.tsv"))
            before = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected}
            command = [sys.executable, "-B", str(Path(__file__).resolve()),
                       "--import-transaction-worker", str(project), str(donor)]
            with subprocess.Popen([*command, phase], stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, text=True) as child:
                try:
                    with selectors.DefaultSelector() as selector:
                        selector.register(child.stdout, selectors.EVENT_READ)
                        self.assertTrue(selector.select(10), "import did not reach the cancellation barrier")
                    line = child.stdout.readline()
                    if not line:
                        _, stderr = child.communicate(timeout=10)
                        self.fail("import exited before cancellation barrier: " + stderr)
                    ready = json.loads(line)
                    stage = Path(ready["stage"])
                    self.assertTrue(stage.is_dir())
                    if phase != "stage-created":
                        self.assertTrue(any(p.stat().st_size for p in stage.rglob("*") if p.is_file()))
                    if phase == "published":
                        self.assertTrue((stage / "provenance.json").is_file())
                        for path in stage.rglob("*"):
                            if path.is_file():
                                protected.append(path)
                                before[path] = (path.read_bytes(), path.stat().st_ino, path.stat().st_mtime_ns)
                    child.send_signal(signal.SIGTERM)
                    stdout, stderr = child.communicate(timeout=10)
                    self.assertEqual(stage.exists(), phase == "published", (child.returncode, stdout, stderr))
                    self.assertEqual(child.returncode, 128 + signal.SIGTERM, stderr)
                finally:
                    if child.poll() is None:
                        child.kill()
                        child.communicate()
            retained = {prior, stage} if phase == "published" else {prior}
            self.assertEqual(set(prior.parent.iterdir()), retained)
            self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected})
            retry = subprocess.run([*command, "retry"], capture_output=True, text=True, timeout=20)
            self.assertEqual(retry.returncode, 0, retry.stderr)
            result = json.loads(retry.stdout)
            final = Path(result["final"])
            self.assertTrue(result["handler_restored"])
            self.assertFalse(result["report"]["runtime_active"])
            self.assertFalse(result["report"]["full_tu15_parity"])
            self.assertNotIn("port_bundled_movies", result["report"])
            self.assertFalse((final / "presentation/movies").exists())
            self.assertNotIn("port_authored_prompts", result["report"])
            self.assertFalse((final / "presentation/prompts").exists())
            self.assertEqual(json.loads((final / "provenance.json").read_text()), result["report"])
            self.assertFalse(list(prior.parent.glob(".staging-*")))
            self.assertEqual(set(prior.parent.iterdir()), retained | {final})
            for path in donor.rglob("*"):
                if path.is_file():
                    self.assertEqual(path.read_bytes(), (final / "halo2" / path.relative_to(donor)).read_bytes())
            self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected})

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
            bundle_fixture(project)
            (project / "assets").mkdir(parents=True)
            registry = project / "assets/asset-sources.tsv"
            registry.write_bytes(b"previous working source")
            with patch.object(loading_presentation, "preflight_v3"), patch.object(preparation, "prepare_maps", side_effect=FormatError("unsupported graph")):
                with self.assertRaisesRegex(FormatError, "unsupported graph"):
                    preparation.prepare(project, donor, lambda _: None)
            self.assertEqual(registry.read_bytes(), b"previous working source")
            self.assertEqual(list((project / "assets/imported").iterdir()), [])
            self.assertEqual((donor / "h2_maps_win64_dx11/shared.map").read_bytes(), b"donor")

    def test_owner_publication_failure_after_final_graphics_preserves_active_sources(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor, project = source(root / 'donor'), root / 'port'
            bundle_fixture(project)
            prior = project / 'assets/imported/mcc-prior'
            prior.mkdir(parents=True)
            (prior / 'keep.bin').write_bytes(b'previous complete cache')
            registry = project / 'assets/mcc-source.tsv'
            registry.write_bytes(b'previous selected cache')
            active_source = project / 'assets/asset-sources.tsv'
            active_source.write_bytes(b'previous active game sources')
            protected = [p for base in (donor, prior) for p in base.rglob('*') if p.is_file()] + [registry, active_source]
            before = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected}
            converted_hashes = {}

            def converted(maps, output, progress):
                rows = []
                for path in sorted(maps.glob('*.map')):
                    source_sha = preparation.file_sha(path)
                    path.write_bytes(path.read_bytes() + b' synthetic conversion')
                    converted_hashes[path.name] = preparation.file_sha(path)
                    rows.append(dict(map=path.name, source_sha256=source_sha,
                                     output_sha256=converted_hashes[path.name]))
                preparation.write_json(output / 'tag-conversion.json', dict(profile=preparation.PROFILE, maps=rows))
                return dict(maps=len(rows))

            def graphics(root, stage, **kwargs):
                path = stage / 'halo2/h2_maps_win64_dx11/mainmenu.map'
                donor_sha = preparation.file_sha(path)
                path.write_bytes(path.read_bytes() + b' final graphics')
                return dict(changes=[dict(path='halo2/h2_maps_win64_dx11/mainmenu.map',
                    donor_sha256=donor_sha, converted_sha256=preparation.file_sha(path))])

            def reject_publication(stage, report):
                self.assertTrue(stage.name.startswith('.staging-mcc-'))
                self.assertFalse((stage / 'provenance.json').exists())
                self.assertEqual(report['native_shader_library']['maps'], {'mainmenu': 'a' * 64})
                change = report['graphics_corrections']['changes'][0]
                self.assertEqual(change['donor_sha256'], converted_hashes['mainmenu.map'])
                self.assertEqual(change['converted_sha256'], preparation.file_sha(stage / change['path']))
                self.assertNotEqual(change['donor_sha256'], change['converted_sha256'])
                self.assertTrue(report['conversion_coverage']['stock_conversion_results_complete'])
                for field in ('native_loading_presentation', 'native_ui_layout', 'menu_resources', 'player_screen_flashes'):
                    self.assertIn(field, report)
                raise FormatError('injected final owner publication failure')

            # This verifies the fresh producer's failure boundary, not native
            # conversion or successful publication of a synthetic shader bank.
            with patch.object(preparation, 'prepare_maps', side_effect=converted), \
                 patch.object(menu_assets, 'prepare', return_value=dict(complete=True, frame_owners=0, unique_dds_pages=0, fonts={})), \
                 patch.object(menu_inventory, 'verify', return_value=dict(complete=True, banks=0, frame_owners=0, unique_dds_pages=0, fonts={})), \
                 patch.object(loading_assets, 'prepare', return_value=dict(complete=True)), \
                 patch.object(loading_presentation, 'preflight_v3'), \
                 patch.object(loading_presentation, 'prepare_v3', side_effect=lambda root, cache, source, **kwargs: v3_summary_fixture(cache)), \
                 patch('preserve_native_shaders.copy_selected', return_value=dict(maps={'mainmenu': 'a' * 64})), \
                 patch('mcc_graphics_corrections.apply_selected', side_effect=graphics), \
                 patch('mcc_native_map_owner_publication.publish_staged_owner_index', side_effect=reject_publication) as publish:
                with self.assertRaisesRegex(FormatError, 'injected final owner publication failure'):
                    preparation.prepare(project, donor, lambda _: None)
            publish.assert_called_once()
            self.assertEqual(list(prior.parent.iterdir()), [prior])
            self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in protected})

    def assert_preparation_without_reference_movie_bank(self, *, invalid_bank=False):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor = source(root / "donor")
            project = root / "port"
            bundle_fixture(project)
            if invalid_bank:
                movie_fixture(project)
                (project / "resources/movies/manifest.json").write_bytes(b"invalid preserved reference metadata")
                (project / "resources/movies/credits_60.mp4").write_bytes(b"invalid preserved reference payload")
            sources = list((project / "resources/movies").glob("*"))
            source_manifest = project / "resources/movie-manifest.json"
            if source_manifest.exists():
                sources.append(source_manifest)
            before = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in sources}
            def converted(maps, output, progress):
                rows = [dict(map=p.name, source_sha256=preparation.file_sha(p),
                             output_sha256=preparation.file_sha(p)) for p in sorted(maps.glob('*.map'))]
                preparation.write_json(output / 'tag-conversion.json', dict(profile=preparation.PROFILE, maps=rows))
                return dict(maps=len(rows))
            with patch('mcc_movies.required_bytes', side_effect=AssertionError('Reference movie preflight selected')), \
                 patch('mcc_movies.prepare', side_effect=AssertionError('Reference movie importer selected')), \
                 patch('mcc_prompts.required_bytes', side_effect=AssertionError('Reference prompt preflight selected')), \
                 patch('mcc_prompts.prepare', side_effect=AssertionError('Reference prompt importer selected')), \
                 patch.object(preparation, "prepare_maps", side_effect=converted), patch.object(menu_assets, "prepare", return_value=dict(complete=True, frame_owners=648, unique_dds_pages=561, fonts={})), patch.object(loading_presentation, "preflight_v3"), patch.object(loading_presentation, "prepare_v3", side_effect=lambda root, cache, source, **kwargs: v3_summary_fixture(cache)), patch.object(loading_assets, "prepare", return_value=dict(complete=True, vista_executable_bank_imported=False)), patch.object(menu_inventory, "verify",
                    return_value=dict(complete=True, banks=287, frame_owners=648, unique_dds_pages=561, fonts={})):
                final, report = preparation.prepare(project, donor, lambda _: None)
            self.assertEqual((final / 'data/tags/variants/bomb-defusal-v1.json').read_bytes(),
                             (project / 'data/tags/variants/bomb-defusal-v1.json').read_bytes())
            self.assertEqual((final / LAYOUTS).read_bytes(), (project / LAYOUTS).read_bytes())
            receipt = json.loads((final / MANIFEST).read_bytes())
            self.assertEqual(receipt['files'][LAYOUTS]['sha256'], preparation.file_sha(project / LAYOUTS))
            self.assertFalse(report["runtime_active"])
            self.assertFalse(report["full_tu15_parity"])
            self.assertEqual(report["native_loading_presentation"]["profile"], loading_presentation.V3_PROFILE)
            self.assertTrue(report['controller_prompts']['complete'])
            self.assertEqual(len(report['controller_prompts']['files']),18)
            self.assertNotIn("port_bundled_loading_ambience", report)
            self.assertNotIn("port_bundled_movies", report)
            self.assertEqual((project / "resources/movies").exists(), invalid_bank)
            self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in sources})
            self.assertFalse((final / "presentation/movies").exists())
            self.assertNotIn("port_authored_prompts", report)
            self.assertFalse((project / "resources/presentation/prompts").exists())
            self.assertFalse((final / "presentation/prompts").exists())
            self.assertIn("verified MCC fonts", report["policy"]["prompt_artwork"])
            self.assertTrue(report['conversion_coverage']['stock_conversion_results_complete'])
            self.assertEqual(report['conversion_coverage']['stock_map_owners'],
                             ['h2_maps_win64_dx11/mainmenu.map', 'h2_maps_win64_dx11/shared.map'])
            self.assertFalse(report['conversion_coverage']['full_tu15_semantic_conversion'])
            self.assertEqual(json.loads((final / "provenance.json").read_text()), report)
            self.assertFalse((project / "assets/asset-sources.tsv").exists())
            self.assertFalse(list((project / "assets/imported").glob(".staging-*")))

    def test_success_publishes_inactive_hashed_cache_without_iso_or_registry(self):
        self.assert_preparation_without_reference_movie_bank()

    def test_fresh_import_preserves_and_ignores_invalid_reference_movie_bank(self):
        self.assert_preparation_without_reference_movie_bank(invalid_bank=True)

    def test_missing_unchanged_map_conversion_result_prevents_fresh_publication(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            donor, project = source(root / 'donor'), root / 'port'
            bundle_fixture(project)
            registry = project / 'assets/mcc-source.tsv'
            registry.parent.mkdir(parents=True)
            registry.write_bytes(b'prior selected cache')
            protected = {p: p.read_bytes() for p in donor.rglob('*') if p.is_file()}
            def incomplete(maps, output, progress):
                path = maps / 'shared.map'
                preparation.write_json(output / 'tag-conversion.json', dict(profile=preparation.PROFILE,
                    maps=[dict(map=path.name, source_sha256=preparation.file_sha(path),
                               output_sha256=preparation.file_sha(path))]))
                return dict(maps=1)
            with patch.object(preparation, 'prepare_maps', side_effect=incomplete), \
                 patch.object(menu_assets, 'prepare', return_value=dict(complete=True, frame_owners=0, unique_dds_pages=0, fonts={})), \
                 patch.object(loading_presentation, 'preflight_v3'), \
                 patch.object(loading_presentation, 'prepare_v3', side_effect=lambda root, cache, source, **kwargs: v3_summary_fixture(cache)), \
                 patch.object(loading_assets, 'prepare', return_value=dict(complete=True)), \
                 patch.object(menu_inventory, 'verify', return_value=dict(complete=True, banks=0, frame_owners=0, unique_dds_pages=0, fonts={})), \
                 self.assertRaisesRegex(FormatError, 'missing converted map owners.*mainmenu.map'):
                preparation.prepare(project, donor, lambda _: None)
            self.assertEqual(list((project / 'assets/imported').iterdir()), [])
            self.assertEqual(registry.read_bytes(), b'prior selected cache')
            self.assertEqual(protected, {p: p.read_bytes() for p in protected})

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
            bundle_fixture(project)
            def update(*_):
                (donor / "h2_maps_win64_dx11/new.map").write_bytes(b"new update")
                return {}
            with patch.object(preparation, "prepare_maps", side_effect=update), patch.object(menu_assets, "prepare", return_value=dict(complete=True, frame_owners=648, unique_dds_pages=561, fonts={})), patch.object(loading_presentation, "preflight_v3"), patch.object(loading_presentation, "prepare_v3", side_effect=lambda root, cache, source, **kwargs: v3_summary_fixture(cache)), patch.object(loading_assets, "prepare", return_value=dict(complete=True, vista_executable_bank_imported=False)), patch.object(menu_inventory, "verify",
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
            bundle_fixture(root / "port")
            with patch.object(preparation.shutil, "disk_usage", return_value=shutil_usage(0)):
                with self.assertRaisesRegex(FormatError, "free space"):
                    preparation.prepare(root / "port", donor, lambda _: None)
            self.assertEqual(list((root / "port/assets/imported").iterdir()), [])


def shutil_usage(free):
    from collections import namedtuple
    return namedtuple("usage", "total used free")(100, 100 - free, free)


if __name__ == "__main__":
    if len(sys.argv) == 5 and sys.argv[1] == "--import-transaction-worker":
        import_transaction_worker(Path(sys.argv[2]), Path(sys.argv[3]), sys.argv[4])
    else:
        unittest.main()
