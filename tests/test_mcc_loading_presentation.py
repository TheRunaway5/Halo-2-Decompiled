import hashlib
import contextlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import mcc_loading_presentation as loading
from audit_rendering import FormatError


def string(value):
    raw = value.encode() + b"\0"
    return struct.pack("<i", len(raw)) + raw


def presentation_fixture(cache):
    """Small owned bank for transaction/routing tests, never a runtime asset."""
    bank = Path(cache) / loading.BANK
    bank.mkdir(parents=True, exist_ok=True)
    files = {}
    for name in ("glow-mask.dds", loading.TONE):
        data = ((Path(__file__).resolve().parents[1] / loading.AMBIENCE / loading.TONE).read_bytes()
                if name == loading.TONE else b"verified fixture")
        (bank / name).write_bytes(data)
        files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    (bank / "manifest.json").write_text(json.dumps(dict(schema=loading.SCHEMA,
        profile=loading.PROFILE, complete=True, files=files)))
    return loading.verify(Path(cache))


def v3_summary_fixture(cache):
    """Stub producer output for broad import transactions, never PCM admission."""
    bank = Path(cache) / loading.V3_BANK
    bank.mkdir(parents=True, exist_ok=True)
    (bank / "manifest.json").write_text('{"synthetic_transaction_fixture":true}')
    return dict(schema=loading.V3_SCHEMA, profile=loading.V3_PROFILE, complete=True,
        path=loading.V3_BANK + "/manifest.json", sha256=loading.file_sha(bank / "manifest.json"),
        mcc_loading_track=True, original_loading_tone=False, authored_frontend_timing=False,
        native_policy=loading.V3_POLICY)


