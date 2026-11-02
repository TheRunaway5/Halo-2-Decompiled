#!/usr/bin/env python3
"""Exact added-listener stats/getter/twoJSON and positive native gate truth."""
import argparse,hashlib,json,os,re,shutil,subprocess
from pathlib import Path
from prepare_native_phantom_added_world_binding_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();stage=a.stage/'staged';report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],authored_values_are_diagnostics_only=True)
 try:
  pins=json.loads((a.stage/'source-pins.json').read_text());assert all(sha((stage/n).read_bytes())==v for n,v in pins['staged_hashes'].items());project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):target=project/'include'/path.relative_to(stage/'include');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
  world=(stage/'src/physics/native_tu15_world_integration_residuals.cpp').read_text();start=world.index('            const auto collect_added=[&]{if(stats){');end=world.index('            try{native_tu15_notify_bound_gameplay_phantom_added',start);aggregate=world[start:end];branch=world[start:world.index('            return;',end)];assert branch.count('collect_added();')==2 and'catch(...){collect_added();throw;}'in branch
  getter=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();getter_lines=[];serializers=[[],[]];initializers=[];assertions=[];sentinel=100;expected={}
  for f,expression in FIELDS.items():
   found=re.findall(r'^    result\.'+f+r'=.*;$',getter,re.M);assert len(found)==1;getter_lines+=found;found=re.findall(r'^\s*\{"'+f+r'",native_frames\.'+f+r'\},$',acceptance,re.M);assert len(found)==2
   for n in range(2):serializers[n].append(found[n].strip())
   sentinel+=11;initializers.append('activity.'+expression+'='+str(sentinel)+'u;');expected[f]=sentinel;assertions.append('check(stats.'+f+'=='+str(sentinel)+'u,"Wrong aggregate '+f+'");check(Harness{&impl}.get().'+f+'==stats.'+f+',"Wrong getter '+f+'");')
  start=acceptance.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_GAMEPLAY_PHANTOM_ADDED"))');end=acceptance.index('\n  }\n',start)+len('\n  }\n');gate=acceptance[start:end]
  source='''#include "halo2/native_tu15_phantom_added_binding.hpp"
#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
using namespace halo2;using J=nlohmann::json;
void check(bool v,const char*why){if(!v)throw std::runtime_error(why);}
struct Impl{NativeTu15WorldIntegrationStats native_physics_integrations;};
struct Harness{Impl*impl_;NativeTu15PhysicsFrameExecutionStats get(){NativeTu15PhysicsFrameExecutionStats result;
'''+ '\n'.join(getter_lines)+'''\nreturn result;}};
void collect(NativeTu15WorldIntegrationStats*stats,const NativeTu15PhantomAddedBindingActivity&activity){
'''+aggregate+'''collect_added();}
bool passes(NativeTu15PhysicsFrameExecutionStats native_frames){J report;bool passed=true;
'''+gate+'''return passed;}
J serialize0(NativeTu15PhysicsFrameExecutionStats native_frames){return J{'''+ '\n'.join(serializers[0])+'''};}
J serialize1(NativeTu15PhysicsFrameExecutionStats native_frames){return J{'''+ '\n'.join(serializers[1])+'''};}
int main(){NativeTu15PhantomAddedBindingActivity activity;
'''+''.join(initializers)+'''Impl impl;auto&stats=impl.native_physics_integrations;collect(&stats,activity);
'''+''.join(assertions)+'''auto frame=Harness{&impl}.get();auto first=serialize0(frame),second=serialize1(frame);check(first==second&&first.size()==8,"JSON routes differ");
'''+''.join('check(first.at("'+f+'") == '+str(v)+'u,"Wrong JSON '+f+'");'for f,v in expected.items())+'''collect(&stats,activity);
'''+''.join('check(stats.'+f+'=='+str(v*2)+'u,"Repeated collection lost activity");'for f,v in expected.items())+'''collect(nullptr,activity);setenv("HALO2_REQUIRE_NATIVE_GAMEPLAY_PHANTOM_ADDED","1",1);
NativeTu15PhysicsFrameExecutionStats good;good.native_phantom_added_listener_attempts=1;good.native_phantom_added_listener_calls=1;check(passes(good),"Qualified positive added gate rejected");check(!passes({}),"Added gate admitted no calls");
'''
  negatives={'native_phantom_added_listener_attempts':2,'native_phantom_added_listener_calls':0,'native_phantom_added_listener_refusals':1,'native_phantom_added_unqualified_deletions':1,'native_phantom_pair_create_unqualified_owners':1,'native_pair_creation_refusals':1}
  for f,v in negatives.items():source+='{auto bad=good;bad.'+f+'='+str(v)+';check(!passes(bad),"Added gate admitted '+f+'");}\n'
  for owner in(0x282700,0xb1870):source+='{auto bad=good;bad.residual_owner_calls['+str(owner)+']=1;check(!passes(bad),"Added gate admitted old owner");}\n'
  source+='auto deletion=good;deletion.native_phantom_added_delete_requests=1;check(passes(deletion),"Native deletion diagnostic rejected");deletion.residual_owner_calls[0xba570u]=1;check(!passes(deletion),"Added gate admitted original deletion");unsetenv("HALO2_REQUIRE_NATIVE_GAMEPLAY_PHANTOM_ADDED");check(passes({}),"Disabled gate changed admission");return 0;}\n';(project/'counters.cpp').write_text(source);commands=[]
  for compiler in('g++','clang++'):
   target=project/compiler.replace('+','x');command=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counters.cpp'),'-o',str(target)];commands.append(command);r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-8192:];subprocess.run([str(target)],check=True)
  report.update(passed=True,commands=commands,fields=len(FIELDS),normal_and_throw_aggregation=True,serializer_occurrences_per_field=2,negative_gate_controls=len(negatives)+3,positive_gate_controls=3,source_pins_sha256=sha((a.stage/'source-pins.json').read_bytes()),aggregate_sha256=sha(aggregate.encode()),gate_sha256=sha(gate.encode()),scope=__doc__)
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
