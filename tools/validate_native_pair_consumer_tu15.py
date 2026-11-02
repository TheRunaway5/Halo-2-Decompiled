#!/usr/bin/env python3
"""Native321510 positive pair consumption at genuine simplex helper entries."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata():
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows={};listings={};counts=[]
 for owner in(0x321510,0x317a40,0x317ad0,0x37cbf0,0x37cb20,0x37c960):
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    if op.mem.segment:
     assert i.reg_name(op.mem.segment)=='fs';continue
    access=CS_AC_READ if i.mnemonic in('test','cmp')else CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   rows[str(pc)]=dict(memory=ops)
   callbacks={0x317a8f:1,0x317b1c:2,0x317b56:3,0x32167e:3,0x321693:2,0x37c9f8:1,0x37ca27:3,0x37ca42:1,0x37ca7f:3,0x37ca9a:1,0x37cac4:1}
   if pc in callbacks:rows[str(pc)].update(callback_arguments=callbacks[pc],next=pc+i.size)
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[150,51,53,35,84,148]
 return rows,dict(body_sites=counts,listing_sha256=listings)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--producer',type=int);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--variants',nargs='+',type=int,default=[0,1,2,3,4,5,6,7,8,10,11]);p.add_argument('--native-filter',action='store_true',help='Compose qualified native2E9A00 instead of executing the original filter service');p.add_argument('--native-allocator',action='store_true',help='Compose qualified149DA0/149E00 routing and233250/233420 pool kernels; refuse all other allocator owners');p.add_argument('--native-arena',action='store_true',help='Compose qualified2E10A0/2E1110 arena kernels over native backing allocator services');a=p.parse_args();a.native_allocator=a.native_allocator or a.native_arena;a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_pair_consumer.cpp';header=ROOT/'include/halo2/native_tu15_pair_consumer.hpp';fragment=ROOT/'tools/observe_native_pair_consumer_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,fragment,bridge,Path(__file__)];files+=([ROOT/'src/physics/native_tu15_activation_services.cpp',ROOT/'include/halo2/native_tu15_activation_services.hpp']if a.native_filter else[]);files+=([ROOT/'src/physics/native_tu15_array_allocator.cpp',ROOT/'include/halo2/native_tu15_array_allocator.hpp']if a.native_allocator else[]);files+=([ROOT/'src/physics/native_tu15_stack_arena.cpp',ROOT/'include/halo2/native_tu15_stack_arena.hpp']if a.native_arena else[]);pins={str(p.relative_to(ROOT)):sha(p)for p in files};meta,authority=metadata();instructions=scratch/'instructions.json';save(instructions,meta);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'native.cpp');(project/'observer.cpp').write_text('#include "halo2/native_tu15_pair_consumer.hpp"\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text());contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  if a.native_filter:
   shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'filter.cpp')
   driver=project/'observer.cpp';native_driver=driver.read_text();needle=' services.filter_pair=[&](auto filter,auto owner,auto decision,auto a,auto b,auto&fp){return call(owner.identity,filter,{decision,a,b},p-0x2cu,decision==scratch.merge.interior_decision?0x37ca29u:0x37ca81u,fp);};'
   replacement=' services.filter_pair=[&](auto filter,auto owner,auto decision,auto a,auto b,auto&){check(owner.identity==0x2e9a00u,"Unqualified native filter owner");callbacks.push_back(J{{"entry",owner.identity},{"this",filter},{"arguments",J::array({decision,a,b})}});Memory child_memory(native);return native_tu15_activation_group_filter(child_memory,filter,decision,{a},{b});};'
   assert native_driver.count(needle)==1
   native_driver='#include "halo2/native_tu15_activation_services.hpp"\n'+native_driver.replace(needle,replacement)
   driver.write_text(native_driver)
  if a.native_allocator:
   shutil.copyfile(ROOT/'src/physics/native_tu15_array_allocator.cpp',project/'allocator.cpp')
   driver=project/'observer.cpp';native_driver=driver.read_text();begin=native_driver.index(' services.arena.release_owned_pairs=');end=native_driver.index(' const NativeTu15PairConsumerScratch scratch',begin)
   replacement=' Memory allocator_memory(native);NativeTu15ArrayAllocatorMemory allocator_routing(allocator_memory);NativeTu15ArrayAllocatorServices allocator_services;\n allocator_services.allocate_owner=[&](const auto&request,auto&){check(request.owner==0x233250u,"Unqualified native pair pool allocation owner");NativeTu15PoolMemory pool(allocator_memory,request.receiver);return native_tu15_pool_allocate(pool,request.bytes);};\n allocator_services.release_owner=[&](const auto&request,auto&){check(request.owner==0x233420u,"Unqualified native pair pool release owner");NativeTu15PoolMemory pool(allocator_memory,request.receiver);check(native_tu15_pool_release(pool,request.data,request.bytes),"Unqualified native pair large pool release");};\n allocator_services.default_heap=[&](auto&)->std::uint32_t{throw std::runtime_error("Unqualified native pair default heap");};\n services.arena.release_owned_pairs=[&](auto a,auto owner,auto block,auto bytes,auto tag,auto&fp){check(owner.identity==0x149e00u,"Unqualified native pair release router");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({block,bytes,tag})}});native_tu15_route_array_release(allocator_routing,block,bytes,tag,fp,allocator_services);};\n services.allocate_owned_records=[&](auto a,auto owner,auto bytes,auto tag,auto&fp){check(owner.identity==0x149da0u,"Unqualified native pair allocation router");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({bytes,tag})}});return native_tu15_route_array_allocation(allocator_routing,bytes,tag,fp,allocator_services);};\n'
   driver.write_text('#include "halo2/native_tu15_array_allocator.hpp"\n'+native_driver[:begin]+replacement+native_driver[end:])
  if a.native_arena:
   shutil.copyfile(ROOT/'src/physics/native_tu15_stack_arena.cpp',project/'arena.cpp')
   driver=project/'observer.cpp';native_driver=driver.read_text();needle=' const NativeTu15PairConsumerScratch scratch';assert native_driver.count(needle)==1
   replacement=' NativeTu15ArrayGrowthServices arena_backing;\n arena_backing.allocate=[&](const auto&r,auto&fp){check(r.owner==0x149da0u,"Unqualified arena backing allocation owner");return native_tu15_route_array_allocation(allocator_routing,r.bytes,r.memory_class,fp,allocator_services);};\n arena_backing.release=[&](const auto&r,auto&fp){check(r.owner==0x149e00u,"Unqualified arena backing release owner");native_tu15_route_array_release(allocator_routing,r.data,r.bytes,r.memory_class,fp,allocator_services);};\n services.arena.allocate_block=[&](auto a,auto owner,auto bytes,auto&fp){check(owner.identity==0x2e10a0u,"Unqualified slow arena allocation owner");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({bytes})}});return native_tu15_allocate_stack_arena(allocator_memory,a.identity,bytes,fp,arena_backing);};\n services.arena.release_block=[&](auto a,auto owner,auto block,auto bytes,auto&fp){check(owner.identity==0x2e1110u,"Unqualified slow arena release owner");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({block,bytes})}});native_tu15_release_stack_arena(allocator_memory,a.identity,block,bytes,fp,arena_backing);};\n'
   driver.write_text('#include "halo2/native_tu15_stack_arena.hpp"\n'+native_driver.replace(needle,replacement+needle))
  for name in(a.context or('gemini-column0','gemini-column1','elongation-crates')):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:]
   for producer in([a.producer]if a.producer is not None else [0]):
    cases.extend(dict(name=f'{name}_{producer}_{variant}',producer=producer,variant=variant,elf=original/'original-hardware',capture=capture/'capture.json')for variant in a.variants if not((variant==3 or 20<=variant<=24) and name=='elongation-crates'))
   contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_raw_exit=r.returncode));print(name,'segments source inputs ready',flush=True)
  text=source.read_text();variants={}
  assert cases,'No genuine segments-helper entries in requested source cohort'
  for name,needle,replacement in(('wrong_stride','reserve_block(m,align16(reserve*16u)','reserve_block(m,align16(reserve*8u)'),('wrong_final_record','m.word(0x3216ccu,destination-4u,d);','m.word(0x3216ccu,destination-4u,d^1u);'),('skip_create','if(!m.word(result,1))return;','if(m.word(result,1))return;')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"],(r.returncode,j)\nprint("Unchanged original segments rejects native mutation")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(native_simplex_segments LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT observer.cpp)
target_include_directories(driver PRIVATE include)
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  extra_source=(' filter.cpp'if a.native_filter else'')+(' allocator.cpp'if a.native_allocator else'')+(' arena.cpp'if a.native_arena else'')
  for mode in('native',*variants):
   cmake+=f'add_executable(segments_{mode} $<TARGET_OBJECTS:driver> {mode}.cpp{extra_source})\ntarget_include_directories(segments_{mode} PRIVATE include)\ntarget_compile_features(segments_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(segments_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   selected=next((c for c in cases if c['variant']==0),cases[0])
   for c in(cases if mode=='native'else[selected]):
    relative='evidence/'+mode+'_'+c['name']+'.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME segments_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:segments_{mode}> "{c["elf"]}" "{c["capture"]}" {c["producer"]} {c["variant"]} "{instructions}" "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','segments_native',*sum((['--target','segments_'+name]for name in variants),[]),'--filter','^segments_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,authority=authority,original_cases_per_compiler=len(cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,production_native_only_closure=False,native_filter_composition=a.native_filter,native_allocator_composition=a.native_allocator,native_arena_composition=a.native_arena,remaining=['source aliases and fault/observer prefixes','actual callback native replacements','live World activation and visible multiplayer'])
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
