#!/usr/bin/env python3
"""Physical317200/320DA0 + native reserve/growth/routing/pool composition."""
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases():
 U=base.UI;H=U+0x120;D=base.DATA;rows=[]
 def add(name,entry,count,capacity,owned=False,**extra):
  pair=U+0x300;words=[(0x120,D),(0x124,count),(0x128,capacity if owned else capacity|0x80000000),(0x300,U+0x400),(0x304,U+0x500)]
  rows.append(dict(name=name,entry=entry,receiver=H-(0x20 if entry==0x317200 else 0x74),arguments=[U+0x400,U+0x500]if entry==0x317200 else[pair],header=H,count=count,capacity=capacity if owned else capacity|0x80000000,user_words=words,copy_stride=8,**({'prepare_bytes':capacity*8}if owned else{}),**extra))
 for entry in(0x317200,0x320da0):
  for count in(0,1,2,3,4):
   add(f'{entry:x}_grow_borrowed_{count}',entry,count,count)
   add(f'{entry:x}_available_{count}',entry,count,count+2)
  for count in(1,4):add(f'{entry:x}_grow_owned_{count}',entry,count,count,owned=True)
 for name,entry,p,n,writing in [('initial_count',0x317200,H+4,1,False),('capacity',0x317200,H+8,1,False),('header_publish',0x317200,H+4,1,True),('first_store',0x317200,D,1,True),('second_store',0x317200,D+4,1,True),('defer_count',0x320da0,H+4,1,False),('defer_pair_first',0x320da0,U+0x300,1,False),('defer_pair_second',0x320da0,U+0x304,1,False),('defer_counter',0x320da0,H+4,1,True)]:
  add('fault_'+name,entry,0,4,fault=dict(address=p,bytes=4,occurrence=n,write=writing))
 for name,entry,pc,p,value in [('merge_first_count_reread',0x317200,0x317230,H+4,2),('merge_second_backing_reread',0x317200,0x317238,H,D+0x400),('defer_second_input_reread',0x320da0,0x320dca,U+0x304,U+0x700),('defer_count_reread',0x320da0,0x320dca,H+4,2)]:
  add(name,entry,0,4,mutations=[dict(instruction=pc,address=p,written=value.to_bytes(4,'little').hex())])
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=['include/halo2/native_tu15_pair_creation_queues.hpp','src/physics/native_tu15_pair_creation_queues.cpp','include/halo2/native_tu15_array_reserve.hpp','src/physics/native_tu15_array_reserve.cpp','include/halo2/native_tu15_array_growth.hpp','src/physics/native_tu15_array_growth.cpp','include/halo2/native_tu15_array_allocator.hpp','src/physics/native_tu15_array_allocator.cpp','tools/validate_native_pair_creation_queues_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp'];pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image);base.OWNERS=(*base.OWNERS,0x2e31e0,0x317200,0x320da0);source=inspect.getsource(base.original);source=source.replace('case.get("count",0)*case["arguments"][1]','case.get("count",0)*case["copy_stride"]');environment=dict(base.__dict__,HEADER=base.UI+0x120);exec(source,environment);all_cases=cases();fixture,authority=environment['original'](image,banks,all_cases,scratch);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in ['pair_creation_queues','array_reserve','array_growth','array_allocator']:shutil.copyfile(ROOT/f'src/physics/native_tu15_{name}.cpp',project/(name+'.cpp'))
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();needle='        else throw std::runtime_error("Unauthenticated native array test owner");';assert driver.count(needle)==1;driver='#include "halo2/native_tu15_pair_creation_queues.hpp"\n'+driver.replace(needle,'        else if(entry==0x317200u)native_tu15_queue_body_pair_merge(memory,receiver,args[0],args[1],fp,growth,observe);\n        else if(entry==0x320da0u)native_tu15_defer_collision_pair(memory,receiver,args[0],fp,growth,observe);\n'+needle);(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(pair_queues LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(queues driver.cpp pair_creation_queues.cpp array_reserve.cpp array_growth.cpp array_allocator.cpp)\ntarget_include_directories(queues PRIVATE include)\ntarget_compile_features(queues PRIVATE cxx_std_20)\ntarget_link_libraries(queues PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME pair_queues COMMAND queues "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','queues','--filter','^pair_queues$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(cmd);r=subprocess.run(cmd);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native pair creation queues differ'
  target=project/'pair_creation_queues.cpp';text=target.read_text();mutants=[('wrong_body_word','first_data+first_count*8u-8u,first);','first_data+first_count*8u-8u,first^1u);'),('wrong_defer_stride','data+current_count*8u,first);','data+current_count*4u,first);'),('wrong_count','header+4u,after+1u);','header+4u,after+2u);')]
  for name,before,after in mutants:
   assert text.count(before)==1;target.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Unchanged original failed to reject '+name;target.write_text(text)
  report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),source_authority=authority,input_provenance=provenance,original_sha256=NEW_PIN,original_cases=len(all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),actual_allocator_calls=sum(len(c['allocator_calls'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),observer_mutations=sum(c['mutations_applied']for c in all_cases),results=results,commands=commands,negative_controls=[m[0]for m in mutants],executed_sites=sorted({pc for c in all_cases for pc in c['executed_sites']}),scope='Declared valid native8B queue headers, borrowedstorage or genuinely allocatedownedstorage; all sourcequeue+reserve/grow/router/pool DATA compared; no capacity/pointeroutcome supplied; actualcreatecomposition separate')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
