"""Build-time world vertex lowering admission and compiled-code regressions."""
import os
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from native_world_vertex_kernel import lower_vertex_program


def token(mac=1, ilu=0, a=(2, 0), b=(2, 0), c=(2, 0),
          reg=0, mm=15, im=0, output=0, om=15, output_ilu=False, relative=False):
    # Literal NV2A fields, independently used by the retained C++ decoder.
    t = [0, (mac << 21) | (ilu << 25) | 0x1b, (0x1b << 17) | (0x1b << 2), 1]
    for which, (mux, index) in enumerate((a, b, c)):
        if mux == 2:
            t[1] |= index << 9
        if mux == 3:
            t[1] |= index << 13
        if which == 0:
            t[2] |= (mux << 26) | ((index if mux == 1 else 0) << 28)
        elif which == 1:
            t[2] |= (mux << 11) | ((index if mux == 1 else 0) << 13)
        else:
            t[3] |= mux << 28
            if mux == 1:
                t[2] |= index >> 2
                t[3] |= (index & 3) << 30
    t[3] |= (reg << 20) | (mm << 24) | (im << 16) | (om << 12) | (output << 3)
    if om:
        t[3] |= 2048
    if output_ilu:
        t[3] |= 4
    if relative:
        t[3] |= 2
    return t


def pack(values):
    return [sum(values[i * 4 + j] << (j * 8) for j in range(4)) for i in range(4)]