class LoadingPresentationTests(unittest.TestCase):
    def setUp(self):
        self.owned = tempfile.TemporaryDirectory(prefix="halo2-mcc-loading-")
        self.addCleanup(self.owned.cleanup)
        self.root = Path(self.owned.name)

    def pak(self, *, damaged_payload=False, damaged_header=False, duplicate=False):
        payload = b"bounded actual media fixture"
        header = struct.pack("<qqqi20sBI", 0, len(payload), len(payload), 0,
                             hashlib.sha1(payload).digest(), 0, 0)
        if damaged_header:
            header = header[:8] + struct.pack("<q", 1) + header[16:]
        index = string("../../../") + struct.pack("<i", 2 if duplicate else 1)
        row = struct.pack("<qqqi20sBI", 0, len(payload), len(payload), 0,
                          hashlib.sha1(payload).digest(), 0, 0)
        index += (string(loading.MEDIA) + row) * (2 if duplicate else 1)
        offset = len(header) + len(payload)
        footer = b"\0" * 17 + struct.pack("<Iiqq20s", 0x5a6f12e1, 7, offset,
                                         len(index), hashlib.sha1(index).digest())
        pak = self.root / "MCC/Content/Paks/MCC-WindowsNoEditor.pak"
        pak.parent.mkdir(parents=True)
        pak.write_bytes(header + (b"x" + payload[1:] if damaged_payload else payload) + index + footer)
        return payload

    def test_exact_archive_owner_and_source_stability(self):
        payload = self.pak()
        with patch.object(loading, "MEDIA_SHA", hashlib.sha256(payload).hexdigest()):
            actual, owner = loading.read_media(self.root / "halo2")
        self.assertEqual(actual, payload)
        self.assertEqual(owner["path"], loading.MEDIA)

    def test_changed_payload_rejects(self):
        self.pak(damaged_payload=True)
        with self.assertRaisesRegex(FormatError, "checksum"):
            loading.read_media(self.root / "halo2")

    def test_changed_duplicate_header_rejects(self):
        self.pak(damaged_header=True)
        with self.assertRaisesRegex(FormatError, "header differs"):
            loading.read_media(self.root / "halo2")

    def test_duplicate_archive_filename_rejects(self):
        self.pak(duplicate=True)
        with self.assertRaisesRegex(FormatError, "Ambiguous"):
            loading.read_media(self.root / "halo2")

    def test_mask_uses_both_mcc_inputs_and_retains_required_role(self):
        glow = Image.new("RGBA", (64, 64), (90, 90, 90, 255))
        logo = Image.new("RGBA", (512, 64))
        before = loading.reconstruct_mask(glow, logo)
        logo.paste((255, 255, 255, 255), (100, 0, 200, 64))
        after = loading.reconstruct_mask(glow, logo)
        other = loading.reconstruct_mask(Image.new("RGBA", (64, 64)), logo)
        self.assertNotEqual(before, after)
        self.assertNotEqual(after, other)
        image = Image.open(io.BytesIO(after))
        self.assertEqual(image.size, (512, 512))
        self.assertEqual(image.getchannel("A").getextrema(), (255, 255))

    def test_incomplete_and_modified_bank_never_becomes_ready(self):
        bank = self.root / loading.BANK
        bank.mkdir(parents=True)
        manifest = dict(schema=loading.SCHEMA, profile=loading.PROFILE, complete=True, files={})
        (bank / "manifest.json").write_text(json.dumps(manifest))
        with self.assertRaisesRegex(FormatError, "missing a required role"):
            loading.verify(self.root)
        presentation_fixture(self.root)
        self.assertTrue(loading.verify(self.root)["complete"])
        (bank / loading.TONE).write_bytes(b"modified fixture")
        with self.assertRaisesRegex(FormatError, "existing files retained"):
            loading.verify(self.root)

    def test_preflight_uses_bundled_tone_without_original_install_or_converter(self):
        # Legacy/reference inputs may be linked into a frozen source cohort.
        # Admit copied regular fixture owners, never relax production _owned.
        reference = Path(__file__).resolve().parents[1]
        project = self.root / "reference-project"
        directory = project / loading.AMBIENCE
        directory.mkdir(parents=True)
        for name in ("manifest.json", loading.TONE):
            (directory / name).write_bytes((reference / loading.AMBIENCE / name).read_bytes())
        with patch.object(loading, "read_media", side_effect=AssertionError("MCC audio opened")), \
             patch.object(loading, "converter", side_effect=AssertionError("Vorbis converter required")):
            loading.preflight(project, self.root / "missing MCC installation")
        data, owner = loading.bundled_ambience(project)
        self.assertEqual(len(data), 18396)
        self.assertEqual(hashlib.sha256(data).hexdigest(), loading.TONE_SHA)
        self.assertEqual(owner["kind"], "port-bundled-original-loading-tone")

    def test_changed_bundled_tone_or_manifest_rejects(self):
        project = Path(__file__).resolve().parents[1]
        directory = self.root / loading.AMBIENCE
        directory.mkdir(parents=True)
        for name in ("manifest.json", loading.TONE):
            (directory / name).write_bytes((project / loading.AMBIENCE / name).read_bytes())
        payload = (directory / loading.TONE).read_bytes()
        (directory / loading.TONE).write_bytes(payload[:-1] + bytes([payload[-1] ^ 1]))
        with self.assertRaisesRegex(FormatError, "missing or changed"):
            loading.bundled_ambience(self.root)
        (directory / loading.TONE).write_bytes(payload)
        manifest = json.loads((directory / "manifest.json").read_text())
        manifest["playback_frequencies"][0] = 22050
        (directory / "manifest.json").write_text(json.dumps(manifest))
        with self.assertRaisesRegex(FormatError, "profile changed"):
            loading.bundled_ambience(self.root)

    def v3_inputs(self):
        """Synthetic transaction inputs; hashes are patched, never authentic-media evidence."""
        payload = self.pak()
        project, cache = self.root / "project", self.root / "cache"
        cache.mkdir()
        (cache / "bitmap-resources.json").write_text("{}")
        executable = project / "bin/halo2_mcc_loading_audio"
        executable.parent.mkdir(parents=True)
        executable.write_bytes(b"private decoder transaction fixture")
        executable.chmod(0o700)
        codebook = project / loading.CODEBOOK
        codebook.parent.mkdir(parents=True)
        codebook.write_bytes(b"private codebook transaction fixture")
        pcm = struct.pack("<ff", 1.5, -.375) * 32
        wav = (b"RIFF" + struct.pack("<I", len(pcm) + 36) + b"WAVEfmt " +
               struct.pack("<IHHIIHH", 16, 3, 2, 44100, 44100 * 8, 8, 32) +
               b"data" + struct.pack("<I", len(pcm)) + pcm)
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        for name, value in (("MEDIA_SHA", hashlib.sha256(payload).hexdigest()),
                            ("CODEBOOK_SHA", hashlib.sha256(codebook.read_bytes()).hexdigest()),
                            ("V3_PCM_SHA", hashlib.sha256(pcm).hexdigest()), ("V3_FRAMES", 32)):
            stack.enter_context(patch.object(loading, name, value))
        stack.enter_context(patch.object(loading, "bundled_ambience", side_effect=AssertionError("V3 touched legacy tone")))
        stack.enter_context(patch.object(loading, "_bitmap", return_value=(
            Image.new("RGBA", (64, 64), (90, 90, 90, 255)), dict(tag="synthetic transaction input"))))
        def convert(command, **options):
            self.assertEqual(command[0], str(executable))
            self.assertEqual(command[3:], [str(codebook), "32"])
            self.assertEqual(Path(command[1]).read_bytes(), payload)
            self.assertTrue(Path(command[2]).is_relative_to(cache))
            self.assertEqual(options["timeout"], 120)
            Path(command[2]).write_bytes(wav)
            return SimpleNamespace(returncode=0)
        decoder = stack.enter_context(patch.object(loading.subprocess, "run", side_effect=convert))
        return project, cache, self.root / "halo2", wav, decoder, convert

    def prepare_v3(self, project, cache, source):
        return loading.prepare_v3(project, cache, source, progress=lambda _: None)

    def assert_v3_unpublished(self, cache):
        self.assertFalse((cache / loading.V3_BANK).exists())
        self.assertFalse(list(cache.glob(".mcc-loading-v3-*")))

    def test_v3_explicit_dependency_admission_and_one_shot_bank_preserve_v2(self):
        project, cache, source, wav, decoder, _ = self.v3_inputs()
        legacy = cache / loading.BANK
        legacy.mkdir(parents=True)
        (legacy / "reference.txt").write_bytes(b"preserved legacy evidence")
        owner = loading.preflight_v3(project, source)
        self.assertEqual(owner["source"]["path"], loading.MEDIA)
        self.assertEqual(owner["source"]["archive"]["path"], loading.ARCHIVE)
        self.assertEqual(owner["codebook"]["sha256"], loading.CODEBOOK_SHA)
        decoder.assert_not_called()
        result = self.prepare_v3(project, cache, source)
        self.assertTrue(result["mcc_loading_track"])
        self.assertFalse(result["original_loading_tone"])
        self.assertFalse(result["authored_frontend_timing"])
        self.assertEqual(result["native_policy"], loading.V3_POLICY)
        self.assertEqual((cache / loading.V3_BANK / loading.V3_WAV).read_bytes(), wav)
        self.assertEqual((legacy / "reference.txt").read_bytes(), b"preserved legacy evidence")
        self.assertEqual(self.prepare_v3(project, cache, source), result)
        self.assertEqual(decoder.call_count, 1)
        self.assertFalse(list(cache.glob(".mcc-loading-v3-*")))

    def test_v3_missing_archive_and_changed_codebook_cannot_use_bundled_tone(self):
        project, cache, source, _, decoder, _ = self.v3_inputs()
        with self.assertRaisesRegex(FormatError, "missing"):
            loading.preflight_v3(project, self.root / "absent/halo2")
        (project / loading.CODEBOOK).write_bytes(b"changed codebook")
        with self.assertRaisesRegex(FormatError, "codebook differs"):
            self.prepare_v3(project, cache, source)
        decoder.assert_not_called()
        self.assert_v3_unpublished(cache)

    def test_v3_failed_or_cancelled_conversion_leaves_no_bank_or_temporary_media(self):
        project, cache, source, _, decoder, convert = self.v3_inputs()
        def failed(*args, **options):
            convert(*args, **options)
            return SimpleNamespace(returncode=9)
        decoder.side_effect = failed
        with self.assertRaisesRegex(FormatError, "converter failed"):
            self.prepare_v3(project, cache, source)
        self.assert_v3_unpublished(cache)
        def cancelled(*args, **options):
            convert(*args, **options)
            raise KeyboardInterrupt()
        decoder.side_effect = cancelled
        with self.assertRaises(KeyboardInterrupt):
            self.prepare_v3(project, cache, source)
        self.assert_v3_unpublished(cache)

    def test_v3_source_or_decoder_change_during_conversion_rejects_publication(self):
        project, cache, source, _, decoder, convert = self.v3_inputs()
        def changed_decoder(*args, **options):
            result = convert(*args, **options)
            Path(args[0][0]).write_bytes(b"changed executable")
            return result
        decoder.side_effect = changed_decoder
        with self.assertRaisesRegex(FormatError, "decoder changed"):
            self.prepare_v3(project, cache, source)
        self.assert_v3_unpublished(cache)
        def changed_archive(*args, **options):
            result = convert(*args, **options)
            with (self.root / loading.ARCHIVE).open("ab") as archive:
                archive.write(b"changed archive")
            return result
        decoder.side_effect = changed_archive
        with self.assertRaisesRegex(FormatError, "archive changed"):
            self.prepare_v3(project, cache, source)
        self.assert_v3_unpublished(cache)

    def test_v3_wrong_pcm_rejected_even_with_self_consistent_file_manifest(self):
        project, cache, source, wav, decoder, convert = self.v3_inputs()
        def wrong_pcm(*args, **options):
            result = convert(*args, **options)
            Path(args[0][2]).write_bytes(wav[:-1] + bytes([wav[-1] ^ 1]))
            return result
        decoder.side_effect = wrong_pcm
        with self.assertRaisesRegex(FormatError, "authenticated source profile"):
            self.prepare_v3(project, cache, source)
        self.assert_v3_unpublished(cache)
        decoder.side_effect = convert
        self.prepare_v3(project, cache, source)
        bank = cache / loading.V3_BANK
        changed = wav[:-1] + bytes([wav[-1] ^ 1])
        (bank / loading.V3_WAV).write_bytes(changed)
        manifest = json.loads((bank / "manifest.json").read_text())
        manifest["files"][loading.V3_WAV]["sha256"] = hashlib.sha256(changed).hexdigest()
        (bank / "manifest.json").write_text(json.dumps(manifest))
        with self.assertRaisesRegex(FormatError, "authenticated source profile"):
            loading.verify_v3(cache)

    def test_v3_linked_wave_and_legacy_substitution_are_rejected(self):
        project, cache, source, _, _, _ = self.v3_inputs()
        self.prepare_v3(project, cache, source)
        bank = cache / loading.V3_BANK
        wav = bank / loading.V3_WAV
        outside = self.root / "outside.wav"
        wav.rename(outside)
        wav.symlink_to(outside)
        with self.assertRaisesRegex(FormatError, "symlinks"):
            loading.verify_v3(cache)
        wav.unlink()
        outside.rename(wav)
        metadata = json.loads((bank / "manifest.json").read_text())
        metadata["audio"]["path"] = loading.TONE
        (bank / "manifest.json").write_text(json.dumps(metadata))
        with self.assertRaisesRegex(FormatError, "legacy ambience is not a fallback"):
            loading.verify_v3(cache)

    def test_v3_unknown_existing_bank_is_not_repaired_in_place(self):
        project, cache, source, _, decoder, _ = self.v3_inputs()
        bank = cache / loading.V3_BANK
        bank.mkdir(parents=True)
        original = b'{"schema":"custom bank"}'
        (bank / "manifest.json").write_bytes(original)
        with self.assertRaisesRegex(FormatError, "existing files retained"):
            self.prepare_v3(project, cache, source)
        self.assertEqual((bank / "manifest.json").read_bytes(), original)
        decoder.assert_not_called()

    def test_v3_repair_updates_only_current_source_and_reuses_bank_offline(self):
        project, cache, source, _, decoder, _ = self.v3_inputs()
        legacy = cache / loading.BANK
        legacy.mkdir(parents=True)
        (legacy / "reference.txt").write_bytes(b"retained legacy reference")
        value = dict(external_installation_requirement="Halo MCC only", source=str(source),
                     runtime_active=False, full_tu15_parity=False,
                     port_bundled_loading_ambience=dict(path=loading.BANK + "/" + loading.TONE))
        provenance = cache / "provenance.json"
        provenance.write_text(json.dumps(value))
        _, result = loading.repair_v3(project, cache, source, progress=lambda _: None)
        self.assertEqual(result["native_loading_presentation"]["profile"], loading.V3_PROFILE)
        self.assertNotIn("port_bundled_loading_ambience", result)
        self.assertEqual(result["legacy_loading_ambience_reference"], value["port_bundled_loading_ambience"])
        self.assertFalse(result["runtime_active"])
        self.assertFalse(result["full_tu15_parity"])
        self.assertEqual((legacy / "reference.txt").read_bytes(), b"retained legacy reference")
        before = provenance.read_bytes()
        with patch.object(loading, "converter_dependencies", side_effect=AssertionError("offline reuse opened decoder")):
            self.assertEqual(loading.repair_v3(project, cache, self.root / "absent/halo2")[1], result)
        self.assertEqual(provenance.read_bytes(), before)
        self.assertEqual(decoder.call_count, 1)

    def test_v3_provenance_failure_retains_unselected_bank_and_old_provenance(self):
        project, cache, source, _, _, _ = self.v3_inputs()
        provenance = cache / "provenance.json"
        before = json.dumps(dict(external_installation_requirement="Halo MCC only", source=str(source))).encode()
        provenance.write_bytes(before)
        with patch("mcc_ui_layout._atomic", side_effect=OSError("publication failed")):
            with self.assertRaisesRegex(OSError, "publication failed"):
                loading.repair_v3(project, cache, source, progress=lambda _: None)
        self.assertEqual(provenance.read_bytes(), before)
        self.assertTrue(loading.verify_v3(cache)["mcc_loading_track"])
        self.assertFalse(list(cache.glob(".mcc-loading-v3-*")))

    def test_v3_provenance_race_is_not_overwritten(self):
        project, cache, source, _, decoder, convert = self.v3_inputs()
        provenance = cache / "provenance.json"
        provenance.write_text(json.dumps(dict(external_installation_requirement="Halo MCC only", source=str(source))))
        changed = b'{"concurrent_owner":"preserved"}'
        def raced(*args, **kwargs):
            result = convert(*args, **kwargs)
            provenance.write_bytes(changed)
            return result
        decoder.side_effect = raced
        with self.assertRaisesRegex(FormatError, "retained unselected"):
            loading.repair_v3(project, cache, source, progress=lambda _: None)
        self.assertEqual(provenance.read_bytes(), changed)
        self.assertTrue(loading.verify_v3(cache)["complete"])

    def packaged_inputs(self):
        project, cache, source, wav, decoder, convert = self.v3_inputs()
        (project / loading.CONVERTER_WRAPPER).write_bytes(loading.converter_wrapper())
        for name in (loading.CONVERTER_PAYLOAD, loading.CONVERTER_LOADER, "lib/libvorbis.so.0"):
            path = project / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(("synthetic owned dependency " + name).encode())
            path.chmod(0o700)
        names = (loading.CONVERTER_WRAPPER, loading.CONVERTER_PAYLOAD, loading.CONVERTER_LOADER,
                 "lib/libvorbis.so.0", loading.CODEBOOK)
        manifest = project / loading.CONVERTER_MANIFEST
        manifest.parent.mkdir(parents=True)
        manifest.write_text(json.dumps(dict(schema=loading.CONVERTER_SCHEMA,
            files={name: dict(bytes=(project / name).stat().st_size,
                              sha256=loading.file_sha(project / name)) for name in names})))
        def packaged_convert(command, **options):
            self.assertEqual(command[:4], [str(project / loading.CONVERTER_LOADER), "--library-path",
                                          str(project / "lib"), str(project / loading.CONVERTER_PAYLOAD)])
            self.assertFalse(any(k.startswith(("LD_", "DYLD_")) for k in options["env"]))
            return convert([str(project / loading.CONVERTER_WRAPPER)] + command[4:], **options)
        decoder.side_effect = packaged_convert
        return project, cache, source, decoder, packaged_convert

    def test_packaged_decoder_pins_complete_runtime_and_uses_owned_loader(self):
        project, cache, source, _, _ = self.packaged_inputs()
        owner = loading.converter_dependencies(project)
        self.assertIn(loading.CONVERTER_PAYLOAD, owner["converter"]["runtime"]["files"])
        with patch.dict(loading.os.environ, {"LD_PRELOAD": "untrusted.so", "DYLD_INSERT_LIBRARIES": "foreign.dylib"}):
            self.prepare_v3(project, cache, source)
        self.assertTrue(loading.verify_v3(cache)["mcc_loading_track"])

    def test_packaged_decoder_missing_manifest_changed_owner_and_foreign_library_reject(self):
        project, _, _, _, _ = self.packaged_inputs()
        manifest = project / loading.CONVERTER_MANIFEST
        original = manifest.read_bytes()
        manifest.unlink()
        with self.assertRaisesRegex(FormatError, "runtime manifest"):
            loading.converter_dependencies(project)
        manifest.write_bytes(original)
        for name in (loading.CONVERTER_PAYLOAD, loading.CONVERTER_LOADER, "lib/libvorbis.so.0"):
            with self.subTest(name=name):
                path = project / name
                before = path.read_bytes()
                path.write_bytes(before + b"changed")
                with self.assertRaisesRegex(FormatError, "dependency changed"):
                    loading.converter_dependencies(project)
                path.write_bytes(before)
        (project / "lib/foreign.so").write_bytes(b"unqualified library")
        with self.assertRaisesRegex(FormatError, "inventory differs"):
            loading.converter_dependencies(project)

    def test_packaged_decoder_runtime_change_during_conversion_cannot_publish(self):
        project, cache, source, decoder, convert = self.packaged_inputs()
        def changed(*args, **kwargs):
            result = convert(*args, **kwargs)
            path = project / loading.CONVERTER_PAYLOAD
            path.write_bytes(path.read_bytes() + b"changed during conversion")
            return result
        decoder.side_effect = changed
        with self.assertRaisesRegex(FormatError, "dependency changed"):
            self.prepare_v3(project, cache, source)
        self.assert_v3_unpublished(cache)

    def test_packaged_decoder_paths_and_manifest_drift_are_not_metadata_only(self):
        project, cache, source, decoder, convert = self.packaged_inputs()
        manifest = project / loading.CONVERTER_MANIFEST
        original = manifest.read_bytes()
        value = json.loads(original)
        value["files"]["lib/../outside"] = dict(bytes=1, sha256="0" * 64)
        manifest.write_text(json.dumps(value))
        with self.assertRaisesRegex(FormatError, "converter path"):
            loading.converter_dependencies(project)
        manifest.write_bytes(original)
        def changed(*args, **kwargs):
            result = convert(*args, **kwargs)
            manifest.write_bytes(original + b" ")
            return result
        decoder.side_effect = changed
        with self.assertRaisesRegex(FormatError, "decoder changed"):
            self.prepare_v3(project, cache, source)
        self.assert_v3_unpublished(cache)


if __name__ == "__main__":
    unittest.main()
