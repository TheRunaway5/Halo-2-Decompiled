"""Strict AOT capability admission, using compiled SPIR-V and synthetic owners.

These fixtures exercise packaging/admission only, never claim gameplay parity.
"""
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
sys.path.insert(0, str(ROOT/'tests'))
import native_renderer_shaders as shaders
import stage_native_renderer_shaders as staging
import test_native_gameplay_test as fixture


class AotCapabilityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which('glslangValidator')
        if not compiler:
            raise unittest.SkipTest('Vulkan shader compiler unavailable')
        owner = Path(os.environ['HALO2_TEST_SCRATCH'])/'native-renderer-capability'
        owner.mkdir()
        cls.code = {}
        sources = {
            'vert': '#version 450\nvoid main(){gl_Position=vec4(0,0,0,1);}',
            'geom': '#version 450\nlayout(points)in;layout(points,max_vertices=1)out;void main(){gl_Position=gl_in[0].gl_Position;EmitVertex();EndPrimitive();}',
            'frag': '#version 450\nlayout(location=0)out vec4 c;void main(){c=vec4(1);}',
            'comp': '#version 450\nlayout(local_size_x=1)in;void main(){}',
            'interpreter.frag': '#version 450\nlayout(set=0,binding=0,std140)uniform Hidden{uvec4 x[136];}b;layout(location=0)out vec4 c;void main(){c=vec4(float(b.x[0].y));}',
            'temporal.frag': '#version 450\nlayout(set=1,binding=6,std430)readonly buffer Screen{uvec4 extent;uvec4 temporal;uint words[];}s;layout(location=0)out vec4 c;void main(){c=vec4(float(s.extent.x+s.temporal.x+s.words[0]));}',
            'temporal-write.frag': '#version 450\nlayout(set=1,binding=6,std430)buffer Screen{uvec4 extent;uvec4 temporal;uint words[];}s;void main(){atomicAdd(s.words[0],s.extent.x+s.temporal.x);}',
            'old-screen.frag': '#version 450\nlayout(set=1,binding=6,std430)readonly buffer Screen{uvec4 extent;uint words[];}s;layout(location=0)out vec4 c;void main(){c=vec4(float(s.extent.x+s.words[0]));}',
            'omitted-temporal.frag': '#version 450\nlayout(set=1,binding=6,std430)readonly buffer Screen{uvec4 extent;layout(offset=32)uint words[];}s;layout(location=0)out vec4 c;void main(){c=vec4(float(s.extent.x+s.words[0]));}',
            'shifted-temporal.frag': '#version 450\nlayout(set=1,binding=6,std430)readonly buffer Screen{uvec4 extent;layout(offset=32)uvec4 temporal;uint words[];}s;layout(location=0)out vec4 c;void main(){c=vec4(float(s.extent.x+s.temporal.x+s.words[0]));}',
            'float-extent.frag': '#version 450\nlayout(set=1,binding=6,std430)readonly buffer Screen{vec4 extent;uvec4 temporal;uint words[];}s;layout(location=0)out vec4 c;void main(){c=s.extent+vec4(float(s.temporal.x+s.words[0]));}',
            'wide-stride.frag': '#version 450\nlayout(set=1,binding=6,std140)readonly buffer Screen{uvec4 extent;uvec4 temporal;uint words[];}s;layout(location=0)out vec4 c;void main(){c=vec4(float(s.extent.x+s.temporal.x+s.words[0]));}',
        }
        surface = '''
layout(set=0,binding=0)uniform sampler2D t0;
layout(set=0,binding=1)uniform sampler2D t1;
layout(set=0,binding=2)uniform sampler2D t2;
layout(set=0,binding=3)uniform sampler2D t3;
layout(set=0,binding=4,std140)uniform Surface{
layout(offset=0)uvec4 program[15];layout(offset=304)vec4 forwardTransforms[8];}m;
vec4 authoredSurface(){vec2 uv=m.forwardTransforms[0].xy;
vec4 color=texture(t0,uv)+texture(t1,uv)+texture(t2,uv)+texture(t3,uv);
return uint(m.program[3].y)==0x53525448u && uint(m.program[5].y)<=52u?color:color*.5;}
'''
        for base in ('frag', 'temporal.frag', 'temporal-write.frag'):
            text = sources[base].replace('#version 450\n', '#version 450\n'+surface)
            text = text.replace('c=vec4(', 'c=authoredSurface()+vec4(')
            text = text.replace('s.extent.x+s.temporal.x);', 's.extent.x+s.temporal.x+uint(authoredSurface().x));')
            sources['surface.frag' if base == 'frag' else 'surface-'+base] = text
        positive = sources['surface-temporal.frag']
        sources.update({
            'surface-missing-stage.frag': positive.replace('layout(set=0,binding=3)uniform sampler2D t3;', '').replace('texture(t3,uv)', 'vec4(0)'),
            'surface-cube-stage.frag': positive.replace('sampler2D t3', 'samplerCube t3').replace('texture(t3,uv)', 'texture(t3,vec3(uv,1))'),
            'surface-wrong-program-offset.frag': positive.replace('offset=0)uvec4 program', 'offset=16)uvec4 program'),
            'surface-wrong-transform-offset.frag': positive.replace('offset=304', 'offset=320'),
            'surface-short-program.frag': positive.replace('program[15]', 'program[14]'),
            'surface-short-transforms.frag': positive.replace('forwardTransforms[8]', 'forwardTransforms[7]'),
            'surface-float-program.frag': positive.replace('uvec4 program', 'vec4 program'),
            'surface-wrong-magic.frag': positive.replace('0x53525448u', '0x53525449u'),
            'surface-wrong-record-words.frag': positive.replace('52u', '51u'),
        })
        for name, text in sources.items():
            path = owner/('module.'+name)
            path.write_text(text)
            target = path.with_name(path.name+'.spv')
            result = subprocess.run([compiler, '-V', str(path), '-o', str(target)], capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout+result.stderr)
            cls.code[name] = target.read_bytes()

    def setUp(self):
        self.f = fixture.NativeGameplayTestCapabilityTests()
        self.f.setUp()
        self.addCleanup(self.f.doCleanups)
        shutil.rmtree(self.f.shaders)
        self.build = self.f.root/'compiler-build'
        source = self.f.root/'frozen-source'
        (self.build/'shaders').mkdir(parents=True)
        (source/'cmake').mkdir(parents=True)
        (source/'shaders').mkdir()
        policy = source/'cmake/NativeRendererProductionShaders.cmake'
        policy.write_text('set(native_renderer_fixed_shaders\n'+'\n'.join(sorted(shaders.AOT_FIXED_FILES))+'\n)\n')
        (self.build/'CMakeCache.txt').write_text('CMAKE_HOME_DIRECTORY:INTERNAL='+str(source)+'\n')
        names = sorted(shaders.AOT_FIXED_FILES)
        for name in names:
            origin = source/'shaders'/staging.ALIASES.get(name, name[:-4])
            origin.parent.mkdir(parents=True, exist_ok=True)
            origin.write_text('synthetic source owner for '+name)
            target = self.build/'shaders'/name
            target.parent.mkdir(parents=True, exist_ok=True)
            kind = ('surface-temporal.frag' if name == 'path_tracing/world.frag.spv' else
                    'surface-temporal-write.frag' if name == 'path_tracing/lighting.frag.spv' else
                    'surface.frag' if name == 'path_tracing/depth.frag.spv' else name.rsplit('.', 2)[-2])
            target.write_bytes(self.code[kind])
        self.generated = ['native_world/v-'+'1'*64+'.vert.spv',
                          'native_world/f-'+'2'*64+'.frag.spv',
                          'native-fragments/hud/'+'3'*64+'.frag.spv',
                          'native_world/v-'+'1'*64+'-source-depth.vert.spv']
        groups = []
        for name in self.generated:
            target = self.build/'shaders'/name
            target.parent.mkdir(parents=True, exist_ok=True)
            data = self.code[name.rsplit('.', 2)[-2]]
            target.write_bytes(data)
            groups.append(dict(path=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
        for relative, report in {
            'native-world-kernel-catalog/compiled.json': dict(schema='HALO2_NATIVE_WORLD_KERNEL_COMPILED_V1', runtime_instruction_interpreter=False, stages=[row for row in groups if row['path'].startswith('native_world/')]),
            'native-fragment-catalog/compiled.json': dict(schema='HALO2_COMPILED_NATIVE_FRAGMENT_KERNELS_V1', passed=True, modules=[row for row in groups if row['path'].startswith('native-fragments/')]),
        }.items():
            target = self.build/relative
            target.parent.mkdir()
            target.write_text(json.dumps(report))
        names += self.generated
        (self.build/'production-shaders.txt').write_text('\n'.join(sorted(names))+'\n')
        self.package = staging.stage_production_shaders(self.build, self.f.shaders)
        self.f.runtime.write_bytes(b'synthetic executable owner\0'+os.fsencode(self.f.shaders)+b'\0'+
                                   b'\0'.join(name.encode() for name in self.generated)+b'\0'+
                                   shaders.PATH_TRACING_EXECUTABLE_MARKER+shaders.PATH_TRACING_VISIBILITY_MARKER+
                                   shaders.PATH_TRACING_LIGHTING_MARKER+shaders.PATH_TRACING_TEMPORAL_MARKER+
                                   shaders.PATH_TRACING_SURFACE_MARKER)
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        self.f.report['shaders'] = dict(binding='compiled-default', directory=str(self.f.shaders),
            **shaders.shader_capability(self.f.shaders, self.f.runtime))
        self.f.publish()

    def rewrite_package(self, report=None):
        if report is not None:
            self.package = report
        (self.f.shaders/shaders.AOT_PACKAGE_NAME).write_text(json.dumps(self.package, indent=2)+'\n')
        self.f.report['shaders']['package']['sha256'] = self.f.digest(self.f.shaders/shaders.AOT_PACKAGE_NAME)
        self.f.publish()

    def test_fixed_abi_matches_the_actual_production_source_policy(self):
        text = (ROOT/'cmake/NativeRendererProductionShaders.cmake').read_text()
        fixed = re.search(r'set\(native_renderer_fixed_shaders\s+([^)]*)\)', text).group(1).split()
        self.assertEqual(set(fixed), shaders.AOT_FIXED_FILES)
        self.assertEqual(len(fixed), len(shaders.AOT_FIXED_FILES))
        marker = shaders.PATH_TRACING_EXECUTABLE_MARKER.rstrip(b'\0').decode()
        self.assertIn('"'+marker+'"', (ROOT/'src/rendering/renderer/renderer.cpp').read_text())
        marker = shaders.PATH_TRACING_VISIBILITY_MARKER.rstrip(b'\0').decode()
        self.assertIn('"'+marker+'"', (ROOT/'src/rendering/renderer/renderer.cpp').read_text())
        marker = shaders.PATH_TRACING_LIGHTING_MARKER.rstrip(b'\0').decode()
        self.assertIn('"'+marker+'"', (ROOT/'src/rendering/renderer/renderer.cpp').read_text())
        marker = shaders.PATH_TRACING_TEMPORAL_MARKER.rstrip(b'\0').decode()
        self.assertIn('"'+marker+'"', (ROOT/'src/rendering/renderer/renderer.cpp').read_text())
        marker = shaders.PATH_TRACING_SURFACE_MARKER.rstrip(b'\0').decode()
        self.assertIn('"'+marker+'"', (ROOT/'src/rendering/renderer/renderer.cpp').read_text())

    def omit_path_tracing_modules(self, names):
        for name in names:
            (self.f.shaders/name).unlink()
        self.package['modules'] = [row for row in self.package['modules'] if row['path'] not in names]
        self.package['module_count'] = len(self.package['modules'])
        inventory = '\n'.join(sorted(row['path'] for row in self.package['modules']))+'\n'
        self.package['inventory_sha256'] = hashlib.sha256(inventory.encode()).hexdigest()
        self.rewrite_package()

    def test_path_tracing_executable_requires_its_complete_shader_pair(self):
        self.omit_path_tracing_modules(shaders.PATH_TRACING_FILES)
        self.assertFalse(self.f.available())

    def test_older_executable_preserves_its_qualified_fixed_shader_abi(self):
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(shaders.PATH_TRACING_EXECUTABLE_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_VISIBILITY_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_LIGHTING_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_TEMPORAL_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_SURFACE_MARKER, b''))
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        self.omit_path_tracing_modules(shaders.PATH_TRACING_FILES | shaders.PATH_TRACING_VISIBILITY_FILES |
                                      shaders.PATH_TRACING_LIGHTING_FILES)
        self.assertTrue(self.f.available())

    def test_visibility_executable_cannot_omit_its_depth_prepass(self):
        self.omit_path_tracing_modules(shaders.PATH_TRACING_VISIBILITY_FILES)
        self.assertFalse(self.f.available())

    def test_older_path_tracing_executable_keeps_its_complete_pair(self):
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(shaders.PATH_TRACING_VISIBILITY_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_LIGHTING_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_TEMPORAL_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_SURFACE_MARKER, b''))
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        self.omit_path_tracing_modules(shaders.PATH_TRACING_VISIBILITY_FILES | shaders.PATH_TRACING_LIGHTING_FILES)
        self.assertTrue(self.f.available())

    def test_older_visibility_executable_preserves_its_shader_abi(self):
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(shaders.PATH_TRACING_LIGHTING_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_TEMPORAL_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_SURFACE_MARKER, b''))
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        self.omit_path_tracing_modules(shaders.PATH_TRACING_LIGHTING_FILES)
        self.assertTrue(self.f.available())

    def test_hdr_executable_requires_its_colorless_lighting_shader(self):
        self.omit_path_tracing_modules(shaders.PATH_TRACING_LIGHTING_FILES)
        self.assertFalse(self.f.available())

    def replace_module(self, name, data):
        (self.f.shaders/name).write_bytes(data)
        row = next(row for row in self.package['modules'] if row['path'] == name)
        row.update(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                   native_contract=shaders.native_module_contract(data, name))
        self.rewrite_package()
        return row

    def test_temporal_contract_is_derived_from_both_actual_fragment_layouts(self):
        for name in shaders.PATH_TRACING_TEMPORAL_FRAGMENTS:
            row = next(row for row in self.package['modules'] if row['path'] == name)
            self.assertEqual(row['native_contract']['path_tracing_screen_abi'], 2)
            self.assertNotIn('path_tracing_screen_abi',
                             shaders.native_module_contract(self.code['old-screen.frag'], name))
        self.assertNotIn('path_tracing_screen_abi',
                         shaders.native_module_contract(self.code['temporal.frag'], 'ui.frag.spv'))

    def test_temporal_host_rejects_old_missing_shifted_and_mistyped_headers(self):
        for name in shaders.PATH_TRACING_TEMPORAL_FRAGMENTS:
            for kind in ('old-screen.frag', 'omitted-temporal.frag', 'shifted-temporal.frag',
                         'float-extent.frag', 'wide-stride.frag', 'frag'):
                with self.subTest(name=name, kind=kind):
                    before = copy.deepcopy(self.package)
                    path = self.f.shaders/name
                    old = path.read_bytes()
                    self.replace_module(name, self.code[kind])
                    with self.assertRaisesRegex(ValueError, '32-byte screen ABI'):
                        shaders.validate_native_package(self.f.shaders, executable=self.f.runtime)
                    self.assertFalse(self.f.available())
                    path.write_bytes(old)
                    self.rewrite_package(before)

    def test_temporal_metadata_cannot_forge_an_old_fragment_layout(self):
        for name in shaders.PATH_TRACING_TEMPORAL_FRAGMENTS:
            before = copy.deepcopy(self.package)
            path = self.f.shaders/name
            old = path.read_bytes()
            row = self.replace_module(name, self.code['old-screen.frag'])
            row['native_contract']['path_tracing_screen_abi'] = 2
            self.rewrite_package()
            with self.assertRaisesRegex(ValueError, 'falsified its executable contract'):
                shaders.validate_native_package(self.f.shaders, executable=self.f.runtime)
            path.write_bytes(old)
            self.rewrite_package(before)

    def test_older_hdr_host_retains_its_qualified_16byte_screen_contract(self):
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(shaders.PATH_TRACING_TEMPORAL_MARKER, b'')
                                   .replace(shaders.PATH_TRACING_SURFACE_MARKER, b''))
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        for name in shaders.PATH_TRACING_TEMPORAL_FRAGMENTS:
            row = self.replace_module(name, self.code['old-screen.frag'])
            self.assertNotIn('path_tracing_screen_abi', row['native_contract'])
        self.assertTrue(self.f.available())

    def test_surface_contract_is_derived_from_all_three_actual_module_interfaces(self):
        for name in shaders.PATH_TRACING_SURFACE_FRAGMENTS:
            row = next(row for row in self.package['modules'] if row['path'] == name)
            self.assertEqual(row['native_contract']['path_tracing_surface_abi'], 1)
            self.assertNotIn('path_tracing_surface_abi',
                             shaders.native_module_contract(self.code['temporal.frag'], name))
        self.assertNotIn('path_tracing_surface_abi',
                         shaders.native_module_contract(self.code['surface-temporal.frag'], 'ui.frag.spv'))

    def test_surface_host_rejects_old_or_incomplete_native_interfaces(self):
        kinds = ('temporal.frag', 'surface-missing-stage.frag', 'surface-cube-stage.frag',
                 'surface-wrong-program-offset.frag', 'surface-wrong-transform-offset.frag',
                 'surface-short-program.frag', 'surface-short-transforms.frag',
                 'surface-float-program.frag', 'surface-wrong-magic.frag', 'surface-wrong-record-words.frag')
        for name in shaders.PATH_TRACING_SURFACE_FRAGMENTS:
            for kind in kinds:
                with self.subTest(name=name, kind=kind):
                    before = copy.deepcopy(self.package)
                    path = self.f.shaders/name
                    old = path.read_bytes()
                    self.replace_module(name, self.code[kind])
                    with self.assertRaisesRegex(ValueError, 'authored surface ABI'):
                        shaders.validate_native_package(self.f.shaders, executable=self.f.runtime)
                    self.assertFalse(self.f.available())
                    path.write_bytes(old)
                    self.rewrite_package(before)

    def test_surface_metadata_cannot_forge_an_old_temporal_fragment(self):
        for name in shaders.PATH_TRACING_SURFACE_FRAGMENTS:
            before = copy.deepcopy(self.package)
            path = self.f.shaders/name
            old = path.read_bytes()
            row = self.replace_module(name, self.code['temporal.frag'])
            row['native_contract']['path_tracing_surface_abi'] = 1
            self.rewrite_package()
            with self.assertRaisesRegex(ValueError, 'falsified its executable contract'):
                shaders.validate_native_package(self.f.shaders, executable=self.f.runtime)
            path.write_bytes(old)
            self.rewrite_package(before)

    def test_older_temporal_host_preserves_its_existing_no_surface_contract(self):
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(shaders.PATH_TRACING_SURFACE_MARKER, b''))
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        for name in shaders.PATH_TRACING_SURFACE_FRAGMENTS:
            kind = ('temporal-write.frag' if name == 'path_tracing/lighting.frag.spv' else
                    'temporal.frag' if name == 'path_tracing/world.frag.spv' else 'frag')
            row = self.replace_module(name, self.code[kind])
            self.assertNotIn('path_tracing_surface_abi', row['native_contract'])
        self.assertTrue(self.f.available())

    def test_partial_path_tracing_pair_is_rejected_even_for_older_executable(self):
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(shaders.PATH_TRACING_EXECUTABLE_MARKER, b''))
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        self.omit_path_tracing_modules({'path_tracing/world.frag.spv'})
        self.assertFalse(self.f.available())

    def test_new_package_and_compact_capability_admit_without_legacy_shaders(self):
        self.assertTrue(self.f.available())
        self.assertEqual(self.f.report['shaders']['profile'], shaders.AOT_PROFILE)
        self.assertNotIn('files', self.f.report['shaders'])
        self.assertLess(self.f.sidecar.stat().st_size, 65536)
        self.assertFalse((self.f.shaders/'hud.frag.spv').exists())
        self.assertFalse((self.f.shaders/'world.frag.spv').exists())

    def test_source_depth_variant_requires_its_actual_executable_binding(self):
        name=self.generated[-1]
        self.assertIn(name,shaders._bound_shader_requirements(self.f.runtime)[0])
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(name.encode(),b'x'*len(name)))
        with self.assertRaisesRegex(ValueError,'executable-bound program catalogs'):
            shaders.validate_native_package(self.f.shaders,executable=self.f.runtime)

    def test_read_only_cli_checks_actual_package_and_reports_changed_modules(self):
        command = [sys.executable, '-B', str(ROOT/'tools/native_renderer_shaders.py'),
                   '--verify-AOT-package', '--directory', str(self.f.shaders),
                   '--executable', str(self.f.runtime)]
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertEqual(json.loads(result.stdout)['profile'], shaders.AOT_PROFILE)
        result = subprocess.run([*command, '--quiet'], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertEqual(result.stdout, '')
        (self.f.shaders/'ui.frag.spv').write_bytes(b'changed')
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertEqual(result.returncode, 1, result.stdout+result.stderr)
        self.assertFalse(json.loads(result.stdout)['passed'])

    def test_portable_inventory_preserves_native_package_and_binds_its_executable(self):
        from packaged_shader_defaults import shader_inventory
        proof = shader_inventory(self.f.shaders, executable=self.f.runtime)
        self.assertEqual(proof, shaders.validate_native_package(self.f.shaders,
                         executable=self.f.runtime)['files'])
        with self.assertRaisesRegex(ValueError, 'executable-bound catalog'):
            shader_inventory(self.f.shaders)
        self.f.runtime.write_bytes(self.f.runtime.read_bytes().replace(
            self.generated[0].encode(), b'x' * len(self.generated[0])))
        with self.assertRaisesRegex(ValueError, 'executable-bound program catalogs'):
            shader_inventory(self.f.shaders, executable=self.f.runtime)

    def test_portable_frontend_keeps_the_compact_native_package_pin(self):
        from package_mcc_test_build import packaged_frontend
        from packaged_shader_defaults import PACKAGE_LOADER, PACKAGE_PAYLOAD, PACKAGE_RUNTIME
        stage = self.f.root/'portable-stage'
        for name in (PACKAGE_RUNTIME, PACKAGE_PAYLOAD, PACKAGE_LOADER):
            path = stage/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'synthetic relocation output identity')
        qualified = dict(report=self.f.report, source_capability_sha256='a'*64)
        packaged_frontend(stage, qualified, dict(compiled_default='./bin/shaders'))
        report = json.loads((stage/(PACKAGE_RUNTIME+'.frontend-test.json')).read_text())
        self.assertEqual(report['shaders']['profile'], shaders.AOT_PROFILE)
        self.assertEqual(report['shaders']['package'], self.f.report['shaders']['package'])
        self.assertNotIn('files', report['shaders'])

    def test_false_profile_schema_and_interpreter_free_claims_are_rejected(self):
        for field, value in [('profile', shaders.PROFILE), ('schema', 'unknown'),
                             ('passed', False), ('runtime_instruction_interpreter', True)]:
            before = copy.deepcopy(self.package)
            self.package[field] = value
            self.rewrite_package()
            self.assertFalse(self.f.available(), field)
            self.rewrite_package(before)
        self.f.report['shaders']['profile'] = shaders.PROFILE
        self.f.report['shaders']['files'] = {}
        self.f.publish()
        self.assertFalse(self.f.available())

    def test_descriptor_tampering_is_not_resealed_by_capability_admission(self):
        path = self.f.shaders/shaders.AOT_PACKAGE_NAME
        path.write_bytes(path.read_bytes()+b' ')
        self.assertFalse(self.f.available())

    def test_changed_module_bytes_and_forged_instruction_contract_are_rejected(self):
        row = self.package['modules'][0]
        path = self.f.shaders/row['path']
        path.write_bytes(path.read_bytes()+b'changed')
        self.assertFalse(self.f.available())
        name = self.generated[1]
        row = next(row for row in self.package['modules'] if row['path'] == name)
        data = self.code['interpreter.frag']
        (self.f.shaders/name).write_bytes(data)
        row.update(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
        # A forged passed flag cannot suppress the independent memory-read audit.
        row['native_contract']['passed'] = True
        self.rewrite_package()
        self.assertFalse(self.f.available())

    def test_vulkan_execution_stage_is_checked_independently_of_report(self):
        name = self.generated[1]
        row = next(row for row in self.package['modules'] if row['path'] == name)
        data = self.code['vert']
        (self.f.shaders/name).write_bytes(data)
        row.update(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
        self.rewrite_package()
        self.assertFalse(self.f.available())

    def test_jointly_omitted_fixed_and_generated_module_owners_are_rejected(self):
        for name in ['ui.frag.spv', self.generated[1]]:
            with self.subTest(name=name):
                old = copy.deepcopy(self.package)
                path = self.f.shaders/name
                data = path.read_bytes()
                path.unlink()
                self.package['modules'] = [row for row in self.package['modules'] if row['path'] != name]
                self.package['module_count'] -= 1
                inventory = '\n'.join(sorted(row['path'] for row in self.package['modules']))+'\n'
                self.package['inventory_sha256'] = hashlib.sha256(inventory.encode()).hexdigest()
                self.rewrite_package()
                self.assertFalse(self.f.available())
                path.write_bytes(data)
                self.rewrite_package(old)

    def test_unlisted_module_and_declared_test_probe_are_rejected(self):
        path = self.f.shaders/'texture_program.comp.spv'
        path.write_bytes(self.code['comp'])
        self.assertFalse(self.f.available())
        self.package['modules'].append(dict(path=path.name, bytes=path.stat().st_size,
            sha256=self.f.digest(path), native_contract=shaders.native_module_contract(path.read_bytes(), path.name)))
        self.package['module_count'] += 1
        self.rewrite_package()
        self.assertFalse(self.f.available())

    def test_missing_module_alias_and_unknown_executable_catalog_are_rejected(self):
        path = self.f.shaders/self.generated[0]
        data = path.read_bytes()
        path.unlink()
        self.assertFalse(self.f.available())
        foreign = self.f.root/'foreign.spv'
        foreign.write_bytes(data)
        path.symlink_to(foreign)
        self.assertFalse(self.f.available())
        path.unlink()
        path.write_bytes(data)
        self.f.runtime.write_bytes(self.f.runtime.read_bytes()+b'native-fragments/hud/'+b'4'*64+b'.frag.spv\0')
        self.f.report['runtime']['sha256'] = self.f.digest(self.f.runtime)
        self.f.publish()
        self.assertFalse(self.f.available())

    def test_module_mutation_during_later_contract_checks_fails_closed(self):
        original = shaders.native_module_contract
        first = self.f.shaders/self.package['modules'][0]['path']
        last = self.package['modules'][-1]['path']
        def check(data, name):
            contract = original(data, name)
            if name == last:
                first.write_bytes(first.read_bytes()+b'changed during validation')
            return contract
        with patch.object(shaders, 'native_module_contract', side_effect=check):
            with self.assertRaises(ValueError):
                shaders.validate_native_package(self.f.shaders, executable=self.f.runtime)

    def test_gameplay_selection_accepts_only_the_same_complete_qualified_package(self):
        import gameplay_runtime_selection as selection
        from types import SimpleNamespace
        runtime = self.f.runtime
        files = {p.relative_to(runtime.parent).as_posix(): dict(sha256=self.f.digest(p), bytes=p.stat().st_size)
                 for p in self.f.shaders.rglob('*') if p.is_file()}
        inventory = runtime.parent/'runtime-files.json'
        inventory.write_text(json.dumps(files))
        package = self.f.shaders/shaders.AOT_PACKAGE_NAME
        proof = runtime.parent/'qualification.json'
        qualification = dict(passed=True, runtime_sha256=self.f.digest(runtime),
            runtime=runtime.relative_to(self.f.root).as_posix(), unresolved_case_failures=[],
            native_renderer_shader_package=dict(passed=True, module_count=self.package['module_count'],
                report=package.relative_to(self.f.root).as_posix(), report_sha256=self.f.digest(package)))
        proof.write_text(json.dumps(qualification))
        record = dict(schema=selection.SCHEMA, runtime=qualification['runtime'],
            runtime_sha256=qualification['runtime_sha256'], qualification=proof.relative_to(self.f.root).as_posix(),
            qualification_sha256=self.f.digest(proof), inventory=inventory.relative_to(self.f.root).as_posix(),
            inventory_sha256=self.f.digest(inventory), cache=self.f.cache.relative_to(self.f.root).as_posix(),
            maps={'lockout.map': self.f.digest(self.f.map)}, default_map='lockout.map', local_players=1)
        (self.f.root/selection.SELECTION).write_text(json.dumps(record))
        with patch.dict(sys.modules, {'mcc_onboarding': SimpleNamespace(selected=lambda root: (self.f.cache, {}))}):
            self.assertEqual(selection.selected_gameplay_runtime(self.f.root), record)
            (self.f.shaders/'undeclared.frag.spv').write_bytes(self.code['frag'])
            with self.assertRaises(ValueError):
                selection.selected_gameplay_runtime(self.f.root)


if __name__ == '__main__':
    unittest.main()
