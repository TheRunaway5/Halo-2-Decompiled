import hashlib
import importlib.util
import json
import os
import shutil
from pathlib import Path
import signal
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("bundle_movies", ROOT / "tools/bundle_movies.py")
movies = importlib.util.module_from_spec(spec)
spec.loader.exec_module(movies)
sys.path.insert(0, str(ROOT / "tools"))
import package_mcc_test_build as preview_package
import mcc_loading_presentation as loading
from test_mcc_movies import bundle_fixture
from test_mcc_prompts import prompt_fixture
from test_mcc_ui_layout import layout_fixture
from native_variant_test_support import variant_fixture


class ReleaseMoviesTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "original Videos"
        self.source.mkdir()
        self.manifest = {"schema": "halo2-upscaled-movies-v1", "files": {}}
        for name in sorted(movies.MOVIES):
            payload = (name + " fixture").encode()
            (self.source / name).write_bytes(payload)
            self.manifest["files"][name] = {
                "bytes": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}
        self.output = self.root / "release/resources/movies"

    def test_bundle_survives_removal_of_original_source(self):
        movies.bundle(self.source, self.output, self.manifest)
        self.source.rename(self.root / "moved Videos")
        movies.verify(self.output, self.manifest)
        self.assertEqual(json.loads((self.output / "manifest.json").read_text()), self.manifest)
        # A later release can use the local bundle as its source.
        destination = self.root / "next release/resources/movies"
        movies.bundle(self.output, destination, self.manifest)
        movies.verify(destination, self.manifest)

    def test_changed_input_preserves_existing_bundle(self):
        movies.bundle(self.source, self.output, self.manifest)
        previous = {p.name: p.read_bytes() for p in self.output.iterdir()}
        path = self.source / "intro_60.mp4"
        path.write_bytes(b"!" * path.stat().st_size)
        with self.assertRaisesRegex(ValueError, "checksum differs"):
            movies.bundle(self.source, self.output, self.manifest)
        self.assertEqual({p.name: p.read_bytes() for p in self.output.iterdir()}, previous)

    def test_missing_credits_stops_staging(self):
        (self.source / "credits_60.mp4").unlink()
        with self.assertRaisesRegex(ValueError, "Missing"):
            movies.bundle(self.source, self.output, self.manifest)
        self.assertFalse(self.output.exists())

    def test_bundled_movie_tampering_is_detected(self):
        movies.bundle(self.source, self.output, self.manifest)
        (self.output / "credits_60.mp4").write_bytes(b"modified")
        with self.assertRaises(ValueError):
            movies.verify(self.output, self.manifest)

    def test_manifest_cannot_introduce_other_files(self):
        self.manifest["files"]["../other.mp4"] = {}
        path = self.root / "manifest.json"
        path.write_text(json.dumps(self.manifest))
        with self.assertRaisesRegex(ValueError, "Invalid"):
            movies.read_manifest(path)


