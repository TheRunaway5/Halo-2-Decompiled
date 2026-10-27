"""Exercise native PE imports and transitive DLL packaging in an owned scratch tree."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/windows'))
from package_runtime import package, imported_dlls, system_dll


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
            report = package(root, build, sdk)
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
            previous = (output / 'windows-package.json').read_bytes()
            (build / 'shaders' / names[1]).write_bytes(b'changed compiled shader')
            with self.assertRaises(ValueError):
                package(root, build, sdk)
            self.assertEqual((output / 'windows-package.json').read_bytes(), previous)
            self.assertFalse(list(Path(os.environ['HALO2_TEST_SCRATCH']).glob('windows-shaders-*')))


if __name__ == '__main__':
    unittest.main()
