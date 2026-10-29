import json
import os
from pathlib import Path
import struct
import shutil
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
sys.path.insert(0,str(ROOT))
from audit_source_world_depth_position import SEEDS,audit,header,position_dag
from tests.test_native_world_vertex_kernel import token


def program(*instructions):
    values=[list(row) for row in instructions]
    for index,row in enumerate(values):row[3]=(row[3]&~1)|int(index+1==len(values))
    return b'\x78\x20'+struct.pack('<H',len(values))+b''.join(struct.pack('<4I',*row) for row in values)


class OriginalOrderedPosition(unittest.TestCase):
    def signature(self,*rows):return position_dag(program(*rows))['sha256']

    def test_dead_texture_output_does_not_change_position(self):
        base=token()
        unused=token(mac=2,a=(2,3),b=(3,18),reg=4,output=9)
        self.assertEqual(self.signature(base),self.signature(base,unused))

    def test_register_renaming_and_r12_output_alias_are_exact(self):
        expected=self.signature(token())
        for reg in (0,4,12):
            self.assertEqual(expected,self.signature(token(reg=reg,om=0),
                token(a=(1,reg),mm=0)))

    def test_order_sign_mask_and_reciprocal_are_not_simplified(self):
        left=token(mac=3,a=(1,0),c=(3,0))
        right=token(mac=3,a=(3,0),c=(1,0))
        self.assertNotEqual(self.signature(left),self.signature(right))
        original=token();negative=original.copy();negative[1]^=1<<8
        self.assertNotEqual(self.signature(original),self.signature(negative))
        self.assertNotEqual(self.signature(token(mac=0,ilu=2,mm=0,output_ilu=True)),
                            self.signature(token(mac=0,ilu=3,mm=0,output_ilu=True)))
        self.assertNotEqual(self.signature(token(om=15)),self.signature(token(om=14,mm=0)))

    def test_paired_results_read_old_register_before_writes(self):
        initial=token(a=(2,0),reg=1,om=0)
        paired=token(mac=1,ilu=1,a=(3,0),c=(1,1),reg=1,mm=15,im=15,output_ilu=True)
        self.assertEqual(self.signature(token()),self.signature(initial,paired))

    def test_source_contract_is_reproducible_and_missing_families_are_proved(self):
        registry=json.loads((ROOT/'shaders/native-programs/world/programs.json').read_bytes())
        programs,admitted=audit(registry)
        self.assertTrue(SEEDS<=admitted.keys())
        families={alias for row in admitted.values() for alias in row['aliases']}
        for name in ('transparent_generic_default','stencil_texture_secondary_detail_full_xform',
                     'lightmap_pervertex_env','lightmap_pervertex_tex_env','stencil_env_bumped_tangentspace'):
            self.assertIn('rasterizer\\vertex_shaders_xbox\\'+name,families)
        self.assertGreater(len(programs),len(admitted),'Unknown position paths must remain rejected')
        self.assertEqual((ROOT/'include/halo2/source_world_depth_position_contract.hpp').read_text(),header(admitted))
        path=Path(os.environ['HALO2_TEST_SCRATCH'])/'position-proof.json'
        path.write_text(json.dumps({'admitted':len(admitted),'unknown':len(programs)-len(admitted)}))

    def test_independent_water_requires_typed_pass_and_source_enum(self):
        compiler=shutil.which('c++')
        if not compiler:self.skipTest('C++ compiler unavailable')
        scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
        source=scratch/'water-scope.cpp';executable=scratch/'water-scope'
        source.write_text(r'''
#include "halo2/source_world_depth_scope.hpp"
#include <stdexcept>
using namespace halo2;
void check(bool value){if(!value)throw std::runtime_error("water role admission changed");}
int main(){
 Material m;m.forward_pass=true;m.forward_pass_index=0;m.passes.emplace_back();
 auto& p=m.passes[0];p.vertex_shader="rasterizer\\vertex_shaders_xbox\\water_pool";
 m.forward_vertex=7;check(!source_world_depth_independent_water(m));
 m.water_pass=true;check(source_world_depth_independent_water(m));
 m.forward_vertex=3;check(!source_world_depth_independent_water(m));
 p.vertex_shader="rasterizer\\vertex_shaders_xbox\\stencil_texture";
 check(!source_world_depth_independent_water(m));
 p.vertex_shader="rasterizer\\vertex_shaders_xbox\\water_edge_blend";
 check(source_world_depth_independent_water(m));
 m.forward_vertex=8;p.vertex_shader="rasterizer\\vertex_shaders_xbox\\water_static";
 check(source_world_depth_independent_water(m));
 p.implementation=1;check(!source_world_depth_independent_water(m));
 p.implementation=0;m.forward_pass_index=1;check(!source_world_depth_independent_water(m));
}
''')
        compiled=subprocess.run([compiler,'-std=c++20','-I'+str(ROOT/'include'),
            str(source),'-o',str(executable)],capture_output=True,text=True)
        self.assertEqual(compiled.returncode,0,compiled.stdout+compiled.stderr)
        result=subprocess.run([str(executable)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_every_admitted_program_matches_independent_cpp_reference(self):
        compiler=shutil.which('c++')
        if not compiler:self.skipTest('C++ compiler unavailable')
        registry=json.loads((ROOT/'shaders/native-programs/world/programs.json').read_bytes())
        _,admitted=audit(registry);ordered=sorted(admitted);indices={key:i for i,key in enumerate(ordered)}
        rows=[]
        for key in ordered:
            code=admitted[key]['code'][4:]
            rows.append('{'+','.join(str(v)+'u' for v in struct.unpack('<'+'I'*(len(code)//4),code))+'}')
        scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
        source=scratch/'position-reference.cpp';executable=scratch/'position-reference'
        source.write_text('''
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>
using namespace halo2;
int main(){
 const std::vector<std::vector<std::uint32_t>> words{
'''+',\n'.join(rows)+'''};
 const std::vector<unsigned> seeds{'''+','.join(str(indices[admitted[key]['seed']])+'u' for key in ordered)+'''};
 std::vector<VertexProgram> programs(words.size());
 for(unsigned n=0;n<words.size();++n){auto& p=programs[n];p.count=words[n].size()/4;
  for(unsigned i=0;i<p.count;++i)std::copy_n(words[n].data()+i*4,4,p.instructions[i].begin());}
 std::mt19937 random(0x53484144u);unsigned cases=0;
 for(unsigned n=0;n<programs.size();++n)for(unsigned sample=0;sample<128;++sample){
  VertexProgramState a;
  for(auto& vector:a.constants)for(auto& value:vector)value=float(int(random()%8193)-4096)/64.f;
  for(auto& vector:a.attributes)for(auto& value:vector)value=float(int(random()%8193)-4096)/64.f;
  a.attributes[0][3]=1.f;
  a.constants[8]={320,0,0,0};a.constants[9]={0,-240,0,0};
  a.constants[10]={0,0,16777215,0};a.constants[11]={320.5f,240.5f,0,0};
  if(sample%4==0)a.constants[3]={0,0,0,1};
  if(sample%8==0)a.attributes[0]={-0.f,0,0,1};
  if(sample==63)a.constants[3]={0,0,0,0};
  auto b=a;execute_vertex_program(programs[n],a);execute_vertex_program(programs[seeds[n]],b);
  if(std::memcmp(a.outputs[0].data(),b.outputs[0].data(),sizeof(ShaderVector))){
   std::cerr<<"Original position mismatch at program "<<n<<" sample "<<sample<<'\\n';return 1;}
  ++cases;
 }
 std::cout<<cases<<" independent original-reference position comparisons\\n";
}
''')
        command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off',
            '-I'+str(ROOT/'include'),str(source),str(ROOT/'tests/reference/vertex_program_reference.cpp'),'-o',str(executable)]
        result=subprocess.run(command,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        result=subprocess.run([str(executable)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertIn(str(len(admitted)*128),result.stdout)


if __name__=='__main__':unittest.main()
