#!/usr/bin/env python3
"""Physical unchanged-TU1.5 slow arena proof over actual retained allocator DATA.

The real original149DA0/233250 allocates every backing block, including each
release case's preparation. Native routing/pool kernels derive their own
results from the same before bytes. No requested pointer outcome is supplied.
"""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
def cases():
 rows=[]
 for size in(0,1,15,16,32,512,4095,4096,4097,5000,8176,0x80000000,0xffffffff):
  rows.append(dict(name=f'allocate_{size:x}',entry=0x2e10a0,receiver=0x486080,arguments=[size]))
 for size in(0,1,16,512,4095,4096,4097,5000,8176):
  rows.append(dict(name=f'release_{size:x}',entry=0x2e1110,receiver=0x486080,arguments=[0x91abcdef,size],prepare_arena_bytes=size))
 for name,entry,args,p,writing in [('allocate_global',0x2e10a0,[32],0x48c938,False),('allocate_save_remaining',0x2e10a0,[32],0x48608c,False),('allocate_publish_cursor',0x2e10a0,[32],0x486088,True),('release_boundary',0x2e1110,[0,32],0x486094,False),('release_restore',0x2e1110,[0,32],0x486088,True)]:
  rows.append(dict(name='fault_'+name,entry=entry,receiver=0x486080,arguments=args,fault=dict(address=p,bytes=4,occurrence=1,write=writing),**({'prepare_arena_bytes':32}if entry==0x2e1110 else{})))
 for site,address,value in[(0x2e10d1,0x48608c,0x71395),(0x2e10d6,0x486090,0x216379),(0x2e112a,0x486090,0x83ac51)]:
  release=site==0x2e112a;rows.append(dict(name=f'alias_{site:x}',entry=0x2e1110 if release else 0x2e10a0,receiver=0x486080,arguments=[0,32]if release else[32],mutations=[dict(instruction=site,address=address,written=value.to_bytes(4,'little').hex())],**({'prepare_arena_bytes':32}if release else{})))
 rows.extend(dict(name=f'null_release_{size:x}',entry=0x149e00,receiver=0x486080,arguments=[0,size,0x12])for size in(0,16,0xFFFFFFFF))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(exist_ok=False,parents=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=['include/halo2/native_tu15_stack_arena.hpp','src/physics/native_tu15_stack_arena.cpp','include/halo2/native_tu15_array_allocator.hpp','src/physics/native_tu15_array_allocator.cpp','include/halo2/native_tu15_array_growth.hpp','src/physics/native_tu15_array_growth.cpp','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp','tools/validate_native_stack_arena_tu15.py'];pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,provenance=base.reference_inputs(image);base.OWNERS=(*base.OWNERS,0x2e10a0,0x2e1110)
  # Preparation executes the unchanged slow-arena owner before the input
  # snapshot. No backing pointer or saved-header outcome is supplied.
  all_cases=cases();fixture,authority=base.original(image,banks,all_cases,scratch)
  save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in('native_tu15_stack_arena','native_tu15_array_allocator','native_tu15_array_growth'):shutil.copyfile(ROOT/f'src/physics/{name}.cpp',project/f'{name}.cpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();needle='        else throw std::runtime_error("Unauthenticated native array test owner");';assert driver.count(needle)==1
  driver='#include "halo2/native_tu15_stack_arena.hpp"\n'+driver.replace(needle,'        else if(entry==0x2e10a0u)result=native_tu15_allocate_stack_arena(memory,receiver,args[0],fp,growth,observe);\n        else if(entry==0x2e1110u)native_tu15_release_stack_arena(memory,receiver,args[0],args[1],fp,growth,observe);\n'+needle)
  driver=driver.replace('c.at("entry")==0x233250u||c.at("entry")==0x149da0u','c.at("entry")==0x233250u||c.at("entry")==0x149da0u||c.at("entry")==0x2e10a0u')
  (project/'driver.cpp').write_text(driver)
  (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(native_stack_arena LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
add_executable(arena driver.cpp native_tu15_stack_arena.cpp native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)
target_include_directories(arena PRIVATE include)
target_compile_features(arena PRIVATE cxx_std_20)
target_compile_options(arena PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(arena PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_stack_arena COMMAND arena "'''+str(fixture/'original.json')+'''")
''')
  commands=[];results=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','arena','--filter','^native_stack_arena$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native slow arena mismatch'
  mutations=[('header_word','m.word(publications[field],block+field*4u,phase.value);','m.word(publications[field],block+field*4u,phase.value^(field==0u?1u:0u));'),('minimum','requested_bytes:0x1000u','requested_bytes:0x800u'),('restore_remaining','m.word(publications[field],arena+8u+field*4u,phase.value);','m.word(publications[field],arena+8u+field*4u,phase.value^(field==1u?1u:0u));')]
  target=project/'native_tu15_stack_arena.cpp';unchanged=target.read_text()
  for name,needle,replacement in mutations:
   assert unchanged.count(needle)==1;target.write_text(unchanged.replace(needle,replacement));assert run('negative-'+name,a.compilers[0])!=0,'Unchanged source failed to reject mutation';target.write_text(unchanged)
  executed={pc for c in all_cases for pc in c['executed_sites']};report.update(passed=all(sha(ROOT/f)==p for f,p in pins.items()),source_authority=authority,input_provenance=provenance,original_cases=len(all_cases),original_instructions=sum(c['original_instructions']for c in all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),genuine_allocator_calls=sum(len(c['allocator_calls'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),observer_mutations=sum(c['mutations_applied']for c in all_cases),commands=commands,results=results,negative_controls=[m[0]for m in mutations],executed_arena_sites=sorted(executed&set(base.complete_body(base._Code(image,'.text'),0x2e10a0).code)|executed&set(base.complete_body(base._Code(image,'.text'),0x2e1110).code)),source_input_domain='Actual retained root/pool; declared byte requests and genuine slow-allocation preparation; no callback mutation of formal byte-value arguments',remaining=['other backing allocators/default heap/large pool release','actual pair-parent slow arena composition and live World binding'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