class PreviewPackageMoviesTest(unittest.TestCase):
    """Small movie closure/publication fixtures, not an engine release build."""
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-preview-package-movies-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "port"
        self.source.mkdir()
        variant_fixture(self.source)
        # Preflight-only executable: these publication fixtures never execute
        # it or claim decoder/media qualification. The codebook keeps its real
        # fixed source identity; no original loading tone is a default input.
        self.converter = self.source / "bin/halo2_mcc_loading_audio"
        self.converter.parent.mkdir()
        self.converter.write_bytes(b"not executed: synthetic decoder dependency")
        self.converter.chmod(0o700)
        self.codebook = self.source / loading.CODEBOOK
        self.codebook.parent.mkdir(parents=True)
        shutil.copyfile(ROOT / loading.CODEBOOK, self.codebook)
        self.runtime = self.root / "compiled/halo2_native"
        self.runtime.parent.mkdir()
        self.runtime.write_bytes(b"not executed: synthetic compiled input")
        self.shaders = self.runtime.parent / "shaders"
        self.shaders.mkdir()
        for name in ["world.vert.spv", "world.frag.spv", "hud.frag.spv"] + [f"fixture-{i}.spv" for i in range(94)]:
            (self.shaders / name).write_bytes(name.encode())
        self.launchers = self.root / "launchers"
        self.launchers.mkdir()
        self.output = self.root / "delivery/preview.zip"
        self.originals = {p.relative_to(self.source): p.read_bytes()
                          for p in self.source.rglob("*") if p.is_file()}

    def package(self):
        return preview_package.package(self.source, self.runtime, self.launchers, self.output, self.shaders)

    def assert_default_mcc_dispatch(self, root, frontend, presentation):
        self.assertIsNone(frontend)
        self.assertEqual(presentation,
                         dict(mcc_loading_converter=preview_package.converter_dependencies(root)))

    def test_reference_four_exact_resource_paths_metadata_and_no_transcoding(self):
        bundle_fixture(self.source)
        verified = preview_package.verify_bundled_movies(self.source)
        stage = self.root / "synthetic-stage"
        copied = preview_package.copy_bundled_movies(self.source, stage, verified)
        expected = {"resources/movie-manifest.json", "resources/movies/manifest.json",
                    "resources/movies/intro_60.mp4", "resources/movies/credits_60.mp4"}
        self.assertEqual(set(copied["files"]), expected)
        self.assertEqual(set(copied["roles"]), {"intro", "credits"})
        self.assertFalse(copied["attract_assigned"])
        self.assertFalse(copied["external_vista_or_xbox_required"])
        for name, row in copied["files"].items():
            self.assertEqual((stage / name).read_bytes(), (self.source / name).read_bytes())
            self.assertEqual(hashlib.sha256((stage / name).read_bytes()).hexdigest(), row["sha256"])
        self.source.rename(self.root / "moved-source")
        manifest = movies.read_manifest(stage / "resources/movie-manifest.json")
        movies.verify(stage / "resources/movies", manifest)

    def test_legacy_reference_helper_copies_original_tone_without_an_original_installation(self):
        # Explicit offline-reference helper only; default package never uses it.
        shutil.copytree(ROOT / preview_package.AMBIENCE, self.source / preview_package.AMBIENCE)
        stage = self.root / "tone-stage"
        owner = preview_package.copy_bundled_loading_ambience(self.source, stage)
        self.source.rename(self.root / "source-unavailable")
        self.assertEqual(preview_package.bundled_ambience(stage)[1], owner)
        self.assertEqual((stage / preview_package.AMBIENCE / preview_package.TONE).stat().st_size, 18396)

    def test_default_package_preflight_uses_decoder_and_codebook_without_original_tone(self):
        def assembled(root, runtime, launchers, output, directory, shaders, stage, pending,
                      *, frontend=None, presentation=None):
            self.assert_default_mcc_dispatch(root, frontend, presentation)
            self.assertFalse((root / loading.AMBIENCE).exists())
            owner = preview_package.converter_dependencies(root)
            self.assertEqual(owner["converter"]["path"], "bin/halo2_mcc_loading_audio")
            self.assertEqual(owner["codebook"]["sha256"], loading.CODEBOOK_SHA)
            stage.mkdir()
            with zipfile.ZipFile(pending, "x"):
                pass
            return dict(preflight_reached_owned_stage=True)
        with patch.object(preview_package, "_assemble", side_effect=assembled), \
                patch.object(preview_package, "subprocess") as process:
            process.run.side_effect = AssertionError("Synthetic executable must not run")
            result = self.package()
        self.assertTrue(result["preflight_reached_owned_stage"])
        self.assertEqual(list(self.output.parent.iterdir()), [self.output])

    def test_real_assembly_has_no_reference_prompt_or_movie_closure(self):
        """Actual staging/manifest/ZIP route; ELF/platform execution is mocked.

        This qualifies dependency omission and preservation only, not a working
        executable, decoder, installed Python, renderer or portable release.
        """
        from test_controller_prompt_assets import controller_prompt_fixture
        controller_prompt_fixture(self.source)
        tools = self.source / 'tools/data'
        tools.mkdir(parents=True)
        (tools / 'fixture.txt').write_bytes(b'unchanged tool data')
        decoder_tools = self.source / 'tools/macos'
        decoder_tools.mkdir()
        for name in ('__init__.py', 'loading_converter.py'):
            (decoder_tools / name).write_bytes(b'# not executed: ' + name.encode())
        layout_fixture(self.source)
        (self.source / 'resources/launcher.svg').write_bytes(b'<svg/>')
        licenses = self.source / 'resources/licenses'
        licenses.mkdir()
        (licenses / 'notice.txt').write_bytes(b'fixture notice')
        for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
            (self.launchers / name).write_bytes(b'not executed: launcher fixture')
        stdlib = self.root / 'platform/python3.13'
        stdlib.mkdir(parents=True)
        (stdlib / 'fixture.py').write_bytes(b'# fixture standard library')
        pillow = self.root / 'platform/PIL'
        pillow.mkdir()
        (pillow / '__init__.py').write_bytes(b'# fixture pillow')
        interpreter = self.root / 'platform/python3'
        interpreter.write_bytes(b'not executed: interpreter fixture')
        loader = self.root / 'platform/ld-linux-x86-64.so.2'
        loader.write_bytes(b'not executed: loader fixture')
        loader.chmod(0o700)
        actual_copytree = shutil.copytree
        actual_merge = preview_package.merge_runtime_library
        def copytree(source, destination, *args, **kwargs):
            if str(source) in ('/usr/lib/tcl8.6', '/usr/lib/tk8.6', '/usr/lib/tcl8'):
                Path(destination).mkdir(parents=True)
                return destination
            return actual_copytree(source, destination, *args, **kwargs)
        def merge(libraries, source):
            return actual_merge(libraries, loader if str(source) == '/lib64/ld-linux-x86-64.so.2' else source)
        for old_bank in (False, True):
            with self.subTest(old_bank=old_bank):
                self.output = self.output.with_name('with-old-bank.zip' if old_bank else 'without-bank.zip')
                bank = self.source / preview_package.PROMPT_SOURCE
                preserved = []
                if old_bank:
                    bank.mkdir(parents=True)
                    sentinel = bank / 'manifest.json'
                    sentinel.write_bytes(b'custom old reference bank; deliberately invalid')
                    supplied = self.source / 'resources/movies'
                    supplied.mkdir()
                    (supplied / 'manifest.json').write_bytes(b'invalid preserved movie metadata')
                    (supplied / 'intro_60.mp4').write_bytes(b'invalid preserved intro')
                    (supplied / 'credits_60.mp4').write_bytes(b'invalid preserved credits')
                    source_manifest = self.source / 'resources/movie-manifest.json'
                    source_manifest.write_bytes(b'invalid preserved source manifest')
                    playback = self.source / 'resources/movie-playback'
                    playback.mkdir()
                    (playback / 'intro.hevc.mp4').write_bytes(b'preserved optional playback copy')
                    (playback / 'manifest.json').write_bytes(b'preserved optional playback owner')
                    preserved = [sentinel, source_manifest, *supplied.iterdir(), *playback.iterdir()]
                before = {p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in preserved}
                with patch.object(preview_package, 'verify_bundled_prompts', side_effect=AssertionError('Reference preflight selected')), \
                     patch.object(preview_package, 'copy_bundled_prompts', side_effect=AssertionError('Reference bank copied')), \
                     patch.object(preview_package, 'verify_bundled_movies', side_effect=AssertionError('Reference movie preflight selected')), \
                     patch.object(preview_package, 'copy_bundled_movies', side_effect=AssertionError('Reference movies copied')), \
                     patch.object(preview_package, 'dependencies', return_value=[]), \
                     patch.object(preview_package, 'merge_runtime_library', side_effect=merge), \
                     patch.object(preview_package, 'relocate_compiled_shader_default', side_effect=lambda payload, _: (payload, {})), \
                     patch.object(preview_package.subprocess, 'run', return_value=SimpleNamespace(returncode=0, stdout='', stderr='')), \
                     patch.object(preview_package.sysconfig, 'get_path', return_value=str(stdlib)), \
                     patch.object(preview_package.sys, 'executable', str(interpreter)), \
                     patch.dict(sys.modules, {'PIL': SimpleNamespace(__path__=[str(pillow)])}), \
                     patch.object(preview_package.shutil, 'copytree', side_effect=copytree), \
                     patch.object(preview_package.shutil, 'which', return_value=None):
                    result = self.package()
                self.assertFalse(result['stage_retained'])
                self.assertNotIn('authored_prompt_source_files', result)
                self.assertNotIn('bundled_movie_payload_bytes', result)
                with zipfile.ZipFile(self.output) as archive:
                    names = archive.namelist()
                    prefix = self.output.stem + '/'
                    manifest = json.loads(archive.read(prefix + 'PRECOMPILED_TEST_BUILD.json'))
                    for name, row in manifest['bundled_native_variants']['files'].items():
                        payload = archive.read(prefix + name)
                        self.assertEqual(payload, (self.source / name).read_bytes())
                        self.assertEqual(hashlib.sha256(payload).hexdigest(), row['sha256'])
                    self.assertIn('data/tags/variants/bomb-defusal-v1.json', manifest['bundled_native_variants']['files'])
                    readme = archive.read(prefix + 'README.txt').decode()
                    for name in ('__init__.py', 'loading_converter.py'):
                        self.assertEqual(archive.read(prefix + 'tools/macos/' + name),
                                         (decoder_tools / name).read_bytes())
                    self.assertNotIn('authored_prompts', manifest)
                    self.assertNotIn('bundled_movies', manifest)
                    self.assertIn('Controller artwork files are included.', readme)
                    self.assertNotIn('for each local player', readme)
                    self.assertIn('Menus use imported MCC fonts.', readme)
                    self.assertIn('Intro, credits and attract remain unavailable', readme)
                    self.assertFalse(any('/resources/presentation/prompts/' in name or '/presentation/prompts/' in name for name in names))
                    self.assertFalse(any('/resources/movies/' in name or '/presentation/movies/' in name or
                                         '/resources/movie-playback/' in name or name.endswith('/resources/movie-manifest.json')
                                         for name in names))
                    self.assertFalse(any(name.endswith(('/intro_60.mp4', '/credits_60.mp4', '/intro.hevc.mp4')) for name in names))
                self.assertEqual({p: (p.read_bytes(), p.stat().st_ino, p.stat().st_mtime_ns) for p in preserved}, before)
                self.assertFalse(any(self.output.parent.glob('.mcc-package-*')))

    def test_missing_loading_converter_stops_package_before_staging(self):
        self.converter.unlink()
        with patch.object(preview_package, "_assemble", side_effect=AssertionError("staging began")):
            with self.assertRaisesRegex(ValueError, "converter.*missing"):
                self.package()
        self.assertFalse(self.output.parent.exists())

    def test_changed_fixed_loading_codebook_stops_package_before_staging(self):
        self.codebook.write_bytes(b"changed codebook")
        with patch.object(preview_package, "_assemble", side_effect=AssertionError("staging began")):
            with self.assertRaisesRegex(ValueError, "codebook differs"):
                self.package()
        self.assertFalse(self.output.parent.exists())

    def test_runtime_library_basename_requires_identical_content(self):
        first = self.root / "first/libvorbis.so.0"
        second = self.root / "second/libvorbis.so.0"
        first.parent.mkdir();second.parent.mkdir()
        first.write_bytes(b"same exact private library bytes")
        second.write_bytes(first.read_bytes())
        libraries = {}
        preview_package.merge_runtime_library(libraries, first)
        preview_package.merge_runtime_library(libraries, second)
        self.assertEqual(libraries, {first.name: first})
        original = second.read_bytes()
        second.write_bytes(b"X" * len(original))
        with self.assertRaisesRegex(ValueError, "Conflicting"):
            preview_package.merge_runtime_library(libraries, second)
        self.assertEqual(libraries, {first.name: first})

    def test_reference_invalid_or_missing_second_movie_fails_before_any_output_stage(self):
        for missing in (False, True):
            with self.subTest(missing=missing):
                bundle_fixture(self.source)
                path = self.source / "resources/movies/credits_60.mp4"
                if missing:
                    path.unlink()
                else:
                    path.write_bytes(b"X" * path.stat().st_size)
                with self.assertRaises(ValueError):
                    preview_package.verify_bundled_movies(self.source)
                self.assertFalse(self.output.parent.exists())

    def test_reference_paired_metadata_and_strict_role_validation_precede_output(self):
        bundle_fixture(self.source)
        (self.source / "resources/movies/manifest.json").write_text('{}')
        with self.assertRaisesRegex(ValueError, "disagree"):
            preview_package.verify_bundled_movies(self.source)
        self.assertFalse(self.output.parent.exists())
        metadata = bundle_fixture(self.source)
        metadata["files"]["../unassigned.mp4"] = metadata["files"].pop("intro_60.mp4")
        (self.source / "resources/movie-manifest.json").write_text(json.dumps(metadata))
        with self.assertRaisesRegex(ValueError, "exactly"):
            preview_package.verify_bundled_movies(self.source)
        self.assertFalse(self.output.parent.exists())

    def test_reference_source_symlink_rejected_before_output(self):
        bundle_fixture(self.source)
        intro = self.source / "resources/movies/intro_60.mp4"
        target = self.root / "intro.mp4"
        intro.rename(target)
        intro.symlink_to(target)
        with self.assertRaisesRegex(ValueError, "symlink"):
            preview_package.verify_bundled_movies(self.source)
        self.assertFalse(self.output.parent.exists())

    def test_source_change_after_preflight_cannot_copy_other_bytes(self):
        bundle_fixture(self.source)
        verified = preview_package.verify_bundled_movies(self.source)
        path = self.source / "resources/movies/intro_60.mp4"
        path.write_bytes(b"X" * path.stat().st_size)
        stage = self.root / "never-created"
        with self.assertRaisesRegex(ValueError, "checksum"):
            preview_package.copy_bundled_movies(self.source, stage, verified)
        self.assertFalse(stage.exists())

    def test_exact_metadata_bytes_survive_equivalent_json_formatting(self):
        manifest = bundle_fixture(self.source)
        canonical = self.source / "resources/movie-manifest.json"
        canonical.write_text(json.dumps(manifest, separators=(",", ":")))
        verified = preview_package.verify_bundled_movies(self.source)
        self.assertNotEqual(verified["files"]["resources/movie-manifest.json"]["sha256"],
                            verified["files"]["resources/movies/manifest.json"]["sha256"])
        stage = self.root / "stage"
        preview_package.copy_bundled_movies(self.source, stage, verified)
        self.assertEqual((stage / "resources/movie-manifest.json").read_bytes(), canonical.read_bytes())

    def test_incomplete_assembly_cleans_every_owned_stage_and_partial_archive(self):
        def failed(root, runtime, launchers, output, directory, shaders, stage, pending,
                   *, frontend=None, presentation=None):
            self.assert_default_mcc_dispatch(root, frontend, presentation)
            stage.mkdir()
            (stage / "partial").write_bytes(b"partial")
            pending.write_bytes(b"incomplete archive")
            raise RuntimeError("assembly failed")
        with patch.object(preview_package, "_assemble", side_effect=failed):
            with self.assertRaisesRegex(RuntimeError, "assembly failed"):
                self.package()
        self.assertEqual(list(self.output.parent.iterdir()), [])

    def test_sigterm_cleans_owned_stage_and_restores_signal_handler(self):
        previous = signal.getsignal(signal.SIGTERM)
        def cancelled(root, runtime, launchers, output, directory, shaders, stage, pending,
                      *, frontend=None, presentation=None):
            self.assert_default_mcc_dispatch(root, frontend, presentation)
            stage.mkdir()
            pending.write_bytes(b"partial")
            os.kill(os.getpid(), signal.SIGTERM)
        with patch.object(preview_package, "_assemble", side_effect=cancelled):
            with self.assertRaises(SystemExit) as result:
                self.package()
        self.assertEqual(result.exception.code, 143)
        self.assertEqual(signal.getsignal(signal.SIGTERM), previous)
        self.assertEqual(list(self.output.parent.iterdir()), [])

    def test_complete_synthetic_archive_publishes_without_retaining_stage(self):
        def assembled(root, runtime, launchers, output, directory, shaders, stage, pending,
                      *, frontend=None, presentation=None):
            self.assert_default_mcc_dispatch(root, frontend, presentation)
            stage.mkdir()
            decoder = preview_package.converter_dependencies(root)
            decoder_files = {decoder[role]["path"] for role in ("converter", "codebook")}
            decoder_files.update(preview_package.variant_sources(root))
            for name in decoder_files:
                preview_package.copy(root / name, stage / name)
            with zipfile.ZipFile(pending, "x", allowZip64=True) as archive:
                for name in decoder_files:
                    archive.write(stage / name, name)
            return dict(stage_retained=False, synthetic=True)
        with patch.object(preview_package, "_assemble", side_effect=assembled):
            result = self.package()
        self.assertFalse(result["stage_retained"])
        self.assertEqual(list(self.output.parent.iterdir()), [self.output])
        with zipfile.ZipFile(self.output) as archive:
            self.assertEqual(set(archive.namelist()), {name.as_posix() for name in self.originals})
            for name, payload in self.originals.items():
                self.assertEqual(archive.read(name.as_posix()), payload)
        for name, payload in self.originals.items():
            self.assertEqual((self.source / name).read_bytes(), payload)

    def test_reference_archive_writer_enables_zip64_and_stores_only_assigned_mp4s(self):
        bundle_fixture(self.source)
        verified = preview_package.verify_bundled_movies(self.source)
        stage = self.root / "synthetic-preview"
        preview_package.copy_bundled_movies(self.source, stage, verified)
        pending = self.root / "tiny-fixture.zip"
        real_zip = zipfile.ZipFile
        with patch.object(preview_package.zipfile, "ZipFile", wraps=real_zip) as opened:
            preview_package.write_archive(stage, pending, verified)
        self.assertTrue(opened.call_args.kwargs["allowZip64"])
        with real_zip(pending) as archive:
            self.assertIsNone(archive.testzip())
            for name, row in verified["files"].items():
                entry = archive.getinfo(stage.name + "/" + name)
                self.assertEqual(hashlib.sha256(archive.read(entry)).hexdigest(), row["sha256"])
                self.assertEqual(entry.compress_type, zipfile.ZIP_STORED if name in verified["roles"].values() else zipfile.ZIP_DEFLATED)

    def test_reference_all91_prompt_sources_survive_and_mcc_font_tree_is_untouched(self):
        prompt_fixture(self.source)
        verified = preview_package.verify_bundled_prompts(self.source)
        self.assertEqual(verified["source_file_count"], 91)
        self.assertEqual(verified["svg_masters"], 82)
        self.assertEqual(verified["runtime_files"], 7)
        self.assertEqual(verified["cache_files"], 9)
        self.assertFalse(verified["replaces_MCC_fonts"])
        self.assertFalse(verified["external_vista_or_xbox_required"])
        stage = self.root / "prompt-stage"
        font = stage / "halo2/h2_fonts/font_table.txt"
        font.parent.mkdir(parents=True)
        font.write_bytes(b"MCC font owner remains unchanged")
        preview_package.copy_bundled_prompts(self.source, stage, verified)
        self.assertEqual(font.read_bytes(), b"MCC font owner remains unchanged")
        for name, row in verified["files"].items():
            self.assertEqual((stage / name).read_bytes(), (self.source / name).read_bytes())
            self.assertEqual(hashlib.sha256((stage / name).read_bytes()).hexdigest(), row["sha256"])
        self.source.rename(self.root / "moved-source")
        self.assertEqual(preview_package.verify_bundled_prompts(stage), verified)

    def test_reference_bad_or_missing_prompt_inputs_reject(self):
        for name in ("atlases/5.dds", "vectors/" + "0" * 64 + ".svg", "manifest.json"):
            with self.subTest(name=name):
                prompt_fixture(self.source)
                path = self.source / preview_package.PROMPT_SOURCE / name
                if name == "manifest.json":
                    path.unlink()
                else:
                    path.write_bytes(b"X" * path.stat().st_size)
                with self.assertRaises(ValueError):
                    preview_package.verify_bundled_prompts(self.source)
                self.assertFalse(self.output.parent.exists())

    def test_reference_unknown_font_and_symlink_are_not_prompt_inputs(self):
        prompt_fixture(self.source)
        bank = self.source / preview_package.PROMPT_SOURCE
        font = bank / "vista-font.font"
        font.write_bytes(b"unassigned font is not part of authored prompts")
        with self.assertRaisesRegex(ValueError, "custom data"):
            preview_package.verify_bundled_prompts(self.source)
        self.assertFalse(self.output.parent.exists())
        font.unlink()
        path = bank / "atlases/0.dds"
        outside = self.root / "same-bytes.dds"
        path.rename(outside)
        path.symlink_to(outside)
        with self.assertRaisesRegex(ValueError, "symlink"):
            preview_package.verify_bundled_prompts(self.source)
        self.assertFalse(self.output.parent.exists())

    def test_reference_prompt_change_after_preflight_is_not_silently_copied(self):
        prompt_fixture(self.source)
        verified = preview_package.verify_bundled_prompts(self.source)
        path = self.source / preview_package.PROMPT_SOURCE / "atlases/0.dds"
        path.write_bytes(b"X" * path.stat().st_size)
        stage = self.root / "never-created"
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            preview_package.copy_bundled_prompts(self.source, stage, verified)
        self.assertFalse(stage.exists())

    def test_reference_destination_prompt_readback_detects_corruption(self):
        prompt_fixture(self.source)
        verified = preview_package.verify_bundled_prompts(self.source)
        stage = self.root / "bad-destination"
        actual_copy = preview_package.copy
        def changed(source, destination):
            actual_copy(source, destination)
            if Path(destination).name == "0.dds":
                destination.write_bytes(b"X" * destination.stat().st_size)
        with patch.object(preview_package, "copy", side_effect=changed):
            with self.assertRaisesRegex(ValueError, "SHA-256"):
                preview_package.copy_bundled_prompts(self.source, stage, verified)

    def test_existing_or_concurrent_output_is_never_replaced(self):
        self.output.parent.mkdir()
        self.output.write_bytes(b"existing ZIP")
        with self.assertRaisesRegex(ValueError, "Refusing"):
            self.package()
        self.assertEqual(self.output.read_bytes(), b"existing ZIP")
        self.output.unlink()
        def raced(root, runtime, launchers, output, directory, shaders, stage, pending,
                  *, frontend=None, presentation=None):
            self.assert_default_mcc_dispatch(root, frontend, presentation)
            pending.write_bytes(b"new verified bytes")
            output.write_bytes(b"other publisher")
            return {}
        with patch.object(preview_package, "_assemble", side_effect=raced):
            with self.assertRaisesRegex(ValueError, "changed during import"):
                self.package()
        self.assertEqual(self.output.read_bytes(), b"other publisher")
        self.assertEqual(list(self.output.parent.iterdir()), [self.output])


