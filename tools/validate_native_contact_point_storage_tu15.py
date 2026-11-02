#!/usr/bin/env python3
"""Physical TU1.5 point/property storage, metadata and genuine native array growth."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
UNITS=('native_tu15_contact_point_storage.cpp','native_tu15_contact_point_ids.cpp','native_tu15_array_growth.cpp','native_tu15_array_reserve.cpp','native_tu15_array_allocator.cpp','native_tu15_island_partition_metadata.cpp')
COLLECTOR,ISLAND,POINTS,PROPS,IDS,POINT_CELL,PROP_CELL=(base.UI+p for p in (0x4000,0x5000,0x1000,0x2000,0x3000,0x6000,0x6004))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def cases():
 rows=[]
 for points in (0,1,2,4):
  for extra in (0,1,3):
   for growth in (False,True):
    for previous in (0,2):
     properties=points+extra
     data={COLLECTOR+0x14:ISLAND,COLLECTOR+0x20:IDS,COLLECTOR+0x24:points,COLLECTOR+0x28:0x80000000|(points if growth else points+2),COLLECTOR+0x34:POINTS,COLLECTOR+0x38:points,COLLECTOR+0x3c:0x80000000|(points if growth else points+2),COLLECTOR+0x50:0x20,COLLECTOR+0x90:PROPS,COLLECTOR+0x94:properties,COLLECTOR+0x98:0x80000000|(properties if growth else properties+2),ISLAND:0x41f080,ISLAND+8:3,ISLAND+12:5,ISLAND+16:7,ISLAND+24:11}
     for n in range(4):data[IDS+n*4]=0x04030201
     if points:data[PROPS+points*28-4]=previous
     rows.append(dict(name=f'storage_{points}_{extra}_{int(growth)}_{previous}',entry=0x325f80,receiver=COLLECTOR,arguments=[POINT_CELL,PROP_CELL],user_words=[(p-base.UI,v)for p,v in data.items()]))
 for source_name in ('storage_2_3_1_0','storage_4_0_1_2'):
  row=copy.deepcopy(next(r for r in rows if r['name']==source_name));row['name']='owned_'+source_name
  words=dict(row['user_words']);points=words[COLLECTOR+0x38-base.UI];properties=words[COLLECTOR+0x94-base.UI]
  for offset in (0x28,0x3c,0x98):words[COLLECTOR+offset-base.UI]&=0x7fffffff
  row['user_words']=list(words.items());row['owned_arrays']=[(COLLECTOR+0x20,IDS,points,1),(COLLECTOR+0x34,POINTS,points,32),(COLLECTOR+0x90,PROPS,properties,28)];rows.append(row)
 for name,source_name,instruction,address,value in (
  ('property_pointer_reread','storage_2_3_0_0',0x32609a,COLLECTOR+0x90,PROPS+0x800),
  ('point_count_after_metadata','storage_0_3_0_0',0x3200c0,COLLECTOR+0x38,1)):
  row=copy.deepcopy(next(r for r in rows if r['name']==source_name));row['name']=name
  row['mutations']=[dict(instruction=instruction,address=address,written=struct.pack('<I',value).hex())];rows.append(row)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],allocation_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image);base.OWNERS=(0x325f80,0x325f20,0x3200b0,0x2e31e0,*base.OWNERS);banks[0x41f080]=image.read(0x41f080,20)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace('case.get("count",0)*case["arguments"][1]','case.get("count",0)*case.get("prepare_stride",1)')
  anchor="        assembly.append(f'''.code32";assert source.count(anchor)==1
  prepare='''        if case.get('owned_arrays'):
            prepare=f'mov ${SP-0x100},%esp\\n'
            for header,data,count,stride in case['owned_arrays']:
                prepare+=f'push $0x12\\n push ${count*stride}\\n mov $0x486080,%ecx\\n call 0x149da0\\n mov %eax,{header}\\n mov %eax,%edi\\n mov ${data},%esi\\n mov ${count*stride},%ecx\\n rep movsb\\n'
'''
  source=source.replace(anchor,prepare+anchor);ns=dict(base.__dict__);exec(source,ns)
  rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+12 for c in rows)
  normal_rows=list(rows);fault_rows=[];seen=set()
  for original in(next(c for c in rows if c['name']==name)for name in ('storage_2_3_0_0','storage_0_1_1_2')):
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     address=event['address'];width=event.get('bytes',len(event.get('written',''))//2);pair=(address,width);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen or not 0x325f80<=event['instruction']<=0x3260f6 or (kind=='writes' and event['instruction'] in (0x325fd7,0x32609a)):continue
     seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=address,bytes=width,occurrence=occurrences[pair],write=kind=='writes'));fault_rows.append(row)
  fault_root=scratch/'faults';fault_root.mkdir();shutil.copyfile(scratch/'observer.cpp',fault_root/'observer.cpp');fault_fixture,_=ns['original'](image,banks,fault_rows,fault_root)
  for row in fault_rows:
   for bank in row['banks']:
    for key in ('before_file','after_file'):bank[key]=str(fault_fixture/bank[key])
  rows+=fault_rows;(fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_point_storage.hpp"\n#include "halo2/native_tu15_island_partition_metadata.hpp"\n'+driver
  driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;','      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;')
  driver=driver.replace('struct Fault{};struct Required{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,read_pass?std::byte(0x5a):std::byte(0xa5)));J actual_stores=J::array(),calls=J::array();').replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15ContactPointStorageStats activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      NativeTu15ContactPointStorageServices services;services.allocation=growth;services.add_info=[&](auto island,auto owner,auto,auto info,auto&){if(owner.identity!=0x3200b0u)throw Required{};native_tu15_add_island_constraint_info(memory,{island},{info},observe);};\n      try{result=native_tu15_allocate_contact_point_storage(memory,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],{PRIVATE},fp,services,observe,&activity);
'''+driver[end:]
  needle='      check(calls==c.at("allocator_calls"),';driver=driver.replace(needle,'      if(!failed&&!required)check(result==c.at("return_word"),name+" contact storage ID result differs");\n'+needle)
  (project/'driver.cpp').write_text(driver);source=(project/UNITS[0]).read_text();mutants={}
  for name,before,after in (('wrong_metadata','special?80u:48u','special?80u:49u'),('wrong_properties','m.word(0x3260c1u,property+4u,0u);','m.word(0x3260c1u,property+4u,1u);'),('wrong_shift','const auto source=destination-28u;','const auto source=destination-24u;')):
   assert source.count(before)==1;mutants[name]=source.replace(before,after);(project/(name+'.cpp')).write_text(mutants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(contact_ids LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp '+' '.join(UNITS[1:])+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for mode in ('native',*mutants):
   unit=UNITS[0]if mode=='native'else mode+'.cpp';target='ids_'+mode;wrapper=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_executable({target} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_ids_{mode} COMMAND {wrapper}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','ids_'+mode]for mode in ('native',*mutants)),[]),'--filter','^native_ids_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native contact point storage differs'
  sites={pc for c in rows for pc in c['executed_sites']if 0x325f80<=pc<=0x3260f6};report.update(passed=True,profiles=len(rows),full_returns=len(normal_rows),operand_fault_prefixes=len(fault_rows),negative_controls=list(mutants),parent_sites_covered=len(sites),source_authority=authority,input_provenance=provenance,results=results,original_sha256=NEW_PIN,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),source_hashes={n:sha(ROOT/n)for n in ('include/halo2/native_tu15_contact_point_storage.hpp',*[f'src/physics/{u}'for u in UNITS])},unqualified_fault_boundaries=[dict(instruction=0x325fd7,reason='RMW write-after-read phase is not modeled by before-instruction oracle'),dict(instruction=0x32609a,reason='REP MOVSD write-after-source-read phase is not modeled by before-instruction oracle; first report retains strict extra-read RED')],scope='Declared coherent point/property/byte-index arrays with genuine native3200B0 metadata, reserve2E31E0, IDs325F20 and router/pool allocation. All retained banks/order/FP/ID DWORD/RET8 compared; fully Source-initialized info16 has independent native-owned DATA. No original private-before-byte equivalence/GP image/World/live claim.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
