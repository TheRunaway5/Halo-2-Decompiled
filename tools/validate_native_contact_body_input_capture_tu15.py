#!/usr/bin/env python3
"""Read-only interface model for the actual native body-update input hook."""
import argparse,json,os,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(exist_ok=False,parents=True);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  cpp=r'''#include "helpers/native_contact_body_update_input_capture_tu15.hpp"
#include <cassert>
using namespace halo2;
struct Memory:SourcePhysicsContactMemory{unsigned writes{};std::array<std::byte,16>b{};void read(std::uint32_t,std::span<std::byte>v)const override{std::fill(v.begin(),v.end(),std::byte{});}void write(std::uint32_t,std::span<const std::byte>)override{++writes;}template<class F>void visit_committed(F&&f)const{f(0x1000000u,std::span<const std::byte>(b));}};
int main(int argc,char**argv){assert(argc==2);Memory m;halo2::test::NativeContactBodyUpdateInputCaptureTu15 capture(argv[1]);unsigned observed{},dispatches{};NativeTu15ContactBodyUpdateInput input{0x10070000u,0x10071000u,0u,true,0x22220002u,7u,{0x27f,0x20,0x1fa0},0};
 {NativeTu15ContactBodyUpdateInputObserverBinding binding(m,[&](const auto&,const auto&i){++observed;assert(i.manager==input.manager&&i.row==input.row&&i.reversed&&i.body_slot==7&&i.floating_point.status==0x20&&i.preserved_x87_depth==0);capture.observe(i,m);});native_tu15_observe_contact_body_update_input(m,input);++dispatches;}
 native_tu15_observe_contact_body_update_input(m,input);assert(observed==1&&dispatches==1&&m.writes==0);std::ifstream in(std::filesystem::path(argv[1])/"capture-native-before.json");nlohmann::json j;in>>j;assert(j["input"]["owner"]==0x1d77d0u&&j["input"]["manager"]==input.manager&&j["input"]["row"]==input.row&&j["input"]["skip_impulse"]==0&&j["input"]["reversed"]==true&&j["input"]["component_datum"]==input.component_datum&&j["input"]["body_slot"]==7&&j["bank_bytes"]==16&&j["regions"].size()==1&&j["source_memory_mutated"]==false);}
''';source=scratch/'model.cpp';source.write_text(cpp);results=[]
  for c in('g++','clang++'):
   b=scratch/c.replace('+','x');cmd=[c,'-std=c++20','-O1','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),'-I'+str(stage/'tests'),str(source),'-o',str(b)];q=subprocess.run(cmd,capture_output=True,text=True);(a.output/(c.replace('+','x')+'.log')).write_text(q.stdout+q.stderr);assert q.returncode==0,q.stderr[-2000:];subprocess.run([str(b),str(scratch/('capture-'+c.replace('+','x')))],check=True);results.append(dict(compiler=c,command=cmd,passed=True))
  r.update(passed=True,compilers=results,physics_source_modified=False,native_admission_added=False,model_only=True,source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()})
 finally:r['allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',r)
 return 0
if __name__=='__main__':raise SystemExit(main())
