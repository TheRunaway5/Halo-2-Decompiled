#!/usr/bin/env python3
"""SAME native-only accepted listener binding on genuine1481 typed DATA."""
import hashlib,inspect,json,os
from pathlib import Path
import validate_native_accepted_body_contact_actual_tu15 as base
ROOT=Path(__file__).resolve().parents[1]
def observer_source():
 text=(ROOT/'tools/observe_native_accepted_body_contact_actual_tu15.cpp').read_text()
 begin=text.index('   NativeTu15PhysicsFrameFp fp{');end=text.index('   check(parent.completed==1',begin)if'   check(parent.completed==1'in text[begin:]else text.index('   if(!failed)check(parent.completed==1',begin)
 block='''   NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};NativeTu15PairDeletionNativeScope scope;
   NativeTu15AcceptedContactScratch scratch{p+0x100u,p+0x104u,p+0x108u,p+0x10cu,p+0x110u,p+0x114u,p+0x118u,p+0x11cu,{p+0x200u},{p+0x300u,p+0x310u}};
   NativeTu15AcceptedBodyContactBindingActivity binding;auto&parent=binding.listener;auto&activity=binding.publication;memory.track=true;bool failed{};
   try{native_tu15_notify_bound_accepted_body_contact(view,typed.at("listener"),p+0x800u,{{p,p+4u,p+8u,p+12u},scratch},fp,0,scope,protected_observer,&binding);}catch(const DataFault&){failed=true;}
   memory.track=false;check(failed==!fault.is_null(),"Bound native declared fault differs");check(!scope.active()&&binding.attempts==1&&binding.completed==unsigned(!failed)&&binding.refusals==unsigned(failed),"Bound native activity/scope restore differs");
'''
 text=text[:begin]+block+text[end:]
 marker=' if(numeric_mutant)check(mutant_profiles'
 assert text.count(marker)==1
 models=r''' {
 struct ModelMemory final:SourcePhysicsContactMemory {
  NativeTu15PairDeletionNativeScope&scope;bool fallback{};mutable unsigned touches{};
  explicit ModelMemory(NativeTu15PairDeletionNativeScope&s):scope(s){}
  void read(std::uint32_t,std::span<std::byte>)const override{if(fallback)scope.reject_unconverted(0x281710u);++touches;throw std::runtime_error("Unexpected bound model read");}
  void write(std::uint32_t,std::span<const std::byte>)override{++touches;throw std::runtime_error("Unexpected bound model write");}
 };
 for(unsigned mode=0;mode<14;++mode){
  NativeTu15PairDeletionNativeScope scope;ModelMemory memory(scope);constexpr std::uint32_t p=0x40000000u;
  const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x1000u}}};NativeTu15ContactCandidatePrivateData view(memory,owned,{});
  NativeTu15AcceptedContactScratch a{p+0x100u,p+0x104u,p+0x108u,p+0x10cu,p+0x110u,p+0x114u,p+0x118u,p+0x11cu,{p+0x200u},{p+0x300u,p+0x310u}};
  NativeTu15AcceptedBodyContactBindingScratch s{{p,p+4u,p+8u,p+12u},a};NativeTu15PhysicsFrameFp fp{0x27fu,0x20u,0x1fa0u};unsigned depth{};
  if(mode==0)s.weights.first_weight=0;if(mode==1)s.weights.second_weight=s.weights.first_weight;
  if(mode==2)++s.accepted.first_material;if(mode==3)s.accepted.constraint_request.identity=0xfffffff0u;
  if(mode==4)++s.accepted.body_update.point_velocity;if(mode==5)s.weights.first_weight=0x20000u;
  if(mode==6)s.weights.first_weight=0x30000u;if(mode==7)depth=1;if(mode==8)fp.control_word=0x27eu;
  if(mode==9)fp.mxcsr|=0x10000u;if(mode==11)s.weights.first_weight=0x10000u;
  if(mode==12)memory.fallback=true;if(mode==13)fp.status|=0x8080u;
  NativeTu15AcceptedBodyContactBindingActivity out;bool refused{};
  try{native_tu15_notify_bound_accepted_body_contact(mode==10?static_cast<SourcePhysicsContactMemory&>(memory):static_cast<SourcePhysicsContactMemory&>(view),0x30000u,0x20000u,s,fp,depth,scope,{},&out);}catch(const std::runtime_error&){refused=true;}
  check(refused&&out.attempts==1&&out.refusals==1&&!out.completed&&!scope.active()&&memory.touches==0,"Bound ownership/FP/depth/native-only model failed");
  if(mode==12)check(out.native_only.refused==1&&out.native_only.owners.at(0x281710u)==1,"Bound Source fallback was admitted");
 }
 report["production_guard_models"]=14;
 }
'''
 text=text.replace(marker,models+marker)
 return text

def main():
 original=(ROOT/'tools/observe_native_accepted_body_contact_actual_tu15.cpp').read_text();bound=observer_source();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);observer=scratch/'bound-observer.cpp';observer.write_text(bound)
 text=inspect.getsource(base.main)
 text=text.replace("'include/halo2/native_tu15_retained_store_observer.hpp'];pins", "'include/halo2/native_tu15_retained_store_observer.hpp','include/halo2/native_tu15_accepted_body_contact_binding.hpp'];pins")
 text=text.replace("driver='#include", "driver='#include \"halo2/native_tu15_accepted_body_contact_binding.hpp\"\\n#include",1)
 needle="(ROOT/'tools/observe_native_accepted_body_contact_actual_tu15.cpp').read_text()";assert text.count(needle)==1;text=text.replace(needle,'BOUND_OBSERVER_SOURCE')
 text=text.replace("'tools/run_test.py');tool_pins", "'tools/run_test.py','tools/validate_native_accepted_body_contact_bound_actual_tu15.py');tool_pins")
 env=dict(base.__dict__,BOUND_OBSERVER_SOURCE=bound,__doc__=__doc__);exec(text,env);return env['main']()
if __name__=='__main__':main()
