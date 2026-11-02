#!/usr/bin/env python3
"""Compile exact queue getter/two-JSON/gate leaves with distinct source sentinels."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from prepare_native_pair_creation_queue_world_binding_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();project=scratch/'project';project.mkdir();report={'passed':False,'owned_scratch':str(scratch),'retained_large_artifacts':[]}
 try:
  shutil.copytree(a.source_root/'include',project/'include')
  for f in(a.stage/'staged/include').rglob('*'):
   if f.is_file():t=project/'include'/f.relative_to(a.stage/'staged/include');t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(f,t)
  getter=(a.stage/'staged/src/physics/physics_live_world_tu15.cpp').read_text();accept=(a.stage/'staged/tests/physics_multiplayer_acceptance.cpp').read_text();lines=[];jsons=[[],[]]
  for field in FIELDS:
   found=[l for l in getter.splitlines()if l.strip().startswith('result.'+field+'=')];assert len(found)==1;lines+=found
   found=[l for l in accept.splitlines()if'{"'+field+'",native_frames.'+field+'}'in l];assert len(found)==2
   for n in range(2):jsons[n].append(found[n])
  start=accept.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES"))');end=accept.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP"))',start);gate=accept[start:end]
  code='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
using namespace halo2;using J=nlohmann::json;
void check(bool value){if(!value)throw std::runtime_error("Actual queue diagnostic transport/gate differs");}
bool test_gate(NativeTu15PhysicsFrameExecutionStats native_frames){J report;bool passed=true;
'''+gate+'''return passed&&report.at("native_pair_creation_queues_gate").get<bool>();}
int main(){
struct Impl{NativeTu15WorldIntegrationStats native_physics_integrations;};Impl impl;auto*impl_=&impl;NativeTu15PhysicsFrameExecutionStats result;
'''
  for n,(field,member)in enumerate(FIELDS.items()):code+='impl.native_physics_integrations.pair_creation_queues.'+member+'='+str(103+17*n)+';\n'
  code+='\n'.join(lines)+'\nauto native_frames=result;J first={\n'+'\n'.join(jsons[0])+'\n};J second={\n'+'\n'.join(jsons[1])+'\n};\n'
  for n,(field,member)in enumerate(FIELDS.items()):code+='check(result.'+field+'=='+str(103+17*n)+'&&first.at("'+field+'")==' +str(103+17*n)+'&&second.at("'+field+'")==' +str(103+17*n)+');\n'
  code+='''setenv("HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES","1",1);
NativeTu15PhysicsFrameExecutionStats good;good.native_pair_merge_queue_attempts=7;good.native_pair_merge_queue_calls=7;good.native_pair_defer_queue_attempts=11;good.native_pair_defer_queue_calls=11;check(test_gate(good));
'''
  for mutation in ['native_pair_merge_queue_calls=0','native_pair_defer_queue_calls=0','native_pair_merge_queue_attempts=8','native_pair_defer_queue_attempts=12','native_pair_merge_queue_refusals=1','native_pair_defer_queue_refusals=1','residual_owner_calls[0x317200u]=1','residual_owner_calls[0x320da0u]=1']:code+='{auto bad=good;bad.'+mutation+';check(!test_gate(bad));}\n'
  code+='}\n';(project/'driver.cpp').write_text(code);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(QueueCounterTruth LANGUAGES CXX)\ninclude(CTest)\nadd_executable(counter_truth driver.cpp)\ntarget_include_directories(counter_truth PRIVATE include)\ntarget_compile_features(counter_truth PRIVATE cxx_std_20)\nadd_test(NAME queue_counter_truth COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:counter_truth>)\n');results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','counter_truth','--filter','^queue_counter_truth$','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler];r=subprocess.run(cmd);results.append({'compiler':compiler,'exit_code':r.returncode,'command':cmd})
  report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,fields=FIELDS,negative_gate_profiles=8,scope='Exact6getter andtwoJSON leaves plus actualpositivegate; independent nativekernel/binding/original/model/live proofs separate.',stage_hashes={str(p.relative_to(a.stage/'staged')):sha(p)for p in(a.stage/'staged').rglob('*')if p.is_file()},driver_sha256=sha(project/'driver.cpp'))
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
