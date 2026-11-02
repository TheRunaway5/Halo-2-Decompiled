#!/usr/bin/env python3
"""Physical355D0/184C50 closed no-function returns and genuine13DE70 prefixes.

Positive function evaluation/math is unclaimed until the native13DE70 child
composition is qualified. Native material scratch is independent ordinary DATA.
"""
import argparse,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases():
 u=base.UI;rows=[]
 def add(name,entry=0x355d0,map_index=0,count=3,kinds=(0,3,6),mapping=0,material=0,conveyor=0,conveyor_count=1,**extra):
  words=[(0x518,u+0x700),(0x724,u+0x800),(0x860,u+0xa00),(0x850,u+0xb00),(0x848,u+0xc00),(0x840,u+0xd00),
   (0xa00,(mapping&65535)<<16),(0xa04,(mapping&65535)<<16),(0xb00,count<<9),(0xb04,0),
   (0x1510,u+0x1600),(0x15b4,conveyor_count),(0x15b8,u+0x1800),(0x1608,(conveyor&65535)<<16),(0x1610,0xabcd0001),
   (0x161c,(conveyor&65535)<<16),(0x1624,0xabcd0001)]
  for n in range(12):words.extend([(0xc00+n*4,((kinds[n%len(kinds)]&65535)<<16)),(0xd00+n*20+8,0x3f800000)])
  args=[map_index,u+0x2000,u+0x2004]if entry==0x355d0 else[]
  row=dict(name=name,entry=entry,receiver=0,arguments=args,incoming_eax=0xabcd0001 if entry==0x355d0 else material,
   incoming_ebx=u+0x2000,user_words=words,**extra);rows.append(row);return row
 for mapping in(-1,0):
  for count in(0,1,4,8):
   for map_index in(0,1):add(f'shader_no_function_{mapping}_{count}_{map_index}',map_index=map_index,count=count,mapping=mapping)
 for kind in(0,1,2,3,6,0xffff):add(f'unsupported_binding_{kind}',count=4,kinds=(kind,))
 for conveyor,count in((-1,1),(0,0),(1,1),(2,1),(0,0xffffffff)):
  add(f'material_no_conveyor_{conveyor}_{count}',entry=0x184c50,conveyor=conveyor,conveyor_count=count)
 for count in(0,1,4,8):add(f'material_two_false_maps_{count}',entry=0x184c50,count=count)
 for bits in(0,0x80000000,0x3f800000,0xbf800000,0x7fc12345,0x7f812345):
  row=add(f'function_entry_prefix_{bits:x}',count=1,kinds=(4,),required_owner=True,stop_boundary=0x13de70)
  row['user_words']=[(p,bits if p==0xd08 else v)for p,v in row['user_words']]
 for label,entry,address,width,nth,write in(
  ('global',0x355d0,0x4f0414,4,1,False),('tag',0x355d0,u+0x518,4,1,False),
  ('template',0x355d0,u+0x724,4,1,False),('first_output',0x355d0,u+0x2000,4,1,True),
  ('second_output',0x355d0,u+0x2004,4,1,True),('map',0x355d0,u+0xa02,2,1,False),
  ('packed_reread',0x355d0,u+0xb00,2,2,False),('binding_type',0x355d0,u+0xc02,2,1,False),
  ('material_word',0x184c50,u+0x160a,2,1,False),('material_count',0x184c50,u+0x15b4,4,1,False)):
  add('fault_'+label,entry=entry,fault=dict(address=address,bytes=width,occurrence=nth,write=write))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=('include/halo2/native_tu15_surface_conveyor.hpp','src/physics/native_tu15_surface_conveyor.cpp','tools/validate_native_surface_conveyor_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp');pins={n:sha(ROOT/n)for n in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],positive_function_math_qualified=False,supplied_function_results=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x355d0,0x184c50)
  banks={0x4f0414:struct.pack('<I',base.UI+0x500),0x4ecc18:struct.pack('<I',base.UI+0x1500),0x46a0d0:image.read(0x46a0d0,4)}
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();anchor='if(pc==stop)break;';assert observer.count(anchor)==1
  observer=observer.replace(anchor,'''if(pc==0x13de70u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,8);std::uint32_t first{},second{};for(unsigned n=0;n<4;++n){first|=std::to_integer<unsigned>(raw[n])<<(8u*n);second|=std::to_integer<unsigned>(raw[n+4])<<(8u*n);}allocator_calls.push_back(J{{"owner",pc},{"function",std::uint32_t(regs.rdi)},{"first",first},{"second",second}});}'''+anchor)
  observer_path=scratch/'observer.cpp';observer_path.write_text(observer)
  original=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)').replace('hex(STOP)]','hex(case.get("stop_boundary",STOP))]')
  original=original.replace(" mov ${case['receiver']},%ecx", " mov ${case['incoming_eax']},%eax\n mov ${case['incoming_ebx']},%ebx\n mov ${case['receiver']},%ecx")
  environment=dict(base.__dict__,observer_path=observer_path);exec(original,environment);rows=cases();fixture,authority=environment['original'](image,banks,rows,scratch);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_surface_conveyor.cpp',project/'kernel.cpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_surface_conveyor.hpp"\n'+driver
  driver=driver.replace('struct Fault{};struct Required{};', 'constexpr std::uint32_t PRIVATE=0x6000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,std::byte{0xa5}));J actual_stores=J::array(),calls=J::array();')
  driver=driver.replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)
  setup='''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15SurfaceConveyorStats activity;NativeTu15ShaderFunctionEvaluationServices services;
      services.evaluate=[&](auto function,auto first,auto second,auto&,auto)->std::uint32_t{calls.push_back(J{{"owner",0x13de70u},{"function",function},{"first",first},{"second",second}});throw Required{};};
''';driver=driver[:begin]+setup+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(entry==0x355d0u)result=native_tu15_shader_conveyor_derivative(memory,c.at("incoming_eax"),args[0],args[1],args[2],fp,0,services,observe,&activity);
        else result=native_tu15_material_surface_velocity(memory,c.at("incoming_eax"),c.at("incoming_ebx"),{PRIVATE,PRIVATE+4u},fp,0,services,observe,&activity);
