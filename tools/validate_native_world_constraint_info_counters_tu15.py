#!/usr/bin/env python3
"""Exact two nativeinfo transport fields/serializers/positive gate."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root.resolve()/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'))
  source=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();start=source.index('    result.native_world_regroup_constant_info_queries=');end=source.index('    result.native_world_regroup_attempts=',start);getter=source[start:end];source=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_CONSTRAINT_INFO"))');end=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE"))',start);gates=source[start:end]
  for name in('constant','extended'):assert source.count('{"native_world_regroup_'+name+'_info_queries",native_frames.native_world_regroup_'+name+'_info_queries},')==2
  cpp='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstdlib>
using namespace halo2;void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
NativeTu15PhysicsFrameExecutionStats get(const NativeTu15IslandRegroupBindingActivity&world_regroup){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_WORLD_CONSTRAINT_INFO","1",1);
'''+gates+'''return passed;}
int main(){NativeTu15IslandRegroupBindingActivity a;a.constant_info_queries=17;a.extended_info_queries=23;auto r=get(a);check(r.native_world_regroup_constant_info_queries==17&&r.native_world_regroup_extended_info_queries==23,"Wrong nativeinfo Getter");NativeTu15PhysicsFrameExecutionStats p;p.native_world_regroup_constant_info_queries=1;p.native_world_regroup_attempts=1;p.native_world_regroup_calls=1;check(gate(p),"Positiveconstant gate rejected");p.native_world_regroup_constant_info_queries=0;p.native_world_regroup_extended_info_queries=1;check(gate(p),"Positiveextended gate rejected");auto b=p;b.native_world_regroup_extended_info_queries=0;check(!gate(b),"Zeroqueries admitted");b=p;b.native_world_regroup_refusals=1;check(!gate(b),"Refusal admitted");b=p;b.native_world_regroup_unbound_services=1;check(!gate(b),"Unknown owner admitted");b=p;b.native_world_regroup_attempts=2;check(!gate(b),"Partialparent admitted");for(auto owner:{0x31d630u,0x31f840u}){b=p;b.residual_owner_calls[owner]=1;check(!gate(b),"Oldinfo owner admitted");}return 0;}
''';(project/'counter.cpp').write_text(cpp);commands=[]
  for compiler in('g++','clang++'):
   binary=project/('counter-'+compiler.replace('+','x'));command=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counter.cpp'),'-o',str(binary)];commands.append(command);r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:];subprocess.run([str(binary)],check=True)
  report.update(passed=True,commands=commands,fields=2,serializers=2,gate_positive_cases=2,gate_negative_cases=6,physics_state_supplied=False)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
