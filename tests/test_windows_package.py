"""Exercise native PE imports and transitive DLL packaging in an owned scratch tree."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/windows'))
from package_runtime import package, imported_dlls, system_dll, stage_existing_shaders
from onboarding_runtime import collect
import native_renderer_shaders as native_shaders


@unittest.skipUnless(shutil.which('glslangValidator'), 'Vulkan shader compiler is required')
class ExistingShaderCohortTests(unittest.TestCase):
    def test_launcher_refresh_reuses_only_the_unchanged_authenticated_engine_bank(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as temporary:
            root = Path(temporary); bank = root / 'package/shaders'; bank.mkdir(parents=True)
            modules = {}
            for stage, body in {
                'vert': 'void main(){gl_Position=vec4(0,0,0,1);}',
                'frag': 'layout(location=0)out vec4 c;void main(){c=vec4(1);}',
                'geom': 'layout(points)in;layout(points,max_vertices=1)out;void main(){gl_Position=gl_in[0].gl_Position;EmitVertex();EndPrimitive();}',
                'comp': 'layout(local_size_x=1)in;void main(){}',
            }.items():
                source = root / ('module.' + stage); source.write_text('#version 450\n' + body)
                target = root / ('module.' + stage + '.spv')
                subprocess.run(['glslangValidator', '-V', str(source), '-o', str(target)], check=True, capture_output=True)
                modules[stage] = target.read_bytes()
            generated = 'native_world/f-' + '1' * 64 + '.frag.spv'
            names = sorted(native_shaders.AOT_BASE_FIXED_FILES | {generated})
            rows = []
            import hashlib
            for name in names:
                raw = modules[name.rsplit('.', 2)[-2]]
                target = bank / name; target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(raw)
                rows.append({'path': name, 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(),
                             'native_contract': native_shaders.native_module_contract(raw, name)})
            descriptor = {'schema': native_shaders.AOT_PACKAGE_SCHEMA, 'profile': native_shaders.AOT_PROFILE,
                          'passed': True, 'runtime_instruction_interpreter': False,
                          'module_count': len(rows), 'modules': rows,
                          'inventory_sha256': hashlib.sha256(('\n'.join(names) + '\n').encode()).hexdigest()}
            manifest = bank / 'production-shaders.json'; manifest.write_text(json.dumps(descriptor))
            # This direct synthetic owner exercises catalog admission only.
            # The separate package fixture below uses genuine MinGW PE images.
            engine = root / 'halo2_native.exe'; engine.write_bytes(b'synthetic catalog owner\0' + generated.encode() + b'\0')
            engine_pin = hashlib.sha256(engine.read_bytes()).hexdigest()
            pin = hashlib.sha256(manifest.read_bytes()).hexdigest()
            (bank.parent / 'windows-package.json').write_text(json.dumps({
                'schema': 'H2_WINDOWS_NATIVE_PACKAGE_V1', 'files': {
                    'halo2_native.exe': {'sha256': engine_pin}, 'shaders/production-shaders.json': {'sha256': pin}}}))
            staged, reuse = stage_existing_shaders(bank, root / 'staged', engine)
            self.assertEqual(staged, descriptor)
            self.assertEqual(reuse['engine_sha256'], engine_pin)
            self.assertEqual(reuse['manifest_sha256'], pin)
            engine.write_bytes(engine.read_bytes() + b'changed engine')
            with self.assertRaisesRegex(RuntimeError, 'unchanged packaged engine'):
                stage_existing_shaders(bank, root / 'changed-engine', engine)
            engine.write_bytes(b'synthetic catalog owner\0' + generated.encode() + b'\0')
            (bank / generated).write_bytes(b'tampered SPIR-V')
            with self.assertRaisesRegex(ValueError, 'differ'):
                stage_existing_shaders(bank, root / 'tampered-bank', engine)


class OnboardingResourceTests(unittest.TestCase):
    def test_native_setup_sources_and_python_are_collected_without_developer_or_game_banks(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as temporary:
            root = Path(temporary)
            sdk, extra = root / 'sdk', root / 'extra'
            required = [
                'tools/setup_assets.py', 'tools/setup_assets_ui.py', 'tools/native_renderer_shaders.py',
                'tools/setup_pipe.py', 'tools/file_lock.py', 'tools/macos/__init__.py',
                'tools/macos/steam_setup.py', 'tools/steam/content-profile.json', 'tools/data/policy.json',
                'tools/reference/owners.tsv', 'resources/launcher.svg',
                'resources/frontend/mcc-native-layout.json', 'resources/licenses/artwork.txt',
                'external/ww2ogg/packed_codebooks_aoTuV_603.bin',
                'sdk/bin/python.exe', 'sdk/lib/python3.14/encodings/__init__.py',
                'sdk/lib/python3.14/tkinter/__init__.py',
                'sdk/lib/python3.14/lib-dynload/_tkinter.cp314.pyd',
                'sdk/lib/tcl8.6/init.tcl', 'sdk/lib/tk8.6/tk.tcl',
                'extra/lib/python3.14/site-packages/PIL/__init__.py',
                'extra/share/licenses/python-pillow/LICENSE',
            ]
            excluded = ['assets/imported/private/mainmenu.map', 'tools/reference/private.exe',
                        'sdk/include/Python.h', 'sdk/lib/python3.14/config-private/Makefile',
                        'sdk/lib/python3.14/libpython.a', 'sdk/lib/python3.14/test/large.py',
                        'sdk/lib/python3.14/site-packages/unrelated.py', 'tools/__pycache__/cached.pyc']
            for relative in required + excluded:
                path = root / relative; path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('owned fixture')
            (extra / 'halo2-sdk.json').write_text('{"packages":[]}')
            inputs, metadata = collect(root, sdk, extra)
            self.assertEqual(inputs['python/python.exe'], sdk / 'bin/python.exe')
            self.assertIn('python/lib/python3.14/site-packages/PIL/__init__.py', inputs)
            self.assertIn('python/lib/python3.14/lib-dynload/_tkinter.cp314.pyd', inputs)
            for name in ['tools/setup_pipe.py', 'tools/file_lock.py', 'tools/macos/steam_setup.py',
                         'tools/steam/content-profile.json', 'tools/data/policy.json',
                         'tools/reference/owners.tsv', 'resources/frontend/mcc-native-layout.json',
                         'external/ww2ogg/packed_codebooks_aoTuV_603.bin']:
                self.assertIn(name, inputs)
            for source in inputs.values():
                self.assertNotIn(source, [root / name for name in excluded])
            self.assertEqual(metadata['interpreter'], 'python/python.exe')
            (extra / 'lib/python3.14/site-packages/PIL/__init__.py').unlink()
            with self.assertRaisesRegex(RuntimeError, 'Missing native onboarding resource'):
                collect(root, sdk, extra)


@unittest.skipUnless(shutil.which('x86_64-w64-mingw32-g++') and
                     shutil.which('x86_64-w64-mingw32-objdump') and shutil.which('glslangValidator'),
                     'MinGW-w64 and glslangValidator are required')
class WindowsPackageTests(unittest.TestCase):
    def test_native_executable_and_its_complete_dll_closure_are_packaged(self):
        with tempfile.TemporaryDirectory(prefix='windows-package-', dir=os.environ.get('HALO2_TEST_SCRATCH')) as name:
            root = Path(name)
            build, sdk = root / 'build', root / 'sdk'
            (build / 'shaders').mkdir(parents=True)
            (sdk / 'bin').mkdir(parents=True)
            (root / 'data/tags').mkdir(parents=True)
            (root / 'data/tags/modes.json').write_text('{}')
            # Exercise the real production shader stager. A normal build has
            # inventories and SPIR-V; no pre-existing package receipt.
            (root / 'cmake').mkdir()
            (root / 'shaders').mkdir()
            shader = root / 'shaders/good.frag'
            shader.write_text('#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1);}')
            subprocess.run(['glslangValidator', '-V', str(shader), '-o', str(build / 'shaders/good.frag.spv')],
                           check=True, capture_output=True)
            raw = (build / 'shaders/good.frag.spv').read_bytes()
            import hashlib
            pin = hashlib.sha256(raw).hexdigest()
            names = ['good.frag.spv', 'native_world/f-' + pin + '.frag.spv',
                     'native-fragments/hud/' + pin + '.frag.spv']
            for name in names[1:]:
                path = build / 'shaders' / name
                path.parent.mkdir(parents=True)
                path.write_bytes(raw)
            (root / 'cmake/NativeRendererProductionShaders.cmake').write_text(
                'set(native_renderer_fixed_shaders good.frag.spv)\n')
            (build / 'CMakeCache.txt').write_text('CMAKE_HOME_DIRECTORY:INTERNAL=' + str(root) + '\n')
            (build / 'production-shaders.txt').write_text('\n'.join(sorted(names)) + '\n')
            (build / 'native-world-kernel-catalog').mkdir()
            (build / 'native-fragment-catalog').mkdir()
            (build / 'native-world-kernel-catalog/compiled.json').write_text(json.dumps({
                'schema': 'HALO2_NATIVE_WORLD_KERNEL_COMPILED_V1', 'runtime_instruction_interpreter': False,
                'stages': [{'path': names[1], 'bytes': len(raw), 'sha256': pin}]}))
            (build / 'native-fragment-catalog/compiled.json').write_text(json.dumps({
                'schema': 'HALO2_COMPILED_NATIVE_FRAGMENT_KERNELS_V1', 'passed': True,
                'modules': [{'path': names[2], 'bytes': len(raw), 'sha256': pin}]}))
            source = root / 'fixture.cpp'
            source.write_text('#include <iostream>\nint main(){std::cout << "Vulkan package fixture\\n";}\n')
            subprocess.run(['x86_64-w64-mingw32-g++', str(source), '-o', str(build / 'halo2_native.exe')], check=True)
            shutil.copy2(build / 'halo2_native.exe', build / 'eos_sdk_worker.exe')
            security = build / 'online-protocol/Release/halo2_online_security_ffi.dll'
            security.parent.mkdir(parents=True)
            source.write_text('#include <iostream>\nextern "C" __declspec(dllexport) void security_fixture(){std::cout << "security fixture";}\n')
            subprocess.run(['x86_64-w64-mingw32-g++', '-shared', str(source), '-o', str(security)], check=True)
            # Windows searches the nested launcher's directory for imports;
            # the package root alone cannot supply this library to it.
            library = build / 'libonboardingonly.dll'
            import_library = build / 'libonboardingonly.dll.a'
            source.write_text('extern "C" __declspec(dllexport) int onboarding_answer(){return 42;}\n')
            subprocess.run(['x86_64-w64-mingw32-g++', '-shared', str(source),
                            '-Wl,--out-implib,' + str(import_library), '-o', str(library)], check=True)
            nested = build / 'onboarding/Halo2-Launcher.exe'
            nested.parent.mkdir()
            source.write_text('extern "C" __declspec(dllimport) int onboarding_answer();\nint main(){return onboarding_answer()!=42;}\n')
            subprocess.run(['x86_64-w64-mingw32-g++', str(source), str(import_library),
                            '-o', str(nested)], check=True)
            report = package(root, build, sdk, onboarding=False)
            output = root / 'dist/builds/windows-x86_64'
            rows = json.loads((output / 'windows-package.json').read_text())
            self.assertEqual(report['files'], rows['files'])
            self.assertIn('halo2_online_security_ffi.dll', report['files'])
            self.assertIn('eos_sdk_worker.exe', report['files'])
            self.assertIn('libstdc++-6.dll', {name.lower() for name in report['files']})
            self.assertIn('onboarding/libonboardingonly.dll', report['files'])
            self.assertNotIn('onboarding/libstdc++-6.dll', report['files'])
            for image in output.rglob('*'):
                if image.suffix.lower() in {'.exe', '.dll'}:
                    for dependency in imported_dlls(image, 'x86_64-w64-mingw32-objdump'):
                        self.assertTrue(system_dll(dependency) or
                                        dependency.lower() in {path.name.lower() for path in image.parent.iterdir()}, dependency)
            self.assertEqual((output / 'shaders/good.frag.spv').read_bytes(), raw)
            self.assertEqual(report['shader_package']['module_count'], 3)
            self.assertFalse((build / 'shaders/production-shaders.json').exists())
            self.assertEqual((output / 'data/tags/modes.json').read_text(), '{}')
            self.assertTrue((output / 'Run-Halo2.cmd').is_file())
            self.assertFalse(list(output.rglob('.windows-package-*')))
            # The shared setup collector is tested separately above. Exercise
            # its integration using a genuine native PE interpreter fixture;
            # no large custom map banks or downloads belong in this test.
            extra = root / 'extra'; (extra / 'bin').mkdir(parents=True)
            setup = root / 'tools/setup_assets.py'; setup.parent.mkdir()
            setup.write_text('# shared setup fixture\n')
            def custom_sources(_, destination):
                destination.mkdir(parents=True, exist_ok=True)
                (destination / 'custom-assets.json').write_text('{"fixture":true}\n')
                return {'schema': 'H2_CUSTOM_ASSET_SOURCES_V1', 'files': {}, 'groups': {}, 'bytes': 0}
            setup_inputs = {'python/python.exe': build / 'halo2_native.exe', 'tools/setup_assets.py': setup}
            with patch('package_runtime.collect_onboarding', return_value=(setup_inputs, {'interpreter': 'python/python.exe'})), \
                 patch('package_runtime.install_custom_assets', side_effect=custom_sources):
                prepared = package(root, build, sdk, onboarding_sdk=extra)
            self.assertIn('python/libstdc++-6.dll', prepared['files'])
            self.assertIn('python/python.exe', prepared['files'])
            self.assertIn('tools/setup_assets.py', prepared['files'])
            self.assertEqual((output / 'Halo2-Launcher.exe').read_bytes(), nested.read_bytes())
            local = json.loads((output / 'build-output.json').read_text())
            self.assertEqual(local['files']['halo2_launcher']['source'], str(nested))
            self.assertEqual(local['files']['halo2_launcher']['sha256'], prepared['files']['Halo2-Launcher.exe']['sha256'])
            marker = json.loads((output / 'PRECOMPILED_TEST_BUILD.json').read_text())
            self.assertFalse(marker['compilation_required'])
            self.assertFalse(marker['native_gameplay_complete'])
            self.assertFalse(marker['full_tu15_parity'])
            previous = (output / 'windows-package.json').read_bytes()
            (build / 'shaders' / names[1]).write_bytes(b'changed compiled shader')
            with self.assertRaises(ValueError):
                package(root, build, sdk, onboarding=False)
            self.assertEqual((output / 'windows-package.json').read_bytes(), previous)
            self.assertFalse(list(Path(os.environ['HALO2_TEST_SCRATCH']).glob('windows-shaders-*')))


if __name__ == '__main__':
    unittest.main()
