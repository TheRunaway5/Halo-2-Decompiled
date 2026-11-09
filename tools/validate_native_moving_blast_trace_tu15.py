#!/usr/bin/env python3
"""Exact Host blast/retained-velocity composition, witness and scope controls."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-moving-blast-retained-trace-stage-20261102/staged'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output.mkdir(parents=True,exist_ok=False)
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(owner),retained_large_artifacts=[],scope=__doc__)
 try:
  host=(STAGE/'tests/physics_multiplayer_acceptance.cpp').read_text();live=(a.source_root/'src/physics_live_world_tu15.cpp').read_text()
  setters=live[live.index('void PhysicsLiveWorldTu15::observe_physics_stores('):live.index('void PhysicsLiveWorldTu15::observe_body_velocity_inputs(')]
  blast=host[host.index('      // BEGIN bounded actual blast damage diagnosis'):host.index('      // END bounded actual blast damage diagnosis')]
  scope=host[host.index('      struct PhysicsObservationScope {'):host.index('      if(std::getenv("HALO2_TU15_CONTRAIL_STORES")) {')]
  start=host.index('        world.source_objects->observe_physics_retained_stores([&](std::uint32_t actual_instruction');end=host.index('        });',start)+len('        });');phantom=host[start:end]
  prefix=r'''
#include "halo2/native_tu15_world_retained_store_observation.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <new>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
static int fail_allocation_after=-1;
void*operator new(std::size_t n){if(fail_allocation_after==0){fail_allocation_after=-1;throw std::bad_alloc{};}if(fail_allocation_after>0)--fail_allocation_after;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc{};}
void operator delete(void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}
using namespace halo2;using Json=nlohmann::json;
using SourcePhysicsStoreObserverTu15=std::function<void(std::uint32_t,const SourcePhysicsContactRecordStore&)>;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::string hex_bytes(std::span<const std::byte>b){std::string s;const char*h="0123456789abcdef";for(auto x:b){auto n=std::to_integer<unsigned>(x);s+=h[n>>4];s+=h[n&15];}return s;}
struct Bytes{std::span<const std::byte>b;Bytes(std::span<const std::byte>x):b(x){}std::uint32_t u32(unsigned p)const{std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::uint32_t(b[p+i])<<(8*i);return v;}};
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks{{0x1000,std::vector<std::byte>(4096)},{0x51df54,std::vector<std::byte>(4)}};
 std::span<const std::byte>view(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"missing trace bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"missing trace extent");return std::span(i->second).subspan(p-i->first,n);}
 void read(std::uint32_t p,std::span<std::byte>b)const override{auto v=view(p,b.size());std::copy(v.begin(),v.end(),b.begin());}
 void write(std::uint32_t p,std::span<const std::byte>b)override{view(p,b.size());auto i=banks.upper_bound(p);--i;std::copy(b.begin(),b.end(),i->second.begin()+p-i->first);}
 void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));write(p,b);}
};
class PhysicsLiveWorldTu15{public:struct Impl{struct PublicCall{explicit PublicCall(Impl&){}};bool physics_store_observer_read_only{},physics_store_observer_retained{};std::uint32_t last_instruction=0x12345678;SourcePhysicsContactRecordObserver physics_store_observer;};std::unique_ptr<Impl>impl_=std::make_unique<Impl>();Memory mem;const Memory&memory()const{return mem;}void observe_physics_stores(SourcePhysicsStoreObserverTu15);void observe_physics_stores_read_only(SourcePhysicsStoreObserverTu15);void observe_physics_retained_stores(SourcePhysicsStoreObserverTu15);void observe_body_velocity_inputs(std::function<void()>){ }void observe_phantom_phase(std::function<void()>){ }void observe_weapon_effect_phase(std::function<void()>){ }};
using MultiplayerObjectRuntime=PhysicsLiveWorldTu15;
std::vector<Json>rows(const std::filesystem::path&p){std::ifstream f(p);std::vector<Json>r;for(std::string s;std::getline(f,s);)r.push_back(Json::parse(s));return r;}
struct Floating{std::uint16_t cw,sw;unsigned mx;std::array<std::byte,10>st0;friend bool operator==(const Floating&,const Floating&)=default;};
Floating floating(){alignas(16)std::array<std::byte,512>b{};asm volatile("fxsave %0":"=m"(b));Floating f;std::memcpy(&f.cw,b.data(),2);std::memcpy(&f.sw,b.data()+2,2);std::memcpy(&f.mx,b.data()+24,4);std::copy_n(b.begin()+32,10,f.st0.begin());return f;}
void disturb(){std::uint16_t cw;asm volatile("fld1;fld1;faddp;fstp %%st(0);fnstcw %0":"=m"(cw)::"st","memory");cw=(cw&~0xc00u)|0xc00u;asm volatile("fldcw %0"::"m"(cw):"memory");_mm_setcsr((_mm_getcsr()&~0x6000u)|0x6000u|0x20u);}
'''
  body=r'''
int main(int argc,char**argv){try{check(argc==2,"owned trace output required");unsigned completed{};
for(unsigned mode=0;mode<11;++mode){
 const bool phantom=mode!=1&&mode!=7,blast_enabled=mode!=0&&mode!=7;
 if(phantom)setenv("HALO2_TU15_PHANTOM_TRACE","1",1);else unsetenv("HALO2_TU15_PHANTOM_TRACE");
 if(blast_enabled)setenv("HALO2_TU15_BLAST_DAMAGE_TRACE","1",1);else unsetenv("HALO2_TU15_BLAST_DAMAGE_TRACE");
 unsetenv("HALO2_TU15_CONTRAIL_STORES");unsetenv("HALO2_TU15_CONTRAIL_STORE_TRACE");unsetenv("HALO2_TU15_SUPPORT_FIELD_STORES");
 if(mode==6)setenv("HALO2_TU15_SUPPORT_FIELD_STORES","1",1);
 struct{std::shared_ptr<PhysicsLiveWorldTu15>source_objects=std::make_shared<PhysicsLiveWorldTu15>();}world;
 auto retained_world=world.source_objects;auto&runtime=*retained_world;
 runtime.mem.put(0x51df54,0x1000);runtime.mem.put(0x1008,23);runtime.mem.put(0x1100,2);runtime.mem.put(0x11a8,0x80010001);runtime.mem.put(0x11ac,0x80020002);
 const auto directory=std::filesystem::path(argv[1])/std::to_string(mode);std::filesystem::create_directories(directory);
 struct{std::filesystem::path source_object_savegame_directory;}options{directory/"save"};
 bool refused{};unsigned adversary_calls{};
 try{
'''+blast+r'''
  std::map<std::uint32_t,std::pair<unsigned,std::string>>velocity_targets{{0x1100,{1,"object_velocity"}}};
  std::ofstream velocity_trace;std::uint32_t source_velocity_observation_stack{};std::array<std::uint32_t,8>source_velocity_observation_registers{};
  if(phantom){velocity_trace.open(directory/"authority-velocity-stores.jsonl");check(bool(velocity_trace),"model velocity trace open failed");}
'''+scope+r'''
  if(mode==4||mode==5){auto saved=blast_damage_store_observer;blast_damage_store_observer=[&,saved](auto pc,const auto&e){saved(pc,e);++adversary_calls;disturb();if(mode==5)throw std::runtime_error("declared blast observer failure");};}
  if(mode==9||mode==10)fail_allocation_after=mode-9;
  if(phantom){
'''+phantom+r'''
  }
  check(runtime.impl_->physics_store_observer_retained==phantom,"retained consent differs");
  check(bool(runtime.impl_->physics_store_observer)==(phantom||blast_enabled),"trace callback installation differs");
  if(mode==7){++completed;continue;}
  if(mode==3)blast_damage_records=4096;
  if(mode==8)velocity_trace.close();
  auto&actual=runtime.impl_->physics_store_observer;
  const auto emit=[&](const SourcePhysicsContactRecordStore&e){
   const auto before=runtime.mem.banks;
   if(phantom){NativeTu15WorldRetainedStoreWitness witness(runtime.mem,actual,NativeTu15WorldStoreObservationPolicy::retained_data_only);auto copied=actual;Memory other;check(!witness.for_invocation(other,actual)&&!witness.for_invocation(runtime.mem,copied),"foreign witness admitted");const auto*typed=witness.for_invocation(runtime.mem,actual);check(typed,"exact witness refused");const auto fp=floating();try{typed->callback()(e);}catch(...){check(floating()==fp,"native FP lost on observation exception");check(runtime.mem.banks==before,"diagnostic mutated physicsDATA on exception");throw;}check(floating()==fp,"native FP lost on observation return");}
   else actual(e);
   check(runtime.mem.banks==before,"diagnostic mutated physicsDATA");
  };
  const SourcePhysicsContactRecordStore area{0xd926d,0x1100,{std::byte{0xa5},std::byte{},std::byte{},std::byte{}},{std::byte{2},std::byte{},std::byte{},std::byte{}}};
  emit(area);emit({0x2e548c,0x1104,{std::byte{1}},{std::byte{2}}});emit({0x123456,0x1500,{},{}});
  velocity_trace.flush();blast_damage_trace.flush();
  const auto damage=rows(directory/"authority-blast-damage.jsonl"),velocity=rows(directory/"authority-velocity-stores.jsonl");
  check(damage.size()==unsigned(blast_enabled)&&velocity.size()==(phantom?2u:0u),"trace composition dropped or duplicated records");
  if(blast_enabled)check(damage[0]["instruction"]==0xd926du&&damage[0]["actual_instruction"]==0x12345678u&&damage[0]["candidate_count"]==2&&damage[0]["candidates"]==Json::array({0x80010001u,0x80020002u}),"blast SourcePC/current candidate data differs");
  if(phantom)check(velocity[0]["instruction"]==0xd926du&&velocity[1]["instruction"]==0x2e548cu&&velocity[0]["actual_instruction"]==0x12345678u&&velocity[0]["before"]=="a5000000","velocity SourcePC/order/preimage differs");
  if(mode==4)check(adversary_calls==3,"shared composition adversary target differs");
 }catch(const std::bad_alloc&){check((mode==9||mode==10)&&fail_allocation_after<0,"unexpected allocation failure");refused=true;}
 catch(const std::runtime_error&e){const std::string message=e.what();const bool expected=(mode==3&&message=="Actual blast damage diagnosis exceeded its bounded record budget")||(mode==5&&message=="declared blast observer failure")||(mode==6&&message=="Blast diagnosis requires compatible read-only store diagnostics")||(mode==8&&message=="Could not retain source velocity store");check(expected,e.what());refused=true;}
 fail_allocation_after=-1;
 check(refused==(mode==3||mode==5||mode==6||mode==8||mode==9||mode==10),"trace failure policy differs");
 check(!runtime.impl_->physics_store_observer&&!runtime.impl_->physics_store_observer_retained,"observer escaped captured target lifetime");
 ++completed;
}
std::cout<<completed<<" exactHosttrace composition/witness/sourcePC/FP/exception/lifetime models passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
  code=prefix+setters+body;source=owner/'driver.cpp';source.write_text(code);(a.output/'driver.cpp').write_text(code)
  pins={str(p.relative_to(ROOT)):sha(p)for p in [STAGE/'tests/physics_multiplayer_acceptance.cpp',a.source_root/'src/physics_live_world_tu15.cpp',a.source_root/'include/halo2/native_tu15_world_retained_store_observation.hpp',a.source_root/'include/halo2/native_tu15_retained_store_observer.hpp']}
  results=[]
  for cc in('g++','clang++'):
   binary=owner/cc;build=subprocess.run([cc,'-std=c++20','-O1','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(a.source_root/'include'),str(source),'-o',str(binary)],capture_output=True,text=True);(a.output/(cc+'-build.log')).write_text(build.stdout+build.stderr);assert build.returncode==0,build.stderr[-5000:]
   run=subprocess.run([str(binary),str(owner/cc.replace('+','p'))],capture_output=True,text=True);(a.output/(cc+'-run.log')).write_text(run.stdout+run.stderr);assert run.returncode==0,run.stdout+run.stderr;results.append(dict(compiler=cc,passed=True,models=11))
  assert all(sha(ROOT/n)==h for n,h in pins.items());r.update(passed=True,results=results,source_sha256=pins,generated_driver_sha256=sha(source),shared_trace_callbacks=True,read_only_standalone_preserved=True,source_pc_preimage_and_order=True,per_invocation_exact_witness=True,fp_adversary_normal_and_throw=True,lifetime_normal_and_throw=True,physics_data_unchanged=True,canonical_and_active_source_unchanged=True)
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
