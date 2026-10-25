"""Capability parsing tests use synthetic owners, never claim gameplay parity."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from native_gameplay_test import CAPABILITY, MAP, SCHEMA, SCOPE, native_gameplay_test_available
from packaged_shader_defaults import native_runtime_wrapper, relocate_compiled_shader_default
from packaged_shader_test_support import elf_with_readonly_literal


class NativeGameplayTestCapabilityTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="halo2-native-test-capability-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.cache = self.root / "assets/imported/mcc-test"
        self.map = self.cache / MAP
        self.map.parent.mkdir(parents=True)
        self.map.write_bytes(b"synthetic MCC Lockout owner")
        self.runtime = self.root / "build/halo2_native"
        self.shaders = self.runtime.parent / "shaders"
        self.shaders.mkdir(parents=True)
        names = ["world.vert.spv", "world.frag.spv", "hud.frag.spv"] + [f"fixture-{i}.spv" for i in range(94)]
        for name in names:
            (self.shaders / name).write_bytes(name.encode())
        self.runtime.write_bytes(b"synthetic executable owner\0" + os.fsencode(self.shaders) + b"\0")
        self.runtime.chmod(0o700)
        self.sidecar = self.runtime.with_name(self.runtime.name + ".native-test.json")
        self.report = dict(schema=SCHEMA, capability=CAPABILITY,
                           runtime=dict(path=str(self.runtime), sha256=self.digest(self.runtime)),
                           shaders=dict(binding="compiled-default", directory=str(self.shaders),
                                        files={name: self.digest(self.shaders / name) for name in names}),
                           map=dict(path=MAP, sha256=self.digest(self.map)),
                           validation=dict(passed=True, scope=SCOPE, report_sha256="1" * 64))

    @staticmethod
    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def publish(self, report=None):
        self.sidecar.write_text(json.dumps(self.report if report is None else report))

    def available(self):
        return native_gameplay_test_available(self.root, self.runtime, self.cache)

    def test_declared_sdk_worker_must_keep_its_owned_executable_bytes(self):
        from native_gameplay_test import eos_worker_descriptor
        worker = self.runtime.with_name('eos_sdk_worker')
        worker.write_bytes(b'synthetic SDK worker')
        worker.chmod(0o700)
        self.report['eos_worker'] = eos_worker_descriptor(self.runtime)
        self.publish()
        self.assertTrue(self.available())
        worker.write_bytes(b'changed SDK worker')
        self.assertFalse(self.available())
        worker.write_bytes(b'synthetic SDK worker')
        worker.chmod(0o600)
        self.assertFalse(self.available())
        worker.chmod(0o700)
        foreign = worker.with_name('foreign-worker')
        worker.rename(foreign)
        worker.symlink_to(foreign)
        self.assertFalse(self.available())
        worker.unlink()
        self.assertFalse(self.available())

    def test_expanded_renderer_inventory_requires_every_added_file_to_be_pinned(self):
        self.publish()
        extra=self.shaders/'pstencil/normal_accumulation.frag.spv'
        extra.parent.mkdir();extra.write_bytes(b'new qualified renderer program')
        self.assertFalse(self.available())
        self.report['shaders']['files'][extra.relative_to(self.shaders).as_posix()]=self.digest(extra)
        self.publish()
        self.assertTrue(self.available())
        extra.write_bytes(b'unvalidated program change')
        self.assertFalse(self.available())

    def test_native_shader_family_requires_its_declared_complete_programs(self):
        from native_renderer_shaders import PROFILE, REQUIRED_FILES
        shutil.rmtree(self.shaders)
        self.shaders.mkdir()
        for name in REQUIRED_FILES:
            path=self.shaders/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(name.encode())
        self.report['shaders']['files']={name:self.digest(self.shaders/name) for name in REQUIRED_FILES}
        self.publish();self.assertFalse(self.available()) # Legacy minimum remains unchanged.
        self.report['shaders']['profile']=PROFILE;self.publish();self.assertTrue(self.available())
        self.report['shaders']['profile']='unknown-family';self.publish();self.assertFalse(self.available())
        self.report['shaders']['profile']=PROFILE
        name='pstencil/normal_accumulation.frag.spv'
        (self.shaders/name).unlink();self.report['shaders']['files'].pop(name)
        self.publish();self.assertFalse(self.available())

    def packaged_capability(self):
        original = os.fsencode(self.shaders)
        payload, relocation = relocate_compiled_shader_default(elf_with_readonly_literal(original), self.shaders)
        directory = self.root / 'bin'
        directory.mkdir()
        shutil.copytree(self.shaders, directory / 'shaders')
        self.runtime = directory / 'halo2_native'
        self.runtime.write_bytes(native_runtime_wrapper())
        self.runtime.chmod(0o700)
        self.payload = directory / 'halo2_native.real'
        self.payload.write_bytes(payload)
        self.payload.chmod(0o700)
        self.loader = self.root / 'lib/ld-linux-x86-64.so.2'
        self.loader.parent.mkdir()
        self.loader.write_bytes(b'synthetic loader owner; not executed')
        self.loader.chmod(0o700)
        self.sidecar = self.runtime.with_name(self.runtime.name + '.native-test.json')
        self.report['runtime'] = dict(path='bin/halo2_native', sha256=self.digest(self.runtime),
                                      payload=dict(path='bin/halo2_native.real', sha256=self.digest(self.payload)),
                                      loader=dict(path='lib/ld-linux-x86-64.so.2', sha256=self.digest(self.loader)))
        self.report['shaders'].update(binding='packaged-compiled-default', directory='bin/shaders',
                                      compiled_default=relocation['compiled_default'])
        self.publish()

    def test_complete_packaged_binding_remains_valid_after_directory_relocation(self):
        self.packaged_capability()
        self.assertTrue(self.available())
        moved = self.root.with_name(self.root.name + ' moved package')
        self.addCleanup(shutil.rmtree, moved, True)
        shutil.copytree(self.root, moved)
        self.assertTrue(native_gameplay_test_available(moved, moved/'bin/halo2_native', moved/'assets/imported/mcc-test'))
        (moved / 'bin/shaders/hud.frag.spv').write_bytes(b'changed')
        self.assertFalse(native_gameplay_test_available(moved, moved/'bin/halo2_native', moved/'assets/imported/mcc-test'))

    def test_packaged_wrapper_must_set_proven_cwd_even_with_an_updated_hash(self):
        self.packaged_capability()
        self.runtime.write_bytes(self.runtime.read_bytes().replace(b'cd "$root"', b'cd /'))
        self.report['runtime']['sha256'] = self.digest(self.runtime)
        self.publish()
        self.assertFalse(self.available())

    def test_packaged_stale_payload_loader_and_aliases_reject(self):
        self.packaged_capability()
        for path in (self.payload, self.loader):
            with self.subTest(path=path):
                before = path.read_bytes()
                path.write_bytes(before+b'changed')
                self.assertFalse(self.available())
                path.write_bytes(before)
        self.assertTrue(self.available())
        external = self.root / 'foreign-payload'
        shutil.copy2(self.payload, external)
        self.payload.unlink()
        self.payload.symlink_to(external)
        self.assertFalse(self.available())

    def test_packaged_literal_owner_and_paths_cannot_be_forged(self):
        self.packaged_capability()
        before = self.payload.read_bytes()
        literal = self.report['shaders']['compiled_default'].encode()
        self.payload.write_bytes(elf_with_readonly_literal(literal, flags=6))
        self.report['runtime']['payload']['sha256'] = self.digest(self.payload)
        self.publish()
        self.assertFalse(self.available())
        self.payload.write_bytes(before)
        self.report['runtime']['payload']['sha256'] = self.digest(self.payload)
        for target, field, value in (('shaders', 'compiled_default', '../bin/shaders'),
                                     ('shaders', 'directory', '../bin/shaders'),
                                     ('runtime', 'path', '/bin/halo2_native')):
            report = copy.deepcopy(self.report)
            report[target][field] = value
            self.publish(report)
            self.assertFalse(self.available())

    def test_missing_sidecar_preserves_preview_availability_only(self):
        self.assertFalse(self.available())
        self.assertFalse(self.sidecar.exists())

    def test_exact_validated_owner_is_available_without_mutating_parity(self):
        provenance = self.cache / "provenance.json"
        provenance.write_text(json.dumps(dict(runtime_active=False, full_tu15_parity=False)))
        self.publish()
        before = provenance.read_bytes(), self.sidecar.read_bytes(), self.runtime.read_bytes()
        self.assertTrue(self.available())
        self.assertEqual(before, (provenance.read_bytes(), self.sidecar.read_bytes(), self.runtime.read_bytes()))

    def test_stale_runtime_or_map_never_admits_native_test(self):
        for path in (self.runtime, self.map):
            with self.subTest(path=path):
                self.publish()
                before = path.read_bytes()
                path.write_bytes(before + b"changed")
                self.assertFalse(self.available())
                path.write_bytes(before)

    def test_absolute_known_length_default_does_not_require_a_terminator(self):
        self.runtime.write_bytes(self.runtime.read_bytes()[:-1] + b'\x02\0\0\0')
        self.report['runtime']['sha256'] = self.digest(self.runtime)
        self.publish()
        self.assertTrue(self.available())

    def test_missing_changed_or_extra_shader_rejects_even_if_adjacent_inventory_looks_complete(self):
        self.publish()
        shader = self.shaders / "hud.frag.spv"
        before = shader.read_bytes()
        shader.unlink()
        self.assertFalse(self.available())
        shader.write_bytes(b"changed shader")
        self.assertFalse(self.available())
        shader.write_bytes(before)
        self.assertTrue(self.available())
        (self.shaders / "extra.spv").write_text("not in the tested inventory")
        self.assertFalse(self.available())

    def test_relocated_runtime_or_fabricated_adjacent_shader_path_rejects(self):
        self.publish()
        other = self.root / "copied/halo2_native"
        other.parent.mkdir()
        shutil.copy2(self.runtime, other)
        shutil.copy2(self.sidecar, other.with_name(other.name + ".native-test.json"))
        self.assertFalse(native_gameplay_test_available(self.root, other, self.cache))
        replacement = self.root / "other-shaders"
        shutil.copytree(self.shaders, replacement)
        self.report["shaders"]["directory"] = str(replacement)
        self.publish()
        self.assertFalse(self.available())

    def test_incomplete_or_unpassed_claims_are_not_capabilities(self):
        mutations = [("schema", "unknown"), ("capability", "full-parity"),
                     ("validation.passed", "true"), ("validation.passed", False),
                     ("validation.scope", "first-frame-only"), ("validation.report_sha256", "unknown"),
                     ("map.path", "halo2/mods/custom.map"), ("runtime.sha256", "0" * 64),
                     ("shaders.binding", "adjacent")]
        for field, value in mutations:
            with self.subTest(field=field, value=value):
                report = copy.deepcopy(self.report)
                owner = report
                parts = field.split(".")
                for part in parts[:-1]:
                    owner = owner[part]
                owner[parts[-1]] = value
                self.publish(report)
                self.assertFalse(self.available())

    def test_missing_map_and_symlink_owners_are_rejected(self):
        self.publish()
        self.map.unlink()
        self.assertFalse(self.available())
        foreign = self.root / "foreign.map"
        foreign.write_text("synthetic MCC Lockout owner")
        self.map.symlink_to(foreign)
        self.assertFalse(self.available())
        self.map.unlink()
        shutil.copy2(foreign, self.map)
        real = self.sidecar.with_suffix(".owned-json")
        self.sidecar.rename(real)
        self.sidecar.symlink_to(real)
        self.assertFalse(self.available())

    def test_malformed_or_oversized_sidecar_rejects(self):
        for data in ("{", "[]", "null", " " * 65537):
            with self.subTest(data=data[:5]):
                self.sidecar.write_text(data)
                self.assertFalse(self.available())


if __name__ == "__main__":
    unittest.main()
