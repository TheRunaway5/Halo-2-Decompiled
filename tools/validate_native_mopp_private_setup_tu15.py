#!/usr/bin/env python3
"""Exact public-path requalification plus explicit unprovided-DATA view guards."""
import argparse,inspect,json,os,subprocess,sys
from pathlib import Path
import validate_native_mopp_ray_setup_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';owned=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[])
 try:
  text=inspect.getsource(parent.main).replace("source=ROOT/'src/physics/native_tu15_mopp_ray_setup.cpp'","source=STAGE/'src/physics/native_tu15_mopp_ray_setup.cpp'")
  text=text.replace('RETURN_CHECK=check,NATIVE_SERVICES=native_services)','RETURN_CHECK=check,NATIVE_SERVICES=native_services,STAGE=STAGE)')
  arguments=sys.argv;sys.argv=[arguments[0],'--output',str(a.output/'public')]
  try:namespace=dict(parent.__dict__,__file__=__file__,STAGE=stage);exec(text,namespace);assert namespace['main']()==0
  finally:sys.argv=arguments
  cpp=r'''#include "halo2/native_tu15_mopp_private_ray_setup.hpp"
#include <algorithm>
#include <cassert>
#include <map>
using namespace halo2;
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>>banks;mutable unsigned reads{},vectors{};unsigned writes{};
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);assert(i!=banks.begin());--i;assert(std::uint64_t(p-i->first)+n<=i->second.size());return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{++reads;auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{++vectors;auto i=at(p,16);std::copy_n(i->second.begin()+p-i->first,16,b.begin());}
 void write(std::uint32_t p,std::span<const std::byte>b)override{++writes;auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
 void put(std::uint32_t p,std::uint32_t n){auto i=at(p,4);for(unsigned k=0;k<4;++k)banks.at(i->first)[p-i->first+k]=std::byte(n>>(k*8u));}
};
int main(){
 constexpr std::uint32_t cache=0x1000100,ray=0x1000300,code=0x1000500,collector=0x1000700;
 Memory m;m.banks.emplace(0x1000000,std::vector<std::byte>(0x1000,std::byte{0xa5}));m.banks.emplace(0x46a0d0,std::vector<std::byte>(4));m.banks.emplace(0x46a1bc,std::vector<std::byte>(4));m.put(0x46a0d0,0x3f800000);m.put(0x46a1bc,0x3f800000);
 for(unsigned n=0;n<8;++n)m.put(ray+n*4,n%4==3?0u:0x3f000000u);m.put(ray+36,0);for(unsigned n=0;n<4;++n)m.put(code+n*4,n==3?0x3f800000u:0u);m.put(collector+4,0x3f800000);
 NativeTu15MoppPrivateRayActivity activity;NativeTu15MoppPrivateRayMemory view(m,ray,cache,activity);std::array<std::byte,16>b{};
 for(auto p:{ray+32,ray+33,cache+64,cache+65}){const auto reads=m.reads;bool failed{};try{view.read(p,std::span(b).first(1));}catch(const std::runtime_error&){failed=true;}assert(failed&&m.reads==reads);}
 for(auto p:{cache+60,ray+28}){const auto reads=m.vectors;bool failed{};try{view.read_vector(p,b);}catch(const std::runtime_error&){failed=true;}assert(failed&&m.vectors==reads);}
 for(auto p:{ray+32,cache+64,cache+61}){const auto writes=m.writes;bool failed{};try{view.write(p,std::span<const std::byte>(b).first(4));}catch(const std::runtime_error&){failed=true;}assert(failed&&m.writes==writes);}
 view.read(ray+28,std::span(b).first(4));view.read(ray+36,std::span(b).first(4));view.read_vector(cache+48,b);view.write(cache+68,std::span<const std::byte>(b).first(4));assert(activity.unprovided_reads==6&&activity.unprovided_writes==3);
 for(unsigned action=0;action<3;++action){
  auto before=m;NativeTu15MoppPrivateRayActivity out;NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};NativeTu15MoppPrivateRayServices service;bool child{},failed{};
  service.traverse=[&](SourcePhysicsContactMemory&guarded,auto received_cache,auto received_code,const auto&v,const auto&r,auto&,auto depth){assert(&guarded!=&m&&received_cache==cache&&received_code==code+0x30&&depth==0&&v.fields10_1c[1]==0x3f800000);child=true;std::array<std::byte,16>bytes{};guarded.read_vector(cache+0x30,bytes);if(action==1)guarded.read(cache+0x40,std::span(bytes).first(4));if(action==2)guarded.read(ray+0x20,std::span(bytes).first(4));(void)r;};
  try{native_tu15_prepare_private_mopp_ray(m,cache,0x1000900,code,ray,0x1000a00,collector,fp,0,service,{},&out);}catch(const std::runtime_error&){failed=true;}
  assert(child&&failed==(action!=0)&&out.attempts==1&&out.completed==(action==0)&&out.refusals==(action!=0)&&out.unprovided_reads==(action!=0));
  std::array<std::byte,4>now{},old{};m.read(cache+0x40,now);before.read(cache+0x40,old);assert(now==old);m.read(ray+0x20,now);before.read(ray+0x20,old);assert(now==old);
 }
}
''';model=owned/'guard.cpp';model.write_text(cpp);results=[]
  for compiler in('g++','clang++'):
   binary=owned/('guard-'+compiler);cmd=[compiler,'-std=c++20','-O1','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(stage/'include'),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src/physics'),str(model),str(stage/'src/physics/native_tu15_mopp_ray_setup.cpp'),str(ROOT/'src/physics/native_tu15_instanced_ray_lifetime.cpp'),str(ROOT/'src/physics/native_tu15_island_activation.cpp'),'-o',str(binary)];q=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(q.stdout+q.stderr);assert q.returncode==0,q.stderr[-2500:];q=subprocess.run([str(binary)],capture_output=True,text=True);(a.output/(compiler+'-run.log')).write_text(q.stdout+q.stderr);assert q.returncode==0,q.stderr;results.append(dict(compiler=compiler,passed=True,command=cmd))
  report.update(passed=True,guard_models_only=True,source_physics_mutated=False,compiler_results=results,tests=['input/cache scalarread refusal-before-delegate','overlapping16Bvector refusal-before-delegate','overlappingwrites refused','adjacentknownfields delegated','sameguardedview reachescallback','callbackunprovidedread refuses','unknowncopy omitted/cachefield untouched','currentnativeFP path/publicnumericphysical matrix unchanged'],source_hashes={str(p.relative_to(stage)):parent.base.sha(p)for p in stage.rglob('*')if p.is_file()},public_report=str(a.output/'public/report.json'))
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());parent.base.save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
