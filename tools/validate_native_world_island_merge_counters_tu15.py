#!/usr/bin/env python3
"""Compile actual staged Getter/gate fragments and prove diagnostic scope sums."""
import argparse,json,os,re,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
from prepare_native_world_island_merge_binding_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);stage=a.stage.resolve()/'staged';report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for header in(stage/'include').rglob('*.hpp'):shutil.copyfile(header,project/'include'/header.relative_to(stage/'include'))
  getter=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();begin=getter.index('    const auto&world_integration=impl_->native_physics_integrations;');end=getter.index('    result.native_object_velocity_payload_predicates=',begin);fragment=getter[begin:end]
  audit=json.loads((a.stage/'aggregation-audit.json').read_text());initialize=[];assertions=[];values={};counter=0
  for row in audit['fields']:
   expected=0
   for worldterm in row['added_world_terms']:
    suffix=worldterm.removeprefix('world_activation.');assert suffix!=worldterm
    paths=['x.native_object_velocity_transport.binding.'+suffix,'x.native_vehicle_flips.activation.'+suffix,'x.native_physics_integrations.world_activation.'+suffix]
    for owner,path in enumerate(paths):
     if path not in values:counter+=1;values[path]=1000*(owner+1)+counter;initialize.append(path+'='+str(values[path])+'u;')
     expected+=values[path]
   if row['field']=='native_gameplay_policy_destructions':initialize.append('x.native_physics_integrations.inactive_island_policy.gameplay_destructions=17u;');expected+=17
   assertions.append('check(result.'+row['field']+'=='+str(expected)+'u,"Diagnostic aggregation differs: '+row['field']+'");')
  worldmap={'native_world_merge_calls':'island_merge.calls','native_world_merge_contact_calls':'island_contact_merge.calls','native_world_merge_body_moves':'island_merge.moved_bodies','native_world_merge_constraint_moves':'island_merge.moved_constraints','native_world_merge_disposals':'island_lifetime.island_disposals','native_world_merge_refusals':'native_world_merge_refusals','native_world_activation_calls':'native_world_activation_calls','native_world_activation_refusals':'native_world_activation_refusals','native_world_activation_body_updates':'world_activation.services.body_updates','native_world_activation_constraint_rows':'world_activation.body_constraints.rows','native_world_activation_residual_calls':'world_activation.residual_calls'}
  for n,(field,path)in enumerate(worldmap.items()):initialize.append('x.native_physics_integrations.'+path+'='+str(800+n)+'u;');assertions.append('check(result.'+field+'=='+str(800+n)+'u,"World diagnostic mapping differs");')
  initialize+=['x.native_physics_integrations.island_merge.unbound_services=3;','x.native_physics_integrations.island_contact_merge.unbound_services=5;','x.native_physics_integrations.island_contact_merge.lifetime.unbound_services=7;','x.native_physics_integrations.island_lifetime.unbound_services=11;','x.native_object_velocity_transport.binding.body_constraints.rows=23;','x.native_vehicle_flips.activation.body_constraints.rows=29;']
  assertions+=['check(result.native_world_merge_unbound_services==26,"World unknown owner aggregation differs");','check(result.native_object_velocity_constraint_rows==23,"World/flip polluted object velocity scope");']
  acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();gstart=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE"))');gend=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_CONSTRAINT_ARENA"))',gstart);gates=acceptance[gstart:gend]
  rows=''.join('{"'+field+'",native_frames.'+field+'},\n'for field in FIELDS)
  for field in FIELDS:assert acceptance.count('{"'+field+'",native_frames.'+field+'}，'.replace('，',','))==2,field
  cpp='''#include "halo2/native_tu15_object_velocity_transport.hpp"
#include "halo2/native_tu15_object_velocity_handoff.hpp"
#include "halo2/native_tu15_vehicle_flip_binding.hpp"
#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
using namespace halo2;void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Impl{NativeTu15ObjectVelocityTransportActivity native_object_velocity_transport;NativeTu15ObjectVelocityHandoffActivity native_object_velocity_handoffs;NativeTu15VehicleFlipBindingActivity native_vehicle_flips;NativeTu15WorldIntegrationStats native_physics_integrations;};
struct Harness{Impl*impl_;NativeTu15PhysicsFrameExecutionStats get(){NativeTu15PhysicsFrameExecutionStats result;
'''+fragment+'''return result;}};
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames,bool merge,bool wake){nlohmann::json report;bool passed=true;if(merge)setenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE");if(wake)setenv("HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION");
'''+gates+'''return passed;}
int main(){Impl x;'''+''.join(initialize)+'''auto result=Harness{&x}.get();'''+''.join(assertions)+'''
NativeTu15PhysicsFrameExecutionStats native_frames;native_frames.native_world_merge_calls=1;native_frames.native_world_merge_contact_calls=1;native_frames.native_world_merge_body_moves=1;native_frames.native_world_merge_disposals=1;native_frames.native_world_activation_calls=1;native_frames.native_world_activation_body_updates=1;
check(gate(native_frames,true,true),"Positive typed counters failed gates");
auto bad=native_frames;bad.native_world_merge_calls=0;check(!gate(bad,true,false),"Zero merge gate admitted");
bad=native_frames;bad.native_world_merge_unbound_services=1;check(!gate(bad,true,true),"Unknown merge owner gate admitted");
bad=native_frames;bad.native_world_merge_refusals=1;check(!gate(bad,true,false),"Merge refusal gate admitted");
bad=native_frames;bad.native_world_activation_refusals=1;check(!gate(bad,true,true),"Activation refusal gate admitted");
bad=native_frames;bad.native_world_activation_residual_calls=1;check(!gate(bad,false,true),"Activation residual gate admitted");
for(auto owner:{0x3190d0u,0x318de0u,0x322100u,0x3213a0u}){bad=native_frames;bad.residual_owner_calls[owner]=1;check(!gate(bad,true,false),"Old merge owner gate admitted");}
nlohmann::json serialized={'''+rows+'''};check(serialized.size()==12,"World serialized counters missing");return 0;}
'''
  (project/'counters.cpp').write_text(cpp);commands=[]
  for compiler in('g++','clang++'):
   target=project/('counter-'+compiler.replace('+','x'));cmd=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counters.cpp'),'-o',str(target)];commands.append(cmd);r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,(compiler,r.stderr[-4000:]);subprocess.run([str(target)],check=True)
  report.update(passed=True,commands=commands,actual_getter_fragment_sha256=__import__('hashlib').sha256(fragment.encode()).hexdigest(),actual_gates_fragment_sha256=__import__('hashlib').sha256(gates.encode()).hexdigest(),general_aggregated_fields=len(audit['fields']),scope_leaf_sentinels=len(values),world_fields=len(FIELDS),serializer_occurrences_per_field=2,gate_declared_counter_cases=10,physics_state_or_original_result_supplied=False,remaining=['Root full LiveWorld/acceptance compile and real visible multiplayer activity'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
