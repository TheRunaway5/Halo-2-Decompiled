#!/usr/bin/env python3
"""Exact staged main-heap getter, both JSON serializers and truthful gates."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
from prepare_native_world_main_heap_release_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'))
  text=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();start=text.index('    result.native_main_heap_release_attempts=');end=text.index('    result.native_contact_entity_calls=',start);getter=text[start:end]
  text=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=text.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_RELEASE"))');end=text.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE"))',start);gate=text[start:end]
  for field in [*FIELDS,'refused_owners']:assert text.count('{"native_main_heap_release_'+field+'",native_frames.native_main_heap_release_'+field+'},')==2
  assignments=''.join('a.main_heap_release.'+value+'='+str(17+n)+';'for n,value in enumerate(FIELDS.values()))
  checks=''.join('check(r.native_main_heap_release_'+field+'=='+str(17+n)+');'for n,field in enumerate(FIELDS))
  rows=''.join('{"native_main_heap_release_'+field+'",r.native_main_heap_release_'+field+'},'for field in [*FIELDS,'refused_owners'])
  negatives=['b.native_main_heap_release_calls=0;','b.native_main_heap_release_attempts=2;','b.native_main_heap_release_wrapper_calls=0;','b.native_main_heap_release_ledger_calls=0;','b.native_main_heap_release_kernel_completed=0;','b.native_main_heap_release_refusals=1;','b.native_main_heap_release_unbound_services=1;','b.native_main_heap_release_refused_owners[0xdead]=1;','b.residual_owner_calls[0x232d20u]=1;','b.residual_owner_calls[0x232c80u]=1;']
  cpp='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
using namespace halo2;void check(bool yes){if(!yes)throw std::runtime_error("Main heap counter transport differs");}
NativeTu15PhysicsFrameExecutionStats get(const NativeTu15WorldIntegrationStats&world_integration){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames,bool removal=false){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_RELEASE","1",1);if(removal)setenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_LEDGER_REMOVAL","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_LEDGER_REMOVAL");
'''+gate+'''return passed;}
int main(){NativeTu15WorldIntegrationStats a;'''+assignments+'''a.main_heap_release.kernel.unbound_owners[0x1111]=3;auto r=get(a);
'''+checks+'''check(r.native_main_heap_release_refused_owners.at(0x1111)==3);nlohmann::json serialized={'''+rows+'''};check(serialized.size()==11);
NativeTu15PhysicsFrameExecutionStats p;p.native_main_heap_release_attempts=1;p.native_main_heap_release_calls=1;p.native_main_heap_release_wrapper_calls=1;p.native_main_heap_release_ledger_calls=1;p.native_main_heap_release_kernel_completed=1;check(gate(p));auto b=p;
'''+''.join('b=p;'+code+'check(!gate(b));'for code in negatives)+'''check(!gate(p,true));p.native_main_heap_release_matches=1;check(gate(p,true));return 0;}
''';(project/'counter.cpp').write_text(cpp);results=[]
  for compiler in('g++','clang++'):
   binary=project/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counter.cpp'),'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-2500:];subprocess.run([str(binary)],check=True);results.append(dict(compiler=compiler,command=cmd,passed=True))
  report.update(passed=True,compiler_results=results,getter_counters=len(FIELDS),getter_owner_maps=1,serializers=2,positive_gate_cases=2,negative_gate_cases=len(negatives)+1,source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Exact source counter transport and separate legitimate ledger-removal selection; no original physics or live claim')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
