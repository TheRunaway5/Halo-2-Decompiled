#!/usr/bin/env python3
"""Fresh actual typed31FFC0 BEFORE: full native phantom insertion composition.

Every captured bank and captured-DATA operand/preimage is compared. Original
private stack is independent of ordinary native16B event DATA, not serialized.
No child return, allocation pointer, CPU register file or after-image is supplied.
"""
import argparse,bisect,hashlib,inspect,json,os,shutil,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
PRIVATE=0x06000000
OWNERS=(0x31ffc0,0x315300,0x315b00,0x314f80,0x2e4d60,0x2e3250,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420)
NATIVE=('native_tu15_phantom_pair_delete.cpp','native_tu15_phantom_overlap_add.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp')
HEADERS=('native_tu15_phantom_pair_delete.hpp','native_tu15_phantom_overlap_add.hpp','native_tu15_phantom_creation_activity.hpp','native_tu15_array_growth.hpp','native_tu15_array_allocator.hpp')
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=[*['src/physics/'+n for n in NATIVE],*['include/halo2/'+n for n in HEADERS],'tools/validate_native_phantom_pair_create_actual_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp'];pins={n:sha(ROOT/n)for n in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False,captured_original_gp_frame=False,captured_bank_exclusions=[],strict_source_private_raw_ledger_claimed=False)
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert typed['owner']==0x31ffc0 and typed['preserved_x87_depth']==0 and not capture['guest_abi_fabricated']and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
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
  pair=typed['pair'];census=[]
  for offset in(0,4):
   handle=word(pair+offset);kind=word(handle+8);row=dict(pair_offset=offset,handle=handle,kind=kind)
   if kind==2:
    phantom=word(handle-0x10+0x20);table=word(phantom);owner=word(table+0x1c);assert owner in(0x315300,0x315b00)
    header=phantom+(0x80 if owner==0x315300 else 0xc0);listeners=[]
    for n in range(word(phantom+0x40)):
     listener=word(word(phantom+0x3c)+n*4)
     if listener:
      callback=word(word(listener));listeners.append(dict(listener=listener,owner=callback));assert callback==0x2e4d60,hex(callback)
    row.update(phantom=phantom,table=table,owner=owner,flag_word=phantom&0xffffff00,header=header,data=word(header),count=word(header+4),capacity=word(header+8),listeners=listeners)
   census.append(row)
  report['actual_input_census']=census;save(a.output/'input-census.json',dict(typed=typed,pairs=census,archive_sha256=a.archive_sha256))
  image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');base.OWNERS=OWNERS
  for owner in OWNERS:
   for pc in complete_body(code,owner).code:
    i=code.at(pc);before=read(pc,i.size)
    if before is not None:assert before==i.bytes,(hex(pc),'Actual code differs from TU1.5')
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();anchor='if(pc==stop)break;';assert observer.count(anchor)==1
  entries=','.join(hex(o)+'u'for o in OWNERS)
  record='''if(std::set<std::uint32_t>{'''+entries+'''}.contains(pc))owner_entries.push_back(pc);
    if(pc==0x2e4d60u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t event{};for(unsigned n=0;n<4;++n)event|=std::to_integer<unsigned>(raw[n])<<(8u*n);allocator_calls.push_back(J{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"data",hex(memory(child.pid,event,9))},{"decision",hex(memory(child.pid,event+12u,4))}});}
'''
  observer=observer.replace(anchor,record+anchor).replace('J writes=J::array(),reads=J::array(),estimates=J::array(),allocator_calls=J::array();','J owner_entries=J::array(),writes=J::array(),reads=J::array(),estimates=J::array(),allocator_calls=J::array();')
  observer=observer.replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)},{"owner_entries",owner_entries}')
  observer_path=scratch/'observer.cpp';observer_path.write_text(observer)
  original=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)').replace("case['before_fp']=fp","fp=case.get('before_fp_input',fp);case['before_fp']=fp")
  fp=typed['floating_point'];all_cases=[dict(name='actual1471_phantom_native_event_'+str(seed),entry=0x31ffc0,receiver=typed['handler'],arguments=[pair],user_words=[],private_seed=seed,before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']])for seed in(0,0xa5,0x5a)]
  environment=dict(base.__dict__,observer_path=observer_path);exec(original,environment);fixture,authority=environment['original'](image,banks,all_cases,scratch)
  assert all(c['after_esp']==base.SP+8 for c in all_cases)
  for c in all_cases:c['expected_entry_counts']={str(owner):c['owner_entries'].count(owner)for owner in OWNERS}
  save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in NATIVE:shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver=''.join('#include "halo2/'+n+'"\n'for n in HEADERS)+driver
  driver=driver.replace('struct Fault{};struct Required{};', 'constexpr std::uint32_t PRIVATE=0x06000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=0x1000;}struct Fault{};struct Required{};')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();std::set<unsigned>applied;', '      memory.banks.emplace(PRIVATE,std::vector<std::byte>(0x1000,std::byte(c.at("private_seed").get<unsigned>())));J actual_stores=J::array(),calls=J::array();std::set<unsigned>applied;native_tu15_phantom_creation_counters={};')
  driver=driver.replace('observe=[&](const auto&event){actual_stores.push_back', 'observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  setup=r'''      NativeTu15PhantomOverlapAddServices overlap;overlap.allocation=growth;
      overlap.added=[&](auto listener,auto owner,auto event,auto&){
        check(owner.identity==0x2e4d60u,"Unqualified actual added listener");
        const auto first=memory.at(event.identity,9),decision=memory.at(event.identity+12u,4);
        calls.push_back(J{{"owner",owner.identity},{"receiver",listener},
          {"data",hex(std::span(first->second).subspan(event.identity-first->first,9))},
          {"decision",hex(std::span(decision->second).subspan(event.identity+12u-decision->first,4))}});
        native_tu15_ignore_removed_phantom_overlap();
      };
      NativeTu15PhantomPairCreateServices pair_services;
      pair_services.add_overlap=[&](auto phantom,auto owner,auto other,auto flag,auto&state){
        if(owner.identity==0x315300u)native_tu15_add_simple_phantom_overlap(memory,phantom,other,flag,{PRIVATE+0x100},state,overlap,observe);
        else if(owner.identity==0x315b00u)native_tu15_add_phantom_overlap(memory,phantom,other,flag,{PRIVATE+0x100},state,overlap,observe);
        else throw Required{};
      };
'''
  anchor='      const auto host_sw=status(),host_cw=control();';assert driver.count(anchor)==1;driver=driver.replace(anchor,setup+anchor)
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{native_tu15_create_phantom_pair(memory,c.at("arguments")[0],fp,pair_services);
'''+driver[end:]
  anchor='      check(calls==c.at("allocator_calls"),name+" actual allocator request order differs");'
  driver=driver.replace(anchor,anchor+r'''
      const auto phantom_activity=native_tu15_phantom_creation_activity();const auto expected=[&](unsigned owner){return c.at("expected_entry_counts").at(std::to_string(owner)).get<unsigned>();};
      check(phantom_activity.pair_calls==expected(0x31ffc0u)&&phantom_activity.pair_completions==expected(0x31ffc0u)&&phantom_activity.simple_calls==expected(0x315300u)&&phantom_activity.complex_calls==expected(0x315b00u)&&phantom_activity.notification_calls==expected(0x314f80u)&&phantom_activity.growths==expected(0x2e3250u),name+" native/source activity differs");
''')
  driver=driver.replace('check(cases>20,','check(cases==3,');driver=driver.replace('J{{"passed",true},{"cases",cases}', 'J{{"passed",true},{"source_private_raw_ledger_claimed",false},{"captured_banks_excluded",0},{"cases",cases}')
  (project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(phantom_actual LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(pipeline driver.cpp '+' '.join(NATIVE)+')\ntarget_include_directories(pipeline PRIVATE include)\ntarget_compile_features(pipeline PRIVATE cxx_std_20)\ntarget_compile_options(pipeline PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(pipeline PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME actual_phantom_pipeline COMMAND pipeline "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','pipeline','--filter','^actual_phantom_pipeline$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Actual native phantom pipeline differs'
  target=project/'native_tu15_phantom_pair_delete.cpp';text=target.read_text();mutants=[('other_payload','{other},phantom&0xffffff00u,fp);','{other+4u},phantom&0xffffff00u,fp);')]
  for name,before,after in mutants:
   assert text.count(before)==2;target.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;target.write_text(text)
  target=project/'native_tu15_phantom_overlap_add.cpp';text=target.read_text();before='m.word(0x315388u,header+4u,m.word(header+4u)+1u);';assert text.count(before)==1;target.write_text(text.replace(before,'m.word(0x315388u,header+4u,m.word(header+4u)+2u);'));assert run('negative-append-count',a.compilers[0])!=0;target.write_text(text)
  executed={pc for c in all_cases for pc in c['executed_sites']}
  report.update(passed=all(sha(ROOT/n)==v for n,v in pins.items()),original_sha256=NEW_PIN,archive_sha256=a.archive_sha256,actual_typed_input=typed,actual_before_banks=len(banks),actual_before_bytes=sum(map(len,banks.values())),original_cases=len(all_cases),original_source_entry_counts=all_cases[0]['expected_entry_counts'],ordered_captured_data_reads=sum(len(c['reads'])for c in all_cases),ordered_captured_data_stores=sum(len(c['writes'])for c in all_cases),source_authority=authority,independent_native_private_data_seeds=[0,0xa5,0x5a],native_private_data_range=dict(first=PRIVATE,bytes=0x1000),results=results,commands=commands,negative_controls=['other_payload','append_count'],body_coverage={hex(o):dict(total=len(complete_body(code,o).code),executed=len(set(complete_body(code,o).code)&executed))for o in OWNERS},scope=__doc__+' Actual selected callbacks are checked by owner; zero CPU children. No live camera/frame claim from this component proof.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
