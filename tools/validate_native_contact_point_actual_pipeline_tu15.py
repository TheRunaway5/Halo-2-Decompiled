#!/usr/bin/env python3
"""All-native322F70 pipeline on every genuine1470 captured BEFORE bank.

The source-private raw ledger remains a separate RED report. This proof checks
every captured bank, FP and ordered captured-DATA operand with independently
owned native event/info/result buffers. No captured bank is waived or excluded.
"""
import argparse,bisect,hashlib,inspect,json,os,shutil,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
PRIVATE=0x06000000
OWNERS=(0x322f70,0x3224d0,0x31fee0,0x325d90,0x281d40,0x1d8880,0x1d8970,
        0x316aa0,0x3223d0,0x3201b0,0x320100,0x325e90,0x2e4d60,0x283450)
NATIVE=('native_tu15_contact_point_removal.cpp','native_tu15_gameplay_contact_removal.cpp',
        'native_tu15_contact_world_update.cpp','native_tu15_island_constraint_detach.cpp',
        'native_tu15_island_partition_metadata.cpp','native_tu15_contact_collector_info.cpp',
        'native_tu15_island_activation.cpp','native_tu15_array_growth.cpp')
HEADERS=('native_tu15_contact_point_removal.hpp','native_tu15_gameplay_contact_removal.hpp',
         'native_tu15_contact_world_update.hpp','native_tu15_island_constraint_detach.hpp',
         'native_tu15_island_partition_metadata.hpp','native_tu15_contact_collector_info.hpp')
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=[*['src/physics/'+n for n in NATIVE],*['include/halo2/'+n for n in HEADERS],'tools/validate_native_contact_point_actual_pipeline_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp'];pins={n:sha(ROOT/n)for n in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False,captured_original_gp_frame=False,strict_source_private_raw_ledger_passed=False,raw_private_ledger_reference='analysis/physics/native-contact-point-removal-tu15-20261101-first/report.json',captured_bank_exclusions=[])
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert capture['schema']=='native-tu15-contact-point-typed-before-v1'and typed['owner']==0x322f70 and typed['preserved_x87_depth']==0 and not capture['guest_abi_fabricated']and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
  def read(p,size):
   n=bisect.bisect_right(starts,p)-1
   if n<0:return None
   first=starts[n];return banks[first][p-first:p-first+size]if first<=p and p+size<=first+len(banks[first])else None
  def word(p,size=4):
   raw=read(p,size);assert raw is not None,(hex(p),size);return int.from_bytes(raw,'little')
  assert not any(p<PRIVATE+0x1000 and p+len(raw)>PRIVATE for p,raw in banks.items())
  assert not any(p<base.UI+0x10000 and p+len(raw)>base.UI for p,raw in banks.items())
  manager=typed['manager'];collector=manager+0x10;island=word(collector+0x14)
  assert word(manager)==0x41f124 and word(collector)==0x41f478 and word(island)==0x41f080 and word(collector+6,2)==2 and word(collector+0x94)==1
  image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');base.OWNERS=OWNERS
  for owner in OWNERS:
   for pc in complete_body(code,owner).code:
    i=code.at(pc);before=read(pc,i.size)
    if before is not None:assert before==i.bytes,(hex(pc),'Actual code differs from TU1.5')
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();anchor='if(pc==stop)break;';assert observer.count(anchor)==1
  entry_set=','.join(hex(o)+'u'for o in OWNERS)
  observer=observer.replace(anchor,'if(std::set<std::uint32_t>{'+entry_set+'}.contains(pc))allocator_calls.push_back(J{{"owner",pc}});'+anchor)
  observer=observer.replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}')
  observer_path=scratch/'observer.cpp';observer_path.write_text(observer)
  original=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)').replace("case['before_fp']=fp","fp=case.get('before_fp_input',fp);case['before_fp']=fp")
  fp=typed['floating_point'];all_cases=[dict(name='actual1470_point_native_data_'+str(seed),entry=0x322f70,receiver=manager,arguments=[typed['point']],user_words=[],private_seed=seed,before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']])for seed in(0,0xa5,0x5a)]
  environment=dict(base.__dict__,observer_path=observer_path);exec(original,environment);fixture,authority=environment['original'](image,banks,all_cases,scratch)
  assert all(c['after_esp']==base.SP+8 for c in all_cases)
  for c in all_cases:c['expected_entry_counts']={str(owner):sum(call['owner']==owner for call in c['allocator_calls'])for owner in OWNERS}
  save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in NATIVE:shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver=''.join('#include "halo2/'+n+'"\n'for n in HEADERS)+driver
  driver=driver.replace('struct Fault{};struct Required{};', 'constexpr std::uint32_t PRIVATE=0x06000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=0x1000;}struct Fault{};struct Required{};')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();std::set<unsigned>applied;', '      memory.banks.emplace(PRIVATE,std::vector<std::byte>(0x1000,std::byte(c.at("private_seed").get<unsigned>())));J actual_stores=J::array(),calls=J::array();std::set<unsigned>applied;std::map<std::uint32_t,unsigned>entries;std::uint64_t private_stores{};')
  driver=driver.replace('observe=[&](const auto&event){actual_stores.push_back', 'observe=[&](const auto&event){if(event.instruction==0x322f8cu)++entries[0x322f70u];if(event.instruction==0x3224dbu)++entries[0x3224d0u];if(event.instruction==0x31feebu)++entries[0x31fee0u];if(event.instruction==0x325da5u)++entries[0x325d90u];if(event.instruction==0x320108u)++entries[0x320100u];if(event.instruction==0x325e96u)++entries[0x325e90u];if(private_data(event.address,event.written.size())){++private_stores;return;}actual_stores.push_back')
  begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)
  setup='''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
      NativeTu15GameplayContactRemovalStats game_stats;NativeTu15ContactWorldUpdateStats world_stats;NativeTu15IslandConstraintDetachStats detach_stats;
      NativeTu15GameplayContactRemovalServices gameplay;NativeTu15ContactWorldUpdateServices world;NativeTu15ContactPointRemovalServices point;
      world.listener=[&](auto,auto owner,auto,auto&){check(owner.identity==0x2e4d60u,"Unqualified actual world listener");if(!read_pass)++entries[0x2e4d60u];};
      world.remove_from_island=[&](auto island,auto owner,auto collector,auto&state){check(owner.identity==0x3201b0u,"Unqualified actual island detach");native_tu15_detach_island_constraint(memory,{island},collector,{PRIVATE+0x300},state,{},observe,&detach_stats);};
      point.listener_removed=[&](auto,auto owner,auto event,auto&state){if(owner.identity==0x2e4d60u){if(!read_pass)++entries[0x2e4d60u];return;}check(owner.identity==0x281d40u,"Unqualified actual point listener");if(!read_pass)++entries[0x281d40u];native_tu15_notify_gameplay_contact_removed(memory,event,state,gameplay,observe,&game_stats);};
      point.collector_removed=[&](auto receiver,auto owner,auto,auto info,auto&){check(owner.identity==0x320100u,"Unqualified actual point metadata");native_tu15_remove_island_constraint_info(memory,{receiver},{info},observe);};
      point.update_world=[&](auto receiver,auto result,auto collector,auto&state){native_tu15_update_contact_world(memory,receiver,result,collector,state,world,observe,&world_stats);};
''';driver=driver[:begin]+setup+driver[end:]
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{native_tu15_remove_contact_point(memory,c.at("receiver"),c.at("arguments")[0],{PRIVATE+0x100,PRIVATE+0x200,PRIVATE+0x240},fp,point,observe);
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','')
  anchor='      check(calls==c.at("allocator_calls"),name+" actual allocator request order differs");';assert driver.count(anchor)==1
  driver=driver.replace(anchor,'''      check(game_stats.notifications==1&&game_stats.searches==1&&game_stats.matches==1&&game_stats.rows==1&&game_stats.constraint_releases==0&&game_stats.part_releases==0,name+" actual gameplay path differs");
      check(world_stats.attempts==1&&world_stats.completed==1&&world_stats.immediate==1&&world_stats.queued==0&&detach_stats.calls==1&&detach_stats.native_info_queries==1&&detach_stats.native_metadata_removals==1&&detach_stats.reference_releases==1&&detach_stats.unbound_services==0&&detach_stats.destructions==0,name+" actual final point native closure differs");
      if(!read_pass){entries[0x1d8880u]=game_stats.searches;entries[0x1d8970u]=game_stats.rows;entries[0x316aa0u]=world_stats.attempts;entries[0x3223d0u]=world_stats.immediate;entries[0x3201b0u]=detach_stats.calls;for(auto i=c.at("expected_entry_counts").begin();i!=c.at("expected_entry_counts").end();++i)check(entries[std::stoul(i.key())]==i.value(),name+" actual source/native logical owner count differs for "+i.key());}
''')
  driver=driver.replace('check(cases>20,','check(cases==3,');driver=driver.replace('J{{"passed",true},{"cases",cases}', 'J{{"passed",true},{"strict_source_private_raw_ledger_passed",false},{"captured_banks_excluded",0},{"cases",cases}')
  (project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(point_actual_pipeline LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(pipeline driver.cpp '+' '.join(NATIVE)+')\ntarget_include_directories(pipeline PRIVATE include)\ntarget_compile_features(pipeline PRIVATE cxx_std_20)\ntarget_link_libraries(pipeline PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME actual_point_pipeline COMMAND pipeline "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','pipeline','--filter','^actual_point_pipeline$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Actual native point pipeline differs'
  kernel=project/'native_tu15_contact_point_removal.cpp';text=kernel.read_text();mutants=(('wrong_count','const auto count=old_count-1u','const auto count=old_count'),('wrong_map_marker','map+(point&0xffffu),0xffu,1','map+(point&0xffffu),0xfeu,1'))
  for name,before,after in mutants:
   assert text.count(before)==1;kernel.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;kernel.write_text(text)
  game=project/'native_tu15_gameplay_contact_removal.cpp';text=game.read_text();assert text.count('last=count-1u')==1;game.write_text(text.replace('last=count-1u','last=count'));assert run('negative-gameplay-count',a.compilers[0])!=0;game.write_text(text)
  report.update(passed=all(sha(ROOT/n)==v for n,v in pins.items()),original_sha256=NEW_PIN,archive_sha256=a.archive_sha256,actual_typed_input=typed,actual_before_banks=len(banks),actual_before_bytes=sum(map(len,banks.values())),original_cases=len(all_cases),original_source_entry_counts=all_cases[0]['expected_entry_counts'],ordered_captured_data_reads=sum(len(c['reads'])for c in all_cases),ordered_captured_data_stores=sum(len(c['writes'])for c in all_cases),source_authority=authority,independent_native_private_data_seeds=[0,0xa5,0x5a],native_private_data_range=dict(first=PRIVATE,bytes=0x1000),results=results,commands=commands,negative_controls=['wrong_count','wrong_map_marker','gameplay_count'],scope='Complete actual1470 typed point-parent pipeline; all captured banks+ordered captured-DATA operands/preimages/FP and logical current-owner counts. Every selected dependency native. Ordinary original leaf ABI only; no captured GP. Fresh native private DATA is intentionally independent; source-private raw ledger remains RED separately, not waived. No live multiplayer/frame/camera claim.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
