#!/usr/bin/env python3
"""Physical TU1.5 island/world contact-constraint registration and native children."""
import argparse,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
UNITS=('native_tu15_contact_constraint_registration.cpp','native_tu15_contact_creation_listeners.cpp','native_tu15_contact_collector_info.cpp','native_tu15_pair_creation_queues.cpp','native_tu15_array_growth.cpp','native_tu15_array_reserve.cpp','native_tu15_array_allocator.cpp','native_tu15_island_partition_metadata.cpp','native_tu15_island_activation.cpp')
WORLD,CONSTRAINT,FIRST,SECOND,I1,I2,DEFAULT,QUEUE=(base.UI+p for p in (0x4000,0x5000,0x6000,0x6100,0x3000,0x3100,0x3200,0x9000))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def cases():
 rows=[]
 for count in (0,1,4):
  for growth in (False,True):
   for props in (0,1,4):
    for refs in (1,0xffff):
     data={CONSTRAINT:0x41f478,CONSTRAINT+4:(refs<<16)|0x1234,CONSTRAINT+12:FIRST,CONSTRAINT+16:SECOND,CONSTRAINT+0x94:props,WORLD+0x20:QUEUE,WORLD+0x24:0,WORLD+0x28:0x80000000|(0 if growth else 4),WORLD+0x2c:DEFAULT,WORLD+0x60:0,WORLD+0x64:0}
     for n,island in enumerate((I1,I2,DEFAULT)):
      data.update({island:0x41f080,island+8:3,island+12:5,island+16:7,island+24:11,island+0x28:0x100,island+0x58:base.UI+0x8000+n*0x200,island+0x5c:count,island+0x60:0x80000000|(count if growth else count+2)})
     rows.append(dict(name=f'direct_{count}_{int(growth)}_{props}_{refs}',entry=0x320e40,receiver=I1,arguments=[CONSTRAINT],user_words=[(p-base.UI,v)for p,v in data.items()]))
     for name,first_island,second_island in (('firstdefault',DEFAULT,I2),('seconddefault',I1,DEFAULT),('same',I1,I1),('different',I1,I2)):
      current=dict(data);current[FIRST+0x44]=first_island;current[SECOND+0x44]=second_island
      rows.append(dict(name=f'world_{name}_{count}_{int(growth)}_{props}_{refs}',entry=0x317350,receiver=WORLD,arguments=[CONSTRAINT],user_words=[(p-base.UI,v)for p,v in current.items()]))
 # Actual Source activation is reached, never supplied a return.
 for source_name in ('world_firstdefault_0_0_1_1','world_different_1_1_4_1'):
  row=copy.deepcopy(next(r for r in rows if r['name']==source_name));row['name']='inactive_'+source_name
  words=dict(row['user_words']);selected=I2 if 'firstdefault' in source_name else I1;words[selected+0x28-base.UI]=0
  row['user_words']=list(words.items());row.update(stop_boundary=0x316d40,required_owner=True);rows.append(row)
 for original in [r for r in rows if r.get('required_owner')]:
  row=copy.deepcopy(original);row['name']='native_'+row['name'];row.pop('stop_boundary');row.pop('required_owner');row['native_activation']=True
  words=dict(row['user_words']);selected=I2 if 'firstdefault' in row['name'] else I1;other=I1 if selected==I2 else I2
  words.update({WORLD+8-base.UI:base.UI+0x9600,WORLD+12-base.UI:0,WORLD+16-base.UI:0x80000000,WORLD+0x14-base.UI:base.UI+0x9700,WORLD+0x18-base.UI:2,base.UI+0x9700-base.UI:selected,base.UI+0x9704-base.UI:other,selected+0x24-base.UI:0,other+0x24-base.UI:1,WORLD+0x78-base.UI:0,WORLD+0x7c-base.UI:0,selected+0x40-base.UI:0})
  row['user_words']=list(words.items());rows.append(row)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],allocation_outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image);base.OWNERS=(0x316d40,0x322660,0x320e40,0x317350,0x322360,0x325e90,0x317200,0x3200b0,0x2e31e0,*base.OWNERS);banks[0x41f080]=image.read(0x41f080,20);banks[0x41f478]=image.read(0x41f478,44)
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
  observer=observer.replace('J writes=J::array(),reads=J::array(),','J activation_calls=J::array();J writes=J::array(),reads=J::array(),')
  activation='''if(std::uint32_t(regs.rip)==0x316d40u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t island{};for(unsigned k=0;k<4;++k)island|=std::to_integer<unsigned>(raw[k])<<(8u*k);activation_calls.push_back(J{{"world",std::uint32_t(regs.rcx)},{"island",island}});}'''
  observer=observer.replace('    const bool repeated_empty=',activation+'\n    const bool repeated_empty=')
  observer=observer.replace('  std::ofstream output(argv[3]);',activation+'\n  std::ofstream output(argv[3]);').replace('{"initial_banks",before}','{"activation_calls",activation_calls},{"initial_banks",before}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')").replace('case.get("count",0)*case["arguments"][1]','case.get("count",0)*case.get("prepare_stride",1)')
  anchor="        assembly.append(f'''.code32";assert source.count(anchor)==1
  prepare='''        if case.get('owned_arrays'):
            prepare=f'mov ${SP-0x100},%esp\\n'
            for header,data,count,stride in case['owned_arrays']:
                prepare+=f'push $0x12\\n push ${count*stride}\\n mov $0x486080,%ecx\\n call 0x149da0\\n mov %eax,{header}\\n mov %eax,%edi\\n mov ${data},%esi\\n mov ${count*stride},%ecx\\n rep movsb\\n'
'''
  source=source.replace(anchor,prepare+anchor).replace('hex(STOP)]','hex(case.get(\"stop_boundary\",STOP))]');ns=dict(base.__dict__);exec(source,ns)
  rows=cases();fixture,authority=ns['original'](image,banks,rows,scratch);assert all(c['after_esp']==base.SP+8 for c in rows if not c.get('required_owner'))
  normal_rows=list(rows);fault_rows=[];seen=set()
  for original in(next(c for c in rows if c['name']==name)for name in ('direct_1_1_4_1','world_different_1_1_4_1')):
   for kind in ('reads','writes'):
    occurrences={}
    for event in original[kind]:
     address=event['address'];width=event.get('bytes',len(event.get('written',''))//2);pair=(address,width);occurrences[pair]=occurrences.get(pair,0)+1;signature=(event['instruction'],kind)
     if signature in seen or not (0x320e40<=event['instruction']<=0x320e9e or 0x317350<=event['instruction']<=0x3173b7) or (kind=='writes' and event['instruction']==0x320e91):continue
     seen.add(signature);row={k:copy.deepcopy(original[k])for k in ('entry','receiver','arguments','user_words')};row.update(name=f'fault_{kind}_{event["instruction"]:x}',fault=dict(address=address,bytes=width,occurrence=occurrences[pair],write=kind=='writes'));fault_rows.append(row)
  fault_root=scratch/'faults';fault_root.mkdir();shutil.copyfile(scratch/'observer.cpp',fault_root/'observer.cpp');fault_fixture,_=ns['original'](image,banks,fault_rows,fault_root)
  for row in fault_rows:
   for bank in row['banks']:
    for key in ('before_file','after_file'):bank[key]=str(fault_fixture/bank[key])
  rows+=fault_rows;(fixture/'original.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows)))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_constraint_registration.hpp"\n#include "halo2/native_tu15_contact_collector_info.hpp"\n#include "halo2/native_tu15_island_activation.hpp"\n#include "halo2/native_tu15_island_partition_metadata.hpp"\n'+driver
  driver=driver.replace('      if(read_pass&&!c.value("mutations",J::array()).empty())continue;','      if(read_pass&&!c.value("mutations",J::array()).empty())continue;\n      if(!read_pass&&c.contains("fault")&&!c.at("fault").value("write",false))continue;')
  driver=driver.replace('struct Fault{};struct Required{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,read_pass?std::byte(0x5a):std::byte(0xa5)));J actual_stores=J::array(),calls=J::array();').replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15ContactConstraintRegistrationStats activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      NativeTu15ContactConstraintRegistrationServices services;services.allocation=growth;
      services.get_info=[&](auto constraint,auto owner,auto info,auto&){if(owner.identity!=0x325e90u)throw Required{};native_tu15_get_contact_collector_info(memory,constraint,{info},observe);};
      services.add_info=[&](auto island,auto owner,auto,auto info,auto&){if(owner.identity!=0x3200b0u)throw Required{};native_tu15_add_island_constraint_info(memory,{island},{info},observe);};
      J activation_calls=J::array();services.activate_island=[&](auto world,auto island,auto&f){activation_calls.push_back(J{{"world",world},{"island",island}});if(!c.value("native_activation",false))throw Required{};NativeTu15IslandActivationServices activation;activation.grow_array=[&](auto header,auto stride,auto&current){NativeTu15ArrayGrowthMemory array(memory,{header.identity},observe);native_tu15_grow_array(array,stride,current,growth);};native_tu15_activate_physics_island(memory,{world},{island},f,activation,observe);};
      try{if(c.at("entry")==0x320e40u)native_tu15_register_island_contact_constraint(memory,c.at("receiver"),c.at("arguments")[0],PRIVATE,fp,services,observe,&activity);else result=native_tu15_register_world_contact_constraint(memory,c.at("receiver"),c.at("arguments")[0],PRIVATE,fp,services,observe,&activity);
'''+driver[end:]
  needle='      check(activation_calls==c.at("activation_calls"),name+" activation request differs");\n      check(calls==c.at("allocator_calls"),';driver=driver.replace(needle,'      if(!failed&&!required&&c.at("entry")==0x317350u)check(result==c.at("return_word"),name+" world contact registration result differs");\n'+needle)
  (project/'driver.cpp').write_text(driver);source=(project/UNITS[0]).read_text();mutants={}
  for name,before,after in (('wrong_reference','m.word(constraint+6u,2)+1u','m.word(constraint+6u,2)+2u'),('wrong_island','m.word(0x320e96u,constraint+0x14u,island);','m.word(0x320e96u,constraint+0x14u,island+4u);'),('wrong_pair','memory,world,first_body,second_body,fp','memory,world,second_body,first_body,fp')):
   assert source.count(before)==1;mutants[name]=source.replace(before,after);(project/(name+'.cpp')).write_text(mutants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(contact_ids LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(common OBJECT driver.cpp '+' '.join(UNITS[1:])+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
  for mode in ('native',*mutants):
   unit=UNITS[0]if mode=='native'else mode+'.cpp';target='ids_'+mode;wrapper=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" '
   cmake+=f'add_executable({target} $<TARGET_OBJECTS:common> {unit})\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_ids_{mode} COMMAND {wrapper}$<TARGET_FILE:{target}> "{fixture}/original.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','ids_'+mode]for mode in ('native',*mutants)),[]),'--filter','^native_ids_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native contact constraint registration differs'
  sites={pc for c in rows for pc in c['executed_sites']if (0x320e40<=pc<=0x320e9e or 0x317350<=pc<=0x3173b7)};report.update(passed=True,profiles=len(rows),full_returns=sum(not c.get("required_owner",False)for c in normal_rows),genuine_activation_prefixes=sum(c.get("required_owner",False)for c in normal_rows),native_activation_full_returns=sum(c.get("native_activation",False)for c in normal_rows),operand_fault_prefixes=len(fault_rows),negative_controls=list(mutants),parent_sites_covered=len(sites),source_authority=authority,input_provenance=provenance,results=results,original_sha256=NEW_PIN,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),source_hashes={n:sha(ROOT/n)for n in ('include/halo2/native_tu15_contact_constraint_registration.hpp',*[f'src/physics/{u}'for u in UNITS])},unqualified_fault_boundaries=[dict(instruction=0x320e91,reason='RMW write-after-read phase is not modeled by before-instruction oracle')],scope='Declared coherent island/world/constraint DATA with sole native325E90 info,3200B0 metadata,317200 queues,322360 notifications, growth/reserve and real router/pool. All retained bytes/order/FP/RET4 and typed world return identity compared. Actual inactive-island316D40 stays a genuine entry prefix, no return supplied. Native info16 initialized before use; no Source-private-before/GP image/World-live claim.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
