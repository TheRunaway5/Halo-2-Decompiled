#!/usr/bin/env python3
"""Exact explicit setters, per-call witness and World admission semantics."""
from pathlib import Path
import hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-moving-shape-retained-world-stage-20261102/staged'
BASE=ROOT/'analysis/physics/native-moving-shape-process-qualified-actual1479-20261102/source'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);out=ROOT/'analysis/physics/native-moving-retained-host-witness-20261102-third';out.mkdir(exist_ok=False);r={'passed':False,'owned_scratch':str(scratch),'retained_large_artifacts':[],'results':[]}
 try:
  live=(STAGE/'src/physics_live_world_tu15.cpp').read_text();world=(STAGE/'src/physics/native_tu15_world_integration_residuals.cpp').read_text();start=live.index('void PhysicsLiveWorldTu15::observe_physics_stores(');end=live.index('void PhysicsLiveWorldTu15::observe_body_velocity_inputs(',start);setters=live[start:end]
  start=world.index('  const auto process_moving=');end=world.index('    NativeTu15RetainedStoreObserver moving_observer;',start);preflight=world[start:end]+'  };'
  code='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/native_tu15_world_retained_store_observation.hpp"
#include <iostream>
#include <memory>
using namespace halo2;
using SourcePhysicsStoreObserverTu15=std::function<void(std::uint32_t,const SourcePhysicsContactRecordStore&)>;
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct Memory:SourcePhysicsContactMemory{mutable unsigned touches{};void read(std::uint32_t,std::span<std::byte>)const override{++touches;throw std::runtime_error("Unexpected DATA read");}void write(std::uint32_t,std::span<const std::byte>)override{++touches;throw std::runtime_error("Unexpected DATA write");}};
class PhysicsLiveWorldTu15 {public:struct Impl{struct PublicCall{explicit PublicCall(Impl&) {}};bool physics_store_observer_read_only{},physics_store_observer_retained{};std::uint32_t last_instruction=0x2e548c;SourcePhysicsContactRecordObserver physics_store_observer;};std::unique_ptr<Impl>impl_=std::make_unique<Impl>();void observe_physics_stores(SourcePhysicsStoreObserverTu15);void observe_physics_stores_read_only(SourcePhysicsStoreObserverTu15);void observe_physics_retained_stores(SourcePhysicsStoreObserverTu15);};
'''+setters+'''
struct Mutable {unsigned calls{};void operator()(const SourcePhysicsContactRecordStore&){++calls;}};
int main(){Memory memory,foreign;SourcePhysicsContactRecordStore event{0x2e548c,0x1000,{},{}};
SourcePhysicsContactRecordObserver actual=Mutable{};NativeTu15WorldRetainedStoreWitness witness(memory,actual,NativeTu15WorldStoreObservationPolicy::retained_data_only);const auto*holder=witness.for_invocation(memory,actual);check(holder,"matching witness rejected");holder->callback()(event);holder->callback()(event);check(actual.target<Mutable>()->calls==2,"witness copied mutable raw target");
SourcePhysicsContactRecordObserver same_value=actual,other=[](const auto&){};check(!witness.for_invocation(foreign,actual)&&!witness.for_invocation(memory,same_value)&&!witness.for_invocation(memory,other),"foreign observer/memory admitted");
PhysicsLiveWorldTu15 host;unsigned delivered{};host.observe_physics_retained_stores([&](auto pc,const auto&e){check(pc==e.instruction,"Host sourcePC tee differs");++delivered;});check(host.impl_->physics_store_observer_read_only&&host.impl_->physics_store_observer_retained,"retained setter missingcapability");host.impl_->physics_store_observer(event);check(delivered==1,"retained setter double delivered");host.observe_physics_stores_read_only([](auto,const auto&){});check(host.impl_->physics_store_observer_read_only&&!host.impl_->physics_store_observer_retained,"legacy readonly retainedcap");host.observe_physics_retained_stores([](auto,const auto&){});host.observe_physics_stores([](auto,const auto&){});check(!host.impl_->physics_store_observer_read_only&&!host.impl_->physics_store_observer_retained,"raw setter retainedcap");host.observe_physics_retained_stores([](auto,const auto&){});host.observe_physics_stores({});check(!host.impl_->physics_store_observer&&!host.impl_->physics_store_observer_retained,"raw clear retainedcap");host.observe_physics_retained_stores({});check(!host.impl_->physics_store_observer&&!host.impl_->physics_store_observer_retained,"retained clear retainedcap");
for(unsigned mode=0;mode<10;++mode){NativeTu15WorldIntegrationStats activity;auto*stats=&activity;SourcePhysicsContactMemory&m=memory;SourcePhysicsContactRecordObserver observe=[](const auto&){};SourcePhysicsContactRecordObserver copied=observe;std::optional<NativeTu15WorldRetainedStoreWitness>token;const NativeTu15WorldRetainedStoreWitness*retained_stores=nullptr;
 if(mode==2||mode==8||mode==9){token.emplace(memory,observe,NativeTu15WorldStoreObservationPolicy::retained_data_only);retained_stores=&*token;}if(mode==3){token.emplace(foreign,observe,NativeTu15WorldStoreObservationPolicy::retained_data_only);retained_stores=&*token;}if(mode==4||mode==5){token.emplace(memory,copied,NativeTu15WorldStoreObservationPolicy::retained_data_only);retained_stores=&*token;}if(mode==6)observe={};if(mode==7){token.emplace(memory,copied,NativeTu15WorldStoreObservationPolicy::retained_data_only);retained_stores=&*token;observe={};}
 NativeTu15PhysicsFrameFp current{0x27f,0x20,0x1fa0};if(mode==8)current.control_word=0x37f;NativeTu15PhysicsFrameScratch scratch{0x60000000u,mode==9?0x1000u:0x10000u};const auto end=std::uint64_t(scratch.first)+scratch.bytes;const auto before=current;
'''+preflight+'''
 bool refused=false;try{process_moving(0,0,0,0,0,current);}catch(const std::runtime_error&){refused=true;}check(refused!=(mode==2||mode==6),"World exactcapability admission differs");check(activity.native_moving_process_dispatches==1&&memory.touches==0&&foreign.touches==0,"World preflight touchedDATA");check(current.control_word==before.control_word&&current.status==before.status&&current.mxcsr==before.mxcsr,"World preflight touchedFP");check(activity.native_moving_process_observer_refusals==unsigned(mode<2||mode==3||mode==4||mode==5||mode==7)&&activity.native_moving_process_fp_refusals==unsigned(mode==8)&&activity.native_moving_process_resource_refusals==unsigned(mode==9),"World capability refusal counters differ");}
std::cout<<"explicitsetter/reset,sharedmutabletarget,exactmemory/rawidentity and10Worldadmissionmodels passed\\n";}
'''
  p=scratch/'test.cpp';p.write_text(code);r['exact_source_sha256']=sha(p)
  for cc in ('g++','clang++'):
   exe=scratch/cc;command=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(STAGE/'include'),'-I'+str(BASE/'include'),'-I'+str(ROOT/'include'),str(p),'-o',str(exe)];build=subprocess.run(command,capture_output=True,text=True);(out/(cc+'-build.log')).write_text(build.stdout+build.stderr);assert build.returncode==0,build.stderr[-5000:];run=subprocess.run([str(exe)],capture_output=True,text=True);(out/(cc+'-run.log')).write_text(run.stdout+run.stderr);r['results'].append({'compiler':cc,'exit_code':run.returncode});assert run.returncode==0,run.stderr
  r.update(passed=True,exact_world_preflight_models=10,explicit_setter_reset_models=True,actual_callback_mutable_target_preserved=True,raw_identity_and_memory_binding=True,scope='Exact Host setter and World preflight/witness source; no wholeHost/runtime/frame claim. Protected FP callback mechanics independently21-controls and actual moving proof.')
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(r,indent=2)+'\n')
 return 0 if r['passed']else 1
if __name__=='__main__':raise SystemExit(main())
