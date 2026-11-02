#!/usr/bin/env python3
"""Exact staged reverse Getter/serializers/parent-vs-positive-vector gates."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import save
from prepare_native_reversed_contact_process_binding_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root.resolve()/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'))
  source=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();start=source.index('    result.native_reversed_contact_process_attempts=');end=source.index('    result.native_contact_manager_cleanups=',start);getter=source[start:end];acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_REVERSED_CONTACT_PROCESS")');end=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_COMPOUND_CONTACT_PROCESS"))',start);gates=acceptance[start:end]
  for field in FIELDS:assert acceptance.count('{"'+field+'",native_frames.'+field+'},')==2
  cpp='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
using namespace halo2;void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Impl{NativeTu15WorldIntegrationStats native_physics_integrations;};struct Harness{Impl*impl_;NativeTu15PhysicsFrameExecutionStats get(){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}};
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames,bool normals){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_REVERSED_CONTACT_PROCESS","1",1);if(normals)setenv("HALO2_REQUIRE_NATIVE_REVERSED_CONTACT_NORMALS","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_REVERSED_CONTACT_NORMALS");
'''+gates+'''return passed;}
int main(){Impl x;auto&s=x.native_physics_integrations.contact_process;s.reversed_attempts=11;s.reversed_completed=12;s.reversed_refusals=13;s.reversed_wrapped.calls=14;s.reversed_compound.calls=15;s.reversed_wrapped.child_returns=16;s.reversed_compound.child_returns=17;s.reversed_wrapped.normals_reversed=18;s.reversed_compound.normals_reversed=19;s.remaining_child_calls=28;s.wrapped.child_calls=23;s.compound.child_calls=24;s.reversed_wrapped.child_calls=25;s.reversed_compound.child_calls=26;
auto r=Harness{&x}.get();check(r.native_reversed_contact_process_attempts==11&&r.native_reversed_contact_process_completions==12&&r.native_reversed_contact_process_refusals==13,"Reverse completion Getter differs");check(r.native_reversed_wrapped_contact_process_calls==14&&r.native_reversed_compound_contact_process_calls==15&&r.native_reversed_contact_process_child_returns==33&&r.native_reversed_contact_normal_vectors==37,"Reverse sum Getter differs");check(r.native_contact_process_child_calls==98&&r.native_contact_process_child_residuals==28,"Existing remaining304350/child diagnostics lost");
NativeTu15PhysicsFrameExecutionStats p;p.native_reversed_contact_process_attempts=1;p.native_reversed_contact_process_completions=1;p.native_reversed_contact_process_child_returns=1;check(gate(p,false),"Closed parent gate rejected");check(!gate(p,true),"No generatednormal admitted as positive");p.native_reversed_contact_normal_vectors=3;check(gate(p,true),"Positive normal vectors rejected");auto b=p;b.native_reversed_contact_process_refusals=1;check(!gate(b,false),"Reverse refusal admitted");b=p;b.native_contact_process_refusals=1;check(!gate(b,true),"Childbinding refusal admitted");b=p;b.native_reversed_contact_process_attempts=2;check(!gate(b,false),"Incompleteparent admitted");b=p;b.native_reversed_contact_process_child_returns=0;check(!gate(b,false),"Child neverreturned admitted");for(auto owner:{0x2f7480u,0x2f7fb0u}){b=p;b.residual_owner_calls[owner]=1;check(!gate(b,false),"Old reverseparent admitted");}
// The narrow parent gate does not misreport remaining real child work as a
// native-only closure. Existing residual fields remain available unchanged.
b=p;b.native_contact_process_child_residuals=2;b.residual_owner_calls[0x304350u]=2;check(gate(b,false),"Parent gate incorrectly conflated remainingchild closure");
return 0;}
''';(project/'counters.cpp').write_text(cpp);commands=[]
  for compiler in('g++','clang++'):
   binary=project/('counters-'+compiler.replace('+','x'));command=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counters.cpp'),'-o',str(binary)];commands.append(command);r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:];subprocess.run([str(binary)],check=True)
  report.update(passed=True,commands=commands,new_fields=7,serializers=2,remaining_child_fields_preserved=True,remaining304350_closure_claimed=False,positive_vector_gate_distinct=True)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
