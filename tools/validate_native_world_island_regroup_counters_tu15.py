#!/usr/bin/env python3
"""Compile exact staged regroup Getter, both serializers and honest phase/split gates."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from prepare_native_world_island_regroup_binding_tu15 import FIELDS,UNKNOWN,sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'))
  text=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();start=text.index('    const auto&world_regroup=world_integration.island_regroup;');end=text.index('    result.native_world_merge_calls=',start);getter=text[start:end]
  text=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=text.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_REGROUP")');end=text.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE"))',start);gates=text[start:end]
  fields=['native_world_regroup_'+n for n in FIELDS]+['native_world_regroup_unbound_services']
  for field in fields:assert text.count('{"'+field+'",native_frames.'+field+'},')==2,field
  initialize=[];checks=[]
  for n,(field,path)in enumerate(FIELDS.items()):initialize.append('x.island_regroup.'+path+'='+str(100+n)+';');checks.append('check(result.native_world_regroup_'+field+'=='+str(100+n)+',"Wrong Getter field");')
  for n,path in enumerate(UNKNOWN):initialize.append('x.island_regroup.'+path+'='+str(1000+n)+';')
  checks.append('check(result.native_world_regroup_unbound_services=='+str(sum(1000+n for n in range(len(UNKNOWN))))+',"Wrong unknown aggregation");')
  cpp='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
using namespace halo2;void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
NativeTu15PhysicsFrameExecutionStats get(const NativeTu15WorldIntegrationStats&world_integration){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames,bool split){nlohmann::json report;bool passed=true;setenv("HALO2_REQUIRE_NATIVE_WORLD_REGROUP","1",1);if(split)setenv("HALO2_REQUIRE_NATIVE_WORLD_REPARTITION","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_WORLD_REPARTITION");
'''+gates+'''return passed;}
int main(){NativeTu15WorldIntegrationStats x;'''+''.join(initialize)+'''auto result=get(x);'''+''.join(checks)+'''
NativeTu15PhysicsFrameExecutionStats p;p.native_world_regroup_attempts=1;p.native_world_regroup_calls=1;p.native_world_regroup_active_visits=7;p.native_world_regroup_flag_skips=7;
check(gate(p,false),"Native phase gate rejected actual flag-skip domain");check(!gate(p,true),"Flag-skip counts fabricated positive partition");
p.native_world_regroup_graph_builds=1;p.native_world_regroup_created_islands=5;p.native_world_regroup_moved_bodies=5;p.native_world_regroup_group_allocations=6;p.native_world_regroup_group_releases=6;p.native_world_regroup_temporary_constructors=3;p.native_world_regroup_temporary_destructors=3;
check(gate(p,true),"Positive declared split counters rejected");
auto b=p;b.native_world_regroup_refusals=1;check(!gate(b,false),"Refusal admitted");b=p;b.native_world_regroup_unbound_services=1;check(!gate(b,true),"Unknown owner admitted");b=p;b.native_world_regroup_attempts=2;check(!gate(b,false),"Incomplete attempt admitted");b=p;b.native_world_regroup_moved_bodies=0;check(!gate(b,true),"Missing positive body movement admitted");b=p;b.native_world_regroup_group_releases=0;check(!gate(b,true),"Incomplete lifetime admitted");
for(auto owner:{0x3187c0u,0x321eb0u}){b=p;b.residual_owner_calls[owner]=1;check(!gate(b,false),"Old owner admitted");}
nlohmann::json serialized={'''+','.join('{"'+f+'",result.'+f+'}'for f in fields)+'''};check(serialized.size()==24,"Missing serialization field");return 0;}
'''
  (project/'counters.cpp').write_text(cpp);commands=[]
  for compiler in('g++','clang++'):
   binary=project/('counters-'+compiler.replace('+','x'));cmd=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counters.cpp'),'-o',str(binary)];commands.append(cmd);r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:];subprocess.run([str(binary)],check=True)
  report.update(passed=True,commands=commands,fields=len(fields),unknown_scope_sentinels=len(UNKNOWN),serializers=2,phase_and_positive_split_gates_distinct=True,physics_state_supplied=False,getter_sha256=__import__('hashlib').sha256(getter.encode()).hexdigest(),gates_sha256=__import__('hashlib').sha256(gates.encode()).hexdigest())
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
