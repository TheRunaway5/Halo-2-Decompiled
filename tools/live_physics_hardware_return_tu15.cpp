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
using J=nlohmann::json;
void require(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void *a=nullptr,void *b=nullptr){errno=0;auto r=ptrace(op,pid,a,b);if(r==-1&&errno)throw std::runtime_error(strerror(errno));return r;}
int main(int argc,char **argv)try{
 require(argc==8,"Supply ELF, output, return, capture trampoline, text end, trace and original RET sites");
 std::set<unsigned long> returns;std::istringstream sites(argv[7]);for(std::string value;std::getline(sites,value,',');)returns.insert(std::stoul(value,nullptr,0));
 require(!returns.empty() && returns.size()<=4,"Original entry requires one to four authenticated RET sites");
 const auto target=std::stoul(argv[3],nullptr,0),capture=std::stoul(argv[4],nullptr,0),text_end=std::stoul(argv[5],nullptr,0);
 auto child=fork();require(child>=0,"Cannot create original hardware process");
 if(!child){auto fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,0600);if(fd<0||dup2(fd,1)<0)_exit(125);close(fd);
   if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status{};auto wait=[&](){require(waitpid(child,&status,0)==child,"Hardware wait failed");};wait();
 require(WIFSTOPPED(status),"Missing hardware exec stop");request(PTRACE_SETOPTIONS,child,nullptr,reinterpret_cast<void *>(PTRACE_O_EXITKILL));
 request(PTRACE_CONT,child);wait();require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Hardware input setup failed");
 request(PTRACE_SINGLESTEP,child);wait();J trace=J::array();user_regs_struct regs{};
 request(PTRACE_GETREGS,child,nullptr,&regs);const auto entry_flags=regs.eflags;
 auto debug=[&](unsigned index,unsigned long value){request(PTRACE_POKEUSER,child,reinterpret_cast<void *>(offsetof(user,u_debugreg)+index*sizeof(unsigned long)),reinterpret_cast<void *>(value));};
 unsigned index=0;unsigned long enabled=0;for(const auto pc:returns){debug(index,pc);enabled|=1ul<<(2*index++);}debug(6,0);debug(7,enabled);
 unsigned return_traps=0;unsigned long final_ret=0;
 for(unsigned n=0;n<1024;++n) {
  request(PTRACE_CONT,child);wait();request(PTRACE_GETREGS,child,nullptr,&regs);
  if(!WIFSTOPPED(status) || WSTOPSIG(status)!=SIGTRAP || !returns.contains(regs.rip)) {
   std::ostringstream message;message<<"Original full-call execution fault at "<<std::hex<<regs.rip<<" signal "<<(WIFSTOPPED(status)?WSTOPSIG(status):0);throw std::runtime_error(message.str());
  }
  ++return_traps;const auto caller=static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(regs.rsp)))&0xfffffffful;
  if(caller==target) {final_ret=regs.rip;debug(7,0);request(PTRACE_SINGLESTEP,child);wait();request(PTRACE_GETREGS,child,nullptr,&regs);break;}
  // A nested invocation may return through the same original instruction.
  // The hardware resume flag suppresses its one-instruction re-trap.
  request(PTRACE_SINGLESTEP,child);wait();require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original nested return did not complete");
 }
 require(regs.rip==target && WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original hardware did not complete its outer RET");
 // Change only the next instruction to a harness-owned capture trampoline,
 // after the original RET completed. No original code byte is patched.
 regs.rip=capture;request(PTRACE_SETREGS,child,nullptr,&regs);request(PTRACE_CONT,child);wait();
 require(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Original hardware output failed");
 std::ofstream out(argv[6]);out<<J{{"original_instructions",nullptr},{"original_return",target},{"entry_eflags",entry_flags},{"return_breakpoints",returns},{"return_traps",return_traps},{"final_original_ret",final_ret},{"observation","Hardware execute breakpoints on authenticated RET sites, then single-step original outer RET; no text patches or callee replacement"}}.dump()<<'\n';
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
