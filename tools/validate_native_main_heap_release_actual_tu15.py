#!/usr/bin/env python3
"""Actual pool service BEFORE: independent232D20 ABI versus native-only helper."""
import argparse,bisect,hashlib,inspect,json,lzma,os,shutil,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x232d20,0x232c80)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);p.add_argument('--world-stage',type=Path);p.add_argument('--source-root',type=Path,default=ROOT);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_GP_or_after_consumed=False,captured_bank_exclusions=[])
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads(a.archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-pair-pool-release-typed-before-v1'and capture['reference']=='English TU1.5 only'and not capture['guest_abi_fabricated']and not capture['source_memory_mutated']and not capture['physics_result_supplied'];typed=capture['input'];assert typed['owner']==0x232d20 and typed['allocator']and typed['preserved_x87_depth']==0
  manifest=json.loads(lzma.decompress(a.archive.with_name('all-file-hashes.json.xz').read_bytes()));wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=str(Path(member.name));name=name[2:]if name.startswith('./')else name
    if name not in wanted:continue
    r=wanted[name];raw=tar.extractfile(member).read();proof=manifest.get('./'+name,manifest.get(name));assert proof is not None and len(raw)==r['size']and hashlib.sha256(raw).hexdigest()==proof['sha256'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted)and sum(map(len,banks.values()))==capture['bank_bytes']
  starts=sorted(banks)
  def word(p):
   at=bisect.bisect_right(starts,p)-1;assert at>=0;first=starts[at];assert p+4<=first+len(banks[first]);return int.from_bytes(banks[first][p-first:p-first+4],'little')
  table=word(typed['allocator']);child=word(table+0xc);assert child==0x232c80,('Actual current virtual+C requires a different native owner',hex(child))
  assert all(not any(first<=p<first+len(raw)for first,raw in banks.items())for p in(base.UI,base.STACK,0x71000000,0x70000000))
  image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');authenticated=0
  for owner in OWNERS:
   for pc in complete_body(code,owner).code:
    i=code.at(pc);index=bisect.bisect_right(starts,pc)-1
    if index>=0:
     first=starts[index]
     if pc+i.size<=first+len(banks[first]):assert banks[first][pc-first:pc-first+i.size]==i.bytes;authenticated+=1
  base.OWNERS=OWNERS
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();observer=observer.replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');(scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')");source=source.replace("case['before_fp']=fp","fp=case['before_fp_input'];case['before_fp']=fp")
  fp=typed['floating_point'];rows=[dict(name='actual_main_heap_'+str(mode),entry=0x232d20,receiver=typed['allocator'],arguments=[typed['data'],typed['bytes'],typed['tag']],user_words=[],before_fp_input=[(fp['x87_control_word']&~0xc00)|(mode<<10),fp['x87_status'],(fp['mxcsr']&~0x6000)|(mode<<13)])for mode in range(4)]
  namespace=dict(base.__dict__);exec(source,namespace);fixture,authority=namespace['original'](image,banks,rows,scratch);assert all(row['after_esp']==base.SP+16 for row in rows);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  for relative in('include/halo2/native_tu15_main_heap_release.hpp','include/halo2/native_tu15_main_heap_release_binding.hpp'):
   shutil.copyfile(ROOT/relative,project/relative)
  world_includes=''
  if a.world_stage:
   for path in(a.world_stage/'staged/include').rglob('*.hpp'):shutil.copyfile(path,project/'include'/path.relative_to(a.world_stage/'staged/include'))
  for name in('native_tu15_main_heap_release.cpp','native_tu15_array_allocator.cpp'):
   owner=ROOT if name=='native_tu15_main_heap_release.cpp'else a.source_root;shutil.copyfile(owner/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_main_heap_release_binding.hpp"\n'+driver
  begin=driver.index('      NativeTu15ArrayGrowthMemory array(');end=driver.index('      const auto host_sw=',begin);driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','NativeTu15MainHeapReleaseBindingActivity activity;')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  invoke='''      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        native_tu15_release_bound_main_heap_allocation(memory,{c.at("receiver"),0x232d20u,args[0],args[1],args[2]},fp,0,observe,&activity);
'''
  if a.world_stage:
   world=(a.world_stage/'staged/src/physics/native_tu15_world_integration_residuals.cpp').read_text();begin_world=world.index('  pair_heap.release_owner=');end_world=world.index('  pair_heap.default_heap=',begin_world);callback=world[begin_world:end_world]
   world_includes='#include "halo2/native_tu15_world_integration.hpp"\n#include "halo2/native_tu15_pair_pool_release_input_observer.hpp"\n'
   invoke='''      NativeTu15WorldIntegrationStats world_stats;auto*stats=&world_stats;auto&m=memory;const struct{unsigned x87_depth;}state{0};
      const auto unqualified_pair_service=[&](const std::string&why){++stats->native_pair_consumer_unqualified_services;throw std::runtime_error(why);};NativeTu15ArrayAllocatorServices pair_heap;
'''+callback+'''      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();pair_heap.release_owner({c.at("receiver"),0x232d20u,args[0],args[1],args[2]},fp);activity=world_stats.main_heap_release;
'''
  driver=world_includes+driver[:begin]+invoke+driver[end:]
  driver=driver.replace('      check(calls==','      check(activity.attempts==1&&activity.completed==1&&activity.refusals==0&&activity.kernel.wrapper_calls==1&&activity.kernel.ledger_calls==1&&activity.kernel.completed==1&&activity.kernel.unbound_services==0,"Actual main heap activity differs");\n      check(calls==')
  driver=driver.replace('check(cases>20,','check(cases==4,');(project/'driver.cpp').write_text(driver)
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(actual_main_heap LANGUAGES CXX)\ninclude(CTest)\nadd_executable(actual driver.cpp native_tu15_main_heap_release.cpp native_tu15_array_allocator.cpp)\ntarget_include_directories(actual PRIVATE include)\ntarget_compile_features(actual PRIVATE cxx_std_20)\ntarget_compile_options(actual PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME actual_main_heap COMMAND actual "'+str(fixture/'original.json')+'")\n';(project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','actual','--filter','^actual_main_heap$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];rc=subprocess.run(command).returncode;results.append(dict(compiler=compiler,exit_code=rc));assert rc==0,'Native actual main heap differs'
  report.update(passed=True,archive=str(a.archive.resolve()),archive_sha256=a.archive_sha256,original_sha256=NEW_PIN,actual_input=typed,actual_allocator_table=table,actual_child=child,actual_before_bytes=sum(map(len,banks.values())),actual_before_banks=len(banks),captured_bytes_excluded=0,profiles=len(rows),ordered_reads=sum(len(row['reads'])for row in rows),ordered_stores=sum(len(row['writes'])for row in rows),source_authority=authority,captured_instructions_authenticated=authenticated,results=results,source_hashes={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()and p.suffix in('.cpp','.hpp')and(p.name in('driver.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_release.hpp','native_tu15_main_heap_release_binding.hpp'))},world_callback_source=None if not a.world_stage else str(a.world_stage/'staged/src/physics/native_tu15_world_integration_residuals.cpp'),world_callback_sha256=None if not a.world_stage else sha(a.world_stage/'staged/src/physics/native_tu15_world_integration_residuals.cpp'),scope=__doc__+' All captured bytes, strict ordered DATA read/store/preimages and FP; no native CPU callback or supplied lifetime outcome. Four ordinary independent leaf input profiles, no captured caller ABI claim.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
