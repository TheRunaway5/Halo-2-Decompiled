// Test-only i386 execution boundary. Original code/inputs stay unchanged.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
#include <set>
#include <sstream>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <array>
#include <vector>
using J=nlohmann::json;
void require(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void *a=nullptr,void *b=nullptr){errno=0;auto r=ptrace(op,pid,a,b);if(r==-1&&errno)throw std::runtime_error(strerror(errno));return r;}
int main(int argc,char **argv)try{
 require(argc==8 || argc==9,"Supply ELF, output, return, capture trampoline, text end, trace, original RET sites and optional recorded platform imports");
 std::set<unsigned long> returns;std::istringstream sites(argv[7]);for(std::string value;std::getline(sites,value,',');)returns.insert(std::stoul(value,nullptr,0));
 require(!returns.empty() && returns.size()<=4,"Original entry requires one to four authenticated RET sites");
 const auto target=std::stoul(argv[3],nullptr,0),capture=std::stoul(argv[4],nullptr,0),text_end=std::stoul(argv[5],nullptr,0);
 auto child=fork();require(child>=0,"Cannot create original hardware process");
 if(!child){auto fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,0600);if(fd<0||dup2(fd,1)<0)_exit(125);close(fd);
   if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status{};auto wait=[&](){require(waitpid(child,&status,0)==child,"Hardware wait failed");};wait();
 require(WIFSTOPPED(status),"Missing hardware exec stop");request(PTRACE_SETOPTIONS,child,nullptr,reinterpret_cast<void *>(PTRACE_O_EXITKILL));
 request(PTRACE_CONT,child);wait();require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Hardware input setup failed");
 J bindings=J::object();
 if(argc==9) {
  std::ifstream input(argv[8]);bindings=J::parse(input);
  for(const auto &binding:bindings.value("writes",J::array())) {
   const auto address=binding.at("address").get<unsigned long>();
   const auto prior=static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(address)));
   require((prior&0xfffffffful)==binding.at("before").get<unsigned long>(),"Recorded platform import preimage changed");
   const auto linked=(prior&~0xfffffffful)|binding.at("linked").get<unsigned long>();
   request(PTRACE_POKEDATA,child,reinterpret_cast<void *>(address),reinterpret_cast<void *>(linked));
  }
 }
 request(PTRACE_SINGLESTEP,child);wait();J trace=J::array();user_regs_struct regs{};
 request(PTRACE_GETREGS,child,nullptr,&regs);const auto entry_flags=regs.eflags;
 auto debug=[&](unsigned index,unsigned long value){request(PTRACE_POKEUSER,child,reinterpret_cast<void *>(offsetof(user,u_debugreg)+index*sizeof(unsigned long)),reinterpret_cast<void *>(value));};
 unsigned index=0;unsigned long enabled=0;for(const auto pc:returns){debug(index,pc);enabled|=1ul<<(2*index++);}debug(6,0);debug(7,enabled);
 const auto sound_views=bindings.value("sound_timing_views",J::array());
 const auto sound_pcs=bindings.value("sound_timing_pcs",std::vector<unsigned long>{});
 if(!sound_views.empty()) {
  require(index<4 && sound_pcs.size()==67,"Sound input views exceed the authenticated hardware boundary");
  debug(index,0x21f590);enabled|=1ul<<(2*index);debug(7,enabled);
 }
 std::size_t sound_index=0,operand_reads=0;
 unsigned return_traps=0;unsigned long final_ret=0;
 const bool step_trace=std::getenv("HALO2_ORIGINAL_STEP_TRACE")!=nullptr;
 std::deque<unsigned long> tail;std::size_t instructions=0;
 for(unsigned n=0;n<(step_trace?5000000u:1024u);++n) {
  if(step_trace){tail.push_back(regs.rip);if(tail.size()>64)tail.pop_front();++instructions;}
  request(step_trace?PTRACE_SINGLESTEP:PTRACE_CONT,child);wait();request(PTRACE_GETREGS,child,nullptr,&regs);
  if(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&regs.rip==0x21f590&&!sound_views.empty()) {
   require(sound_index<sound_views.size(),"Unrecorded original sound timing input view");
   const auto &view=sound_views[sound_index++];
   const std::array<unsigned long,8> actual{regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi};
   for(unsigned k=0;k<8;++k)if(k!=5)
    require((actual[k]&0xfffffffful)==view.at("gp")[k].get<unsigned long>(),"Original sound timing entry GP differs");
   for(unsigned k=0;k<3;++k)
    require((static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(regs.rsp+4*k)))&0xfffffffful)==view.at("stack")[k].get<unsigned long>(),"Original sound timing caller stack differs");
   unsigned consumed=0;
   for(const auto pc:sound_pcs) {
    require(regs.rip==pc,"Original sound timing leaf instruction sequence differs");
    unsigned long address=0,prior=0,linked=0;bool projected=false;
    for(const auto &read:view.at("reads"))if(read.at("pc").get<unsigned long>()==pc) {
     require(!projected,"Ambiguous original sound operand read");
     const auto width=read.at("width").get<unsigned>();require(width==1||width==4,"Invalid sound input read width");
     const auto mask=width==1?0xfful:0xfffffffful;address=read.at("address").get<unsigned long>();
     prior=static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(address)));
     require((prior&mask)==read.at("before").get<unsigned long>(),"Canonical sound input changed before its read");
     linked=(prior&~mask)|read.at("value").get<unsigned long>();
     request(PTRACE_POKEDATA,child,reinterpret_cast<void *>(address),reinterpret_cast<void *>(linked));projected=true;
    }
    request(PTRACE_SINGLESTEP,child);wait();
    require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original sound timing instruction faulted");
    if(projected) {
     require(static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(address)))==linked,"Read-only sound operand was written by original code");
     request(PTRACE_POKEDATA,child,reinterpret_cast<void *>(address),reinterpret_cast<void *>(prior));++consumed;++operand_reads;
    }
    request(PTRACE_GETREGS,child,nullptr,&regs);
   }
   require(consumed==4&&regs.rip==view.at("return_address").get<unsigned long>(),"Original sound timing leaf did not consume all inputs and return");
   continue;
  }
  if(step_trace&&WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&!returns.contains(regs.rip))continue;
  if(!WIFSTOPPED(status) || WSTOPSIG(status)!=SIGTRAP || !returns.contains(regs.rip)) {
   J fault{{"status",status},{"entry_eflags",entry_flags},{"fault_instruction",regs.rip},
       {"gp",J::array({regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi})}};
   fault["instructions_tail"]=tail;fault["stepped_instructions"]=instructions;
   if(WIFSTOPPED(status)) {
    siginfo_t info{};request(PTRACE_GETSIGINFO,child,nullptr,&info);
    fault["signal"]=WSTOPSIG(status);fault["fault_address"]=reinterpret_cast<std::uintptr_t>(info.si_addr);
   }
   std::ofstream out(argv[6]);out<<fault.dump()<<'\n';out.close();
   std::ostringstream message;message<<"Original full-call execution fault at "<<std::hex<<regs.rip<<" signal "<<(WIFSTOPPED(status)?WSTOPSIG(status):0);throw std::runtime_error(message.str());
  }
  ++return_traps;const auto caller=static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(regs.rsp)))&0xfffffffful;
  if(caller==target) {final_ret=regs.rip;debug(7,0);request(PTRACE_SINGLESTEP,child);wait();request(PTRACE_GETREGS,child,nullptr,&regs);break;}
  // A nested invocation may return through the same original instruction.
  // The hardware resume flag suppresses its one-instruction re-trap.
  request(PTRACE_SINGLESTEP,child);wait();require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original nested return did not complete");
 }
 require(regs.rip==target && WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original hardware did not complete its outer RET");
 require(sound_index==sound_views.size(),"Original did not consume the recorded sound timing views");
 // Change only the next instruction to a harness-owned capture trampoline,
 // after the original RET completed. No original code byte is patched.
 regs.rip=capture;request(PTRACE_SETREGS,child,nullptr,&regs);request(PTRACE_CONT,child);wait();
 require(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Original hardware output failed");
 std::ofstream out(argv[6]);out<<J{{"original_instructions",nullptr},{"original_return",target},{"entry_eflags",entry_flags},{"return_breakpoints",returns},{"return_traps",return_traps},{"final_original_ret",final_ret},{"sound_timing_input_views",sound_index},{"sound_timing_operand_reads",operand_reads},{"observation","Hardware execute breakpoints on authenticated RET sites, then single-step original outer RET; no text patches or callee replacement"}}.dump()<<'\n';
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
