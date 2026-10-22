"""Synthetic admission controls; these do not establish a rendered frontend."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from native_gameplay_test import (FRONTEND_CAPABILITY, FRONTEND_MAP, FRONTEND_SCHEMA,
                                 FRONTEND_SCOPE, frontend_runtime_available,
                                 native_gameplay_test_available, _check,
                                 _frontend_contract, _runtime_owner)
from package_mcc_test_build import qualified_frontend, packaged_frontend
from packaged_shader_defaults import native_runtime_wrapper, relocate_compiled_shader_default
from packaged_shader_test_support import elf_with_readonly_literal


class FrontendCapabilityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="halo2-frontend-capability-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.cache = self.root / "assets/imported/mcc-test"
        resources = [FRONTEND_MAP, "halo2/h2_maps_win64_dx11/shared.map",
                     "halo2/h2_maps_win64_dx11/textures.dat",
                     "halo2/h2_maps_win64_dx11/sounds_neutral.dat",
                     "halo2/h2_fonts/font_table.txt", "halo2/h2_fonts/fixture.font",
                     "halo2/h2_fonts/nonactive-language/fixture.font"]
        resources += ["presentation/loading-v3/" + name for name in
                      ("manifest.json", "glow-mask.dds", "loading.wav")]
        for name in resources:
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(name.encode())
        self.runtime = self.root / "build/halo2_native"
        self.shaders = self.runtime.parent / "shaders"
        self.shaders.mkdir(parents=True)
        names = ["world.vert.spv", "world.frag.spv", "hud.frag.spv"] + [f"fixture-{i}.spv" for i in range(94)]
        for name in names:
            (self.shaders / name).write_bytes(name.encode())
        self.runtime.write_bytes(b"synthetic owner\0" + os.fsencode(self.shaders) + b"\0")
        self.runtime.chmod(0o700)
        self.sidecar = self.runtime.with_name(self.runtime.name + ".frontend-test.json")
        self.report = dict(schema=FRONTEND_SCHEMA, capability=FRONTEND_CAPABILITY,
                           runtime=dict(path=str(self.runtime), sha256=self.digest(self.runtime)),
                           shaders=dict(binding="compiled-default", directory=str(self.shaders),
                                        files={name: self.digest(self.shaders / name) for name in names}),
                           map=dict(path=FRONTEND_MAP, sha256=self.digest(self.cache / FRONTEND_MAP)),
                           resources={name: self.digest(self.cache / name) for name in resources[1:]},
                           validation=dict(passed=True, scope=FRONTEND_SCOPE, report_sha256="1" * 64,
                               loading_presentation=True, original_loading_ambience=False,
                               mcc_loading_presentation_profile="mcc-logo-glow-mcc-track-oneshot-v1",
                               mcc_prompt_font_profile="mcc-native-control-fonts-v1"))
        self.publish()

    @staticmethod
    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def publish(self):
        self.sidecar.write_text(json.dumps(self.report))

    def available(self):
        return frontend_runtime_available(self.root, self.runtime, self.cache)

    def test_fresh_native_font_profile_admits_without_authored_prompt_bank(self):
        self.assertFalse((self.cache / "presentation/prompts").exists())
        before = self.sidecar.read_bytes()
        _check(self.root, self.runtime, self.cache, frontend=True)
        self.assertTrue(self.available())
        self.assertEqual(self.sidecar.read_bytes(), before)

    def test_converted_native_ui_asset_is_exactly_pinned(self):
        name = "presentation/ui/native-layout.json"
        asset = self.cache/name
        asset.parent.mkdir(parents=True)
        asset.write_bytes(b'converted logical UI asset')
        self.report['resources'][name] = self.digest(asset)
        self.publish();self.assertTrue(self.available())
        asset.write_bytes(b'changed converted asset')
        self.assertFalse(self.available())
        asset.write_bytes(b'converted logical UI asset')
        other = self.cache/'presentation/ui/unqualified.json'
        other.write_bytes(b'unknown presentation owner')
        self.report['resources'][other.relative_to(self.cache).as_posix()] = self.digest(other)
        self.publish();self.assertFalse(self.available())

    def test_frontend_admission_does_not_grant_native_gameplay(self):
        self.assertTrue(self.available())
        self.assertFalse(native_gameplay_test_available(self.root, self.runtime, self.cache))

    def test_packager_preserves_passed_scope_in_a_relocated_portable_owner(self):
        # Synthetic metadata admission only; no rendered-game claim is created.
        qualified = qualified_frontend(self.runtime, self.cache)
        stage = self.root / "portable"
        cache = stage / "assets/imported/mcc-test"
        shutil.copytree(self.cache, cache)
        shutil.copytree(self.shaders, stage / "bin/shaders")
        runtime = stage / "bin/halo2_native"
        runtime.write_bytes(native_runtime_wrapper());runtime.chmod(0o700)
        loader = stage / "lib/ld-linux-x86-64.so.2"
        loader.parent.mkdir();loader.write_bytes(b"synthetic owned loader");loader.chmod(0o700)
        payload, relocation = relocate_compiled_shader_default(
            elf_with_readonly_literal(os.fsencode(self.shaders)), self.shaders)
        binary = stage / "bin/halo2_native.real"
        binary.write_bytes(payload);binary.chmod(0o700)
        result = packaged_frontend(stage, qualified, relocation)
        self.assertEqual(result["path"], "bin/halo2_native.frontend-test.json")
        report = json.loads((stage / result["path"]).read_text())
        self.assertEqual(report["validation"], self.report["validation"])
        self.assertEqual(report["resources"], self.report["resources"])
        self.assertTrue(frontend_runtime_available(stage, runtime, cache))
        self.assertFalse(native_gameplay_test_available(stage, runtime, cache))
        for changed in (binary, loader, runtime, cache / FRONTEND_MAP):
            with self.subTest(changed=changed.name):
                original = changed.read_bytes();changed.write_bytes(original + b"changed")
                self.assertFalse(frontend_runtime_available(stage, runtime, cache))
                changed.write_bytes(original)
                self.assertTrue(frontend_runtime_available(stage, runtime, cache))

    def test_packager_rejects_unqualified_or_changed_source_before_rebinding(self):
        self.sidecar.unlink()
        with self.assertRaises(ValueError):qualified_frontend(self.runtime, self.cache)
        self.publish();self.runtime.write_bytes(self.runtime.read_bytes() + b"changed")
        with self.assertRaises(ValueError):qualified_frontend(self.runtime, self.cache)

    def test_loading_capability_requires_all_three_exact_mcc_v3_assets(self):
        self.assertTrue(self.available())
        for name in ("manifest.json", "glow-mask.dds", "loading.wav"):
            with self.subTest(resource=name):
                relative = "presentation/loading-v3/" + name
                path = self.cache / relative
                owner = self.report["resources"].pop(relative)
                self.publish();self.assertFalse(self.available())
                self.report["resources"][relative] = owner
                self.publish();self.assertTrue(self.available())
                original = path.read_bytes()
                path.write_bytes(original + b"changed")
                self.assertFalse(self.available())
                path.write_bytes(original)
                self.assertTrue(self.available())

    def test_legacy_or_missing_loading_profile_cannot_upgrade_frontend_admission(self):
        for profile in (None, "mcc-logo-glow-original-tone-v1", "legacy-converted-vista"):
            with self.subTest(profile=profile):
                if profile is None:
                    self.report["validation"].pop("mcc_loading_presentation_profile")
                else:
                    self.report["validation"]["mcc_loading_presentation_profile"] = profile
                self.publish();self.assertFalse(self.available())

    def test_original_tone_claim_is_rejected_even_with_exact_v3_resources(self):
        self.assertTrue(self.available())
        self.report["validation"]["original_loading_ambience"] = True
        self.publish();self.assertFalse(self.available())
        self.report["validation"]["original_loading_ambience"] = False
        self.publish();self.assertTrue(self.available())

    def authored_presentation_family(self):
        """Retained synthetic legacy proof and assets for diagnostic controls."""
        self.report['validation'].pop('mcc_loading_presentation_profile')
        self.report['validation'].pop('mcc_prompt_font_profile')
        self.report['validation']['original_loading_ambience'] = True
        for name in list(self.report['resources']):
            if name.startswith('presentation/loading-v3/'):
                self.report['resources'].pop(name)
        names = ['presentation/prompts/' + name for name in
                 ('manifest.json', 'prompt-source.tsv', 'prompts.tsv')]
        names += [f'presentation/prompts/atlases/{index}.dds' for index in range(6)]
        names += ['presentation/loading-v2/' + name for name in
                  ('manifest.json', 'glow-mask.dds', 'loading-tone.xbox-adpcm')]
        for name in names:
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(name.encode())
            self.report['resources'][name] = self.digest(path)
        self.publish()
        return names

    def test_authenticated_legacy_family_requires_mcc_runtime_update_with_exact_donors(self):
        names = self.authored_presentation_family()
        # Authenticate the real old executable/shader/capability contract first;
        # refusal must not be an accidentally absent proof or donor resource.
        owner = _runtime_owner(self.root, self.runtime, frontend=True)
        self.assertEqual(owner['report'], self.report)
        self.assertFalse(_frontend_contract(owner['report'])['native_fonts'])
        for name in names:
            self.assertEqual(self.digest(self.cache / name), self.report['resources'][name])
        files = ([self.runtime, self.sidecar] + list(self.shaders.iterdir()) +
                 [self.cache / name for name in self.report['resources']] +
                 [self.cache / FRONTEND_MAP])
        before = {p: (self.digest(p), p.stat().st_mtime_ns, p.stat().st_ctime_ns) for p in files}
        with self.assertRaisesRegex(ValueError, 'Update.*MCC-capable runtime'):
            _check(self.root, self.runtime, self.cache, frontend=True)
        self.assertFalse(self.available())
        with self.assertRaisesRegex(ValueError, 'Update.*MCC-capable runtime'):
            qualified_frontend(self.runtime, self.cache)
        self.assertEqual(before, {p: (self.digest(p), p.stat().st_mtime_ns, p.stat().st_ctime_ns)
                                  for p in files})
        self.assertNotIn('mcc_loading_presentation_profile', self.report['validation'])
        self.assertNotIn('mcc_prompt_font_profile', self.report['validation'])
        self.assertFalse(native_gameplay_test_available(self.root, self.runtime, self.cache))

    def test_legacy_donor_removal_or_tamper_cannot_restore_frontend_admission(self):
        names = self.authored_presentation_family()
        self.assertEqual(_runtime_owner(self.root, self.runtime, frontend=True)['report'], self.report)
        for name in names:
            with self.subTest(resource=name):
                digest = self.report['resources'].pop(name)
                self.publish()
                with self.assertRaisesRegex(ValueError, 'Update.*MCC-capable runtime'):
                    _check(self.root, self.runtime, self.cache, frontend=True)
                self.report['resources'][name] = digest
                self.publish();self.assertFalse(self.available())
                path = self.cache / name
                before = path.read_bytes()
                path.write_bytes(before + b'changed')
                with self.assertRaisesRegex(ValueError, 'Update.*MCC-capable runtime'):
                    _check(self.root, self.runtime, self.cache, frontend=True)
                path.write_bytes(before)
                self.assertFalse(self.available())

    def test_legacy_family_cannot_mix_in_a_partial_or_full_mcc_claim(self):
        self.authored_presentation_family()
        self.assertEqual(_runtime_owner(self.root, self.runtime, frontend=True)['report'], self.report)
        self.assertFalse(self.available())
        for fields in (
                {'mcc_loading_presentation_profile': None},
                {'mcc_prompt_font_profile': None},
                {'mcc_loading_presentation_profile': 'mcc-logo-glow-mcc-track-oneshot-v1',
                 'mcc_prompt_font_profile': 'mcc-native-control-fonts-v1'}):
            with self.subTest(fields=fields):
                self.report['validation'].update(fields)
                self.publish();self.assertFalse(self.available())
                for name in fields:
                    self.report['validation'].pop(name)
                self.publish();self.assertFalse(self.available())

    def test_v3_profile_cannot_authorize_legacy_loading_resource_paths(self):
        relative = "presentation/loading-v3/loading.wav"
        owner = self.report["resources"][relative]
        legacy = "presentation/loading/loading.wav"
        path = self.cache / legacy
        path.parent.mkdir(parents=True)
        path.write_bytes((self.cache / relative).read_bytes())
        self.report["resources"][legacy] = owner
        self.publish();self.assertFalse(self.available())
        self.report["resources"].pop(legacy)
        self.publish();self.assertTrue(self.available())

    def test_mutated_runtime_shader_or_imported_resource_expires_admission(self):
        for path in [self.runtime, self.shaders / "hud.frag.spv", self.cache / FRONTEND_MAP,
                     self.cache / "halo2/h2_maps_win64_dx11/shared.map",
                     self.cache / "halo2/h2_maps_win64_dx11/textures.dat",
                     self.cache / "halo2/h2_maps_win64_dx11/sounds_neutral.dat",
                     self.cache / "halo2/h2_fonts/fixture.font"]:
            with self.subTest(path=path):
                old = path.read_bytes()
                path.write_bytes(old + b"changed")
                self.assertFalse(self.available())
                path.write_bytes(old)
                self.assertTrue(self.available())

    def test_frontend_audio_bank_must_exist_and_match_the_receipt(self):
        name = "halo2/h2_maps_win64_dx11/sounds_neutral.dat"
        digest = self.report["resources"].pop(name)
        self.publish()
        self.assertFalse(self.available())
        self.report["resources"][name] = digest
        self.publish()
        self.assertTrue(self.available())
        (self.cache / name).unlink()
        self.assertFalse(self.available())

    def test_absent_or_foreign_prompt_font_profile_cannot_upgrade_old_proof(self):
        self.assertTrue(self.available())
        for profile in (None, False, "mcc-native-control-fonts-v0", "legacy-authored-prompts", "custom-art"):
            with self.subTest(profile=profile):
                if profile is None:
                    self.report["validation"].pop("mcc_prompt_font_profile")
                else:
                    self.report["validation"]["mcc_prompt_font_profile"] = profile
                self.publish()
                before = self.sidecar.read_bytes()
                with self.assertRaisesRegex(ValueError, "native MCC prompt font profile"):
                    _check(self.root, self.runtime, self.cache, frontend=True)
                self.assertFalse(self.available())
                self.assertEqual(self.sidecar.read_bytes(), before)

    def test_claimed_legacy_prompt_resource_is_rejected_even_with_matching_hash(self):
        for name in ("manifest.json", "prompt-source.tsv", "prompts.tsv", "atlases/0.dds"):
            with self.subTest(resource=name):
                path = self.cache / "presentation/prompts" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"retained reference-only prompt owner")
                relative = path.relative_to(self.cache).as_posix()
                self.report["resources"][relative] = self.digest(path)
                self.publish()
                with self.assertRaisesRegex(ValueError, "resource escaped"):
                    _check(self.root, self.runtime, self.cache, frontend=True)
                self.assertFalse(self.available())
                self.report["resources"].pop(relative)
                self.publish()
                self.assertTrue(self.available())

    def test_unclaimed_legacy_prompt_files_are_ignored_and_preserved(self):
        legacy = self.cache / "presentation/prompts"
        files = {"manifest.json": b"malformed retained manifest", "prompts.tsv": b"old selectors",
                 "atlases/0.dds": b"retained prompt pixels"}
        for name, data in files.items():
            path = legacy / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        before = {p: (p.read_bytes(), p.stat().st_mtime_ns, p.stat().st_ctime_ns)
                  for p in legacy.rglob("*") if p.is_file()}
        sidecar = self.sidecar.read_bytes()
        _check(self.root, self.runtime, self.cache, frontend=True)
        self.assertTrue(self.available())
        self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_mtime_ns, p.stat().st_ctime_ns)
                                  for p in before})
        self.assertEqual(self.sidecar.read_bytes(), sidecar)

    def test_complete_native_font_tree_includes_nonactive_languages(self):
        for name in ("font_table.txt", "fixture.font", "nonactive-language/fixture.font"):
            with self.subTest(font=name):
                relative = "halo2/h2_fonts/" + name
                path = self.cache / relative
                expected = self.report["resources"].pop(relative)
                self.publish()
                with self.assertRaisesRegex(ValueError, "omits MCC resources"):
                    _check(self.root, self.runtime, self.cache, frontend=True)
                self.report["resources"][relative] = expected
                self.publish()
                original = path.read_bytes()
                path.write_bytes(original + b"changed native font bytes")
                with self.assertRaisesRegex(ValueError, "MCC resource changed"):
                    _check(self.root, self.runtime, self.cache, frontend=True)
                path.write_bytes(original)
                self.assertTrue(self.available())

    def test_missing_font_unknown_font_or_foreign_resource_is_rejected(self):
        name = "halo2/h2_fonts/fixture.font"
        old = self.report["resources"].pop(name)
        self.publish()
        self.assertFalse(self.available())
        self.report["resources"][name] = old
        extra = self.cache / "halo2/h2_fonts/another.font"
        extra.write_bytes(b"unqualified font")
        self.publish()
        self.assertFalse(self.available())
        extra.unlink()
        self.report["resources"]["../../outside"] = "1" * 64
        self.publish()
        self.assertFalse(self.available())

    def test_different_scope_and_non_owned_cache_are_rejected(self):
        self.report["validation"]["scope"] = "mcc-lockout-native-gameplay-test"
        self.publish()
        self.assertFalse(self.available())
        self.assertFalse(frontend_runtime_available(self.root, self.runtime, self.root / "vista"))
        self.assertFalse(frontend_runtime_available(self.root, None, self.cache))

    def test_optional_ui_pack_must_be_covered_by_the_frontend_receipt(self):
        ui = self.cache / "ui"
        ui.mkdir()
        (ui / "pack.tsv").write_bytes(b"synthetic custom UI manifest")
        (ui / "custom.dds").write_bytes(b"owned custom image")
        self.assertFalse(self.available())
        for path in ui.iterdir():
            self.report["resources"][path.relative_to(self.cache).as_posix()] = self.digest(path)
        self.publish()
        self.assertTrue(self.available())
        (ui / "custom.dds").write_bytes(b"changed image")
        self.assertFalse(self.available())


if __name__ == "__main__":
    unittest.main()