class RuntimeAdjacentVariantPackageTest(unittest.TestCase):
    """Metadata-only package ownership checks; no runtime or game assets."""
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix="halo2-runtime-bank-")
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)
        self.runtime = self.root / "compiled/halo2_native"
        self.runtime.parent.mkdir()
        self.runtime.write_bytes(b"synthetic runtime identity; never executed")
        self.files = {}
        for name in sorted(preview_package.RUNTIME_BANK_FILES):
            # Use the real small authored metadata, not game payloads.
            raw = (ROOT / name).read_bytes()
            path = self.runtime.parent / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
            self.files[name] = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
        self.report = dict(runtime_adjacent_data=dict(
            schema=preview_package.RUNTIME_BANK_SCHEMA, files=self.files))
        self.stage = self.root / "portable"

    def test_explicit_adjacent_bank_is_copied_beside_actual_binary(self):
        # The newer root bank can keep BombDefusal without entering the old
        # executable's independently owned lookup directory.
        support = self.stage / "data/tags/variants/bomb-defusal-v1.json"
        support.parent.mkdir(parents=True)
        support.write_bytes((ROOT / "data/tags/variants/bomb-defusal-v1.json").read_bytes())
        before = support.read_bytes()
        result = preview_package.copy_runtime_adjacent_data(
            self.runtime, self.stage, dict(report=self.report))
        self.assertEqual(result, self.report['runtime_adjacent_data'])
        self.assertEqual(support.read_bytes(), before)
        bank = self.stage / "bin/data/tags/variants"
        self.assertEqual({p.name for p in bank.iterdir()}, {Path(p).name for p in self.files})
        for name, row in self.files.items():
            self.assertEqual((self.stage / 'bin' / name).read_bytes(),
                             (self.runtime.parent / name).read_bytes())
            self.assertEqual(preview_package.sha(self.stage / 'bin' / name), row['sha256'])

    def test_no_declaration_does_not_copy_arbitrary_adjacent_directory(self):
        self.assertIsNone(preview_package.copy_runtime_adjacent_data(
            self.runtime, self.stage, dict(report={})))
        self.assertFalse(self.stage.exists())

    def test_undeclared_json_stops_before_any_copy(self):
        path = self.runtime.parent / 'data/tags/variants/bomb-defusal-v1.json'
        path.write_bytes((ROOT / 'data/tags/variants/bomb-defusal-v1.json').read_bytes())
        with self.assertRaisesRegex(ValueError, 'undeclared'):
            preview_package.copy_runtime_adjacent_data(self.runtime, self.stage, dict(report=self.report))
        self.assertFalse(self.stage.exists())

    def test_missing_or_changed_payload_stops_before_any_copy(self):
        path = self.runtime.parent / sorted(self.files)[0]
        original = path.read_bytes()
        for replacement in (None, b'changed authored metadata'):
            with self.subTest(replacement=replacement):
                if replacement is None:
                    path.unlink()
                else:
                    path.write_bytes(replacement)
                with self.assertRaises(ValueError):
                    preview_package.copy_runtime_adjacent_data(self.runtime, self.stage, dict(report=self.report))
                self.assertFalse(self.stage.exists())
                path.write_bytes(original)

    def test_file_or_directory_symlink_cannot_supply_a_bank(self):
        directory = self.runtime.parent / 'data/tags/variants'
        payload = directory / Path(sorted(self.files)[0]).name
        raw = payload.read_bytes()
        outside = self.root / 'outside.json'
        outside.write_bytes(raw)
        payload.unlink()
        payload.symlink_to(outside)
        with self.assertRaisesRegex(ValueError, 'regular'):
            preview_package.runtime_adjacent_data(self.runtime, self.report)
        payload.unlink()
        payload.write_bytes(raw)
        moved = self.root / 'moved-bank'
        directory.rename(moved)
        directory.symlink_to(moved, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'owned directories'):
            preview_package.runtime_adjacent_data(self.runtime, self.report)

    def test_inventory_cannot_redirect_or_omit_a_payload(self):
        for name in ('../outside.json', '/outside.json', 'data/tags/variants/bomb-defusal-v1.json'):
            with self.subTest(name=name):
                report = json.loads(json.dumps(self.report))
                report['runtime_adjacent_data']['files'][name] = report['runtime_adjacent_data']['files'].pop(sorted(self.files)[0])
                with self.assertRaisesRegex(ValueError, 'inventory'):
                    preview_package.copy_runtime_adjacent_data(self.runtime, self.stage, dict(report=report))
                self.assertFalse(self.stage.exists())

    def test_existing_destination_is_preserved(self):
        target = self.stage / 'bin' / sorted(self.files)[0]
        target.parent.mkdir(parents=True)
        target.write_bytes(b'other owner')
        with self.assertRaisesRegex(ValueError, 'destination already exists'):
            preview_package.copy_runtime_adjacent_data(self.runtime, self.stage, dict(report=self.report))
        self.assertEqual(target.read_bytes(), b'other owner')


if __name__ == "__main__":
    unittest.main()
