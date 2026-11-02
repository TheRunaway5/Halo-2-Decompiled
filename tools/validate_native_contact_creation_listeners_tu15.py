#!/usr/bin/env python3
"""Physical TU1.5 contact creation/constraint listeners with genuine RET4 callbacks."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
UNITS=('native_tu15_contact_creation_listeners.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp')
OWNER,LIST,TABLE,EVENT=(base.UI+p for p in (0x400,0x1000,0x2000,0x5000))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def cases():
 rows=[]
 for entry,data_offset,count_offset in ((0x322440,0xb4,0xb8),(0x31fe70,0x60,0x64),(0x322360,0x60,0x64)):
  for count in (0,1,2,4,6):
   for mask in (0,0x3f,0x15,0x3e):
    data={OWNER+data_offset:LIST,OWNER+count_offset:count,TABLE:0x2e4d60,TABLE+8:0x2e4d60}
    for n in range(8):
     listener=base.UI+0x3000+n*16;data[listener]=TABLE;data[LIST+n*4]=listener if mask&(1<<n) else 0
    rows.append(dict(name=f'listeners_{entry:x}_{count}_{mask:x}',entry=entry,receiver=0,arguments=[OWNER,EVENT],user_words=[(p-base.UI,v)for p,v in data.items()]))
  row=copy.deepcopy(next(r for r in rows if r['name']==f'listeners_{entry:x}_4_3e'));row['name']=f'count_reread_{entry:x}'
  move_pc={0x322440:0x3224ae,0x31fe70:0x31feca,0x322360:0x3223ba}[entry]
  row['mutations']=[dict(instruction=move_pc,address=OWNER+count_offset,written=struct.pack('<I',4).hex())];rows.append(row)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],allocation_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance={},{};base.OWNERS=(0x322440,0x31fe70,0x322360,0x2e4d60)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
  observer=observer.replace('J writes=J::array(),reads=J::array(),','J listener_calls=J::array();J writes=J::array(),reads=J::array(),')
  observer=observer.replace('    const bool repeated_empty=','''    if(pc==0x2e4d60u){const auto raw=memory(child.pid,registers[4]+4u,4);std::uint32_t value{};for(unsigned k=0;k<4;++k)value|=std::to_integer<unsigned>(raw[k])<<(8u*k);listener_calls.push_back(J{{"owner",pc},{"receiver",registers[1]},{"argument",value}});}
    const bool repeated_empty=''').replace('{"initial_banks",before}','{"listener_calls",listener_calls},{"initial_banks",before}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace('case.get("count",0)*case["arguments"][1]','case.get("count",0)*case.get("prepare_stride",1)');ns=dict(base.__dict__);exec(source,ns)
  rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+4 for c in rows)
  normal_rows=list(rows);fault_rows=[];seen=set()
  for original in(next(c for c in rows if c['name']==name)for name in ('listeners_322440_4_15','listeners_31fe70_4_15','listeners_322360_4_15')):
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     address=event['address'];width=event.get('bytes',len(event.get('written',''))//2);pair=(address,width);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen or not (0x322440<=event['instruction']<=0x3224c1 or 0x31fe70<=event['instruction']<=0x31feda or 0x322360<=event['instruction']<=0x3223ca):continue
     seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=address,bytes=width,occurrence=occurrences[pair],write=kind=='writes'));fault_rows.append(row)
  fault_root=scratch/'faults';fault_root.mkdir();shutil.copyfile(scratch/'observer.cpp',fault_root/'observer.cpp');fault_fixture,_=ns['original'](image,banks,fault_rows,fault_root)
  for row in fault_rows:
   for bank in row['banks']:
    for key in ('before_file','after_file'):bank[key]=str(fault_fixture/bank[key])
  rows+=fault_rows;(fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_creation_listeners.hpp"\n'+driver
  driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;','      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;')
  driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15ContactCreationListenerStats activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      NativeTu15ContactCreationListenerServices services;J listener_calls=J::array();services.notify=[&](auto listener,auto owner,auto argument,auto&){listener_calls.push_back(J{{"owner",owner.identity},{"receiver",listener},{"argument",argument}});if(owner.identity!=0x2e4d60u)throw Required{};};\n      try{auto function=c.at("entry")==0x322440u?native_tu15_notify_world_contact_creation:c.at("entry")==0x31fe70u?native_tu15_notify_body_contact_creation:native_tu15_notify_world_constraint_addition;function(memory,c.at("arguments")[0],c.at("arguments")[1],fp,services,observe,&activity);
'''+driver[end:]
  needle='      check(calls==c.at("allocator_calls"),';driver=driver.replace(needle,'      check(listener_calls==c.at("listener_calls"),name+" listener callback order differs");\n'+needle)
  (project/'driver.cpp').write_text(driver);source=(project/UNITS[0]).read_text();mutants={}
  for name,before,after in (('wrong_event','word(0x32244bu,event+0x10u,0u)','word(0x32244bu,event+0x10u,1u)'),('wrong_move','current+destination*4u+4u','current+destination*4u'),('stale_count','const auto count=m.word(owner+layout.count);','const auto count=last;')):
   assert source.count(before)==1;mutants[name]=source.replace(before,after);(project/(name+'.cpp')).write_text(mutants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(contact_ids LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp '+' '.join(UNITS[1:])+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for mode in ('native',*mutants):
   unit=UNITS[0]if mode=='native'else mode+'.cpp';target='ids_'+mode;wrapper=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_executable({target} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_ids_{mode} COMMAND {wrapper}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','ids_'+mode]for mode in ('native',*mutants)),[]),'--filter','^native_ids_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native contact creation listeners differ'
  sites={pc for c in rows for pc in c['executed_sites']if (0x322440<=pc<=0x3224c1 or 0x31fe70<=pc<=0x31feda or 0x322360<=pc<=0x3223ca)};report.update(passed=True,profiles=len(rows),full_returns=len(normal_rows),operand_fault_prefixes=len(fault_rows),negative_controls=list(mutants),parent_sites_covered=len(sites),source_authority=authority,input_provenance=provenance,results=results,original_sha256=NEW_PIN,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),source_hashes={n:sha(ROOT/n)for n in ('include/halo2/native_tu15_contact_creation_listeners.hpp',*[f'src/physics/{u}'for u in UNITS])},scope='Declared coherent listener arrays and caller-owned initialized event DATA. Original callbacks execute genuine2E4D60 RET4; native callbacks implement that exact no-op only. All retained bytes/order/FP/RET0 and callback receiver/argument/owner order compared, including count mutation during compaction. No padding/GP outcome supplied; no actual live callback admission.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
