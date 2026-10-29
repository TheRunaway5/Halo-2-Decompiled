"""Actual authored cull lists plus unchanged original SDK command consumers."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'tests/fixtures/source_world_depth_raster_tu15.json'


class AuthoredWorldDepthCull(unittest.TestCase):
    def test_actual_cache_apply_and_restore_have_distinct_source_owners(self):
        fixture=json.loads(FIXTURE.read_text())
        self.assertEqual({case['map'] for case in fixture['cases']},
                         {'ascension','dune','elongation','warlock'})
        rows=[]
        for case in fixture['cases']:
            apply=bytes.fromhex(case['apply_hex']);restore=bytes.fromhex(case['restore_hex'])
            self.assertEqual(len(apply)%5,0);self.assertEqual(len(restore)%5,0)
            self.assertIn(b'\x93\0\0\0\0',[apply[i:i+5] for i in range(0,len(apply),5)])
            self.assertEqual(restore,b'\x93\0\x09\0\0')
            rows.append('{'+','.join(str(v) for v in apply)+'}')
        compiler=shutil.which('c++')
        if not compiler:self.skipTest('C++ compiler unavailable')
        scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
        source=scratch/'authored-cull.cpp';binary=scratch/'authored-cull'
        source.write_text(r'''
#include "halo2/source_world_depth_raster_contract.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
using namespace halo2;
void check(bool value){if(!value)throw std::runtime_error("authored two-sided cull admission changed");}
int main(){
 const std::vector<std::vector<unsigned char>> lists{'''+','.join(rows)+r'''};
 for(const auto& bytes:lists){
  SourceWorldDepthCullState current;
  for(unsigned i=0;i<bytes.size();i+=5){std::uint32_t value=0;
   for(unsigned n=0;n<4;++n)value|=std::uint32_t(bytes[i+1+n])<<(n*8);
   check(current.apply(bytes[i],value));}
  check(current.cull()==0 && source_world_depth_cull_admitted(current.cull()));
  // +11C belongs to retirement, not the current material's draw state.
  SourceWorldDepthCullState retired(current.cull());
  check(retired.apply(147,0x900));check(retired.cull()==0x900);
 }
 SourceWorldDepthCullState ordinary;
 check(ordinary.cull()==0x900 && source_world_depth_cull_admitted(ordinary.cull()));
 for(auto state:{146u,149u,154u,155u})check(!ordinary.apply(state,0));
 for(auto value:{1u,0x901u,0x902u,0xffffffffu}){
  check(!source_world_depth_cull_admitted(value));check(!ordinary.apply(147,value));
  check(ordinary.cull()==0x900);
 }
 check(ordinary.apply(147,0));check(ordinary.apply(147,0x900));check(ordinary.cull()==0x900);
}
''')
        result=subprocess.run([compiler,'-std=c++20','-O2','-I'+str(ROOT/'include'),str(source),'-o',str(binary)],
                              capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_unchanged_original_cull_setter_disables_culling_for_zero(self):
        # The project's existing oracle environment owns the optional emulator.
        # No package install or global Python mutation is needed by this test.
        for site in sorted((ROOT/'.tools/python/lib').glob('python*/site-packages')):
            if (site/'unicorn/__init__.py').is_file():sys.path.insert(0,str(site))
        try:import unicorn
        except ModuleNotFoundError:self.skipTest('Unicorn oracle dependency unavailable')
        from unicorn import x86_const as x
        fixture=json.loads(FIXTURE.read_text())
        owner=ROOT/fixture['original_setter']['path']
        code=bytes.fromhex(fixture['original_setter']['machine_code_hex'])
        if owner.exists():self.assertEqual(owner.read_bytes(),code)
        self.assertEqual(hashlib.sha256(code).hexdigest(),fixture['original_setter']['sha256'])
        for argument,expected in ((0,[0x40308,0]),(0x900,[0x40308,1,0x4039c,0x404])):
            with self.subTest(argument=argument):
                machine=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
                for address,size in ((0x3ff000,4096),(0x411000,4096),(0x600000,0x4000)):
                    machine.mem_map(address,size)
                machine.mem_write(0x3ff4b0,code)
                def write(at,value):machine.mem_write(at,struct.pack('<I',value))
                def word(at):return struct.unpack('<I',machine.mem_read(at,4))[0]
                write(0x411568,0x600000);write(0x600000,0x601000);write(0x600004,0x601800)
                write(0x4117c0,0x900);write(0x4117c4,0x900)
                write(0x602800,0x603000);write(0x602804,argument)
                machine.reg_write(x.UC_X86_REG_ESP,0x602800)
                machine.emu_start(0x3ff4b0,0x603000,count=100)
                self.assertEqual(word(0x4117c4),argument)
                end=word(0x600000)
                self.assertEqual(list(struct.unpack('<'+'I'*((end-0x601000)//4),machine.mem_read(0x601000,end-0x601000))),expected)


if __name__=='__main__':unittest.main()
