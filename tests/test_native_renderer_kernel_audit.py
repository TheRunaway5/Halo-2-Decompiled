"""Executable negative controls for the complete renderer program-read audit."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_native_renderer_kernels import (AuditError, audit_spirv, direct_files,
    source_runtime_calls, production_archive_violations)


class RendererKernelAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = shutil.which('glslangValidator')
        cls.optimizer = shutil.which('spirv-opt')
        if not cls.compiler or not cls.optimizer:
            raise unittest.SkipTest('Vulkan shader tools unavailable')
        cls.owner = Path(os.environ['HALO2_TEST_SCRATCH']) / 'renderer-kernel-audit-tests'
        cls.owner.mkdir()
        cls.serial = 0

    def compile(self, declaration, expression, strip=False, extra=''):
        type(self).serial += 1
        stem = self.owner / str(self.serial)
        source, raw, output = stem.with_suffix('.frag'), stem.with_suffix('.raw.spv'), stem.with_suffix('.spv')
        source.write_text('#version 450\nlayout(location=0)out vec4 pixel;\n' + declaration + '\n' + extra +
                          '\nvoid main(){pixel=vec4(float(' + expression + '));}\n')
        result = subprocess.run([self.compiler, '-V', '--target-env', 'vulkan1.3', str(source), '-o', str(raw)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        command = [self.optimizer, '--preserve-interface', '-O']
        if strip:
            command.append('--strip-debug')
        result = subprocess.run(command + [str(raw), '-o', str(output)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return output

    def test_literal_colors_are_data_but_controls_are_code(self):
        declaration = 'layout(set=0,binding=4,std140) uniform Bank{uvec4 a[15];}b;'
        for word in (0, 8, 9, 26, 34, 42, 45, 53, 54, 55, 56, 59):
            with self.subTest(word=word):
                path = self.compile(declaration, f'b.a[{word//4}][{word%4}]')
                result = audit_spirv(path.read_bytes())
                self.assertFalse(result['passed'])
                self.assertEqual(result['violations'][0]['kind'], 'pixel-control-bank-read')
        for word in (10, 17, 25, 43, 44):
            with self.subTest(word=word):
                self.assertTrue(audit_spirv(self.compile(declaration, f'b.a[{word//4}][{word%4}]').read_bytes())['passed'])

    def test_vertex_reads_fail_with_renamed_reordered_nested_members_and_no_debug(self):
        declarations = [
            ('layout(set=1,binding=7,std140) uniform Bank{uvec4 entirelyRenamed[136];}b;', 'b.entirelyRenamed[0].y'),
            ('layout(set=0,binding=1,std140) uniform Bank{vec4 c;uvec4 foo[136];}b;', 'b.foo[10].z'),
            ('struct Nest{vec4 x;uvec4 y[136];};layout(set=0,binding=0,std430) buffer Bank{Nest data;}b;', 'b.data.y[5].w'),
            ('layout(set=2,binding=3,std140) uniform Bank{uint opaque[544];}b;', 'b.opaque[3]'),
        ]
        for declaration, expression in declarations:
            with self.subTest(expression=expression):
                result = audit_spirv(self.compile(declaration, expression, strip=True).read_bytes())
                self.assertFalse(result['passed'])
                self.assertEqual(result['violations'][0]['kind'], 'vertex-instruction-bank-read')
                self.assertIsNone(result['violations'][0]['owner_debug_name'])

    def test_dynamic_color_address_is_not_a_finite_color_owner(self):
        declaration = 'layout(set=0,binding=0,std140)uniform Bank{uvec4 a[15];uint choice;}b;'
        self.assertFalse(audit_spirv(self.compile(declaration, 'b.a[b.choice].z').read_bytes())['passed'])

    def test_unread_abi_reservations_and_ordinary_dynamic_state_pass(self):
        declaration = 'layout(set=0,binding=0,std140)uniform Bank{uvec4 a[136];uvec4 b[15];vec4 color;uint state;}u;'
        self.assertTrue(audit_spirv(self.compile(declaration, 'u.color.x+float(u.state)').read_bytes())['passed'])

    def test_runtime_array_of_nested_program_owners_is_checked(self):
        declaration = 'struct Element{uvec4 program[15];};layout(set=0,binding=0,std430)buffer Bank{Element rows[];}u;'
        self.assertFalse(audit_spirv(self.compile(declaration, 'u.rows[uint(gl_FragCoord.x)].program[0].x').read_bytes())['passed'])

    def test_private_runtime_program_bank_is_not_hidden_by_uniform_only_checks(self):
        declaration = 'layout(set=0,binding=0,std140)uniform Bank{uint control;}u;'
        extra = 'uint hidden[60];uint lookup(){hidden[u.control%60u]=u.control;return hidden[u.control%60u];}'
        self.assertFalse(audit_spirv(self.compile(declaration, 'lookup()', extra=extra).read_bytes())['passed'])

    def test_invalid_extent_or_module_without_entry_point_fails(self):
        for raw in (b'', struct.pack('<5I', 0x07230203, 0x10000, 0, 1, 0),
                    struct.pack('<6I', 0x07230203, 0x10000, 0, 1, 0, (10<<16)|21)):
            with self.assertRaises(AuditError):
                audit_spirv(raw)

    def test_packaged_inventory_rejects_unlisted_module_and_changed_bytes(self):
        owner = self.owner / 'package';owner.mkdir()
        shader = owner / 'shader.spv';shader.write_bytes(b'original')
        inventory = {'shader.spv': {'bytes': 8, 'sha256': hashlib.sha256(b'original').hexdigest()}}
        self.assertEqual(len(direct_files(owner, inventory)[1]), 1)
        hidden = owner / 'hidden.spv';hidden.write_bytes(b'hidden')
        with self.assertRaisesRegex(AuditError, 'unlisted'):
            direct_files(owner, inventory)
        hidden.unlink();shader.write_bytes(b'changed!')
        with self.assertRaisesRegex(AuditError, 'immutable inventory'):
            direct_files(owner, inventory)

    def test_cpu_runtime_definition_and_calls_are_not_hidden_by_comments(self):
        root = self.owner / 'source';(root / 'src/rendering').mkdir(parents=True)
        (root / 'src/rendering/vertex_program.cpp').write_text('void execute_vertex_program(const X& x,Y& y){}\nvoid runtime(){execute_vertex_program(x,y);}')
        (root / 'src/rendering/caller.cpp').write_text('// execute_vertex_program(x,y);\nvoid f(){execute_vertex_program (x,y);}\nconst char* s="execute_vertex_program(x,y)";')
        calls = source_runtime_calls(root)
        (root / 'tests/reference').mkdir(parents=True)
        (root / 'tests/reference/vertex_program_reference.cpp').write_text('void execute_vertex_program(const X& x,Y& y){}')
        self.assertEqual(len(calls), 3)
        self.assertEqual({row['path'] for row in calls}, {'src/rendering/vertex_program.cpp', 'src/rendering/caller.cpp'})

    def test_runtime_codegen_and_instruction_analysis_are_rejected(self):
        root=self.owner/'codegen-source';(root/'src/ui').mkdir(parents=True)
        (root/'src/ui/menu_world_vertex_source.cpp').write_text(
            'X menu_world_vertex_source(const P& p){}')
        (root/'src/ui/runtime.cpp').write_text(
            'void f(){menu_world_vertex_source(p);menu_world_vertex_input_components(p,false);}\n'
            '// menu_world_vertex_shader_source(p,d);\n'
            'const char* ignored="menu_world_vertex_default_texture_w_mask(p)";')
        calls=source_runtime_calls(root)
        self.assertEqual([row['symbol'] for row in calls],
            ['menu_world_vertex_source','menu_world_vertex_input_components'])
        self.assertTrue(all(row['kind']=='runtime-vertex-program-analysis-or-codegen' for row in calls))

    def test_offline_archives_may_be_build_dependencies_but_never_runtime_links(self):
        graph=self.owner/'engine-targets.txt'
        graph.write_text('halo2_native|halo2_cache||src/app/main.cpp\n'
            'halo2_cache||halo2_vertex_program_codegen|src/ui/menu_world_vertex_catalog.cpp\n'
            'halo2_vertex_program_codegen|halo2_cache||src/ui/menu_world_vertex_source.cpp\n')
        self.assertEqual(production_archive_violations(graph),[])
        graph.write_text(graph.read_text().replace('halo2_native|halo2_cache|',
            'halo2_native|halo2_cache;$<LINK_ONLY:halo2_vertex_program_codegen>|'))
        rows=production_archive_violations(graph)
        self.assertEqual({row['kind'] for row in rows},
            {'offline-archive-linked-to-production','offline-source-linked-to-production'})


if __name__ == '__main__':
    unittest.main()
