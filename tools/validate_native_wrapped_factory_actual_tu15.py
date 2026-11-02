#!/usr/bin/env python3
"""Full native wrapped/transformed factory closure on actualElongation04 DATA."""
import argparse,hashlib,inspect,json,os,shutil,struct,subprocess,sys,tarfile
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
OWNERS=(0x2f1cc0,0x2f0df0,0x2f7190,0x2e44d0,0x2e4eb0,0x2f83b0,0x2f8300,0x2e4550,0x309060,0x309150,0x2e4690,0x2e47b0,0x2e5da0,0x3077c0,0x2e7e10,0x306750,0x2e5db0,0x3077f0,0x1f6610,0x306790,0x2e7d90,0x2e8d70,0x2e8d20,0x1cb030,0xa9520,0xab4b0,0xad1d0,0xae690,0xad390,0x31dd70,0x31edf0,0x31ede0,0x31d520,0xad740,0xadb50,0x149da0,0x149e00,0x233250,0x233420)
UNITS=('native_tu15_wrapped_contact_agent.cpp','native_tu15_convex_agent_factory.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_geometry.cpp','native_tu15_inverse_geometry.cpp','native_tu15_contact_cache.cpp','native_tu15_contact_shape.cpp','native_tu15_activation_services.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp','native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);p.add_argument('--base-source-root',type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],original_gp_supplied=False,expected_after_consumed=False,source_private_raw_ledger_claimed=False,captured_bank_exclusions=[])
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
  capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert typed['owner']==0x2f1cc0 and typed['preserved_x87_depth']==0;wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted)
  # Preserve every captured byte; merge exactly adjacent banks without filling
  # gaps so vector reads have the same committed-storage contract.
  merged={}
  for first,raw in sorted(banks.items()):
   if merged and last+len(merged[last])==first:merged[last]+=raw
   else:merged[first]=raw;last=first
  banks=merged;image=Xbe(NEW_DEFAULT);base.OWNERS=OWNERS
  import bisect
  from generate_physics_live_support_tu15 import complete_body
  starts=sorted(banks);code=_Code(image,'.text');authenticated=0
  for owner in OWNERS:
   for pc in complete_body(code,owner).code:
    instruction=code.at(pc);at=bisect.bisect_right(starts,pc)-1
    if at>=0:
     first=starts[at]
     if pc+instruction.size<=first+len(banks[first]):assert banks[first][pc-first:pc-first+instruction.size]==instruction.bytes;authenticated+=1
  meta_source=inspect.getsource(base.metadata);meta_source=meta_source.replace("assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'","if op.mem.segment and ins.reg_name(op.mem.segment)=='fs':continue\n                    assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'")
  meta_source=meta_source.replace('read=bool(op.access&CS_AC_READ)','read=bool(op.access&CS_AC_READ)or ins.mnemonic in("test","cmp")');namespace=dict(base.__dict__,OWNERS=OWNERS);exec(meta_source,namespace);base.metadata=namespace['metadata']
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().replace('check(site!=sites.end(),"Original array escaped authenticated complete bodies");','check(site!=sites.end(),("Original escaped pc="+std::to_string(pc)).c_str());')
  observer=observer.replace('J writes=J::array(),reads=J::array(),estimates=J::array(),allocator_calls=J::array();','J owner_entries=J::array(),writes=J::array(),reads=J::array(),estimates=J::array(),allocator_calls=J::array();')
  observer=observer.replace('if(pc==stop)break;','if(std::set<std::uint32_t>{0x2f1cc0u,0x2f7190u,0x2e4eb0u,0x2f83b0u}.contains(pc))owner_entries.push_back(pc);if(pc==stop)break;')
  observer=observer.replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)},{"owner_entries",owner_entries}')
  (scratch/'observer.cpp').write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')","str(scratch/'observer.cpp')")
  source=source.replace("case['before_fp']=fp","fp=case.get('before_fp_input',fp);case['before_fp']=fp")
  anchor="        assembly.append(f'''.code32";assert source.count(anchor)==1
  source=source.replace(anchor,"        assembly.append('.section .fs,\"aw\"\\n.zero 4096');sections.append('.fs 0x71000000:{*(.fs)}')\n"+anchor)
  source=source.replace(' cld\n fninit',''' cld
 mov $123,%eax
 mov $1,%ebx
 mov $descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 jnz setup_failed
 mov $7,%eax
 mov %ax,%fs
 movl $0xffffffff,%fs:0
 fninit''')
  source=source.replace('entry: .long', '''setup_failed:
 mov $1,%eax
 mov $99,%ebx
 int $0x80
descriptor:.long 0,0x71000000,0xfffff,0x51
entry: .long''')
  fp=typed['floating_point'];rows=[dict(name='actual_elongation_wrapped_'+str(mode),entry=typed['owner'],receiver=0,arguments=typed['arguments'],private_seed=(0xa5+mode*17)&255,user_words=[],before_fp_input=[(fp['x87_control_word']&~0xc00)|(mode<<10),fp['x87_status'],(fp['mxcsr']&~0x6000)|(mode<<13)])for mode in range(4)]
  namespace=dict(base.__dict__);exec(source,namespace);fixture,authority=namespace['original'](image,banks,rows,scratch);assert all(r['after_esp']==base.SP+4 for r in rows);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows));print('Actual original wrapped factory FULLRETURN',flush=True)
  candidate=a.base_source_root or ROOT;overrides=ROOT/'analysis/physics/native-wrapped-factory-actual1472-fullnative-20261102-third/source';project=scratch/'project';project.mkdir();shutil.copytree(candidate/'include',project/'include')
  for name in UNITS:
   owner=overrides if a.base_source_root and name=='native_tu15_contact_shape.cpp'else candidate
   shutil.copyfile(owner/'src/physics'/name,project/name)
  for name in ('native_tu15_contact_shape.hpp','native_tu15_wrapped_factory_binding.hpp'):
   if a.base_source_root:shutil.copyfile((ROOT if name=='native_tu15_wrapped_factory_binding.hpp'else overrides)/'include/halo2'/name,project/'include/halo2'/name)
  shutil.copyfile(candidate/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_wrapped_factory_binding.hpp"\n'+driver
  driver=driver.replace('struct Fault{};struct Required{};','constexpr std::uint32_t PRIVATE=0x06000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
  driver=driver.replace('  void write(std::uint32_t p,','  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}\n  void write(std::uint32_t p,')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,std::byte(c.at("private_seed").get<unsigned>())));J actual_stores=J::array(),calls=J::array();')
  driver=driver.replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
  begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin)
  driver=driver[:begin]+'''      NativeTu15WrappedFactoryBindingActivity binding;
      try{const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        const NativeTu15WrappedContactArguments input{[&]{return args[0];},[&]{return args[1];},[&]{return args[2];},[&]{return args[3];}};
        result=native_tu15_create_bound_wrapped_contact_factory(memory,{0x2f1cc0u},input,{PRIVATE,4096},fp,0,growth,observe,&binding).identity;
'''+driver[end:]
  driver=driver.replace('      check(calls==','''      const auto entries=c.at("owner_entries").get<std::vector<unsigned>>();const auto expected=[&](unsigned owner){return std::count(entries.begin(),entries.end(),owner);};
      check(!failed&&!required&&result==c.at("return_word")&&binding.wrapped_calls==expected(0x2f1cc0u)&&binding.transformed_calls==expected(0x2f7190u)&&binding.convex_calls==expected(0x2e4eb0u)+expected(0x2f83b0u)&&binding.attempts==entries.size()&&binding.completed==binding.attempts&&binding.unqualified_owners==0&&binding.refusals==0,"Actual native wrapped factory return/activity differs");
      check(calls==''')
  driver=driver.replace('check(cases>20,','check(cases==4,');(project/'driver.cpp').write_text(driver)
  pins={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()}
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(actual_wrapped LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(actual driver.cpp '+' '.join(UNITS)+')\ntarget_include_directories(actual PRIVATE include)\ntarget_compile_features(actual PRIVATE cxx_std_20)\ntarget_compile_options(actual PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(actual PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME actual_wrapped COMMAND actual "'+str(fixture/'original.json')+'")\n';(project/'CMakeLists.txt').write_text(cmake);results=[]
  commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','actual','--filter','^actual_wrapped$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
   command[command.index('--output')+1]=str(a.output/label);commands.append(command);return subprocess.run(command).returncode
  for compiler in('g++','clang++'):
   rc=run(compiler,compiler);results.append(dict(compiler=compiler,exit_code=rc));assert rc==0,'Native actual wrapped factory differs'
  kernel=project/'native_tu15_wrapped_contact_agent.cpp';text=kernel.read_text();mutants=(('wrong_size','agent.identity+4u,20u,2','agent.identity+4u,24u,2'),('wrong_derived_table','0x41caf0u','0x41ca9cu'))
  for name,before,after in mutants:
   assert text.count(before)==1;kernel.write_text(text.replace(before,after));assert run('negative-'+name,'g++')!=0,'Actual original failed to reject '+name;kernel.write_text(text)
  frozen=a.output/'source'
  for name in [*['src/physics/'+n for n in UNITS],'src/physics/native_tu15_contact_unwind_internal.hpp',*['include/halo2/'+n.replace('.cpp','.hpp')for n in UNITS],'include/halo2/native_tu15_wrapped_factory_binding.hpp']:
   src=project/(Path(name).name if name.startswith('src/')else name)
   if not src.exists():continue
   dst=frozen/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
  report.update(passed=True,archive_sha256=a.archive_sha256,original_sha256=NEW_PIN,actual_input=typed,actual_before_bytes=sum(map(len,banks.values())),actual_before_banks=len(capture['regions']),captured_bytes_excluded=0,profiles=len(rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),source_authority=authority,captured_instructions_authenticated=authenticated,source_hashes=pins,frozen_source_hashes={str(p.relative_to(frozen)):sha(p)for p in frozen.rglob('*')if p.is_file()},results=results,commands=commands,negative_controls=[v[0]for v in mutants],original_factory_entries=rows[0]['owner_entries'],original_return_esp=rows[0]['after_esp'],candidate_base_source_root=str(candidate.resolve()),production_native_only_closure=True,scope=__doc__+' Typedordinarycdecl caller; sourceprivateCPUcontroloperandledgerunclaimed. NativeDATA aliasesfullyowned; noGP/controlpostimages or childresults.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
