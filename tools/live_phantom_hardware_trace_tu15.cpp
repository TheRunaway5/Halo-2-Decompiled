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
using J=nlohmann::json;
void require(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void *a=nullptr,void *b=nullptr){errno=0;auto r=ptrace(op,pid,a,b);if(r==-1&&errno)throw std::runtime_error(strerror(errno));return r;}
int main(int argc,char **argv)try{
 require(argc==7 || argc==8,"Supply ELF, output, return, capture trampoline, text end, trace and optional authenticated sites");
 std::set<unsigned long> authenticated;
 if(argc==8){std::istringstream input(argv[7]);std::string site;while(std::getline(input,site,','))authenticated.insert(std::stoul(site,nullptr,0));}
 const auto target=std::stoul(argv[3],nullptr,0),capture=std::stoul(argv[4],nullptr,0),text_end=std::stoul(argv[5],nullptr,0);
 auto child=fork();require(child>=0,"Cannot create original hardware process");
 if(!child){auto fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,0600);if(fd<0||dup2(fd,1)<0)_exit(125);close(fd);
   if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status{};auto wait=[&](){require(waitpid(child,&status,0)==child,"Hardware wait failed");};wait();
 require(WIFSTOPPED(status),"Missing hardware exec stop");request(PTRACE_SETOPTIONS,child,nullptr,reinterpret_cast<void *>(PTRACE_O_EXITKILL));
 request(PTRACE_CONT,child);wait();require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Hardware input setup failed");
 request(PTRACE_SINGLESTEP,child);wait();J trace=J::array();user_regs_struct regs{};
 request(PTRACE_GETREGS,child,nullptr,&regs);const auto entry_flags=regs.eflags;
 for(unsigned n=0;n<5000000;++n){
  if(!(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP)) {
    J fault{{"status",status},{"original_instructions",trace.size()},{"entry_eflags",entry_flags}};
    J tail=J::array();const auto begin=trace.size()>64?trace.size()-64:0;
    for(auto i=begin;i<trace.size();++i)tail.push_back(trace[i]);fault["instructions_tail"]=tail;
    if(WIFSTOPPED(status)) {
      request(PTRACE_GETREGS,child,nullptr,&regs);siginfo_t info{};
      request(PTRACE_GETSIGINFO,child,nullptr,&info);
      fault["signal"]=WSTOPSIG(status);fault["fault_instruction"]=regs.rip;
      fault["fault_address"]=reinterpret_cast<std::uintptr_t>(info.si_addr);
      fault["gp"]=J::array({regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi});
    }
    std::ofstream out(argv[6]);out<<fault.dump()<<'\n';
    throw std::runtime_error("Original hardware execution fault; bounded registers/signal journal retained");
  }
  request(PTRACE_GETREGS,child,nullptr,&regs);
  if(regs.rip==target)break;
  if(!(regs.rip>=0x12000 && regs.rip<text_end) && !authenticated.contains(regs.rip)) {
    std::ofstream out(argv[6]);out<<J{{"instructions",trace},{"original_instructions",trace.size()},
        {"escaped_address",regs.rip},{"esp",regs.rsp},{"entry_eflags",entry_flags}}.dump()<<'\n';
    throw std::runtime_error("Original execution escaped authenticated text at "+std::to_string(regs.rip));
  }
  trace.push_back(regs.rip);
  request(PTRACE_SINGLESTEP,child);wait();
 }
 require(regs.rip==target,"Original hardware did not return");
 // Change only the next instruction to a harness-owned capture trampoline,
 // after the original RET completed. No original code byte is patched.
 regs.rip=capture;request(PTRACE_SETREGS,child,nullptr,&regs);request(PTRACE_CONT,child);wait();
 require(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Original hardware output failed");
 std::ofstream out(argv[6]);out<<J{{"instructions",trace},{"original_instructions",trace.size()},{"original_return",target},{"entry_eflags",entry_flags}}.dump()<<'\n';
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
