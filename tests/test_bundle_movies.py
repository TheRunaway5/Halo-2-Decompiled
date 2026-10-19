import hashlib
import importlib.util
import json
import os
from pathlib import Path
import signal
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("bundle_movies", ROOT / "tools/bundle_movies.py")
movies = importlib.util.module_from_spec(spec)
spec.loader.exec_module(movies)
sys.path.insert(0, str(ROOT / "tools"))
import package_mcc_test_build as preview_package
from test_mcc_movies import bundle_fixture
from test_mcc_prompts import prompt_fixture


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
        self.manifest = bundle_fixture(self.source)
        self.prompt_manifest = prompt_fixture(self.source)
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

    def test_four_exact_resource_paths_metadata_and_no_transcoding(self):
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

    def test_invalid_or_missing_second_movie_fails_before_any_output_stage(self):
        for missing in (False, True):
            with self.subTest(missing=missing):
                bundle_fixture(self.source)
                path = self.source / "resources/movies/credits_60.mp4"
                if missing:
                    path.unlink()
                else:
                    path.write_bytes(b"X" * path.stat().st_size)
                with patch.object(preview_package, "_assemble", side_effect=AssertionError("staging began")):
                    with self.assertRaises(ValueError):
                        self.package()
                self.assertFalse(self.output.parent.exists())

    def test_paired_metadata_and_strict_role_validation_precede_output(self):
        (self.source / "resources/movies/manifest.json").write_text('{}')
        with self.assertRaisesRegex(ValueError, "disagree"):
            self.package()
        self.assertFalse(self.output.parent.exists())
        metadata = bundle_fixture(self.source)
        metadata["files"]["../unassigned.mp4"] = metadata["files"].pop("intro_60.mp4")
        (self.source / "resources/movie-manifest.json").write_text(json.dumps(metadata))
        with self.assertRaisesRegex(ValueError, "exactly"):
            self.package()
        self.assertFalse(self.output.parent.exists())

    def test_source_symlink_rejected_before_output(self):
        intro = self.source / "resources/movies/intro_60.mp4"
        target = self.root / "intro.mp4"
        intro.rename(target)
        intro.symlink_to(target)
        with self.assertRaisesRegex(ValueError, "symlink"):
            self.package()
        self.assertFalse(self.output.parent.exists())

    def test_source_change_after_preflight_cannot_copy_other_bytes(self):
        verified = preview_package.verify_bundled_movies(self.source)
        path = self.source / "resources/movies/intro_60.mp4"
        path.write_bytes(b"X" * path.stat().st_size)
        stage = self.root / "never-created"
        with self.assertRaisesRegex(ValueError, "checksum"):
            preview_package.copy_bundled_movies(self.source, stage, verified)
        self.assertFalse(stage.exists())

    def test_exact_metadata_bytes_survive_equivalent_json_formatting(self):
        canonical = self.source / "resources/movie-manifest.json"
        canonical.write_text(json.dumps(self.manifest, separators=(",", ":")))
        verified = preview_package.verify_bundled_movies(self.source)
        self.assertNotEqual(verified["files"]["resources/movie-manifest.json"]["sha256"],
                            verified["files"]["resources/movies/manifest.json"]["sha256"])
        stage = self.root / "stage"
        preview_package.copy_bundled_movies(self.source, stage, verified)
        self.assertEqual((stage / "resources/movie-manifest.json").read_bytes(), canonical.read_bytes())

    def test_incomplete_assembly_cleans_every_owned_stage_and_partial_archive(self):
        def failed(root, runtime, launchers, output, directory, shaders, stage, pending, metadata, prompts):
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
        def cancelled(root, runtime, launchers, output, directory, shaders, stage, pending, metadata, prompts):
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
        def assembled(root, runtime, launchers, output, directory, shaders, stage, pending, metadata, prompts):
            stage.mkdir()
            preview_package.copy_bundled_movies(root, stage, metadata)
            preview_package.copy_bundled_prompts(root, stage, prompts)
            with zipfile.ZipFile(pending, "x", allowZip64=True) as archive:
                for name in {*metadata["files"], *prompts["files"]}:
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

    def test_real_archive_writer_enables_zip64_and_stores_only_assigned_mp4s(self):
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

    def test_all91_prompt_sources_survive_and_mcc_font_tree_is_untouched(self):
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

    def test_bad_or_missing_prompt_runtime_master_and_manifest_prevent_staging(self):
        for name in ("atlases/5.dds", "vectors/" + "0" * 64 + ".svg", "manifest.json"):
            with self.subTest(name=name):
                prompt_fixture(self.source)
                path = self.source / preview_package.PROMPT_SOURCE / name
                if name == "manifest.json":
                    path.unlink()
                else:
                    path.write_bytes(b"X" * path.stat().st_size)
                with patch.object(preview_package, "_assemble", side_effect=AssertionError("staging began")):
                    with self.assertRaises(ValueError):
                        self.package()
                self.assertFalse(self.output.parent.exists())

    def test_unknown_font_and_symlink_are_not_prompt_distribution_inputs(self):
        bank = self.source / preview_package.PROMPT_SOURCE
        font = bank / "vista-font.font"
        font.write_bytes(b"unassigned font is not part of authored prompts")
        with self.assertRaisesRegex(ValueError, "custom data"):
            self.package()
        self.assertFalse(self.output.parent.exists())
        font.unlink()
        path = bank / "atlases/0.dds"
        outside = self.root / "same-bytes.dds"
        path.rename(outside)
        path.symlink_to(outside)
        with self.assertRaisesRegex(ValueError, "symlink"):
            self.package()
        self.assertFalse(self.output.parent.exists())

    def test_prompt_change_after_preflight_is_not_silently_copied(self):
        verified = preview_package.verify_bundled_prompts(self.source)
        path = self.source / preview_package.PROMPT_SOURCE / "atlases/0.dds"
        path.write_bytes(b"X" * path.stat().st_size)
        stage = self.root / "never-created"
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            preview_package.copy_bundled_prompts(self.source, stage, verified)
        self.assertFalse(stage.exists())

    def test_destination_prompt_readback_detects_corruption(self):
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
        def raced(root, runtime, launchers, output, directory, shaders, stage, pending, metadata, prompts):
            pending.write_bytes(b"new verified bytes")
            output.write_bytes(b"other publisher")
            return {}
        with patch.object(preview_package, "_assemble", side_effect=raced):
            with self.assertRaisesRegex(ValueError, "changed during import"):
                self.package()
        self.assertEqual(self.output.read_bytes(), b"other publisher")
        self.assertEqual(list(self.output.parent.iterdir()), [self.output])


if __name__ == "__main__":
    unittest.main()
