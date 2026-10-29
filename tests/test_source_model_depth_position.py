"""Exact native model prefix/projection composition and GPU W-depth boundary.

The controlled arithmetic inputs are component services, not captured gameplay
or a physical Xbox framebuffer. Production instruction interpretation remains
absent: only the independent expected-value side links the CPU token reference.
"""
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
from audit_source_model_depth_position import audit, header
from audit_source_world_depth_position import audit as world_audit, sha
from native_cpu_vertex_kernel import _program, build_position_header
from native_world_vertex_kernel import lower_vertex_program


def receiver_rows(registry, report):
    keys = {item['program_sha256'] for item in report['admitted']
            if 'shadow_buffer_application_' in item['family']}
    return sorted((row for row in registry['cpu_vertex_programs'] if sha(_program(row)[0]) in keys),
                  key=lambda row: sha(_program(row)[0]))


def direct_glsl(words, name):
    # Use the production fixed ALU lowering, with an explicit raw-attribute
    # producer rather than pretending class1..6 is a world declaration.
    statements = lower_vertex_program(words, extended_vertex_bank=True)['statements']
    statements = statements[statements.index('    // Original instruction 0:'):]
    return 'void ' + name + '(){\n' + statements + '}\n'


class NativeModelDepth(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.registry = json.loads((ROOT / 'shaders/native-programs/world/programs.json').read_bytes())
        cls.proof = audit(cls.registry)
        cls.rows = receiver_rows(cls.registry, cls.proof)

    def test_complete_program_and_class_contract_is_reproducible(self):
        self.assertEqual((ROOT / 'include/halo2/source_model_depth_position_contract.hpp').read_text(),
                         header(self.registry, self.proof))
        actual = [item for item in self.proof['admitted'] if 'shadow_buffer_application_' in item['family']]
        self.assertEqual(len(actual), 18)
        self.assertEqual({item['class'] for item in actual}, set(range(1, 7)))
        self.assertTrue(self.proof['rejected'])
        self.assertTrue(all(item['world_programs'] for item in actual))
        compiler = shutil.which('c++')
        if not compiler:self.skipTest('C++ compiler unavailable')
        scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
        unit = scratch / 'model-position-identity.cpp'
        rows = []
        for item in actual:
            row = next(row for row in self.rows if sha(_program(row)[0]) == item['program_sha256'])
            code = _program(row)[0]
            rows.append('{'+str(item['class'])+', {'+','.join(map(str, code))+'}}')
        originals = {sha(_program(row)[0]):_program(row)[0] for row in self.registry['cpu_vertex_programs']}
        ordinary = []
        for item in self.proof['admitted'] + self.proof['rejected']:
            if 'program_sha256' not in item:continue
            code = originals[item['program_sha256']]
            expected = bool(item.get('world_programs') and item.get('matches_apply_prefix'))
            ordinary.append('{'+str(item['class'])+','+str(expected).lower()+', {'+','.join(map(str, code))+'}}')
        unit.write_text('''
#include "halo2/source_model_depth_position_contract.hpp"
#include <stdexcept>
#include <vector>
int main(){
 struct Row{unsigned classification;std::vector<unsigned char> code;};
 const std::vector<Row> rows{'''+','.join(rows)+'''};
 for(const auto& row:rows){
  const auto valid=[&](const auto& bytes,unsigned classification){return halo2::source_model_depth_private::position_program_admitted(std::as_bytes(std::span(bytes)),classification);};
  if(!valid(row.code,row.classification))throw std::runtime_error("Actual native receiver identity rejected");
  if(valid(row.code,0)||valid(row.code,7))throw std::runtime_error("Foreign class admitted");
  for(unsigned n=0;n<row.code.size();++n){auto changed=row.code;changed[n]^=128;
   if(valid(changed,row.classification))throw std::runtime_error("Changed original token admitted");}
 }
 struct Ordinary{unsigned classification;bool expected;std::vector<unsigned char> code;};
 const std::vector<Ordinary> ordinary{'''+','.join(ordinary)+'''};
 for(const auto& row:ordinary){
  const auto valid=[&](const auto& bytes){return halo2::source_model_depth_private::ordinary_position_program_admitted(std::as_bytes(std::span(bytes)),row.classification);};
  if(valid(row.code)!=row.expected)throw std::runtime_error("Native ordinary/Apply skin-prefix proof changed");
  for(unsigned index:{0u,unsigned(row.code.size()/2),unsigned(row.code.size()-1)}){
   auto changed=row.code;changed[index]^=128;if(valid(changed))throw std::runtime_error("Changed ordinary skin program admitted");}
 }
 for(unsigned classification=1;classification<=6;classification++){
  const auto expected=classification==1?1u:classification<4?3u:7u;
  if(halo2::source_model_depth_private::position_input_mask(classification)!=expected)throw std::runtime_error("Native skin input semantic owner changed");
 }
}
''')
        compiled = subprocess.run([compiler, '-std=c++20', '-O2', '-I'+str(ROOT/'include'),
                                   str(unit), '-o', str(scratch/'model-position-identity')], capture_output=True, text=True)
        self.assertEqual(compiled.returncode, 0, compiled.stdout+compiled.stderr)
        result = subprocess.run([str(scratch/'model-position-identity')], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)

    def test_native_prefix_cpu_and_gpu_projection_w_clip_d24(self):
        compiler, glslang, validator = [shutil.which(name) for name in ('c++', 'glslangValidator', 'spirv-val')]
        if not all((compiler, glslang, validator)):self.skipTest('Native compiler/Vulkan shader tools unavailable')
        scratch = Path(os.environ['HALO2_TEST_SCRATCH']) / 'model-depth-composition'
        scratch.mkdir()
        source_header, _ = build_position_header({'cpu_vertex_programs': self.rows})
        (scratch/'native_position_kernels.generated.hpp').write_text(source_header)
        _, world = world_audit(self.registry)
        world_code = world[sorted(world)[0]]['code']
        world_words = list(struct.iter_unpack('<4I', world_code[4:]))
        functions = [direct_glsl(_program(row)[1], 'raw'+str(n)) for n, row in enumerate(self.rows)]
        functions.append(direct_glsl(world_words, 'worldProgram'))
        shader = '''#version 450
#extension GL_GOOGLE_include_directive:require
layout(local_size_x=1) in;
struct Fixture{uvec4 meta;vec4 constants[192];vec4 attributes[48];vec4 positions[3];vec4 sampleClip;};
layout(set=0,binding=0,std430) readonly buffer Inputs{Fixture rows[];} source;
layout(set=0,binding=1,std430) writeonly buffer Outputs{vec4 values[];} result;
vec4 vpTemp[13],vpOut[16],vpInput[16];int vpAddress;uint current;
vec4 vpConstant(uint index){return source.rows[current].constants[index];}
vec4 vpProduct(vec4 a,vec4 b){return vec4(a.x==0.0||b.x==0.0?0.0:a.x*b.x,a.y==0.0||b.y==0.0?0.0:a.y*b.y,a.z==0.0||b.z==0.0?0.0:a.z*b.z,a.w==0.0||b.w==0.0?0.0:a.w*b.w);}
void initialize(uint lane){vpAddress=0;for(uint n=0;n<13;n++)vpTemp[n]=vec4(0);for(uint n=0;n<16;n++){vpOut[n]=vec4(0,0,0,1);vpInput[n]=source.rows[current].attributes[lane*16+n];}}
vec4 windowPosition(){vec4 p=vpOut[0];p.xy=trunc(p.xy*16.0)/16.0;p.w=(p.w<0.0?-1.0:1.0)*clamp(abs(p.w),5.421010862427522e-20,1.8446744073709552e19);return p;}
#include "world_depth_fragment.glsl"
'''+''.join(functions)+'''
void main(){current=gl_GlobalInvocationID.x;vec4 rawWindow[3],worldWindow[3];
 for(uint lane=0;lane<3;lane++){initialize(lane);switch(source.rows[current].meta.x){
'''+''.join('case '+str(n)+':raw'+str(n)+'();break;\n' for n in range(len(self.rows)))+'''}
 rawWindow[lane]=windowPosition();result.values[current*8+lane]=rawWindow[lane];
 initialize(lane);vpInput[0]=vec4(source.rows[current].positions[lane].xyz,1);worldProgram();
 worldWindow[lane]=windowPosition();result.values[current*8+3+lane]=worldWindow[lane];}
 float a=worldWDepth(source.rows[current].sampleClip.xy,vec2(1),rawWindow[0],rawWindow[1],rawWindow[2],0,0,0);
 float b=worldWDepth(source.rows[current].sampleClip.xy,vec2(1),worldWindow[0],worldWindow[1],worldWindow[2],0,0,0);
 result.values[current*8+6]=vec4(a,worldD24Depth(a),float(a>=source.rows[current].sampleClip.z&&a<=source.rows[current].sampleClip.w),0);
 result.values[current*8+7]=vec4(b,worldD24Depth(b),float(b>=source.rows[current].sampleClip.z&&b<=source.rows[current].sampleClip.w),0);
}
'''
        (scratch/'model-depth.comp').write_text(shader)
        compiled = subprocess.run([glslang, '-V', '--target-env', 'vulkan1.1', '-I'+str(ROOT/'shaders'),
                                   str(scratch/'model-depth.comp'), '-o', str(scratch/'model-depth.spv')], capture_output=True, text=True)
        self.assertEqual(compiled.returncode, 0, compiled.stdout+compiled.stderr)
        valid = subprocess.run([validator, '--target-env', 'vulkan1.1', str(scratch/'model-depth.spv')], capture_output=True, text=True)
        self.assertEqual(valid.returncode, 0, valid.stdout+valid.stderr)
        words = []
        for row in self.rows:
            words.append('{'+','.join(str(value)+'u' for token in _program(row)[1] for value in token)+'}')
        world_words_flat = [value for token in world_words for value in token]
        cpp = r'''
#include "gpu_compute.hpp"
#include "halo2/pstencil/geometry.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <random>
using namespace halo2;
struct alignas(16) Fixture{std::array<unsigned,4> meta;std::array<ShaderVector,192> constants;std::array<ShaderVector,48> attributes;std::array<ShaderVector,3> positions;ShaderVector sampleClip;};
static_assert(sizeof(Fixture)==3920);
void need(bool value,const char* message){if(!value)throw FormatError(message);}
VertexProgram program(const std::vector<unsigned>& words){VertexProgram p;p.count=words.size()/4;for(unsigned n=0;n<p.count;n++)std::copy_n(words.data()+n*4,4,p.instructions[n].begin());return p;}
bool bits(ShaderVector a,ShaderVector b){return std::memcmp(a.data(),b.data(),sizeof(a))==0;}
float area(ShaderVector a,ShaderVector b,ShaderVector c){
 const float ax=b[0]-a[0],ay=b[1]-a[1],bx=c[0]-a[0],by=c[1]-a[1];
 const volatile float cd=ay*bx;const float error=std::fma(-ay,bx,cd);
 const volatile float first=std::fma(ax,by,-cd);return first+error;
}
float wdepth(ShaderVector sample,ShaderVector p0,ShaderVector p1,ShaderVector p2){
 float b0=area(sample,p1,p2)/p0[3],b1=area(sample,p2,p0)/p1[3],b2=area(sample,p0,p1)/p2[3];
 const volatile float sum=b0+b1;float inverse=1.f/(sum+b2);if(std::isinf(inverse))inverse=0;
 b1*=inverse;b2*=inverse;const volatile float one=b1*(p1[3]-p0[3]),two=b2*(p2[3]-p0[3]);
 const volatile float delta=one+two;float depth=p0[3]+delta;
 if(!(depth>0)||std::isnan(depth))depth=std::bit_cast<float>(0x7f7fffffu);return depth;
}
int main(int argc,char** argv){try{
 need(argc==2,"Supply compiled exact model projection component shader");
 const std::vector<std::vector<unsigned>> words{RAW_WORDS};
 const VertexProgram world=program({WORLD_WORDS});std::vector<Fixture> fixtures;
 std::mt19937 random(0x4d444550);unsigned cpu=0,negative=0;std::vector<std::array<ShaderVector,3>> expected;
 for(unsigned p=0;p<words.size();p++)for(unsigned sample=0;sample<48;sample++){
  Fixture f{};f.meta[0]=p;for(auto& c:f.constants)for(auto& lane:c)lane=float(int(random()%2049)-1024)/1024.f;
  f.constants[0]={0,-16777215.f/1024,0,0};f.constants[1]={0,0,-16777215.f/1024,0};f.constants[2]={1,0,0,-.05f};f.constants[3]={16777215.f/1024,0,0,0};
  f.constants[8]={320,0,0,0};f.constants[9]={0,-240,0,0};f.constants[10]={0,0,16777215,0};f.constants[11]={320.5f,240.5f,0,0};
  // Forty actual c50..169 matrix slots. No generic skinning substitutes the
  // fixed raw program's own rigid/relative/weighted palette arithmetic.
  for(unsigned matrix=0;matrix<40;matrix++){
   f.constants[50+matrix*3]={1,0,0,float(matrix%4)/8};
   f.constants[51+matrix*3]={0,1,0,float(matrix%3)/16};
   f.constants[52+matrix*3]={0,0,1,float(matrix%5)/32};}
  std::array<ShaderVector,3> window;
  for(unsigned lane=0;lane<3;lane++){
   auto& a=f.attributes[lane*16];for(unsigned input=0;input<16;input++)for(auto& x:f.attributes[lane*16+input])x=float(int(random()%33)-16)/16.f;
   const float x=sample<4?std::array<float,4>{.049999f,.050001f,1023.999f,1024.001f}[sample]:2.f+float(sample)/4.f+float(lane)/64.f;
   a={x,float(int(lane)-1)/4.f,float(lane==1?1:-1)/4.f,1};
   f.attributes[lane*16+1]={0,1.f/255,2.f/255,3.f/255};
   f.attributes[lane*16+2]={.4f,.3f,.2f,.1f};
   VertexProgramState source;source.constants=f.constants;std::copy_n(f.attributes.data()+lane*16,16,source.attributes.begin());
   auto native=source;execute_vertex_program(program(words[p]),source);
   pstencil::execute_native_position_prefix(program(words[p]),native);f.positions[lane]=native.outputs[9];
   auto composed=native;std::copy_n(f.attributes.data()+lane*16,16,composed.attributes.begin());
   composed.attributes[0]={f.positions[lane][0],f.positions[lane][1],f.positions[lane][2],1};execute_vertex_program(world,composed);
   need(bits(source.outputs[0],composed.outputs[0]),"Native fixed prefix + WORLD projection differs from original complete raw program");
   window[lane]=source.outputs[0];for(unsigned axis=0;axis<2;axis++)window[lane][axis]=std::trunc(window[lane][axis]*16.f)/16.f;
   window[lane][3]=std::copysign(std::clamp(std::abs(window[lane][3]),0x1p-64f,0x1p64f),window[lane][3]);
   // A one-ULP changed pose cannot acquire equality merely through its model
   // identity. The test's source token reference is not a production route.
   if(sample==16){auto wrong=composed;wrong.attributes[0][0]=std::nextafter(wrong.attributes[0][0],INFINITY);execute_vertex_program(world,wrong);negative+=!bits(source.outputs[0],wrong.outputs[0]);}
   ++cpu;
  }
  f.sampleClip={320.5f,240.5f,.05f/1024.f*16777215.f,16777215.f};fixtures.push_back(f);expected.push_back(window);
 }
 need(negative>0,"Foreign palette/pose negative control was insensitive");
 halo2_test::Compute<Fixture,8> gpu(argv[1]);const auto actual=gpu.run(fixtures);unsigned clipped=0,visible=0,z_rounding=0;
 for(unsigned n=0;n<fixtures.size();n++){
  for(unsigned lane=0;lane<3;lane++)if(!bits(actual[n*8+lane],actual[n*8+3+lane])){
   std::cerr<<"GPU exact original/composed position differs fixture="<<n<<" program="<<fixtures[n].meta[0]<<" vertex="<<lane<<'\n';return 2;}
  for(unsigned lane=0;lane<3;lane++) {
   for(unsigned axis:{0u,1u,3u})need(std::bit_cast<unsigned>(actual[n*8+lane][axis])==std::bit_cast<unsigned>(expected[n][lane][axis]),
      "GPU snapped XY/W differs from independent CPU token reference");
   // The renderer enables native W clipping/depth clamp. Camera Z is not
   // the receiver depth operand; retain its host reciprocal rounding facts
   // separately, while requiring full raw/composed GPU oPos equality above.
   z_rounding+=std::bit_cast<unsigned>(actual[n*8+lane][2])!=std::bit_cast<unsigned>(expected[n][lane][2]);
  }
  need(bits(actual[n*8+6],actual[n*8+7]),"GPU ordered triangle W/clip/integer D24 differs");
  const auto& depth=actual[n*8+6];need(std::isfinite(depth[0])&&std::isfinite(depth[1]),"Nonfinite actual GPU W-depth");
  const float expectedW=wdepth(fixtures[n].sampleClip,expected[n][0],expected[n][1],expected[n][2]);
  const float expectedD24=std::bit_cast<float>(std::bit_cast<unsigned>(std::floor(expectedW)/16777216.f)+1u);
  need(std::bit_cast<unsigned>(depth[0])==std::bit_cast<unsigned>(expectedW),"GPU ordered W differs from independent scalar determinant/FMA reference");
  need(std::bit_cast<unsigned>(depth[1])==std::bit_cast<unsigned>(expectedD24),"GPU integer D24 differs at original floor/ULP boundary");
  if(depth[2]>0)++visible;else ++clipped;
 }
 if(!clipped || !visible)std::cerr<<"clip diagnostic accepted="<<visible<<" clipped="<<clipped
  <<" firstW="<<actual[0][3]<<" firstDepth="<<actual[6][0]<<" laterW="<<actual[4*8][3]<<" laterDepth="<<actual[4*8+6][0]<<'\n';
 need(clipped && visible,"GPU near/far W clip controls did not exercise both outcomes");
 std::cout<<"PASS "<<cpu<<" independent complete-program CPU comparisons; "<<fixtures.size()<<" real GPU model triangle W/clip/D24 comparisons; "<<negative<<" changed-pose controls; "<<visible<<" accepted / "<<clipped<<" clipped; CPU/GPU unused camera-Z rounding differences="<<z_rounding<<"\n";
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
'''.replace('RAW_WORDS', ','.join(words)).replace('WORLD_WORDS', ','.join(str(v)+'u' for v in world_words_flat))
        unit = scratch/'model-depth.cpp';unit.write_text(cpp)
        binary = scratch/'model-depth'
        compiled = subprocess.run([compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
            '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections', '-I'+str(ROOT/'include'),
            '-I'+str(ROOT/'tests'), '-I'+str(scratch), str(unit), str(ROOT/'src/pstencil/geometry.cpp'),
            str(ROOT/'tests/reference/vertex_program_reference.cpp'), '-lvulkan', '-o', str(binary)], capture_output=True, text=True)
        self.assertEqual(compiled.returncode, 0, compiled.stdout+compiled.stderr)
        result = subprocess.run([str(binary), str(scratch/'model-depth.spv')], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertNotIn('Validation Error', result.stderr)
        print(result.stdout.strip())


if __name__ == '__main__':unittest.main()