'''+driver[end:]
  anchor='      check(calls==';driver=driver.replace(anchor,'      if(!failed&&!required)check(result==(c.at("return_word").get<std::uint32_t>()&255u),name+" source bool result differs");\n'+anchor)
  driver=driver.replace('name+" actual allocator request order differs"','name+" actual function-entry arguments differ native="+calls.dump()+" original="+c.at("allocator_calls").dump()')
  (project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(surface_conveyor LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(conveyor driver.cpp kernel.cpp)\ntarget_include_directories(conveyor PRIVATE include)\ntarget_compile_features(conveyor PRIVATE cxx_std_20)\ntarget_compile_options(conveyor PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(conveyor PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME surface_conveyor COMMAND conveyor "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','conveyor','--filter','^surface_conveyor$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native surface/conveyor negative/prefix domain differs'
  executed={pc for c in rows for pc in c['executed_sites']};code=base._Code(image,'.text')
  report.update(passed=all(sha(ROOT/n)==v for n,v in pins.items()),original_sha256=NEW_PIN,source_authority=authority,original_cases=len(rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),function_entry_prefixes=sum(c.get('required_owner',False)for c in rows),fault_prefixes=sum(c['declared_fault']for c in rows),body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),executed=len(set(base.complete_body(code,owner).code)&executed))for owner in base.OWNERS},commands=commands,results=results,scope='Closed source no-function returns and actual13DE70 service-entry prefixes only. Complete persistentDATA/orderedoperands/preimages/stores/FP/lowALresult. Independent native material U/V scratch; sourceprivatebeforeledger unclaimed. Positive evaluator/range/derivative/vector output remains unqualified.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
