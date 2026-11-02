#!/usr/bin/env python3
"""Physical321510 from actual typed multiplayer BEFORE; no captured CPU ABI.

Only current typed receiver, four formal DATA arguments and FP are marshaled
into an explicitly declared ordinary offline ABI. No GP/SIMD after image,
allocator result, old capacity or expected bank is supplied to the kernel.
"""
import argparse,hashlib,json,lzma,os,shutil,struct,subprocess,sys,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from replay_live_phantom_hardware_tu15 import reference_text_gaps
from validate_native_pair_consumer_tu15 import metadata

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--native-allocator',action='store_true');p.add_argument('--native-filter',action='store_true');p.add_argument('--native-arena',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_after_consumed=False,supplied_original_outcomes=False,captured_guest_ABI=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN and sha(a.capture)==a.archive_sha256;capture=scratch/'before';capture.mkdir();subprocess.run(['tar','--zstd','-xf',str(a.capture.resolve()),'-C',str(capture)],check=True)
  data=json.loads((capture/'capture-native-before.json').read_text());assert data['schema']=='native-tu15-pair-consumer-typed-before-v1'and data['reference']=='English TU1.5 only';typed=data['input'];assert typed['phase']==0 and typed['preserved_x87_depth']==0 and typed['first']['count']>0
  manifest=json.loads(lzma.decompress(a.capture.with_name('all-file-hashes.json.xz').read_bytes()));regions=data['regions']
  # The retention manifest includes every exact before bank. Check independently
  # against the archive member hashes before constructing any original process.
  verified=0
  for r in regions:
   name=r['before_file'];row=manifest.get('./'+name,manifest.get(name));assert row is not None,(name,list(manifest)[:2]);assert (capture/name).stat().st_size==r['size']and sha(capture/name)==row['sha256'];verified+=1
  image=Xbe(NEW_DEFAULT);text,gaps,authentication=reference_text_gaps(image,capture,regions,scratch);sp=typed['temporary_header']+0x20;private_first=sp&~0xffff;stop=0x70001000;fs=0x71000000
  assert any(r['address']<=private_first and sp+0x100<=r['address']+r['size']for r in regions);assert all(not private_first<=v<sp+20 for v in(typed['island'],typed['first']['data'],typed['second']['data']))
  assembly=[];sections=[]
  for n,r in enumerate(sorted([*regions,*gaps],key=lambda r:r['address'])):
   path=r.get('reference_file',capture/r.get('before_file',''));assembly.append(f'.section .bank{n},"awx"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{r["address"]:x}:{{*(.bank{n})}}');assert not r['address']<=0x70000000<r['address']+r['size']and not r['address']<=fs<r['address']+r['size']
  args=[typed['first']['data'],typed['first']['count'],typed['second']['data'],typed['second']['count']];fp=typed['floating_point'];env=bytearray(28);struct.pack_into('<H',env,0,fp['x87_control_word']);struct.pack_into('<H',env,4,fp['x87_status']);struct.pack_into('<H',env,8,0xffff);(scratch/'env.bin').write_bytes(env)
  setup='\n'.join(f'movl ${v},{sp+4*n}'for n,v in enumerate([stop,*args]));assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 mov $123,%eax
 mov $1,%ebx
 mov $descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js failed
 mov $7,%eax
 mov %ax,%fs
 fninit
 fldenv environment
 ldmxcsr mxcsr
 {setup}
 mov ${sp},%esp
 xor %eax,%eax
 xor %edx,%edx
 xor %ebx,%ebx
 xor %ebp,%ebp
 xor %esi,%esi
 xor %edi,%edi
 mov ${typed['island']},%ecx
 cld
 int3
 jmp 0x321510
failed:
 mov $1,%eax
 mov $125,%ebx
 int $0x80
