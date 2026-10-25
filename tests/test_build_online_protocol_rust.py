"""Worker build/staging boundaries without invoking Cargo or a native compiler."""
import importlib.util
import json
import os
from pathlib import Path
import stat
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('online_rust_build', ROOT/'tools/build_online_protocol_rust.py')
BUILD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILD)


class WorkerBuildTests(unittest.TestCase):
    def build(self, targets, *, sdk=False, missing_worker=False):
        with tempfile.TemporaryDirectory(prefix='halo2-worker-build-') as temporary:
            root = Path(temporary)
            module = root/'source/online_protocol_bridge'
            (module/'src').mkdir(parents=True)
            manifest = module/'Cargo.toml'
            manifest.write_text('[package]\nname="fixture"\n')
            (module/'Cargo.lock').write_text('locked fixture\n')
            (module/'src/eos_sdk.hpp').write_text('// fixture header\n')
            output = root/'build'
            calls = []
            sdk_root = root/'vendor/SDK'
            sdk_library = sdk_root/'Bin/libEOSSDK-Linux-Shipping.so'
            args = ['build_online_protocol_rust.py', '--manifest', str(manifest),
                    '--output', str(output), '--cargo', 'cargo', '--rustc', 'rustc',
                    '--cxx', 'clang++', '--library-name', 'libbridge.a', '--offline']
            for target in targets:
                args += ['--target', target]
            if sdk:
                sdk_library.parent.mkdir(parents=True)
                sdk_library.write_bytes(b'selected vendor library')
                args += ['--eos-sdk-root', str(sdk_root), '--eos-sdk-library', str(sdk_library)]

            def run(command, cwd, env):
                calls.append((command, dict(env)))
                if command[0] == 'rustc':
                    return 'rustc fixture\nrelease: 1.96.0\n'
                if command[0] == 'lipo':
                    Path(command[-1]).write_bytes(b'combined slices')
                    return ''
                target = command[command.index('--target')+1]
                release = output/'cargo'/target/'release'
                release.mkdir(parents=True, exist_ok=True)
                include = output/'cargo/cxxbridge'
                for name in ['rust/cxx.h', 'halo2-online-protocol-bridge/src/lib.rs.h']:
                    path = include/name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(b'generated interface')
                if '--lib' in command:
                    (release/'libbridge.a').write_bytes(b'archive')
                    return 'note: native-static-libs: -lpthread\n'
                self.assertIn('--bin', command)
                self.assertEqual(command[command.index('--bin')+1], 'eos_sdk_worker')
                if not missing_worker:
                    worker = release/'eos_sdk_worker'
                    worker.write_bytes(b'worker slice')
                    worker.chmod(0o751)
                return ''

            with patch.object(sys, 'argv', args), patch.object(BUILD, 'run', side_effect=run):
                result = BUILD.main()
            self.assertEqual(result, 0)
            report = json.loads((output/'build-report.json').read_text())
            worker = output/'eos_sdk_worker'
            self.assertEqual(Path(report['worker']), worker)
            self.assertEqual(report['worker_sha256'], BUILD.digest(worker))
            if os.name != 'nt':
                self.assertEqual(stat.S_IMODE(worker.stat().st_mode), 0o751)
            self.assertEqual((output/'libbridge.a').read_bytes(), b'combined slices' if len(targets)>1 else b'archive')
            if sdk:
                self.assertEqual((output/sdk_library.name).read_bytes(), sdk_library.read_bytes())
            return calls

    def test_offline_stub_worker_is_built_and_executable(self):
        calls = self.build(['x86_64-unknown-linux-gnu'])
        commands = [command for command, _ in calls if '--bin' in command]
        self.assertEqual(len(commands), 1)
        self.assertIn('--release', commands[0])
        self.assertIn('--offline', commands[0])
        self.assertIn('link-arg=-Wl,-rpath,$ORIGIN', commands[0])
        self.assertFalse(any(value.startswith('dylib=EOSSDK') for value in commands[0]))

    def test_selected_sdk_is_linked_and_staged_with_worker(self):
        calls = self.build(['x86_64-unknown-linux-gnu'], sdk=True)
        command, env = next(row for row in calls if '--bin' in row[0])
        self.assertIn('dylib=EOSSDK-Linux-Shipping', command)
        self.assertTrue(any(value.startswith('native=') and value.endswith('/vendor/SDK/Bin') for value in command))
        self.assertTrue(env['HALO2_EOS_SDK_ROOT'].endswith('/vendor/SDK'))

    def test_universal_build_combines_worker_and_library_slices(self):
        calls = self.build(['aarch64-apple-darwin', 'x86_64-apple-darwin'])
        commands = [command for command, _ in calls]
        worker_lipo = next(command for command in commands if command[0]=='lipo' and command[-1].endswith('/eos_sdk_worker'))
        self.assertTrue(any('/aarch64-apple-darwin/release/eos_sdk_worker' in value for value in worker_lipo))
        self.assertTrue(any('/x86_64-apple-darwin/release/eos_sdk_worker' in value for value in worker_lipo))

    def test_missing_worker_is_a_build_failure(self):
        with self.assertRaisesRegex(ValueError, 'Missing executable Rust SDK worker'):
            self.build(['x86_64-unknown-linux-gnu'], missing_worker=True)

    def test_macos_loader_path_uses_selected_dylib_name(self):
        args = BUILD.worker_link_arguments('aarch64-apple-darwin', '/tool/clang++',
            '-Wl,-dead_strip', Path('/sdk/Bin/libEOSSDK-Mac-Shipping.dylib'))
        self.assertIn('dylib=EOSSDK-Mac-Shipping', args)
        self.assertIn('link-arg=-Wl,-rpath,@loader_path', args)
        self.assertIn('link-arg=-Wl,-dead_strip', args)

    def test_windows_requires_and_uses_actual_import_library(self):
        runtime = Path('/sdk/Bin/EOSSDK-Win64-Shipping.dll')
        with self.assertRaisesRegex(ValueError, 'import library'):
            BUILD.worker_link_arguments('x86_64-pc-windows-msvc', 'link.exe', '', runtime)
        args = BUILD.worker_link_arguments('x86_64-pc-windows-msvc', 'link.exe', '',
            runtime, Path('/sdk/Lib/EOSSDK-Win64-Shipping.lib'))
        self.assertIn('linker=link.exe', args)
        self.assertIn('link-arg=/sdk/Lib/EOSSDK-Win64-Shipping.lib', args)


if __name__ == '__main__':
    unittest.main()
