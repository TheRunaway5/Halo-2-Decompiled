#!/usr/bin/env python3
"""Compile exact World aggregation/getter/two JSON paths and joint gates."""
import argparse,json,os,re,shutil,subprocess
from pathlib import Path
from prepare_native_joint_constraint_world_tu15 import FIELDS,PREFIX
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);stage=a.stage/'staged';report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  pins=json.loads((a.stage/'source-pins.json').read_text());assert all(sha(stage/n)==h for n,h in pins['staged_hashes'].items());include=scratch/'include';shutil.copytree(a.source_root/'include',include)
  for path in(stage/'include').rglob('*.hpp'):target=include/path.relative_to(stage/'include');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
  world=(stage/'src/physics/native_tu15_world_integration_residuals.cpp').read_text();live=(stage/'src/physics/physics_live_world_tu15.cpp').read_text();accept=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();record=[];getter=[];json_paths=[[],[]]
  for field in FIELDS:
   name=PREFIX+field
   r=re.findall(r'^\s*stats->'+name+r'\+=activity\.joint_builder\.[^;]+;$',world,re.M);g=re.findall(r'^\s*result\.'+name+r'=impl_->native_physics_integrations\.'+name+r';$',live,re.M);rows=re.findall(r'^\s*\{"'+name+r'",native_frames\.'+name+r'\},$',accept,re.M);assert len(r)==len(g)==1 and len(rows)==2,name;record+=r;getter+=g
   for n,row in enumerate(rows):json_paths[n].append(row.strip())
  begin=accept.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_JOINT_CONSTRAINT_BUILDER"))');end=accept.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_RELEASE"))',begin);gate=accept[begin:end]
  tests=[]
  for n,(field,path)in enumerate(FIELDS.items()):tests.append('a.joint_builder.'+path+'='+str(n+17)+';')
  checks=['check(result.'+PREFIX+field+'=='+str(2*(n+17))+'u,"record/getter '+field+'");'for n,field in enumerate(FIELDS)]
  positive=['calls=2','attempts=2','kernel_entries=2','kernel_completed=2','headers=2','angular_rows=4','motors=2','linear_requests=2','linear_calls=2','linear_completed=2','linear_rows=6']
  failures=['calls=0','attempts=3','kernel_entries=1','kernel_completed=1','headers=1','angular_rows=3','motors=1','linear_requests=1','linear_calls=1','linear_completed=1','linear_rows=5','refusals=1','unsupported_fp=1','linear_refusals=1','unbound_linear=1','linear_unaligned=1','linear_unsupported_fp=1']
  code='''#include "halo2/native_tu15_constraint_step_residuals.hpp"
#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <iostream>
using namespace halo2;using J=nlohmann::json;
void check(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
void aggregate(NativeTu15WorldIntegrationStats&out,const NativeTu15ConstraintBindingActivity&activity){auto*stats=&out;
'''+ '\n'.join(record)+'''
}
struct Impl{NativeTu15WorldIntegrationStats native_physics_integrations;};
NativeTu15PhysicsFrameExecutionStats get(const Impl&value){auto*impl_=&value;NativeTu15PhysicsFrameExecutionStats result;
'''+ '\n'.join(getter)+'''return result;}
'''
  for n,rows in enumerate(json_paths):code+='J json'+str(n)+'(const NativeTu15PhysicsFrameExecutionStats&native_frames){return J{'+''.join(rows)+'};}\n'
  code+='''bool gate(const NativeTu15PhysicsFrameExecutionStats&native_frames,bool limits=false){setenv("HALO2_REQUIRE_NATIVE_JOINT_CONSTRAINT_BUILDER","1",1);if(limits)setenv("HALO2_REQUIRE_NATIVE_JOINT_CONSTRAINT_LIMITS","1",1);else unsetenv("HALO2_REQUIRE_NATIVE_JOINT_CONSTRAINT_LIMITS");bool passed=true;J report;
'''+gate+'''return passed;}
int main(){try{NativeTu15ConstraintBindingActivity a;'''+''.join(tests)+'''Impl out;aggregate(out.native_physics_integrations,a);aggregate(out.native_physics_integrations,a);const auto result=get(out);
'''+''.join(checks)+'''check(json0(result)==json1(result)&&json0(result).size()==20,"Both real serializers");NativeTu15PhysicsFrameExecutionStats good;'''+''.join('good.'+PREFIX+s+';'for s in positive)+'''check(gate(good),"Positive nativejoint gate");
'''+''.join(' {auto bad=good;bad.'+PREFIX+s+';check(!gate(bad),"Negative gate '+s+'");}\n'for s in failures)+''' {auto bad=good;bad.residual_owner_calls[0x31d930]=1;check(!gate(bad),"Old owner remains");}check(!gate(good,true),"No unexercised limit success");good.native_joint_builder_limits=1;check(gate(good,true),"Positive separate limits");std::cout<<J{{"passed",true},{"fields",20},{"JSON_paths",2},{"negative_gates",19}}.dump()<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
''';(scratch/'driver.cpp').write_text(code);results=[]
  for cc in('g++','clang++'):
   exe=scratch/('counter-'+cc);command=[cc,'-std=c++20','-O0','-I'+str(include),str(scratch/'driver.cpp'),'-o',str(exe)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(cc+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:];r=subprocess.run([str(exe)],capture_output=True,text=True);(a.output/(cc+'-run.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr;results.append(dict(compiler=cc,passed=True,result=json.loads(r.stdout)))
  report.update(passed=True,results=results,source_hashes=pins['staged_hashes'],exact_source_fragments=True,aggregation_includes_exception_record=True,scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
