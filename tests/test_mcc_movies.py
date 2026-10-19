"""Bundled presentation copies resume without replacing MCC/custom content."""
import hashlib
import json
import os
from pathlib import Path
import signal
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import mcc_movies as movies
from audit_rendering import FormatError


def bundle_fixture(root):
    bundle = root / "resources/movies"
    bundle.mkdir(parents=True, exist_ok=True)
    files = {}
    for name in movies.ROLES.values():
        data = b"Small bundled role fixture: " + name.encode()
        (bundle / name).write_bytes(data)
        files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    metadata = dict(schema=movies.SOURCE_SCHEMA, description="fixture", files=files)
    raw = (json.dumps(metadata, indent=2) + "\n").encode()
    (root / movies.SOURCE_MANIFEST).write_bytes(raw)
    (bundle / "manifest.json").write_bytes(raw)
    return metadata


class MccMoviesTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-mcc-movies-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "port"
        self.source = bundle_fixture(self.root)
        self.cache = self.root / "assets/imported/mcc-fixture"
        self.cache.mkdir(parents=True)
        self.report = dict(schema="H2_MCC_TU15_PREPARATION_V1",
                           external_installation_requirement="Halo MCC only",
                           runtime_active=False, full_tu15_parity=False, custom="preserve")
        (self.cache / "provenance.json").write_text(json.dumps(self.report))
        for name in ("halo2/data/My Mod/unchanged.bin", "native-shaders/keep.bin"):
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(name.encode())
        self.originals = {p.relative_to(self.root): p.read_bytes()
                          for p in (self.root / "resources").rglob("*") if p.is_file()}
        self.protected = {p.relative_to(self.cache): p.read_bytes()
                          for directory in ("halo2", "native-shaders")
                          for p in (self.cache / directory).rglob("*") if p.is_file()}

    def prepare(self):
        return movies.prepare(self.root, self.cache, progress=lambda _: None)

    def repair(self):
        return movies.repair(self.root, self.cache, progress=lambda _: None)

    def tearDown(self):
        self.assertFalse(list(self.cache.rglob(".movie-copy-*")))
        self.assertFalse(list(self.cache.rglob(".movie-manifest-*")))
        self.assertFalse(list(self.cache.glob(".movie-provenance-*")))
        for name, data in self.protected.items():
            self.assertEqual((self.cache / name).read_bytes(), data)

    def test_exact_two_roles_and_provenance_do_not_change_readiness(self):
        _, report = self.repair()
        entry = report["port_bundled_movies"]
        self.assertEqual(set(entry["roles"]), {"intro", "credits"})
        bank = self.cache / movies.BANK
        raw = (bank / "manifest.json").read_bytes()
        self.assertEqual(entry["sha256"], hashlib.sha256(raw).hexdigest())
        metadata = json.loads(raw)
        self.assertFalse(metadata["attract_assigned"])
        self.assertFalse(metadata["external_vista_or_xbox_required"])
        self.assertTrue(metadata["copied_without_transcoding"])
        self.assertFalse(report["runtime_active"])
        self.assertFalse(report["full_tu15_parity"])
        self.assertEqual(report["custom"], "preserve")
        for role, row in entry["roles"].items():
            self.assertEqual((bank / row["path"]).read_bytes(),
                             (self.root / "resources/movies" / movies.ROLES[role]).read_bytes())
        for name, data in self.originals.items():
            self.assertEqual((self.root / name).read_bytes(), data)

    def test_matching_repair_hashes_both_sides_but_skips_every_rewrite(self):
        self.repair()
        paths = [self.cache / movies.MANIFEST, self.cache / "provenance.json"] + [
            self.cache / movies.BANK / name for name in movies.ROLES.values()]
        before = [(p.stat().st_ino, p.stat().st_mtime_ns, p.read_bytes()) for p in paths]
        with patch.object(movies, "_verify", wraps=movies._verify) as verify, \
             patch.object(movies.shutil, "copyfileobj", side_effect=AssertionError("matching copy")):
            self.repair()
        self.assertEqual(len(verify.call_args_list), 4)
        self.assertEqual(before, [(p.stat().st_ino, p.stat().st_mtime_ns, p.read_bytes()) for p in paths])

    def test_partial_verified_bank_resumes_only_missing_role(self):
        bank = self.cache / movies.BANK
        bank.mkdir(parents=True)
        intro = bank / movies.ROLES["intro"]
        intro.write_bytes((self.root / "resources/movies" / intro.name).read_bytes())
        stamp = intro.stat().st_ino, intro.stat().st_mtime_ns
        self.repair()
        self.assertEqual(stamp, (intro.stat().st_ino, intro.stat().st_mtime_ns))
        self.assertTrue((bank / movies.ROLES["credits"]).is_file())

    def test_bad_second_original_prevents_any_movie_publication(self):
        credits = self.root / "resources/movies/credits_60.mp4"
        credits.write_bytes(b"X" * credits.stat().st_size)
        with self.assertRaisesRegex(FormatError, "SHA-256"):
            self.repair()
        self.assertFalse((self.cache / movies.BANK).exists())
        self.assertNotIn("port_bundled_movies", json.loads((self.cache / "provenance.json").read_text()))

    def test_changed_destination_is_preserved_even_when_same_size(self):
        self.repair()
        intro = self.cache / movies.BANK / movies.ROLES["intro"]
        data = b"C" * intro.stat().st_size
        intro.write_bytes(data)
        before = (self.cache / "provenance.json").read_bytes()
        with self.assertRaisesRegex(FormatError, "SHA-256"):
            self.repair()
        self.assertEqual(intro.read_bytes(), data)
        self.assertEqual((self.cache / "provenance.json").read_bytes(), before)

    def test_unknown_manifest_or_provenance_is_preserved(self):
        bank = self.cache / movies.BANK
        bank.mkdir(parents=True)
        manifest = bank / "manifest.json"
        manifest.write_text('{"custom":true}')
        with self.assertRaisesRegex(FormatError, "customized"):
            self.repair()
        self.assertEqual(manifest.read_text(), '{"custom":true}')
        manifest.unlink()
        report = dict(self.report, port_bundled_movies={"custom": True})
        (self.cache / "provenance.json").write_text(json.dumps(report))
        with self.assertRaisesRegex(FormatError, "customized"):
            self.repair()
        self.assertFalse(list(bank.glob("*.mp4")))

    def test_source_symlink_and_destination_parent_symlink_rejected(self):
        intro = self.root / "resources/movies/intro_60.mp4"
        other = self.root / "same-bytes.mp4"
        intro.rename(other)
        intro.symlink_to(other)
        with self.assertRaisesRegex(FormatError, "symlink"):
            self.prepare()
        intro.unlink()
        other.rename(intro)
        outside = self.root / "outside"
        outside.mkdir()
        (self.cache / "presentation").symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(FormatError, "symlink"):
            self.prepare()
        self.assertEqual(list(outside.iterdir()), [])

    def test_role_schema_and_paired_source_manifests_are_strict(self):
        path = self.root / movies.SOURCE_MANIFEST
        metadata = json.loads(path.read_text())
        metadata["files"]["../intro_60.mp4"] = metadata["files"].pop("intro_60.mp4")
        path.write_text(json.dumps(metadata))
        with self.assertRaisesRegex(FormatError, "exactly"):
            self.prepare()
        bundle_fixture(self.root)
        (self.root / "resources/movies/manifest.json").write_text('{}')
        with self.assertRaisesRegex(FormatError, "disagree"):
            self.prepare()

    def test_interrupted_copy_cleans_temp_and_matching_retry_resumes(self):
        before = (self.cache / "provenance.json").read_bytes()
        actual = movies.shutil.copyfileobj
        calls = []
        def interrupted(source, output, length):
            calls.append(source.name)
            if len(calls) == 2:
                output.write(b"partial")
                raise KeyboardInterrupt()
            return actual(source, output, length)
        with patch.object(movies.shutil, "copyfileobj", side_effect=interrupted):
            with self.assertRaises(KeyboardInterrupt):
                self.repair()
        self.assertEqual((self.cache / "provenance.json").read_bytes(), before)
        self.assertFalse((self.cache / movies.MANIFEST).exists())
        self.assertFalse(list(self.cache.rglob(".movie-copy-*")))
        self.repair()

    def test_no_replace_publication_keeps_concurrent_custom_file(self):
        source, destination = self.cache / "pending", self.cache / "custom"
        source.write_bytes(b"verified")
        destination.write_bytes(b"custom")
        with self.assertRaisesRegex(FormatError, "changed during import"):
            movies._publish_new(source, destination)
        self.assertEqual(destination.read_bytes(), b"custom")
        self.assertEqual(source.read_bytes(), b"verified")

    def test_sigterm_cleans_incomplete_copy_and_restores_callers_handler(self):
        previous = signal.getsignal(signal.SIGTERM)
        def stopped(source, output, length):
            output.write(b"partial")
            os.kill(os.getpid(), signal.SIGTERM)
        with patch.object(movies.shutil, "copyfileobj", side_effect=stopped):
            with self.assertRaises(SystemExit) as result:
                self.repair()
        self.assertEqual(result.exception.code, 143)
        self.assertEqual(signal.getsignal(signal.SIGTERM), previous)
        self.assertFalse(list(self.cache.rglob(".movie-copy-*")))
        self.assertFalse((self.cache / movies.MANIFEST).exists())
        self.repair()

    def test_source_manifest_changes_during_copy_cannot_pin_mixed_import(self):
        actual = movies.shutil.copyfileobj
        def changed(source, output, length):
            result = actual(source, output, length)
            path = self.root / movies.SOURCE_MANIFEST
            data = json.loads(path.read_text())
            data["description"] = "changed while importing"
            for p in (path, self.root / "resources/movies/manifest.json"):
                p.write_text(json.dumps(data))
            return result
        with patch.object(movies.shutil, "copyfileobj", side_effect=changed):
            with self.assertRaisesRegex(FormatError, "manifests changed"):
                self.repair()
        self.assertFalse((self.cache / movies.MANIFEST).exists())


if __name__ == "__main__":
    unittest.main()
