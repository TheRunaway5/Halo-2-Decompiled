#!/usr/bin/env python3
"""Native2F3140 with genuine unchanged class and factory services.

Use actual protected BEFORE contexts and caller payloads. Execute every service
against its current mutable bank. Never install a recorded classification,
factory result, GP postimage or mutable-memory result into the native side.
"""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2)+'\n')
DRIVER=r'''
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}};try{
 check(argc==6,"Supply ELF, capture, producer ordinal, reversal and output");std::ifstream in(argv[2]);const auto capture=J::parse(in);const unsigned ordinal=std::stoul(argv[3]);const bool reverse=std::stoul(argv[4]);
 Child original;original.start(argv[1],ordinal);const auto entry=original.regs();const auto before=original.fp();check(before.ftw==0,"Dispatch requires declared empty incoming x87");const auto source_sp=std::uint32_t(entry.rsp),packet=((source_sp-4u)&~15u)-0x600u;
 auto a=original.word(source_sp+4),b=original.word(source_sp+8);if(reverse)std::swap(a,b);const auto context=original.word(source_sp+12),user_data=original.word(std::uint32_t(entry.rcx)+8u);
 struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
 
 const std::array<std::uint32_t,5>call_words{0x1ccaca,a,b,context,user_data};original.write(packet-20u,{reinterpret_cast<const std::byte*>(call_words.data()),20});auto full_entry=entry;full_entry.rip=0x2f3140;full_entry.rcx=0;full_entry.rsp=packet-20u;request(PTRACE_SETREGS,original.pid,nullptr,&full_entry);
 J actual_reads=J::array();std::size_t original_instructions{};
 for(;;){const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x1ccaca)break;check(++original_instructions<64,"Dispatch left authenticated31-site owner");std::uint32_t address{};bool retained=true;
  switch(pc){case 0x2f3146:address=r.rbp;break;case 0x2f3149:address=r.rcx;break;case 0x2f3151:address=r.rdi;break;case 0x2f3153:address=r.rax+20u;break;case 0x2f315a:case 0x2f315c:address=r.rcx;break;case 0x2f3160:address=r.rdx+20u;break;case 0x2f3170:address=std::uint32_t(r.rbx+r.rsi*4u+0x18cu);break;default:retained=false;}
  if(retained)actual_reads.push_back(J::array({address,4}));
  if(pc==0x2f3153||pc==0x2f3160||pc==0x2f317b){original.step();original.go(pc==0x2f3153?0x2f3156u:pc==0x2f3160?0x2f3163u:0x2f317du);}else original.step();
 }
 const auto returned=std::uint32_t(original.regs().rax);const auto completed_fp=original.fp();NativeTu15PhysicsFrameFp expected{completed_fp.cwd,completed_fp.swd,completed_fp.mxcsr};for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());
 Child native;native.start(argv[1],ordinal);for(const auto&bank:banks)native.write(bank.address,bank.before);auto installed=entry;request(PTRACE_SETREGS,native.pid,nullptr,&installed);auto initial_fp=before;request(PTRACE_SETFPREGS,native.pid,nullptr,&initial_fp);Memory memory(native);Bridge dependencies{native,packet-36u,before,report};NativeTu15ContactAgentDispatchServices services;unsigned classifications{};
 services.classify_shape=[&](NativeTu15ContactChild shape,NativeTu15ContactVirtualOwner owner,NativeTu15PhysicsFrameFp&fp){return dependencies.call(owner.identity,shape.identity,{},classifications++==0?0x2f3156u:0x2f3163u,fp);};
 services.create_agent=[&](NativeTu15ContactVirtualOwner owner,NativeTu15ContactPayload first,NativeTu15ContactPayload second,NativeTu15ContactContext settings,std::uint32_t data,NativeTu15PhysicsFrameFp&fp){return NativeTu15ContactAgent{dependencies.call(owner.identity,0,{first.identity,second.identity,settings.identity,data},0x2f317d,fp)};};
 NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};const auto result=native_tu15_dispatch_contact_agent(memory,{a},{b},{context},user_data,fp,services);
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();std::size_t compared{},different{},private_different{};J diffs=J::array();for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n){const auto address=bank.address+n;const bool is_private=address>=private_first&&address<private_end;if(!is_private)++compared;if(actual[n]==bank.after[n])continue;if(is_private){++private_different;continue;}++different;if(diffs.size()<32)diffs.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
 check(memory.ledger==actual_reads,"Native dispatch ordered operand read differs");const bool fp_equal=J::array({fp.control_word,fp.status,fp.mxcsr})==J::array({expected.control_word,expected.status,expected.mxcsr});report.update(J{{"passed",different==0&&fp_equal&&result.identity==returned},{"source_entry",0x2f3140},{"producer_ordinal",ordinal},{"reversed_payload_inputs",reverse},{"arguments",J::array({a,b,context,user_data})},{"returned_original_agent",returned},{"returned_native_agent",result.identity},{"fp_equal",fp_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},{"private_control_stack_different_bytes",private_different},{"first_differences",diffs},{"native_reads",memory.reads},{"ordered_original_operand_reads",actual_reads},{"original_dispatch_instructions",original_instructions},{"actual_original_service_calls",dependencies.calls},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 source=ROOT/'src/physics/native_tu15_contact_agent_dispatch.cpp';header=ROOT/'include/halo2/native_tu15_contact_agent_dispatch.hpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,bridge,Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;prefix=bridge.read_text().split('int main(int argc,char**argv)')[0];assert prefix.count('struct Child{')==1;prefix=prefix.replace('mutable std::size_t reads{};std::size_t writes{};','mutable std::size_t reads{};std::size_t writes{};mutable J ledger=J::array();').replace('++reads;}void write','++reads;ledger.push_back(J::array({p,b.size()}));}void write');assert 'ledger.push_back' in prefix;driver=scratch/'dispatch.cpp';driver.write_text('#include "halo2/native_tu15_contact_agent_dispatch.hpp"\n'+prefix+DRIVER)
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458';cases=[];contexts=[]
  for name,count in(('gemini-column0',2),('gemini-column1',2),('elongation-crates',3)):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,text=True,capture_output=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_exit_code=r.returncode))
   for ordinal in range(count):
    for reverse in(0,1):cases.append(dict(name=f'{name}_{ordinal}_{reverse}',elf=str(replay/'original-hardware'),capture=str(capture/'capture.json'),ordinal=ordinal,reverse=reverse))
  project=scratch/'project';project.mkdir();shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={};needle='(first_class<<5u)+second_class';assert text.count(needle)==1;variants['wrong_stride']=text.replace(needle,'(first_class<<4u)+second_class');needle='word(memory,dispatch+index*4u+0x18cu)';assert text.count(needle)==1;variants['wrong_slot']=text.replace(needle,'word(memory,dispatch+index*4u+0x190u)')
  for name,body in variants.items():(project/(name+'.cpp')).write_text(body)
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and (j.get("error")or j.get("different_bytes",0)>0 or j.get("returned_original_agent")!=j.get("returned_native_agent")),(r.returncode,j)\nprint("Original dispatch rejects wrong factory selection")\n')
  cmake=f'cmake_minimum_required(VERSION 3.20)\nproject(NativeContactDispatch LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\nadd_library(driver OBJECT "{driver}")\ntarget_include_directories(driver PRIVATE "{ROOT}/include")\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\nfile(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")\n';retain=[]
  for variant in('native',*variants):
   cmake+=f'add_executable(dispatch_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp)\ntarget_include_directories(dispatch_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(dispatch_{variant} PRIVATE cxx_std_20)\n'
   for case in cases if variant=='native'else cases[:1]:
    name=variant+'_'+case['name'];path='evidence/'+name+'.json';retain.append(path);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:dispatch_{variant}> "{case["elf"]}" "{case["capture"]}" {case["ordinal"]} {case["reverse"]} "${{CMAKE_BINARY_DIR}}/{path}"';cmake+=f'add_test(NAME dispatch_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','dispatch_native',*sum((['--target','dispatch_'+n]for n in variants),[]),'--filter','^dispatch_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results),contexts=contexts,compiler_results=results,physical_cases_per_compiler=len(cases),ordered_data_operand_reads_per_complete_call=8,negative_controls=list(variants),source_hashes=pins,current_sources_unchanged=all(sha(ROOT/f)==v for f,v in pins.items()),image_sha256=NEW_PIN,before_inputs_only=True,supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
