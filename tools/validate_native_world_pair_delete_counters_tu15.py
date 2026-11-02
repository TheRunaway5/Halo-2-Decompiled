#!/usr/bin/env python3
"""Compile exact staged pair-deletion Getter/serializers/gate transport."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
from prepare_native_world_pair_delete_tu15 import FIELDS,PREFIX
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'))
  source=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();getter='\n'.join(line for line in source.splitlines()if line.startswith('    result.'+PREFIX));assert len(getter.splitlines())==8
  source=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_DELETION"))');end=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_ISLAND_WAKE"))',start);gate=source[start:end]
  for name in[*FIELDS,'refused_owners']:assert source.count('{"'+PREFIX+name+'",native_frames.'+PREFIX+name+'},')==2
  assignments=''.join('a.'+field+'='+str(17+n)+';'for n,field in enumerate(FIELDS.values()))
  checks=''.join('check(r.'+PREFIX+name+'=='+str(17+n)+');'for n,name in enumerate(FIELDS))
  cpp='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
using namespace halo2;void check(bool b){if(!b)throw std::runtime_error("Pair deletion transport differs");}
NativeTu15PhysicsFrameExecutionStats get(const NativeTu15WorldIntegrationStats&world_integration){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_PAIR_DELETION","1",1);
'''+gate+'''return passed;}
int main(){NativeTu15WorldIntegrationStats a;
'''+assignments+'''a.pair_deletion_scope.owners[0x316aa0]=9;auto r=get(a);'''+checks+'''
check(r.native_pair_deletion_refused_owners.at(0x316aa0)==9);
NativeTu15PhysicsFrameExecutionStats p;p.native_pair_deletion_attempts=1;p.native_pair_deletion_calls=1;check(gate(p));auto b=p;
b.native_pair_deletion_calls=0;check(!gate(b));b=p;b.native_pair_deletion_attempts=2;check(!gate(b));b=p;b.native_pair_deletion_refusals=1;check(!gate(b));b=p;b.native_pair_deletion_unqualified_owners=1;check(!gate(b));b=p;b.native_pair_deletion_nested_refusals=1;check(!gate(b));b=p;b.native_pair_deletion_refused_owners[0x316aa0]=1;check(!gate(b));b=p;b.residual_owner_calls[0x320260]=1;check(!gate(b));return 0;}
''';(project/'counter.cpp').write_text(cpp);results=[]
  for compiler in('g++','clang++'):
   binary=project/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counter.cpp'),'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-2500:];subprocess.run([str(binary)],check=True);results.append(dict(compiler=compiler,command=cmd,passed=True))
  report.update(passed=True,compiler_results=results,getter_counters=7,getter_owner_maps=1,serializers=2,positive_gate_cases=1,negative_gate_cases=7,source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Exact source counter transport; no original physics or live selection claim')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