.balign 4
descriptor:.long 0,{fs},0xfffff,0x51
mxcsr:.long {fp['mxcsr']}
environment:.incbin "{scratch/'env.bin'}"
.org 0x1000
stop:int3
.section .fs,"aw"
.zero 4096
''');sections.extend(['.bridge 0x70000000:{*(.bridge)}',f'.fs 0x{fs:x}:{{*(.fs)}}']);(scratch/'reference.s').write_text('\n'.join(assembly));(scratch/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],check=True,capture_output=True);elf=scratch/'reference';subprocess.run(['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(elf)],check=True,capture_output=True)
  normalized=dict(data,typed_entry=typed,private_returned_stack=dict(first=private_first,end=sp+20));save(scratch/'capture.json',normalized);instructions,authority=metadata();save(scratch/'instructions.json',instructions)
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';fragment=ROOT/'tools/observe_native_pair_consumer_tu15.cpp';driver='#include "halo2/native_tu15_pair_consumer.hpp"\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text();driver=driver.replace('input_data=sp-0x2000u','input_data=capture.at("typed_entry").at("first").at("data").get<std::uint32_t>()');begin=driver.index('const auto first_object=');end=driver.index(' struct Bank{',begin);driver=driver[:begin]+'''const auto&typed=capture.at("typed_entry");const std::array<std::uint32_t,4>args{typed.at("first").at("data"),typed.at("first").at("count"),typed.at("second").at("data"),typed.at("second").at("count")};'''+driver[end:];driver=driver.replace('{"declared_pair_inputs_from_actual_existing_records",true}','{"actual_typed_pair_inputs",true},{"offline_ordinary_ABI_declared",true},{"captured_GP_SIMD_supplied",false}')
  units=['native.cpp'];files=['include/halo2/native_tu15_pair_consumer.hpp','src/physics/native_tu15_pair_consumer.cpp','tools/observe_native_contact_producer_tu15.cpp','tools/observe_native_pair_consumer_tu15.cpp','tools/validate_native_pair_consumer_tu15.py','tools/validate_native_pair_consumer_actual_before_tu15.py'];shutil.copyfile(ROOT/'src/physics/native_tu15_pair_consumer.cpp',project/'native.cpp')
  if a.native_filter:
   files+=['include/halo2/native_tu15_activation_services.hpp','src/physics/native_tu15_activation_services.cpp'];units.append('filter.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'filter.cpp');needle=' services.filter_pair=[&](auto filter,auto owner,auto decision,auto a,auto b,auto&fp){return call(owner.identity,filter,{decision,a,b},p-0x2cu,decision==scratch.merge.interior_decision?0x37ca29u:0x37ca81u,fp);};';assert driver.count(needle)==1;driver='#include "halo2/native_tu15_activation_services.hpp"\n'+driver.replace(needle,' services.filter_pair=[&](auto filter,auto owner,auto decision,auto a,auto b,auto&){check(owner.identity==0x2e9a00u,"Unqualified native filter owner");callbacks.push_back(J{{"entry",owner.identity},{"this",filter},{"arguments",J::array({decision,a,b})}});Memory child_memory(native);return native_tu15_activation_group_filter(child_memory,filter,decision,{a},{b});};')
  if a.native_allocator or a.native_arena:
   files+=['include/halo2/native_tu15_array_allocator.hpp','src/physics/native_tu15_array_allocator.cpp'];units.append('allocator.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_array_allocator.cpp',project/'allocator.cpp');begin=driver.index(' services.arena.release_owned_pairs=');end=driver.index(' const NativeTu15PairConsumerScratch scratch',begin)
   replacement=''' Memory allocator_memory(native);NativeTu15ArrayAllocatorMemory router(allocator_memory);NativeTu15ArrayAllocatorServices heap;
 heap.allocate_owner=[&](const auto&r,auto&){check(r.owner==0x233250u,"Unqualified native pool allocation");NativeTu15PoolMemory pool(allocator_memory,r.receiver);return native_tu15_pool_allocate(pool,r.bytes);};
 heap.release_owner=[&](const auto&r,auto&){check(r.owner==0x233420u,"Unqualified native pool release");NativeTu15PoolMemory pool(allocator_memory,r.receiver);check(native_tu15_pool_release(pool,r.data,r.bytes),"Unqualified large pool release");};
 heap.default_heap=[&](auto&)->std::uint32_t{throw std::runtime_error("Unqualified default heap");};
 services.arena.release_owned_pairs=[&](auto a,auto owner,auto block,auto bytes,auto tag,auto&fp){check(owner.identity==0x149e00u,"Unqualified native release router");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({block,bytes,tag})}});native_tu15_route_array_release(router,block,bytes,tag,fp,heap);};
 services.allocate_owned_records=[&](auto a,auto owner,auto bytes,auto tag,auto&fp){check(owner.identity==0x149da0u,"Unqualified native allocation router");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({bytes,tag})}});return native_tu15_route_array_allocation(router,bytes,tag,fp,heap);};
