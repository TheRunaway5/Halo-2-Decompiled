#!/usr/bin/env python3
"""Exact moving World getters/JSON/gate and fail-before-helper admission."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
from prepare_native_moving_shape_world_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--stage',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();stage=a.stage.resolve()/'staged';out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report={'passed':False,'owned_scratch':str(scratch),'retained_large_artifacts':[]}
 try:
  accept=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();live=(stage/'src/physics_live_world_tu15.cpp').read_text();world=(stage/'src/physics/native_tu15_world_integration_residuals.cpp').read_text()
  begin=accept.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_MOVING_SHAPE_PROCESS")) {');end=accept.index('  if(',begin+5);gate=accept[begin:end]
  getter='\n'.join(line.strip()for line in live.splitlines()if line.strip().startswith('result.native_moving_process_'));assert len(getter.splitlines())==len(FIELDS)
  begin=live.index('    for(const auto*owners:{&world_integration.moving_shape_process.refused_owners,');end=live.index('\n',live.index('result.native_moving_process_refused_owners[owner]+=count;',begin))+1;getter+='\n'+live[begin:end]
  sets=[];checks=[];expected={}
  for i,(name,path)in enumerate(FIELDS.items()):
   if name=='refusals':continue
   sets.append('world_integration.'+path+'='+str(17+i)+';');expected[name]=17+i
  sets.append('world_integration.moving_shape_process.refusals=1000;');expected['refusals']=1000+sum(expected[n]for n in ('observer_refusals','entry_fp_refusals','entry_resource_refusals'))
  for name,value in expected.items():checks.append('check(result.native_moving_process_'+name+'=='+str(value)+',"getter '+name+'");')
  groups=[[],[]]
  for name in (*FIELDS,'refused_owners'):
   rows=[line.strip().rstrip(',')for line in accept.splitlines()if line.strip().startswith('{"native_moving_process_'+name+'",')];assert len(rows)==2
   for index,row in enumerate(rows):groups[index].append(row)
  jsonchecks='\n'.join('check(j'+str(i)+'["native_moving_process_'+name+'"]=='+str(value)+',"JSON '+name+'");'for i in range(2)for name,value in expected.items())
  begin=world.index('  const auto process_moving=');end=world.index('    NativeTu15MovingShapeNativeServices native;',begin);preflight=world[begin:end]+'  };'
  code='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <iostream>
using namespace halo2;using J=nlohmann::json;
void check(bool ok,const char*s){if(!ok)throw std::runtime_error(s);}
bool evaluate(const NativeTu15PhysicsFrameExecutionStats&native_frames){bool passed=true;J report;
'''+gate+'''
check(report.contains("native_moving_shape_process_gate"),"gate absent");return passed;}
int main(){NativeTu15WorldIntegrationStats world_integration;NativeTu15PhysicsFrameExecutionStats result;
'''+''.join(sets)+'''
world_integration.moving_shape_process.refused_owners[0xabc]=3;world_integration.moving_shape_process.native_only.owners[0xabc]=2;world_integration.moving_shape_process.gameplay.unqualified_owners[0xdef]=7;
'''+getter+'\n'+''.join(checks)+'''
check(result.native_moving_process_refused_owners.at(0xabc)==5&&result.native_moving_process_refused_owners.at(0xdef)==7,"refused map aggregate");const auto&native_frames=result;
J j0={'''+','.join(groups[0])+'''};J j1={'''+','.join(groups[1])+'''};
'''+jsonchecks+'''
check(j0["native_moving_process_refused_owners"]==J(result.native_moving_process_refused_owners)&&j1["native_moving_process_refused_owners"]==J(result.native_moving_process_refused_owners),"JSON refused map");
setenv("HALO2_REQUIRE_NATIVE_MOVING_SHAPE_PROCESS","1",1);NativeTu15PhysicsFrameExecutionStats good;
good.native_moving_process_attempts=good.native_moving_process_calls=good.native_moving_process_helper_entries=good.native_moving_process_kernel_calls=good.native_moving_process_kernel_completed=2;check(evaluate(good),"positive gate");
for(unsigned i=0;i<12;++i){auto bad=good;switch(i){case0:bad.native_moving_process_attempts=bad.native_moving_process_calls=bad.native_moving_process_helper_entries=bad.native_moving_process_kernel_calls=bad.native_moving_process_kernel_completed=0;break;case1:++bad.native_moving_process_attempts;break;case2:++bad.native_moving_process_helper_entries;break;case3:++bad.native_moving_process_kernel_calls;break;case4:++bad.native_moving_process_kernel_completed;break;case5:++bad.native_moving_process_refusals;break;case6:++bad.native_moving_process_unprovided_reads;break;case7:++bad.native_moving_process_native_scope_refusals;break;case8:++bad.native_moving_process_unqualified_owners;break;case9:++bad.native_moving_process_flag_domain_refusals;break;case10:bad.native_moving_process_refused_owners[0xabc]=0;break;case11:bad.residual_owner_calls[0x2e5320]=0;break;}check(!evaluate(bad),"negative gate survived");}
for(unsigned mode=0;mode<4;++mode){NativeTu15WorldIntegrationStats activity;auto*stats=&activity;SourcePhysicsContactRecordObserver observe;if(mode==0)observe=[](const auto&){};NativeTu15PhysicsFrameFp current{0x27f,0x20,0x1fa0};NativeTu15PhysicsFrameScratch scratch{0x60000000u,0x10000u};if(mode==1)current.control_word=0x37f;if(mode==2)scratch.bytes=0x1000;const auto before=current;const auto end=std::uint64_t(scratch.first)+scratch.bytes;
'''+preflight+'''
bool refused=false;try{process_moving(0,0,0,0,0,current);}catch(const std::runtime_error&){refused=true;}check(refused==(mode<3)&&activity.native_moving_process_dispatches==1,"preflight admission");check(current.control_word==before.control_word&&current.status==before.status&&current.mxcsr==before.mxcsr,"preflight FP touched");check(activity.native_moving_process_observer_refusals==(mode==0)&&activity.native_moving_process_fp_refusals==(mode==1)&&activity.native_moving_process_resource_refusals==(mode==2),"preflight refusal truth");}
std::cout<<"33 getters,68 JSON fields including maps,12 negative gates and4 exact preflight models passed\\n";}
'''
  # Keep valid C++ switch spelling while leaving values literal/source-derived.
  import re
  code=re.sub(r'\bcase(\d+):',r'case \1:',code);p=scratch/'test.cpp';p.write_text(code);report['exact_probe_sha256']=sha(p);report['results']=[]
  for cc in ('g++','clang++'):
   exe=scratch/cc;command=[cc,'-std=c++20','-O2','-I'+str(stage/'include'),'-I'+str(ROOT/'include'),str(p),'-o',str(exe)];b=subprocess.run(command,capture_output=True,text=True);(out/(cc+'-build.log')).write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr[-5000:];execution=subprocess.run([str(exe)],capture_output=True,text=True);(out/(cc+'-run.log')).write_text(execution.stdout+execution.stderr);report['results'].append({'compiler':cc,'exit_code':execution.returncode});assert execution.returncode==0,execution.stderr
  report.update(passed=True,fields=33,json_scalar_fields=66,json_maps=2,negative_gates=12,exact_preflight_models=4,full_native_helper_completion_evidence='0bf0369 component proof, separately qualified')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
