#!/usr/bin/env python3
"""Same sole production moving helper, exact immutable c58 solver baseline.

Genuine1479 typed input, full unchanged-original return/all retained DATA/FP.
Current Root solver is not edited and its unqualified flag1 domain is not used.
"""
import hashlib,inspect,json,os,sys,subprocess
from pathlib import Path
import validate_native_moving_shape_process_full_native_actual_tu15 as full
import validate_native_moving_shape_process_solved_actual_tu15 as prefix
ROOT=Path(__file__).resolve().parents[1]
FULL_OBSERVER=full.observer_source
def observer_source():
 text=FULL_OBSERVER().replace('#include "halo2/native_tu15_contact_creation_gameplay.hpp"','#include "halo2/native_tu15_contact_creation_gameplay.hpp"\n#include "halo2/native_tu15_moving_shape_process_binding.hpp"')
 begin=text.index('   NativeTu15MovingShapeProcessServices services;');end=text.index('   tracked.enabled=false;',begin)
 call='''   NativeTu15MovingShapeBindingActivity bound;NativeTu15PairDeletionNativeScope native_scope;tracked.enabled=true;
   native_tu15_process_bound_moving_shape_contact(tracked,{typed.at("agent")},{typed.at("first")},{typed.at("second")},{typed.at("configuration")},{typed.at("output")},{p,0x8000u},fp,0,{},native_scope,&bound);
   const auto&activity=bound.moving;const auto&convex_activity=bound.convex;const auto&solver_activity=bound.solver;const bool boundary_seen=bound.completed==1;
'''
 text=text[:begin]+call+text[end:]
 text=text.replace('!view.refused_reads()','!bound.unprovided_reads')
 begin=text.index('   const auto unprovided_reads=view.refused_reads();');end=text.index('   passes.push_back(',begin)
 text=text[:begin]+'''   const auto unprovided_reads=bound.unprovided_reads;
   check(bound.attempts==1&&bound.completed==1&&!bound.refusals&&!native_scope.active()&&bound.contact_publications==1&&bound.distance_publications==1&&bound.publication.creation.completed==1&&bound.publication.creation.creation.rejected_points==1&&bound.removal.completed==1&&bound.gameplay.limit_calls==1&&bound.gameplay.body_rejected_early_outs==2,"Bound moving production activities differ");
   const auto guard_controls=5u; // Independently qualified same-view controls; the completed caller lifetime has ended.
'''+text[end:]
 text=text.replace('{"guard_controls",guard_controls}', '{"guard_class_prior_controls",guard_controls},{"native_contact_creations",bound.publication.creation.completed},{"native_point_rejections",bound.publication.creation.creation.rejected_points},{"native_point_removals",bound.removal.completed},{"native_world_listeners",bound.gameplay.world_calls},{"native_limit_listeners",bound.gameplay.limit_calls},{"native_body_early_outs",bound.gameplay.body_rejected_early_outs}')
 marker=' report["passed"]=true;'
 assert text.count(marker)==1
 models=r''' {
  struct ModelMemory:SourcePhysicsContactMemory {NativeTu15PairDeletionNativeScope&scope;bool reject_owner{};mutable unsigned touches{};explicit ModelMemory(NativeTu15PairDeletionNativeScope&s):scope(s){}void read(std::uint32_t,std::span<std::byte>)const override{if(reject_owner)scope.reject_unconverted(0x30e930u);++touches;throw std::runtime_error("Unexpected model DATA read");}void write(std::uint32_t,std::span<const std::byte>)override{++touches;throw std::runtime_error("Unexpected model DATA write");}};
  J models=J::array();
  for(unsigned mode=0;mode<8;++mode){NativeTu15PairDeletionNativeScope scope;ModelMemory memory(scope);NativeTu15PhysicsFrameFp f{0x27fu,0x20u,0x1fa0u};NativeTu15MovingShapeBindingActivity a;NativeTu15MovingShapeBindingScratch s{0x6000000u,0x8000u};unsigned depth{};
   if(mode==0)s.identity=0;if(mode==1)++s.identity;if(mode==2)s.bytes=0x7fffu;if(mode==3)s.identity=0xfffffff0u;if(mode==4)depth=1;if(mode==5)f.control_word=0x27eu;if(mode==6)f.mxcsr|=0x10000u;if(mode==7)memory.reject_owner=true;
   bool refused{};try{native_tu15_process_bound_moving_shape_contact(memory,{1},{2},{3},{4},{5},s,f,depth,{},scope,&a);}catch(const std::runtime_error&){refused=true;}
   check(refused&&a.attempts==1&&a.refusals==1&&!a.completed&&!scope.active()&&memory.touches==0,"Moving resource/native-only model differs");
   if(mode<4)check(a.resource_refusals==1,"Moving resource count differs");if(mode==4)check(a.depth_refusals==1,"Moving depth count differs");if(mode==5||mode==6)check(a.fp_refusals==1,"Moving FP count differs");if(mode==7)check(a.native_only.refused==1&&a.native_only.owners.at(0x30e930u)==1,"Moving native-only owner count differs");models.push_back(mode);
  }
  report["production_guard_models"]=models;
 }
'''
 text=text.replace(marker,models+marker)
 return text
