// Offline unchanged i386 observer for actor/order-script assignment ownership.
#include <sys/ptrace.h>
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
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void *a=nullptr,void *b=nullptr){errno=0;const auto n=ptrace(op,pid,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child{pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int s{};waitpid(pid,&s,0);}}};
}
int main(int argc,char **argv){try{
 check(argc==7,"Supply ELF,stopPC,occurrence,bank,size,output");
 const auto stop=std::stoul(argv[2],nullptr,0),occurrence=std::stoul(argv[3],nullptr,0),address=std::stoul(argv[4],nullptr,0),size=std::stoul(argv[5],nullptr,0);
 Child child;child.pid=fork();check(child.pid>=0,"Cannot fork physical reference");
 if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Reference wait failed");};
 wait();check(WIFSTOPPED(status),"Missing reference exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
 request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Reference setup failed");
 request(PTRACE_SINGLESTEP,child.pid);wait();unsigned hits=0,steps=0;user_regs_struct regs{};
 for(;steps<10000;++steps){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Physical original assignment faulted");request(PTRACE_GETREGS,child.pid,nullptr,&regs);if(regs.rip==stop&&++hits==occurrence)break;request(PTRACE_SINGLESTEP,child.pid);wait();}
 check(steps<10000,"Original assignment boundary was not reached");
 user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
 std::vector<char> data(size);iovec local{data.data(),size},remote{reinterpret_cast<void*>(address),size};
 check(process_vm_readv(child.pid,&local,1,&remote,1,0)==ssize_t(size),"Cannot inspect physical source bank");
 std::ofstream output(argv[6],std::ios::binary);const std::uint32_t header[]{fp.mxcsr,steps};output.write(reinterpret_cast<const char*>(header),sizeof(header));output.write(data.data(),data.size());check(bool(output),"Cannot write physical reference receipt");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
