#!/usr/bin/env python3
"""Compile exact staged Frame aggregation, two serializers and native gates."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
from prepare_native_dirty_island_frame_tu15 import FIELDS,PREFIX
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  for p in(stage/'include').rglob('*.hpp'):shutil.copyfile(p,project/'include'/p.relative_to(stage/'include'))
  cpp=(stage/'src/physics/native_tu15_physics_frame_residuals.cpp').read_text();begin=cpp.index('    const auto aggregate=',cpp.index('  s.post_step='));end=cpp.index('    try{native_tu15_process_bound_dirty_island_objects',begin);aggregate=cpp[begin:end]
  text=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();begin=text.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_DIRTY_ISLAND_OBJECTS"))');end=text.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION"))',begin);gates=text[begin:end]
  for field in [*FIELDS,'unqualified_owners']:assert text.count('{"'+PREFIX+field+'",native_frames.'+PREFIX+field+'},')==2
  assignments=''.join('activity.'+member.split('+')[0]+'='+str(17+n)+';'for n,member in enumerate(FIELDS.values()));checks=''.join('check(result.'+PREFIX+field+'=='+str(17+n)+');'for n,field in enumerate(FIELDS));serializers=''.join('{"'+PREFIX+field+'",result.'+PREFIX+field+'},'for field in [*FIELDS,'unqualified_owners'])
  cpp='''#include "halo2/native_tu15_dirty_island_objects_binding.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
using namespace halo2;void check(bool yes){if(!yes)throw std::runtime_error("Dirty Frame transport differs");}
NativeTu15PhysicsFrameExecutionStats get(const NativeTu15DirtyIslandObjectsBindingActivity&activity){NativeTu15PhysicsFrameExecutionStats result;auto*stats=&result;
'''+aggregate+'''aggregate();return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames,bool cleanup=false){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_DIRTY_ISLAND_OBJECTS","1",1);if(cleanup)setenv("HALO2_REQUIRE_NATIVE_DIRTY_ISLAND_CLEANUP","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_DIRTY_ISLAND_CLEANUP");
'''+gates+'''return passed;}
int main(){NativeTu15DirtyIslandObjectsBindingActivity activity;
'''+assignments+'''activity.update.unbound_owners[0x1d7ca0]=2;activity.state.unbound_owners[0x1d7ca0]=3;activity.state.unbound_owners[0x1d8470]=4;auto result=get(activity);
'''+checks+'''check(result.native_dirty_island_objects_unqualified_owners.at(0x1d7ca0)==5&&result.native_dirty_island_objects_unqualified_owners.at(0x1d8470)==4);
nlohmann::json j={'''+serializers+'''};check(j.size()=='''+str(len(FIELDS)+1)+''');NativeTu15PhysicsFrameExecutionStats p;p.native_dirty_island_objects_attempts=1;p.native_dirty_island_objects_calls=1;check(gate(p));auto b=p;
'''+''.join('b=p;b.'+PREFIX+field+'='+value+';check(!gate(b));'for field,value in [('calls','0'),('attempts','2'),('refusals','1'),('unqualified_services','1'),('unsupported_local_sizes','1')])+'''b=p;b.native_dirty_island_objects_unqualified_owners[0xdead]=1;check(!gate(b));b=p;b.residual_owner_calls[0x282c10]=1;check(!gate(b));check(!gate(p,true));p.native_dirty_island_objects_dirty_islands=1;check(gate(p,true));return 0;}
''';(project/'counter.cpp').write_text(cpp);results=[]
  for compiler in('g++','clang++'):
   binary=project/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counter.cpp'),'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-2500:];subprocess.run([str(binary)],check=True);results.append(dict(compiler=compiler,command=cmd,passed=True))
  report.update(passed=True,compiler_results=results,aggregate_counters=len(FIELDS),owner_maps=1,serializers=2,positive_main_gate_cases=1,negative_main_gate_cases=7,positive_cleanup_gate_cases=1,negative_cleanup_gate_cases=1,source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Exact production Frame aggregation/counter-map/twoJSON/gate fragments; no live selected cleanup claim')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
