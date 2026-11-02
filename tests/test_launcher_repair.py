"""Real admission failures and repair orchestration; no fabricated runtime proof."""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import launcher_health
import repair_launcher
import test_frontend_capability as fixtures
from windows_shader_binding import readonly_shader_literal


class HealthTests(unittest.TestCase):
    def setUp(self):
        self.f = fixtures.FrontendCapabilityTests()
        self.f.setUp()
        self.addCleanup(self.f.doCleanups)

    def inspect(self):
        with patch('mcc_onboarding.selected', return_value=(self.f.cache, {})):
            return launcher_health.inspect_launcher(self.f.root, self.f.runtime)

    def test_missing_startup_record_is_actionable_and_read_only(self):
        self.f.sidecar.unlink()
        result = self.inspect()
        self.assertFalse(result)
        self.assertEqual(result.category, 'validation')
        self.assertIn('startup check', result.message)
        self.assertFalse(self.f.sidecar.exists())

    def test_ready_requires_actual_underlying_resource_admission(self):
        self.assertTrue(self.inspect())
        (self.f.cache/'halo2/h2_maps_win64_dx11/shared.map').write_bytes(b'changed')
        self.assertFalse(self.inspect())

    def test_repair_verifies_existing_cache_and_does_not_reimport_it(self):
        with patch('mcc_onboarding.selected', return_value=(self.f.cache, {})), \
             patch('mcc_onboarding.select', return_value=(self.f.cache, {})) as select, \
             patch('mcc_onboarding.setup') as setup:
            repair_launcher.repair(self.f.root, self.f.runtime, progress=lambda _: None)
        self.assertTrue(select.call_args.kwargs['verify'])
        setup.assert_not_called()

    def test_failed_verification_repairs_from_mcc_and_rechecks(self):
        with patch('mcc_onboarding.selected', return_value=(self.f.cache, {})), \
             patch('mcc_onboarding.select', side_effect=ValueError('corrupt private copy')), \
             patch('mcc_onboarding.setup', return_value=(self.f.cache, {})) as setup:
            repair_launcher.repair(self.f.root, self.f.runtime, mcc_root=self.f.root/'MCC', progress=lambda _: None)
        self.assertEqual(setup.call_args.args[1], self.f.root/'MCC')

    def test_cache_repair_does_not_bypass_missing_runtime_validation(self):
        self.f.sidecar.unlink()
        with patch('mcc_onboarding.selected', return_value=(self.f.cache, {})), \
             patch('mcc_onboarding.select', return_value=(self.f.cache, {})):
            with self.assertRaisesRegex(ValueError, 'validated game package'):
                repair_launcher.repair(self.f.root, self.f.runtime, progress=lambda _: None)
        self.assertFalse(self.f.sidecar.exists())


class WindowsPackageRepairTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-repair-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.runtime = self.root/'halo2_native.exe'
        self.runtime.write_bytes(b'package engine fixture, not a qualified runtime')
        raw = self.runtime.read_bytes()
        self.record = dict(schema='H2_WINDOWS_NATIVE_PACKAGE_V1', platform='windows-x86_64',
            files={'halo2_native.exe': dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())})
        self.publish()

    def publish(self):
        (self.root/'windows-package.json').write_text(json.dumps(self.record))

    def test_checksum_failure_names_the_file(self):
        updates = []
        repair_launcher.verify_windows_package(self.root, updates.append)
        self.assertEqual(updates[-1].percent, 100)
        self.runtime.write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'halo2_native.exe'):
            repair_launcher.verify_windows_package(self.root, lambda _: None)

    def test_inventory_cannot_escape_its_package(self):
        self.record['files']['../outside'] = self.record['files']['halo2_native.exe']
        self.publish()
        with self.assertRaisesRegex(ValueError, 'file owner'):
            repair_launcher.verify_windows_package(self.root, lambda _: None)

    def test_failed_native_check_does_not_enable_play_or_leave_private_captures(self):
        cache = self.root/'cache'
        cache.mkdir()
        private = []
        def fail(root, runtime, cache, output, scratch, **options):
            private.append(scratch)
            (scratch/'failed.ppm').write_bytes(b'private capture fixture')
            raise RuntimeError('title — black or uniform frame')
        health = launcher_health.LauncherHealth(False, 'Missing startup check', 'validation')
        with patch.object(repair_launcher, 'WINDOWS', True), \
             patch('mcc_onboarding.selected', return_value=(cache, {})), \
             patch('mcc_onboarding.select', return_value=(cache, {})), \
             patch.object(repair_launcher, 'inspect_launcher', return_value=health), \
             patch('qualify_frontend_runtime.qualify', side_effect=fail):
            with self.assertRaisesRegex(ValueError, 'black or uniform frame'):
                repair_launcher.repair(self.root, self.runtime, progress=lambda _: None)
        self.assertEqual(len(private), 1)
        self.assertFalse(private[0].exists())
        self.assertFalse(self.runtime.with_name(self.runtime.name+'.frontend-test.json').exists())
        self.assertTrue(cache.is_dir())


def pe_literal(flags=0x40000040, literal=b'\0shaders\0'):
    data = bytearray(1024)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 60, 128)
    data[128:132] = b'PE\0\0'
    struct.pack_into('<HH', data, 132, 0x8664, 1)
    struct.pack_into('<H', data, 148, 112)
    struct.pack_into('<H', data, 152, 0x20b)
    struct.pack_into('<II', data, 280, 128, 512)
    struct.pack_into('<I', data, 300, flags)
    data[512:512+len(literal)] = literal
    return data


class WindowsShaderBindingTests(unittest.TestCase):
    def test_literal_requires_readonly_x64_pe_section(self):
        self.assertEqual(readonly_shader_literal(pe_literal()), 513)
        for flags in (0xc0000040, 0x60000040, 0):
            with self.subTest(flags=flags), self.assertRaises(ValueError):
                readonly_shader_literal(pe_literal(flags))
        for data in (b'shaders\0', pe_literal(literal=b'notshaders\0'), pe_literal()[:600]):
            with self.assertRaises(ValueError):
                readonly_shader_literal(data)

    def test_windows_admission_requires_exact_portable_layout_and_hash(self):
        f = fixtures.FrontendCapabilityTests(); f.setUp(); self.addCleanup(f.doCleanups)
        runtime = f.root/'halo2_native.exe'
        f.runtime.rename(runtime)
        f.shaders.rename(f.root/'shaders')
        f.runtime, f.shaders = runtime, f.root/'shaders'
        f.sidecar.unlink()
        f.sidecar = runtime.with_name(runtime.name+'.frontend-test.json')
        runtime.write_bytes(pe_literal())
        f.report['runtime'] = dict(path='halo2_native.exe', sha256=f.digest(runtime))
        f.report['shaders'].update(binding='windows-executable-adjacent', directory='shaders')
        f.publish()
        self.assertTrue(f.available())
        runtime.write_bytes(pe_literal(literal=b'\0changed\0'))
        self.assertFalse(f.available())


if __name__ == '__main__':
    unittest.main()