def main():
 baseline=ROOT/'analysis/physics/native-convex-simplex-solver-qualified-actual1479-20261102/source/src/physics/native_tu15_convex_simplex_solver.cpp';pin='c58c0ba274a0c894db23fdcda49a12d471d42e4e2fc8dd521aca0726e09800ad';assert hashlib.sha256(baseline.read_bytes()).hexdigest()==pin
 helper=ROOT/'include/halo2/native_tu15_moving_shape_process_binding.hpp';helper_pin=hashlib.sha256(helper.read_bytes()).hexdigest();tool_pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();project=scratch/'qualified-solver';project.mkdir();source=project/'native_tu15_convex_simplex_solver.cpp';source.write_bytes(baseline.read_bytes());original_run=subprocess.run
 compiled={};mutants=[]
 changes={'wrong_radius':(ROOT/'src/physics/native_tu15_convex_contact_process.cpp','minus(m.word(radius_shape+0xcu));','plus(m.word(radius_shape+0xcu));'),'wrong_second_point':(ROOT/'src/physics/native_tu15_convex_contact_process.cpp','transpose_direction(x,y,z,relative)','transpose_direction(z,y,x,relative)'),'disabled_native_scope':(helper,'NativeTu15PairDeletionNativeScopeGuard scope(native_only,&out.native_only);','(void)native_only;')}
 def run(command,*args,**kwargs):
  if isinstance(command,list)and str(ROOT/'src/physics/native_tu15_convex_simplex_solver.cpp')in command:
   command=[str(source)if p==str(ROOT/'src/physics/native_tu15_convex_simplex_solver.cpp')else p for p in command];command.insert(1,'-I'+str(ROOT/'src/physics'))
   compiled[command[command.index('-o')+1]]=list(command)
  result=original_run(command,*args,**kwargs)
  if isinstance(command,list)and command and command[0]in compiled and result.returncode==0:
   build=compiled[command[0]];compiler=Path(build[0]).name;output=Path(command[4]).parent
   for name,(kernel,old,new)in changes.items():
    content=kernel.read_text();assert content.count(old)==1,(name,content.count(old));overlay=scratch/name;overlay.mkdir();target=overlay/(compiler+'-mutant')
    variant=[str(target)if p==command[0]else p for p in build]
    if kernel==helper:
     include=overlay/'include/halo2';include.mkdir(parents=True);(include/helper.name).write_text(content.replace(old,new));variant.insert(1,'-I'+str(overlay/'include'))
    else:
     modified=overlay/kernel.name;modified.write_text(content.replace(old,new));variant=[str(modified)if p==str(kernel)else p for p in variant]
    b=original_run(variant,capture_output=True,text=True);(output/f'{compiler}-{name}-build.log').write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr[-5000:]
    path=output/f'{compiler}-{name}.json';execution=original_run([str(target),*command[1:4],str(path)],capture_output=True,text=True);(output/f'{compiler}-{name}.log').write_text(execution.stdout+execution.stderr);assert execution.returncode!=0 and path.exists()and json.loads(path.read_text()).get('passed')is False,name+' survived'
    mutants.append(dict(compiler=compiler,mutant=name,rejected=True,report=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
  return result
 subprocess.run=run;old_observer=full.observer_source;full.observer_source=observer_source
 old_extra=full.EXTRA;full.EXTRA=(*old_extra,'native_tu15_contact_cache.cpp')
 # The production wrapper uses the full caller's outer builder, preserving
 # current source-before/after pins and adding the independent baseline pin.
 try:rc=full.main()
 finally:full.observer_source=old_observer;full.EXTRA=old_extra;subprocess.run=original_run
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(p.read_text());r['production_moving_helper']=True;r['compiled_solver_baseline_sha256']=pin;r['compiled_solver_baseline']=str(baseline.relative_to(ROOT));r['production_helper_sha256']=helper_pin;r['production_validator_sha256']=tool_pin;r['production_sources_unchanged']=hashlib.sha256(helper.read_bytes()).hexdigest()==helper_pin and hashlib.sha256(Path(__file__).read_bytes()).hexdigest()==tool_pin;r['mutants']=mutants;r['flag1_domain_admitted']=False
 if not r['production_sources_unchanged']:r['passed']=False;rc=1
 if r.get('passed'):
  assert len(mutants)==6
  for compiler in ('g++','clang++'):
   payload=json.loads((p.parent/f'{compiler}-actual.json').read_text());assert payload.get('passed')is True and not payload.get('error');assert payload['production_guard_models']==list(range(8));assert len(payload['profiles'])==4;assert all(profile.get('passed')is True for profile in payload['profiles'])
  r['selected_actual_full_return_qualified']=True
 p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
