#!/usr/bin/env python3
"""Physical TU1.5 contact byte-ID allocation with genuine native pool growth."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
UNITS=('native_tu15_contact_point_ids.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def cases():
 rows=[]
 for count in (0,1,2,4,7,16,32):
  holes=sorted(set((-1,0,max(0,count-1),count//2))) if count else [-1]
  for hole in holes:
   for value in (0,1,255,0x12345678,0xffffffff):
    for growth in (False,True):
     raw=bytearray((n+1)%254 for n in range(64))
     if hole>=0 and count:raw[hole]=255
     if hole>0:raw[0]=255
     row=dict(name=f'id_{count}_{hole}_{value:x}_{int(growth)}',entry=0x325f20,receiver=base.HEADER,header=base.HEADER,arguments=[value],count=count,capacity=0x80000000|(count if growth else count+4),user_words=[(0x1000+i,struct.unpack_from('<I',raw,i)[0])for i in range(0,64,4)])
     rows.append(row)
 for count in (4,16):
  raw=bytes(range(1,65))
  rows.append(dict(name=f'owned_growth_{count}',entry=0x325f20,receiver=base.HEADER,header=base.HEADER,arguments=[0x42],count=count,capacity=count,prepare_bytes=count,prepare_stride=1,user_words=[(0x1000+i,struct.unpack_from('<I',raw,i)[0])for i in range(0,64,4)]))
 # A byte-store observer changes count; the source MOV count must reread it.
 rows.append(dict(name='append_count_reread',entry=0x325f20,receiver=base.HEADER,header=base.HEADER,arguments=[0x42],count=0,capacity=0x80000004,mutations=[dict(instruction=0x325f59,address=base.HEADER+4,written=struct.pack('<I',7).hex())]))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],allocation_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image);base.OWNERS=(0x325f20,*base.OWNERS)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace('case.get("count",0)*case["arguments"][1]','case.get("count",0)*case.get("prepare_stride",1)');ns=dict(base.__dict__);exec(source,ns)
  rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+8 for c in rows)
  normal_rows=list(rows);fault_rows=[];seen=set()
  for original in(next(c for c in rows if c['name']==name)for name in ('id_7_3_12345678_0','id_4_-1_12345678_1')):
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     address=event['address'];width=event.get('bytes',len(event.get('written',''))//2);pair=(address,width);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen or not 0x325f20<=event['instruction']<=0x325f74:continue
     seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','header','arguments','count','capacity','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=address,bytes=width,occurrence=occurrences[pair],write=kind=='writes'));fault_rows.append(row)
  fault_root=scratch/'faults';fault_root.mkdir();shutil.copyfile(scratch/'observer.cpp',fault_root/'observer.cpp');fault_fixture,_=ns['original'](image,banks,fault_rows,fault_root)
  for row in fault_rows:
   for bank in row['banks']:
    for key in ('before_file','after_file'):bank[key]=str(fault_fixture/bank[key])
  rows+=fault_rows;(fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_point_ids.hpp"\n'+driver
  driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;','      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;')
  driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15ContactPointIdStats activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      try{result=native_tu15_allocate_contact_point_id(memory,{c.at("receiver")},c.at("arguments")[0],fp,growth,observe,&activity);
'''+driver[end:]
  needle='      check(calls==c.at("allocator_calls"),';driver=driver.replace(needle,'      if(!failed&&!required)check(result==c.at("return_word"),name+" contact ID result differs");\n'+needle)
  (project/'driver.cpp').write_text(driver);source=(project/UNITS[0]).read_text();mutants={}
  for name,before,after in (('wrong_search','m.read(data+candidate,1)==0xffu','m.read(data+candidate,1)!=0xffu'),('wrong_byte','m.write(0x325f59u,data+count,index,1);','m.write(0x325f59u,data+count,index^1u,1);'),('stale_count','const auto next_count=m.read(header.identity+4u)+1u;','const auto next_count=count+1u;')):
   assert source.count(before)==1;mutants[name]=source.replace(before,after);(project/(name+'.cpp')).write_text(mutants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(contact_ids LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp '+' '.join(UNITS[1:])+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for mode in ('native',*mutants):
   unit=UNITS[0]if mode=='native'else mode+'.cpp';target='ids_'+mode;wrapper=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_executable({target} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_ids_{mode} COMMAND {wrapper}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','ids_'+mode]for mode in ('native',*mutants)),[]),'--filter','^native_ids_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native contact point ID differs'
  sites={pc for c in rows for pc in c['executed_sites']if 0x325f20<=pc<=0x325f74};report.update(passed=True,profiles=len(rows),full_returns=len(normal_rows),operand_fault_prefixes=len(fault_rows),negative_controls=list(mutants),parent_sites_covered=len(sites),source_authority=authority,input_provenance=provenance,results=results,original_sha256=NEW_PIN,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),source_hashes={n:sha(ROOT/n)for n in ('include/halo2/native_tu15_contact_point_ids.hpp',*[f'src/physics/{u}'for u in UNITS])},scope='Declared coherent current byte-index array DATA; genuine original/native router149DA0/E00 and pool233250/233420. All retained banks, ordered reads/stores, return DWORD, RET4 and FP compared; no allocation outcome/GP/private control frame supplied. No World/live claim.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