class NativeWorldVertex(unittest.TestCase):
    def test_fixed_declaration_and_constant_attributes(self):
        declaration = [128, 3, 4, 5, 6, 8, 9, 14] + [255] * 8
        a = lower_vertex_program([token()], declaration=pack(declaration))
        b = lower_vertex_program([token()], declaration=declaration)
        self.assertEqual(a, b)
        self.assertIn('vpInput[0]=vec4(position,1.0);', a['glsl'])
        self.assertIn('vpInput[7]=vec4(color,1.0);', a['glsl'])
        self.assertIn('vpInput[10]=material.vertexAttributes[10];', a['glsl'])
        for forbidden in ('vertexInstructions', 'vertexDeclaration', 'flags.w',
                          'vpSource(', 'vpWrite(', 'for(', 'while(', 'switch('):
            self.assertNotIn(forbidden, a['glsl'])

    def test_legacy_inputs_retain_actual_draw_controls(self):
        a = lower_vertex_program([token()], declaration=[0xffffffff] * 4,
                                 extended_vertex_bank=True)
        self.assertIn('vpInput[6]=vec4(lightmapCoord,0.0,1.0);', a['glsl'])
        self.assertIn('if(vertexBank.range.z!=0u)', a['glsl'])
        self.assertIn('if(material.flags.z==4u)vpInput[6]=vec4(color,1.0);', a['glsl'])
        self.assertIn('if(material.flags.z==12u)vpInput[11]=vec4(color,sourceColorAlpha);', a['glsl'])

    def test_paired_operands_snapshot_before_any_write(self):
        a = lower_vertex_program([token(mac=1, ilu=1, a=(1, 1), c=(1, 0), im=15)])['glsl']
        self.assertIn('native_a0=(1.0*vpTemp[1].xyzw)', a)
        self.assertIn('native_c0=(1.0*vpTemp[0].xyzw)', a)
        self.assertLess(a.index('native_c0='), a.index('vpTemp[0].xyzw='))
        self.assertLess(a.index('native_l0=native_c0;'), a.index('vpTemp[0].xyzw='))
        self.assertIn('vpTemp[1].xyzw=native_l0.xyzw;', a)

    def test_masked_writes_and_paired_r1_suppression(self):
        a = lower_vertex_program([token(mac=2, ilu=1, reg=1, mm=10, im=5, om=9,
                                       output=9, output_ilu=True)])
        self.assertNotIn('vpTemp[1].xz=native_m0.xz;', a['glsl'])
        self.assertIn('vpTemp[1].yw=native_l0.yw;', a['glsl'])
        self.assertIn('vpOut[9].xw=native_l0.xw;', a['glsl'])
        self.assertEqual(a['default_texture_w_mask'], 14)

    def test_position_temporary_alias_is_static(self):
        a = lower_vertex_program([token(a=(1, 12), reg=12, om=0)])['glsl']
        self.assertIn('native_a0=(1.0*vpOut[0].xyzw)', a)
        self.assertIn('vpOut[0].xyzw=native_m0.xyzw;', a)

    def test_strict_dot_and_reciprocal_boundaries(self):
        for mac in (5, 6, 7):
            a = lower_vertex_program([token(mac=mac)])['glsl']
            self.assertIn('precise float native_dot0=(native_product0.x+native_product0.y)+native_product0.z;', a)
            self.assertNotIn('dot(', a)
            self.assertEqual('native_dot0+=native_b0.w;' in a, mac == 6)
            self.assertEqual('native_dot0+=native_product0.w;' in a, mac == 7)
        a = lower_vertex_program([token(mac=0, ilu=3, mm=0, im=15, output_ilu=True)])['glsl']
        self.assertIn('floatBitsToUint(native_reciprocal0)==0u', a)
        self.assertIn('else if(!isnan(native_reciprocal0))', a)
        self.assertIn('-1.8446744073709552e19,-5.421010862427522e-20', a)

    def test_relative_address_is_native_index_arithmetic(self):
        address = token(mac=13, mm=0, om=0); address[3] &= ~1
        a = lower_vertex_program([address, token(a=(3, 50), relative=True)],
                                 extended_vertex_bank=True)
        self.assertIn('vpAddress=int(floor(native_a0.x));', a['glsl'])
        self.assertIn('vpConstant(uint((int(50)+vpAddress)))', a['glsl'])
        self.assertTrue(a['relative_constants'])
        self.assertEqual(a['consumed_constants'], list(range(192)))

    def test_reject_unowned_or_malformed_programs(self):
        bad_rows = ([], [[0, 1, 2]], [[0, -1, 0, 1]], [[0, True, 0, 1]],
                    [token(mac=14)], [token(a=(0, 0))], [token(a=(1, 13))],
                    [token(a=(3, 192))], [token(reg=13)], [token(output=16)],
                    [token(a=(3, 18), relative=True)], [token(mac=13, om=0, mm=0)])
        for rows in bad_rows:
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                lower_vertex_program(rows)
        for decl in ([0xffffffff], [255] + [0] * 15, pack([128, 19] + [255] * 14),
                     pack([128, 250] + [255] * 14)):
            with self.subTest(decl=decl), self.assertRaises(ValueError):
                lower_vertex_program([token()], declaration=decl)
        wrong = token(); wrong[3] &= ~1
        with self.assertRaises(ValueError):
            lower_vertex_program([wrong])

    def test_complete_actual_water_and_sky_vertex_registry_compiles(self):
        from compile_native_world_kernels import validate_entry, vertex_source, spirv_contract
        registry = json.loads((ROOT / 'shaders/native-programs/world/programs.json').read_bytes())
        self.assertEqual(registry['schema'], 'HALO2_NATIVE_WORLD_KERNEL_PROGRAMS_V1')
        rows = [validate_entry(entry) for entry in registry['entries']]
        self.assertGreaterEqual(len(rows), 14)
        kinds = {alias['kind'] for entry in registry['entries'] for alias in entry['aliases']}
        self.assertTrue({'water', 'sky'} <= kinds)
        unique, operations = {}, set()
        for row in rows:
            source, lowered = vertex_source(ROOT / 'shaders', row)
            self.assertEqual(lowered['instruction_count'], struct.unpack_from('<H', row['program'], 2)[0])
            self.assertEqual(lowered['declaration_bytes'], row['semantics'])
            operations.update(lowered['operations'])
            if row['vertex_key'] in unique:
                self.assertEqual(unique[row['vertex_key']], source)
            unique[row['vertex_key']] = source
        # Actual water/pool owners require SGE, RCP and paired LIT; limiting
        # this test to the earlier title-menu arithmetic corridor misses them.
        self.assertTrue({(12, 0), (1, 2), (2, 7)} <= operations)
        tools = [shutil.which(x) for x in ('glslangValidator', 'spirv-opt', 'spirv-val')]
        if not all(tools):
            self.skipTest('Native Vulkan build tools unavailable')
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as folder:
            for key, source in unique.items():
                path = Path(folder) / (key + '.vert');path.write_text(source)
                raw, output = path.with_suffix('.raw.spv'), path.with_suffix('.spv')
                subprocess.run([tools[0], '-V', '--target-env', 'vulkan1.3', '-I' + str(ROOT / 'shaders'),
                                str(path), '-o', str(raw)], check=True, capture_output=True)
                subprocess.run([tools[1], '--target-env=vulkan1.3', '-O', str(raw), '-o', str(output)],
                               check=True, capture_output=True)
                subprocess.run([tools[2], '--target-env', 'vulkan1.3', str(output)], check=True, capture_output=True)
                contract = spirv_contract(output.read_bytes(), 'vert')
                self.assertTrue(contract['no_runtime_instruction_reads'])
                self.assertTrue(contract['no_runtime_declaration_reads'])
                self.assertTrue(contract['no_runtime_program_control_reads'])

    def test_compiled_native_code_has_no_instruction_loop_or_dynamic_decoder(self):
        compiler, validator = shutil.which('glslangValidator'), shutil.which('spirv-val')
        if not compiler or not validator:
            self.skipTest('Native shader build tools unavailable')
        functions = []
        for mac in range(14):
            rows = [token(mac=mac, mm=0 if mac == 13 else 15)]
            functions.append(lower_vertex_program(rows, extended_vertex_bank=True)['glsl'])
        for ilu in range(1, 8):
            functions.append(lower_vertex_program([token(mac=0, ilu=ilu, mm=0, im=15,
                                                         output_ilu=True)], extended_vertex_bank=True)['glsl'])
        prelude = '''#version 450
layout(set=0,binding=4,std140) uniform Material {vec4 vertexAttributes[16];uvec4 flags;uvec4 sourceCameraControl;} material;
layout(set=0,binding=7,std140) uniform Bank {uvec4 range;vec4 constants[192];} vertexBank;
vec3 position=vec3(1.0),texcoord3=vec3(0.0),normal=vec3(0.0,0.0,1.0),tangent=vec3(1.0,0.0,0.0),binormal=vec3(0.0,1.0,0.0),incident=vec3(1.0),color=vec3(1.0);
vec2 texcoord=vec2(0.0),lightmapCoord=vec2(0.0);float sourceColorAlpha=1.0,prtCluster=0.0;vec4 prtWeights=vec4(1.0);
vec4 vpTemp[13],vpOut[16],vpInput[16];int vpAddress;
vec4 vpConstant(uint i){return vertexBank.constants[i];}
vec4 vpProduct(vec4 a,vec4 b){return vec4(a.x==0.0||b.x==0.0?0.0:a.x*b.x,a.y==0.0||b.y==0.0?0.0:a.y*b.y,a.z==0.0||b.z==0.0?0.0:a.z*b.z,a.w==0.0||b.w==0.0?0.0:a.w*b.w);}
'''
        source = prelude + '\n'.join(f.replace('runVertexProgram()', f'native{i}()') for i, f in enumerate(functions))
        source += '\nvoid main(){vec4 total=vec4(0.0);\n' + ''.join(f'native{i}();total+=vpOut[0];\n' for i in range(len(functions))) + 'gl_Position=total;}\n'
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as directory:
            path = Path(directory) / 'kernels.vert'; path.write_text(source)
            spv = path.with_suffix('.spv')
            subprocess.run([compiler, '-V', '--target-env', 'vulkan1.3', str(path), '-o', str(spv)], check=True, capture_output=True)
            subprocess.run([validator, '--target-env', 'vulkan1.3', str(spv)], check=True, capture_output=True)
            words = struct.unpack('<' + 'I' * (spv.stat().st_size // 4), spv.read_bytes())
            opcodes = []
            at = 5
            while at < len(words):
                count, opcode = words[at] >> 16, words[at] & 65535
                self.assertGreater(count, 0)
                opcodes.append(opcode); at += count
            self.assertNotIn(246, opcodes)  # OpLoopMerge
            self.assertNotIn(251, opcodes)  # OpSwitch
            self.assertNotIn(148, opcodes)  # OpDot: preserve ordered zero-aware products.


if __name__ == '__main__':
    unittest.main()
