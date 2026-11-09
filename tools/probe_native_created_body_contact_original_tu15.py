#!/usr/bin/env python3
"""Unchanged original281710 on actual typed1481 DATA; no native child outcomes."""
import argparse,hashlib,inspect,json,os,subprocess
from pathlib import Path
from native_created_body_contact_before_input_tu15 import NativeCreatedBodyContactBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--sha256',required=True);p.add_argument('--output',required=True,type=Path);p.add_argument('--modes',nargs='+',type=int,default=[0,1,2,3]);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);sources=('tools/probe_native_created_body_contact_original_tu15.py','tools/native_created_body_contact_before_input_tu15.py','tools/native_contact_process_before_input_tu15.py','tools/build_native_tu15_constraint_before_reference.py','tools/observe_native_array_growth_tu15.cpp','tools/replay_live_phantom_hardware_tu15.py');pins={n:sha(ROOT/n)for n in sources};r=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],original_only=True,native_outcomes_supplied=False,source_sha256=pins)
 try:
  for n in pins:
   q=a.output/'source'/n;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((ROOT/n).read_bytes())
  before=NativeCreatedBodyContactBeforeInput(a.archive,a.sha256,scratch/'before');assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
  # The standalone caller owns its normal SEH root. This is original-only
  # scaffold, never a semantic/native GP or control input.
  source=inspect.getsource(build_constraint_reference).replace(' mov %ax,%fs\n',' mov %ax,%fs\n movl $0xffffffff,%fs:0\n');env=dict(build_constraint_reference.__globals__);exec(source,env);authority=env['build_constraint_reference'](before,scratch/'reference');authority['entry']='Declared standalone281710 listener/event DATA; no captured original GP ABI';reference=Path(authority['elf'])
  prefix=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
  driver=prefix+r'''
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"original_only",true},{"native_results_supplied",false}};try{
 check(argc==5,"Supply reference/input/report/mode");std::ifstream in(argv[2]);const auto data=J::parse(in);const unsigned mode=std::stoul(argv[4]);
 const auto&input=data.at("input");const auto directory=std::filesystem::path(argv[2]).parent_path();
 const auto file=[&](const char*name){std::ifstream f(directory/name,std::ios::binary);const std::vector<char>raw((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b;for(auto v:raw)b.push_back(std::byte(v));return b;};
 constexpr std::uint32_t sp=0x7200f000u,event=0x72008000u,stop=0x5000000u;
 Child child;child.pid=fork();check(child.pid>=0,"Cannot fork original body callback");if(!child.pid){ptrace(PTRACE_TRACEME,0,nullptr,nullptr);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status{};const auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original body wait failed");};wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original body setup failed");
 const auto put=[&](std::uint32_t p,std::span<const std::byte>b){iovec local{const_cast<std::byte*>(b.data()),b.size()},remote{reinterpret_cast<void*>(std::uintptr_t(p)),b.size()};check(process_vm_writev(child.pid,&local,1,&remote,1,0)==std::ptrdiff_t(b.size()),"Original body DATA marshaling failed");};
 const auto id=file("event-id-2.bin"),fields=file("event-fields-4-35.bin");check(id.size()==2&&fields.size()==32,"Original event fragments differ");put(event,id);put(event+4u,fields);
 const std::array<std::uint32_t,2>args{stop,event};put(sp,std::as_bytes(std::span(args)));
 user_regs_struct regs{};request(PTRACE_GETREGS,child.pid,nullptr,&regs);regs.rip=0x281710u;regs.rsp=sp;regs.rcx=input.at("listener").get<std::uint32_t>();regs.eflags=2;request(PTRACE_SETREGS,child.pid,nullptr,&regs);
 user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);const auto&policy=input.at("floating_point");fp.cwd=policy.at("x87_control_word");fp.swd=policy.at("x87_status");fp.mxcsr=policy.at("mxcsr");fp.ftw=0;if(mode){fp.cwd=(fp.cwd&~0xc00u)|(mode<<10u);fp.mxcsr=(fp.mxcsr&~0x6000u)|(mode<<13u);}request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);
 J calls=J::array(),entries=J::array();std::map<std::uint32_t,std::string>executed;unsigned instructions{};bool returned{};
 for(;instructions<200000u;++instructions){
  request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);if(pc==stop){check(std::uint32_t(regs.rsp)==sp+8u,"Original body RET4 stack delta differs");returned=true;break;}
  const auto opcode=memory(child.pid,pc,15);executed.try_emplace(pc,hex(opcode));
  if(pc==0x281890u){J ordinary=J::array();const auto bytes=memory(child.pid,std::uint32_t(regs.rsp)+4u,40u);for(unsigned n=0;n<10;++n){std::uint32_t v{};std::memcpy(&v,bytes.data()+n*4u,4);ordinary.push_back(v);}entries.push_back(J{{"owner",pc},{"ordinary_ret40_arguments",ordinary}});}
  if(pc==0x1d86b0u||pc==0x1d76b0u||pc==0x20d8b0u){
   const auto count=pc==0x1d86b0u?13u:9u;J ordinary=J::array();const auto bytes=memory(child.pid,std::uint32_t(regs.rsp)+4u,count*4u);for(unsigned n=0;n<count;++n){std::uint32_t v{};std::memcpy(&v,bytes.data()+n*4u,4);ordinary.push_back(v);}
   J entry{{"owner",pc},{"ordinary_stack_arguments",ordinary}};
   if(pc==0x1d76b0u)entry["ordinary_register_operands"]={{"destination",std::uint32_t(regs.rax)},{"point_word",std::uint32_t(regs.rcx)&0xffffu},{"row_field_c_bits",std::uint32_t(regs.rdx)}};
   if(pc==0x20d8b0u)entry["ordinary_register_operands"]={{"request",std::uint32_t(regs.rax)},{"flags_byte",std::uint32_t(regs.rdx)&0xffu},{"forward",std::uint32_t(regs.rcx)}};
   entries.push_back(entry);
  }
  const auto b0=std::to_integer<unsigned>(opcode[0]);const bool call=b0==0xe8u||(b0==0xffu&&((std::to_integer<unsigned>(opcode[1])>>3u)&7u)==2u);
  request(PTRACE_SINGLESTEP,child.pid);wait();
  if(!WIFSTOPPED(status)||WSTOPSIG(status)!=SIGTRAP){report["failed_instruction"]=pc;report["failed_opcode15"]=hex(opcode);report["stop_signal"]=WIFSTOPPED(status)?WSTOPSIG(status):0;report["calls_before_failure"]=calls;J partial=J::array();for(const auto&[p,b]:executed)partial.push_back(J{{"instruction",p},{"original_bytes15",b}});report["executed_sites_before_failure"]=partial;throw std::runtime_error("Original body callback stopped before RET4");}
  if(call){user_regs_struct next{};request(PTRACE_GETREGS,child.pid,nullptr,&next);calls.push_back(J{{"callsite",pc},{"actual_target",std::uint32_t(next.rip)}});}
 }
 check(returned,"Original body callback exceeded finite budget");request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);J sites=J::array();for(const auto&[p,b]:executed)sites.push_back(J{{"instruction",p},{"original_bytes15",b}});
 report["profiles"].push_back(J{{"passed",true},{"actual_input_fp",mode==0},{"declared_rounding_control",mode},{"instructions",instructions},{"sites",sites},{"calls",calls},{"typed_entries",entries},{"after_fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"ret4_checked",true},{"event_relocated_to_ordinary_caller_data",event},{"actual_event_identity",input.at("event")}});report["passed"]=true;
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;}
'''
  driver=driver.replace('#include <vector>','#include <vector>\n#include <filesystem>')
  # The prefix contains its includes; add filesystem for the standalone driver.
  driver='#include <filesystem>\n#include <span>\n'+driver;(scratch/'observer.cpp').write_text(driver);exe=scratch/'observer';subprocess.run(['g++','-std=c++20','-O1',str(scratch/'observer.cpp'),'-o',str(exe)],check=True,capture_output=True)
  profiles=[]
  for mode in a.modes:
   assert mode in(0,1,2,3);out=a.output/f'original-mode{mode}.json';command=[str(exe),str(reference),str(before.path/'capture-native-before.json'),str(out),str(mode)];p=subprocess.run(command,capture_output=True,text=True);(a.output/f'original-mode{mode}.log').write_text(p.stdout+p.stderr);v=json.loads(out.read_text());
   sites=v['profiles'][0]['sites']if v['passed']else v.get('executed_sites_before_failure',[])
   for row in sites:assert image.read(row['instruction'],15)==bytes.fromhex(row['original_bytes15']),'Executed original text differs'
   profiles.append(dict(mode=mode,passed=v['passed'],report_sha256=sha(out),authenticated_opcode_windows=len(sites)));assert p.returncode==0,'Original-only body callback stopped; inspect real child graph/diagnostics'
  assert all(sha(ROOT/n)==h for n,h in pins.items())
  r.update(passed=True,before=before.receipt,original_sha256=NEW_PIN,profiles=profiles,reference_authority=authority,scope='Only actual typedBefore DATA with a declared standalone281710 caller and relocated34-byte provided event. No capturedGP, native outcomes, fullaccepted native/World claim.',tool_sha256=sha(__file__),driver_sha256=hashlib.sha256(driver.encode()).hexdigest(),source_inputs_unchanged=True)
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
