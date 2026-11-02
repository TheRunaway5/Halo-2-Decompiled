#!/usr/bin/env python3
"""Exact19 manager fields, normal/throw counts, twoJSON and positive gate."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
from prepare_native_world_manager_factory_binding_tu15 import FIELDS,PREFIX
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';base=a.source_root.resolve();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir();shutil.copytree(base/'include',project/'include')
  for path in(stage/'include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(stage/'include'))
  source=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();start=source.index('    const auto&pair_manager_factory=');end=source.index('    result.native_contact_manager_base_lock_releases=',start);getter=source[start:end].replace('impl_->native_physics_integrations.pair_manager_factory','activity')
  source=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();start=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_MANAGER_FACTORY"))');end=source.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE"))',start);gate=source[start:end]
  for name in FIELDS:assert source.count('{"'+PREFIX+name+'",native_frames.'+PREFIX+name+'},')==2
  assignments=''.join('a.'+field+'='+str(17+n)+';\n'for n,field in enumerate(FIELDS.values()))
  checks=''.join('check(r.'+PREFIX+name+'=='+str(17+n)+',"Wrong exact manager Getter");\n'for n,name in enumerate(FIELDS))
  failures=''.join('b=p;b.'+PREFIX+name+'=1;check(!gate(b),"Manager refusal/unknown admitted");\n'for name in('refusals','unqualified_factories','body_destructor_refusals','base_lock_refusals'))
  cpp='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
using namespace halo2;void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
NativeTu15PhysicsFrameExecutionStats get(const NativeTu15ActivationManagerFactoryBindingActivity&activity){NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
bool gate(NativeTu15PhysicsFrameExecutionStats native_frames){bool passed=true;nlohmann::json report;setenv("HALO2_REQUIRE_NATIVE_PAIR_MANAGER_FACTORY","1",1);
'''+gate+'''return passed;}
struct Memory:SourcePhysicsContactMemory{mutable unsigned reads{};void read(std::uint32_t,std::span<std::byte>)const override{++reads;throw std::runtime_error("Declared first read stop");}void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("No fixture writes admitted");}};
int main(){NativeTu15ActivationManagerFactoryBindingActivity a;
'''+assignments+'''auto r=get(a);
'''+checks+'''NativeTu15PhysicsFrameExecutionStats p;p.native_pair_manager_factory_attempts=1;p.native_pair_manager_factory_calls=1;p.native_pair_manager_factory_source_factories=1;p.native_pair_manager_factory_constructors=1;p.native_pair_manager_factory_collectors=1;p.native_pair_manager_factory_registrations=2;check(gate(p),"Positive manager gate rejected");auto b=p;
'''+failures+'''b=p;b.native_pair_manager_factory_calls=0;check(!gate(b),"Zero completion admitted");b=p;b.native_pair_manager_factory_attempts=2;check(!gate(b),"Partial attempt admitted");b=p;b.native_pair_manager_factory_registrations=0;check(!gate(b),"No registration admitted");b=p;b.residual_owner_calls[0x3232e0]=1;check(!gate(b),"Old manager transport admitted");
Memory m;NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};NativeTu15ActivationManagerFactoryBindingActivity unknown;
try{native_tu15_create_bound_activation_contact_manager(m,1,{0xdead},{2},{3},fp,{}, {},&unknown);throw 1;}catch(const std::runtime_error&){}
check(unknown.attempts==1&&unknown.completed==0&&unknown.refusals==1&&unknown.unqualified_factories==1&&unknown.managers.factories==0&&m.reads==0,"Unknown manager owner count/early refusal differs");
NativeTu15ActivationManagerFactoryBindingActivity stopped;try{native_tu15_create_bound_activation_contact_manager(m,1,{0x3232e0},{2},{3},fp,{}, {},&stopped);throw 1;}catch(const std::runtime_error&){}
check(stopped.attempts==1&&stopped.completed==0&&stopped.refusals==1&&stopped.managers.factories==1&&stopped.managers.constructors==0&&m.reads==1,"Native first-read throw count differs");
auto u=get(unknown),s=get(stopped);check(!gate(u)&&!gate(s)&&s.native_pair_manager_factory_refusals==1&&s.native_pair_manager_factory_source_factories==1,"Throw Getter/gate erased activity");return 0;}
'''
  (project/'counter.cpp').write_text(cpp);commands=[];results=[]
  for compiler in('g++','clang++'):
   binary=project/('counter-'+compiler.replace('+','x'));units=[str(base/('src/physics/native_tu15_'+name+'.cpp'))for name in('activation_contact_manager','island_activation','array_growth')]
   command=[compiler,'-std=c++20','-O2','-I'+str(project/'include'),str(project/'counter.cpp'),*units,'-o',str(binary)];commands.append(command);r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stderr[-4000:];subprocess.run([str(binary)],check=True)
  report.update(passed=True,commands=commands,results=results,fields=len(FIELDS),serializers=2,gate_positive_cases=1,gate_negative_cases=8,builder_unknown_and_first_read_throw_counts_checked=True,physics_state_supplied=False,
   source_hashes={str(path.relative_to(stage)):sha(path)for path in stage.rglob('*')if path.is_file()})
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
