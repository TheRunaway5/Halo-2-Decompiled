#!/usr/bin/env python3
"""Compile exact sweep getter/JSON/gate fragments and bound attempt lifetime."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess
from prepare_native_broad_phase_edges_world_tu15 import FIELDS
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();stage=a.stage.resolve()/'staged';out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  accept=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();live=(stage/'src/physics_live_world_tu15.cpp').read_text();start=accept.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_BROAD_PHASE_EDGES")) {');end=accept.index('  if(',start+5);gate=accept[start:end];getter='\n'.join(x.strip()for x in live.splitlines()if 'result.native_broad_phase_edge_' in x);assert len(getter.splitlines())==len(FIELDS)
  sets='\n'.join('world_integration.broad_phase_edges.'+path+'='+str(17+i)+';'for i,(name,path)in enumerate(FIELDS.items()))
  checks='\n'.join('check(result.native_broad_phase_edge_'+name+'=='+str(17+i)+',"sparse getter '+name+'");'for i,name in enumerate(FIELDS))
  groups=[[],[]]
  for name in FIELDS:
   lines=[x.strip().rstrip(',')for x in accept.splitlines()if x.strip().startswith('{"native_broad_phase_edge_'+name+'",')];assert len(lines)==2,(name,len(lines))
   for i,line in enumerate(lines):groups[i].append(line)
  jsonchecks='\n'.join('check(json'+str(i)+'["native_broad_phase_edge_'+name+'"]=='+str(17+n)+',"JSON path'+str(i)+' '+name+'");'for i in range(2)for n,name in enumerate(FIELDS))
  code='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;using J=nlohmann::json;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
bool evaluate(const NativeTu15PhysicsFrameExecutionStats&native_frames){bool passed=true;J report;
'''+gate+'''
check(report.contains("native_broad_phase_edges_gate"),"exact gate missing");return passed;}
struct Memory:SourcePhysicsContactMemory {void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("actual model read failure");}void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("unexpected write");}};
int main(){NativeTu15WorldIntegrationStats world_integration;NativeTu15PhysicsFrameExecutionStats result;
'''+sets+'\n'+getter+'\n'+checks+'''
const auto&native_frames=result;
J json0={'''+',\n'.join(groups[0])+'''};
J json1={'''+',\n'.join(groups[1])+'''};
'''+jsonchecks+'''
setenv("HALO2_REQUIRE_NATIVE_BROAD_PHASE_EDGES","1",1);
NativeTu15PhysicsFrameExecutionStats positive;positive.native_broad_phase_edge_attempts=2;positive.native_broad_phase_edge_calls=2;positive.native_broad_phase_edge_objects=4;check(evaluate(positive),"positive gate");
for(unsigned kind=0;kind<5;++kind){auto bad=positive;if(kind==0){bad.native_broad_phase_edge_calls=0;bad.native_broad_phase_edge_attempts=0;}if(kind==1)bad.native_broad_phase_edge_objects=0;if(kind==2)++bad.native_broad_phase_edge_attempts;if(kind==3)++bad.native_broad_phase_edge_refusals;if(kind==4)bad.residual_owner_calls[0x2ea380u]=0;check(!evaluate(bad),"negative gate did not reject");}
Memory memory;NativeTu15PhysicsFrameFp fp{0x27f,0x320,0x1f80};NativeTu15BroadPhaseEdgeBindingActivity activity;NativeTu15ArrayGrowthServices storage;
native_tu15_update_bound_broad_phase_edges(memory,{0,0,0,0,{0},{0}},fp,storage,{},&activity);check(activity.attempts==1&&activity.completed==1&&activity.refusals==0&&activity.edges.objects==0,"empty bound lifetime");
bool failed=false;try{native_tu15_update_bound_broad_phase_edges(memory,{0,16,32,1,{0},{0}},fp,storage,{},&activity);}catch(const std::runtime_error&e){failed=std::string(e.what())=="actual model read failure";}check(failed&&activity.attempts==2&&activity.completed==1&&activity.refusals==1,"bound exception lifetime");
std::cout<<"10 exact getters,20 JSON fields,5 negative+positive gate controls and real bound completion/refusal lifetimes passed\\n";}
'''
  project=scratch/'project';project.mkdir();shutil.copytree(stage/'include',project/'include');(project/'test.cpp').write_text(code);report['exact_probe_sha256']=sha(project/'test.cpp');report['source_sha256']={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()};report['results']=[]
  for cc in ('g++','clang++'):
   exe=scratch/('probe-'+cc);command=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(project/'include'),'-I'+str(ROOT/'include'),str(project/'test.cpp'),str(stage/'src/physics/native_tu15_broad_phase_edges.cpp'),str(ROOT/'src/physics/native_tu15_array_growth.cpp'),str(ROOT/'src/physics/native_tu15_array_allocator.cpp'),'-o',str(exe)]
   r=subprocess.run(command,capture_output=True,text=True);(out/(cc+'-compile.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,'counter compile '+cc;r=subprocess.run([str(exe)],capture_output=True,text=True);(out/(cc+'-run.log')).write_text(r.stdout+r.stderr);report['results'].append(dict(compiler=cc,exit_code=r.returncode,command=command));assert r.returncode==0,'counter truth '+cc
  report.update(passed=True,scalar_fields=10,json_fields=20,negative_gate_controls=5,positive_gate_controls=1,bound_empty_and_exception_lifetimes=True)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
