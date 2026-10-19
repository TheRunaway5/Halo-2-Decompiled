"""Tiny metadata/copy fixtures for the authored prompt import, without rendering."""
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
import mcc_prompts as prompts
from audit_rendering import FormatError


def prompt_fixture(root):
    """Synthetic bytes exercise the copy contract, never visual acceptance."""
    bank = root / prompts.SOURCE
    records = {}
    for name in sorted(prompts.RUNTIME_FILES):
        records[name] = b"synthetic prompt runtime fixture: " + name.encode()
    records["reconstruction.json"] = b'{"synthetic":true}\n'
    for i in range(82):
        records[f"vectors/{i:064x}.svg"] = f'<svg><path id="fixture-{i}"/></svg>'.encode()
    files = {}
    for name, data in records.items():
        path = bank / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    metadata = dict(schema=prompts.SCHEMA, source_schema=prompts.SOURCE_SCHEMA,
                    density=4, selectors=list(prompts.SELECTORS),
                    runtime_files={n: files[n] for n in sorted(prompts.RUNTIME_FILES)},
                    artwork_files={n: row for n, row in files.items() if n not in prompts.RUNTIME_FILES},
                    source_provenance=dict(delivery_sha256="1" * 64,
                        bundle_manifest_sha256="2" * 64,
                        reconstruction_sha256=files["reconstruction.json"]["sha256"]),
                    external_vista_or_xbox_required=False)
    (bank / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n")
    return metadata


class MccPromptsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-mcc-prompts-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "port"
        self.source = prompt_fixture(self.root)
        self.cache = self.root / "assets/imported/mcc-fixture"
        self.cache.mkdir(parents=True)
        self.report = dict(schema="H2_MCC_TU15_PREPARATION_V1",
                           external_installation_requirement="Halo MCC only",
                           runtime_active=False, full_tu15_parity=False, custom="preserved")
        (self.cache / "provenance.json").write_text(json.dumps(self.report))
        self.protected = {}
        for name in ("halo2/h2_fonts/font_table.txt", "halo2/data/My Mod/Keep.bin",
                     "native-shaders/keep.bin", "presentation/movies/keep.mp4"):
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(name.encode())
            self.protected[name] = path.read_bytes()
        self.bank = self.cache / prompts.BANK
        self.originals = {p.relative_to(self.root): p.read_bytes()
                          for p in (self.root / "resources").rglob("*") if p.is_file()}

    def repair(self):
        return prompts.repair(self.root, self.cache, progress=lambda _: None)

    def tearDown(self):
        self.assertFalse(list(self.cache.rglob(".prompt-*")))
        for name, data in self.protected.items():
            self.assertEqual((self.cache / name).read_bytes(), data)

    def test_runtime_only_cache_binding_is_relative_and_readiness_stays_false(self):
        _, report = self.repair()
        self.assertFalse(report["runtime_active"])
        self.assertFalse(report["full_tu15_parity"])
        self.assertEqual(report["custom"], "preserved")
        entry = report["port_authored_prompts"]
        self.assertEqual(entry["path"], prompts.MANIFEST)
        self.assertEqual(set(entry["runtime_files"]), prompts.RUNTIME_FILES)
        self.assertEqual((self.bank / "prompt-source.tsv").read_bytes(), prompts.BINDING)
        self.assertEqual({p.relative_to(self.bank).as_posix() for p in self.bank.rglob("*") if p.is_file()},
                         {*prompts.RUNTIME_FILES, "manifest.json", "prompt-source.tsv"})
        self.assertEqual(entry["sha256"], hashlib.sha256((self.bank / "manifest.json").read_bytes()).hexdigest())
        for name in prompts.RUNTIME_FILES:
            self.assertEqual((self.bank / name).read_bytes(), (self.root / prompts.SOURCE / name).read_bytes())
        for name, data in self.originals.items():
            self.assertEqual((self.root / name).read_bytes(), data)

    def test_matching_repair_rehashes_source_and_destination_without_rewrites(self):
        self.repair()
        paths = [self.cache / "provenance.json", *[p for p in self.bank.rglob("*") if p.is_file()]]
        before = [(p.stat().st_ino, p.stat().st_mtime_ns, p.read_bytes()) for p in paths]
        with patch.object(prompts, "_verify", wraps=prompts._verify) as verify, \
             patch.object(prompts.shutil, "copyfileobj", side_effect=AssertionError("unneeded copy")):
            self.repair()
        checked = {Path(call.args[0]) for call in verify.call_args_list}
        self.assertTrue({self.bank / name for name in prompts.RUNTIME_FILES} <= checked)
        self.assertEqual(before, [(p.stat().st_ino, p.stat().st_mtime_ns, p.read_bytes()) for p in paths])

    def test_partial_matching_bank_resumes_only_missing_files(self):
        target = self.bank / "atlases/0.dds"
        target.parent.mkdir(parents=True)
        target.write_bytes((self.root / prompts.SOURCE / "atlases/0.dds").read_bytes())
        before = target.stat().st_ino, target.stat().st_mtime_ns
        self.repair()
        self.assertEqual(before, (target.stat().st_ino, target.stat().st_mtime_ns))
        self.assertTrue((self.bank / "manifest.json").is_file())

    def test_bad_original_or_master_prevents_any_bank_publication(self):
        for name in ("atlases/5.dds", "vectors/" + "0" * 64 + ".svg"):
            with self.subTest(name=name):
                path = self.root / prompts.SOURCE / name
                old = path.read_bytes()
                path.write_bytes(b"X" * len(old))
                with self.assertRaisesRegex(FormatError, "SHA-256"):
                    self.repair()
                self.assertFalse(self.bank.exists())
                path.write_bytes(old)

    def test_changed_destination_is_preserved_without_provenance_rewrite(self):
        self.repair()
        path = self.bank / "atlases/0.dds"
        path.write_bytes(b"C" * path.stat().st_size)
        old = (self.cache / "provenance.json").read_bytes()
        with self.assertRaisesRegex(FormatError, "SHA-256"):
            self.repair()
        self.assertEqual(path.read_bytes(), b"C" * path.stat().st_size)
        self.assertEqual((self.cache / "provenance.json").read_bytes(), old)

    def test_unknown_bank_metadata_files_and_provenance_are_never_overwritten(self):
        self.bank.mkdir(parents=True)
        for name in ("manifest.json", "prompt-source.tsv", "custom-art.dds"):
            with self.subTest(name=name):
                path = self.bank / name
                path.write_bytes(b"custom")
                with self.assertRaisesRegex(FormatError, "custom"):
                    self.repair()
                self.assertEqual(path.read_bytes(), b"custom")
                path.unlink()
        report = dict(self.report, port_authored_prompts={"custom": True})
        (self.cache / "provenance.json").write_text(json.dumps(report))
        with self.assertRaisesRegex(FormatError, "customized"):
            self.repair()
        self.assertEqual(list(self.bank.iterdir()), [])

    def test_source_and_destination_parent_symlinks_rejected(self):
        path = self.root / prompts.SOURCE / "atlases/0.dds"
        outside = self.root / "outside.dds"
        path.rename(outside)
        path.symlink_to(outside)
        with self.assertRaisesRegex(FormatError, "symlink"):
            self.repair()
        path.unlink(); outside.rename(path)
        other = self.root / "outside"
        other.mkdir()
        self.bank.symlink_to(other, target_is_directory=True)
        with self.assertRaisesRegex(FormatError, "symlink"):
            self.repair()
        self.assertEqual(list(other.iterdir()), [])

    def test_strict_schema_rejects_missing_runtime_names_bad_selectors_and_metadata_types(self):
        path = self.root / prompts.SOURCE / "manifest.json"
        original = path.read_bytes()
        for mutate in (lambda v: v["runtime_files"].pop("atlases/5.dds"),
                       lambda v: v["selectors"].reverse(),
                       lambda v: v.update(runtime_files=[]),
                       lambda v: v.update(artwork_files={}),
                       lambda v: v.update(external_vista_or_xbox_required=True)):
            value = json.loads(original)
            mutate(value)
            path.write_text(json.dumps(value))
            with self.assertRaises(FormatError):
                self.repair()
            self.assertFalse(self.bank.exists())
        path.write_text('[]')
        with self.assertRaises(FormatError):
            self.repair()
        path.write_bytes(original)

    def test_interrupted_copy_cleans_pending_then_resumes_complete_files(self):
        actual = prompts.shutil.copyfileobj
        calls = []
        def interrupt(source, output, length):
            calls.append(source.name)
            if len(calls) == 2:
                output.write(b"partial")
                raise KeyboardInterrupt()
            return actual(source, output, length)
        before = (self.cache / "provenance.json").read_bytes()
        with patch.object(prompts.shutil, "copyfileobj", side_effect=interrupt):
            with self.assertRaises(KeyboardInterrupt):
                self.repair()
        self.assertEqual((self.cache / "provenance.json").read_bytes(), before)
        self.assertFalse((self.bank / "manifest.json").exists())
        self.assertFalse(list(self.bank.glob(".prompt-*")))
        self.repair()

    def test_sigterm_cleans_pending_and_restores_handler(self):
        previous = signal.getsignal(signal.SIGTERM)
        def interrupt(source, output, length):
            output.write(b"partial")
            os.kill(os.getpid(), signal.SIGTERM)
        with patch.object(prompts.shutil, "copyfileobj", side_effect=interrupt):
            with self.assertRaises(SystemExit) as result:
                self.repair()
        self.assertEqual(result.exception.code, 143)
        self.assertEqual(signal.getsignal(signal.SIGTERM), previous)
        self.assertFalse((self.bank / "manifest.json").exists())
        self.repair()

    def test_concurrent_custom_file_cannot_be_replaced(self):
        publish = prompts._publish_new
        def insert(source, target):
            if target.suffix == ".dds":
                target.write_bytes(b"concurrent custom artwork")
            return publish(source, target)
        with patch.object(prompts, "_publish_new", side_effect=insert):
            with self.assertRaisesRegex(FormatError, "changed during import"):
                self.repair()
        self.assertEqual((self.bank / "atlases/0.dds").read_bytes(), b"concurrent custom artwork")
        self.assertNotIn("port_authored_prompts", json.loads((self.cache / "provenance.json").read_text()))

    def test_source_manifest_changed_during_copy_cannot_publish_mixed_bank(self):
        actual = prompts.shutil.copyfileobj
        def change(source, output, length):
            result = actual(source, output, length)
            path = self.root / prompts.SOURCE / "manifest.json"
            value = json.loads(path.read_text())
            value["note"] = "changed while importing"
            path.write_text(json.dumps(value))
            return result
        with patch.object(prompts.shutil, "copyfileobj", side_effect=change):
            with self.assertRaisesRegex(FormatError, "sources changed"):
                self.repair()
        self.assertFalse((self.bank / "manifest.json").exists())


if __name__ == "__main__":
    unittest.main()
