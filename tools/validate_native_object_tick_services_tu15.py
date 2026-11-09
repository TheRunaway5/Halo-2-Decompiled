#!/usr/bin/env python3
"""B8F40 native service/counter protocol controls, separate from physical proof."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-object-tick-stage-20261102/staged'
DRIVER=r'''
#include "halo2/native_tu15_object_tick.hpp"
#include <array>
#include <cstring>
#include <map>
#include <stdexcept>
#include <iostream>
using namespace halo2;
struct M:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::byte>b;
 void read(std::uint32_t p,std::span<std::byte>v)const override{for(auto&x:v)x=b.at(p++);}
 void write(std::uint32_t p,std::span<const std::byte>v)override{for(auto x:v)b.at(p++)=x;}
 void put(std::uint32_t p,std::uint32_t x,unsigned n=4){for(unsigned i=0;i<n;++i)b[p+i]=std::byte(x>>(i*8));}
 std::uint32_t get(std::uint32_t p,unsigned n=4)const{std::uint32_t x{};for(unsigned i=0;i<n;++i)x|=std::uint32_t(b.at(p+i))<<(i*8);return x;}
};
void check(bool b){if(!b)throw std::runtime_error("Native tick service protocol differs");}
M fixture(bool first){M m;m.put(0x51daf0,0x1000);m.put(0x4eabc4,0x6000);m.put(0x2d80,first?1:0);m.put(0x6003,0xa5,1);m.put(0x6013,0x5a,1);m.put(0x4ecbd0,0x3000);m.put(0x3038,0);m.put(0x3044,0x4000);m.put(0x2d00,0x80010001);m.put(0x400e,0x27,1);return m;}
int main(){unsigned done{};
 for(unsigned mode=0;mode<8;++mode){
  auto m=fixture(mode!=0&&mode!=5&&mode!=6);NativeTu15ObjectTickStats s;NativeTu15PhysicsFrameFp fp{0x27f,0x4520,0x1fa0};
  NativeTu15ObjectTickServices services;unsigned updates{},finalizations{},events{};
  if(mode!=1)services.update_object=[&](std::uint32_t datum,auto&){check(datum==0x80010001);++updates;if(mode==2)throw std::runtime_error("Declared update refusal");if(mode==7)m.put(0x4eabc4,0x6010);};
  if(mode!=3)services.finalize_object=[&](std::uint32_t datum,auto&){check(datum==0x80010001);++finalizations;if(mode==4)throw std::runtime_error("Declared finalizer refusal");};
  SourcePhysicsContactRecordObserver observer=[&](const auto&e){++events;check(e.before.size()==1&&e.written.size()==1);if((mode==5&&events==1)||(mode==6&&events==2))throw std::runtime_error("Declared poststore observation refusal");};
  bool failed{};try{native_tu15_tick_objects(m,fp,services,observer,&s);}catch(const std::runtime_error&){failed=true;}
  const bool success=mode==0||mode==7;check(failed!=success);check(s.attempts==1&&s.completed==unsigned(success)&&s.refusals==unsigned(!success));
  check(s.unbound_owner==(mode==1?0xbe4a0u:mode==3?0x10e630u:0u));check(fp.control_word==0x27f&&fp.status==0x4520&&fp.mxcsr==0x1fa0);
  check(m.get(0x6003,1)==(mode==0||mode==6?0u:1u));if(mode==7)check(m.get(0x6013,1)==0&&s.updates==1&&s.finalizations==1);
  check(s.updates==updates&&s.finalizations==finalizations);++done;
 }
 std::cout<<done<<" native service/counter protocol controls passed; callbacks are declared controls, not native child qualification\n";
}
'''
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(owner),retained_large_artifacts=[],scope=__doc__)
 try:
  source=owner/'driver.cpp';source.write_text(DRIVER);results=[]
  dependencies=[STAGE/'src/physics/native_tu15_object_tick.cpp',STAGE/'include/halo2/native_tu15_object_tick.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp'];pins={str(x.relative_to(ROOT)):sha(x)for x in dependencies}
  for cc in('g++','clang++'):
   binary=owner/cc;cmd=[cc,'-std=c++20','-O1','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(STAGE/'include'),'-I'+str(ROOT/'include'),str(source),str(dependencies[0]),'-o',str(binary)]
   build=subprocess.run(cmd,capture_output=True,text=True);(a.output/(cc+'-build.log')).write_text(build.stdout+build.stderr);assert build.returncode==0,build.stderr
   run=subprocess.run([str(binary)],capture_output=True,text=True);(a.output/(cc+'.log')).write_text(run.stdout+run.stderr);assert run.returncode==0,run.stdout+run.stderr;results.append(dict(compiler=cc,passed=True,controls=8))
  assert all(sha(ROOT/x)==h for x,h in pins.items());report.update(passed=True,source_inputs_unchanged=True,results=results,source_sha256=pins,generated_driver_sha256=sha(source),actual_child_closure_qualified=False)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(x.stat().st_blocks*512 for x in owner.rglob('*')if x.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
