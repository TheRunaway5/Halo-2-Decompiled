#!/usr/bin/env python3
"""Exact pair-wake Getter/JSON/gate transport; no runtime results supplied."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'))
  source=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();getter='\n'.join(line for line in source.splitlines()if line.startswith('    result.native_pair_island_wake_'))
  assert getter.count('result.native_pair_island_wake_')==3
  source=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE"))');end=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION"))',start);gate=source[start:end]
  for name in('attempts','calls','refusals'):assert source.count('{"native_pair_island_wake_'+name+'",native_frames.native_pair_island_wake_'+name+'},')==2
  cpp='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
using namespace halo2;
void check(bool b){if(!b)throw std::runtime_error("Pair wake transport differs");}
NativeTu15PhysicsFrameExecutionStats get(const NativeTu15WorldIntegrationStats&world_integration){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE","1",1);
'''+gate+'''return passed;}
int main(){NativeTu15WorldIntegrationStats a;a.pair_island_wake={17,13,4};auto r=get(a);
check(r.native_pair_island_wake_attempts==17&&r.native_pair_island_wake_calls==13&&r.native_pair_island_wake_refusals==4);
NativeTu15PhysicsFrameExecutionStats p;p.native_pair_island_wake_attempts=1;p.native_pair_island_wake_calls=1;check(gate(p));
auto b=p;b.native_pair_island_wake_calls=0;check(!gate(b));b=p;b.native_pair_island_wake_attempts=2;check(!gate(b));
b=p;b.native_pair_island_wake_refusals=1;check(!gate(b));b=p;b.native_world_activation_refusals=1;check(!gate(b));
b=p;b.native_world_activation_residual_calls=1;check(!gate(b));b=p;b.residual_owner_calls[0x316d40]=1;check(!gate(b));
check(!gate(r));return 0;}
''';(project/'counter.cpp').write_text(cpp);results=[]
  for compiler in('g++','clang++'):
   binary=project/('counter-'+compiler.replace('+','x'));cmd=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counter.cpp'),'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:];subprocess.run([str(binary)],check=True);results.append(dict(compiler=compiler,command=cmd,passed=True))
  report.update(passed=True,compiler_results=results,getter_fields=3,serializers=2,positive_gate_cases=1,negative_gate_cases=7,runtime_kernel_claimed=False,
   source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()})
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
