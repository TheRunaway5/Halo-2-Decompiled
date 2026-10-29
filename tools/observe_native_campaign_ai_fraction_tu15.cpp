// Offline only: execute unchanged i386 TU1.5 count/evaluator instructions and
// observe hardware FP status at RET or before a declared source read fault.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <iostream>
namespace {
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void *a=nullptr,void *b=nullptr){errno=0;const auto n=ptrace(op,pid,a,b);if(n==-1 && errno)throw std::runtime_error(strerror(errno));return n;}
struct Child {pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int status{};waitpid(pid,&status,0);}}};
}
int main(int argc,char **argv){try{
  check(argc==6,"Supply reference ELF, stop PC, stop occurrence, result address and output");
  const auto stop=std::stoul(argv[2],nullptr,0),occurrence=std::stoul(argv[3],nullptr,0),result_at=std::stoul(argv[4],nullptr,0);
  Child child;child.pid=fork();check(child.pid>=0,"Cannot fork reference");
  if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Reference wait failed");};
  wait();check(WIFSTOPPED(status),"Missing reference exec stop");
  request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
  request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Reference setup failed");
  request(PTRACE_SINGLESTEP,child.pid);wait();
  unsigned hits=0,steps=0;user_regs_struct regs{};
  for(;steps<200000;++steps){
    check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Physical original query faulted");
    request(PTRACE_GETREGS,child.pid,nullptr,&regs);
    if(regs.rip==stop && ++hits==occurrence)break;
    request(PTRACE_SINGLESTEP,child.pid);wait();
  }
  check(steps<200000,"Physical original query did not reach its declared boundary");
  user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
  const auto raw_count=std::uint32_t(regs.rax);
  std::uint32_t result=0;
  if(result_at){iovec local{&result,4},remote{reinterpret_cast<void*>(result_at),4};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==4,"Cannot read original HS result");}
  const std::uint32_t output[]{raw_count,result,fp.mxcsr,steps};
  std::ofstream file(argv[5],std::ios::binary);file.write(reinterpret_cast<const char*>(output),sizeof(output));
  check(bool(file),"Cannot write hardware query receipt");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