''';driver='#include "halo2/native_tu15_array_allocator.hpp"\n'+driver[:begin]+replacement+driver[end:]
  if a.native_arena:
   files+=['include/halo2/native_tu15_stack_arena.hpp','src/physics/native_tu15_stack_arena.cpp'];units.append('arena.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_stack_arena.cpp',project/'arena.cpp');needle=' const NativeTu15PairConsumerScratch scratch';replacement=''' NativeTu15ArrayGrowthServices backing;
 backing.allocate=[&](const auto&r,auto&fp){check(r.owner==0x149da0u,"Unknown arena allocation router");return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,fp,heap);};
 backing.release=[&](const auto&r,auto&fp){check(r.owner==0x149e00u,"Unknown arena release router");native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,fp,heap);};
 services.arena.allocate_block=[&](auto a,auto owner,auto bytes,auto&fp){check(owner.identity==0x2e10a0u,"Unknown arena allocation owner");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({bytes})}});return native_tu15_allocate_stack_arena(allocator_memory,a.identity,bytes,fp,backing);};
 services.arena.release_block=[&](auto a,auto owner,auto block,auto bytes,auto&fp){check(owner.identity==0x2e1110u,"Unknown arena release owner");callbacks.push_back(J{{"entry",owner.identity},{"this",a.identity},{"arguments",J::array({block,bytes})}});native_tu15_release_stack_arena(allocator_memory,a.identity,block,bytes,fp,backing);};
''';assert driver.count(needle)==1;driver='#include "halo2/native_tu15_stack_arena.hpp"\n'+driver.replace(needle,replacement+needle)
  pins={f:sha(ROOT/f)for f in files};(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(actual_pair LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(pair driver.cpp '+' '.join(units)+')\ntarget_include_directories(pair PRIVATE include)\ntarget_compile_features(pair PRIVATE cxx_std_20)\ntarget_compile_options(pair PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(pair PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME actual_pair COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:pair> "'+str(elf)+'" "'+str(scratch/'capture.json')+'" 0 50 "'+str(scratch/'instructions.json')+'" "${CMAKE_BINARY_DIR}/actual.json")\n')
  results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','pair','--filter','^actual_pair$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','actual.json'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==v for f,v in pins.items()),source_hashes=pins,compiler_results=results,archive_sha256=a.archive_sha256,metadata_sha256=sha(capture/'capture-native-before.json'),verified_bank_files=verified,before_bytes=sum(r['size']for r in regions),actual_typed_input=typed,original_sha256=NEW_PIN,source_authority=authority,code_authentication=authentication,declared_offline_ABI=dict(entry=0x321510,ECX=typed['island'],arguments=args,ESP=sp,FS_owned_private=fs,return_boundary=stop,private_returned_control_interval=[private_first,sp+20],nonargument_GP='independent zero definitions, never captured/post-return values',FP=fp),native_allocator=a.native_allocator,native_filter=a.native_filter,native_arena=a.native_arena,scope='Actual captured positive typed native input; offline normal original ABI declared explicitly; allpersistent bytes/FP/strictparent events/callbacks compared; no productionCPU ABI inferred')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
