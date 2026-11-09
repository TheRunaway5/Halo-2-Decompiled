// Independent physical i386 FST/FSTP write-fault authority for the private
// ownership utility. The child executes real opcodes; ptrace only observes.
#include <nlohmann/json.hpp>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using J=nlohmann::json;
namespace {
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct Child {pid_t pid{-1};~Child(){if(pid>0){kill(pid,SIGKILL);int status;waitpid(pid,&status,0);}}};
std::vector<unsigned char> bytes(const std::string&s){std::vector<unsigned char>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::stoul(s.substr(n,2),nullptr,16));return v;}
void word(std::array<unsigned char,128>&data,unsigned at,std::uint32_t value){std::memcpy(data.data()+at,&value,4);}
}
int main(int argc,char**argv)try{
 check(argc==3,"Supply faulting i386 ELF and fixture");std::ifstream in(argv[2]);const auto fixture=J::parse(in);unsigned cases{};
 for(const auto&row:fixture.at("cases")){
  const auto op=row.at("op").get<unsigned>();if(op!=5&&op!=6)continue;
  int pipefd[2];check(pipe(pipefd)==0,"Cannot create physical input pipe");Child child;child.pid=fork();check(child.pid>=0,"Cannot fork i386 fault authority");
  if(!child.pid){close(pipefd[1]);dup2(pipefd[0],0);close(pipefd[0]);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  close(pipefd[0]);std::array<unsigned char,128>input{};const auto raw=bytes(row.at("before").at("x87"));std::memcpy(input.data(),raw.data(),108);word(input,108,row.at("before").at("mxcsr"));word(input,112,op);word(input,116,row.at("operand"));check(write(pipefd[1],input.data(),input.size())==ssize_t(input.size()),"Cannot install physical input");close(pipefd[1]);
  int status{};const auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Cannot wait for i386 authority");};
  wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"No original exec stop");check(ptrace(PTRACE_SETOPTIONS,child.pid,nullptr,PTRACE_O_EXITKILL)==0,"Cannot own i386 child");
  check(ptrace(PTRACE_CONT,child.pid,nullptr,nullptr)==0,"Cannot start i386 store fixture");wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"No pre-store hardware boundary");
  user_fpregs_struct before{},after{};check(ptrace(PTRACE_GETFPREGS,child.pid,nullptr,&before)==0,"Cannot observe pre-store FP");
  check(before.cwd==std::uint16_t(raw[0]|(raw[1]<<8))&&before.swd==std::uint16_t(raw[4]|(raw[5]<<8))&&before.mxcsr==row.at("before").at("mxcsr"),"Actual fault input differs");
  check(ptrace(PTRACE_CONT,child.pid,nullptr,nullptr)==0,"Cannot execute actual store");wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGSEGV,"Actual unmapped store did not fault");
  check(ptrace(PTRACE_GETFPREGS,child.pid,nullptr,&after)==0,"Cannot observe faulted FP");
  check(before.cwd==after.cwd&&before.swd==after.swd&&before.ftw==after.ftw&&before.mxcsr==after.mxcsr,"Faulting native store committed controls/status/TOP/tags");
  for(unsigned n=0;n<row.at("count").get<unsigned>();++n)check(!std::memcmp(reinterpret_cast<const unsigned char*>(before.st_space)+16*n,reinterpret_cast<const unsigned char*>(after.st_space)+16*n,10),"Faulting native store committed logical80");
  ++cases;
 }
 std::cout<<J{{"passed",true},{"physical_i386_write_fault_cases",cases},{"instruction_patches",J::array()},{"supplied_outcomes",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
