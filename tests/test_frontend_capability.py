"""Synthetic admission controls; these do not establish a rendered frontend."""
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from native_gameplay_test import (FRONTEND_CAPABILITY, FRONTEND_MAP, FRONTEND_SCHEMA,
                                 FRONTEND_SCOPE, frontend_runtime_available,
                                 native_gameplay_test_available)


class FrontendCapabilityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="halo2-frontend-capability-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.cache = self.root / "assets/imported/mcc-test"
        resources = [FRONTEND_MAP, "halo2/h2_maps_win64_dx11/shared.map",
                     "halo2/h2_maps_win64_dx11/textures.dat",
                     "halo2/h2_maps_win64_dx11/sounds_neutral.dat",
                     "halo2/h2_fonts/font_table.txt", "halo2/h2_fonts/fixture.font"]
        resources += ["presentation/prompts/" + name for name in
                      ("manifest.json", "prompt-source.tsv", "prompts.tsv")]
        resources += [f"presentation/prompts/atlases/{index}.dds" for index in range(6)]
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
                           validation=dict(passed=True, scope=FRONTEND_SCOPE, report_sha256="1" * 64))
        self.publish()

    @staticmethod
    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def publish(self):
        self.sidecar.write_text(json.dumps(self.report))

    def available(self):
        return frontend_runtime_available(self.root, self.runtime, self.cache)

    def test_frontend_admission_does_not_grant_native_gameplay(self):
        self.assertTrue(self.available())
        self.assertFalse(native_gameplay_test_available(self.root, self.runtime, self.cache))

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

    def test_custom_prompt_bank_is_required_and_its_inventory_is_exact(self):
        name = "presentation/prompts/atlases/0.dds"
        digest = self.report["resources"].pop(name)
        self.publish()
        self.assertFalse(self.available())
        self.report["resources"][name] = digest
        self.publish()
        self.assertTrue(self.available())
        path = self.cache / name
        original = path.read_bytes()
        path.write_bytes(original + b"changed prompt pixels")
        self.assertFalse(self.available())
        path.write_bytes(original)
        self.assertTrue(self.available())
        extra = self.cache / "presentation/prompts/atlases/unqualified.dds"
        extra.write_bytes(b"not in the validation inventory")
        self.assertFalse(self.available())
        extra.unlink()
        path.unlink()
        self.assertFalse(self.available())

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
