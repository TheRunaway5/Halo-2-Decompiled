// Offline test observer. Stops unchanged original i386 instructions before a
// declared dependency or after RET. Never patches code or supplies a result.
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
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void*a=nullptr,void*b=nullptr){errno=0;auto n=ptrace(op,pid,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child{pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int status{};waitpid(pid,&status,0);}}};
}
int main(int argc,char**argv){try{
  require(argc==6,"Supply ELF, stop boundary, original text end, region descriptor, output");
  const auto stop=std::stoul(argv[2],nullptr,0),text_end=std::stoul(argv[3],nullptr,0);
  Child child;child.pid=fork();require(child.pid>=0,"Cannot create original reference process");
  if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{require(waitpid(child.pid,&status,0)==child.pid,"Reference wait failed");};wait();
  require(WIFSTOPPED(status),"Missing reference exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
  request(PTRACE_CONT,child.pid);wait();require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Reference input setup failed");
  request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};std::uint32_t sites{};
  for(;sites<200000;++sites){
    require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original frame reference fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
    if(regs.rip==stop)break;
    require(regs.rip>=0x12000&&regs.rip<text_end,"Original frame escaped authenticated text");
    request(PTRACE_SINGLESTEP,child.pid);wait();
  }
  require(regs.rip==stop,"Original reference boundary not reached");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
  std::ofstream output(argv[5],std::ios::binary);require(bool(output),"Cannot open reference output");
  auto word=[&](std::uint32_t address){std::uint32_t value{};iovec local{&value,4},remote{reinterpret_cast<void*>(std::uintptr_t(address)),4};require(process_vm_readv(child.pid,&local,1,&remote,1,0)==4,"Cannot read original semantic argument");return value;};
  const auto step=stop==0x31b1a0?word(std::uint32_t(regs.rsp)+4):0;
  for(auto n:{std::uint32_t(fp.cwd),std::uint32_t(fp.swd),std::uint32_t(fp.mxcsr),sites,
      std::uint32_t(stop==0x31b1a0?regs.rcx:regs.rax),step?word(step):0,step?word(step+4):0})output.write(reinterpret_cast<const char*>(&n),4);
  std::ifstream regions(argv[4]);std::uint32_t address{},size{};
  while(regions>>address>>size){std::vector<char> data(size);iovec local{data.data(),size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};
    require(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Cannot read unchanged original reference bank");output.write(data.data(),data.size());}
  require(bool(output),"Original reference output incomplete");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
