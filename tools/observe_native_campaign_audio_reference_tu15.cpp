// Offline unchanged original i386 observer. No instruction patches or supplied
// dependency returns: both admitted leaves must reach their genuine RET.
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
#include <set>
#include <stdexcept>
#include <vector>
namespace {
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
long request(__ptrace_request operation,pid_t pid,void* a=nullptr,void* b=nullptr) {
    errno=0;const auto result=ptrace(operation,pid,a,b);if(result==-1&&errno)throw std::runtime_error(strerror(errno));return result;
}
struct Child {pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int status{};waitpid(pid,&status,0);}}};
struct Store {std::uint32_t site,address;char before[4],after[4];};
}
int main(int argc,char** argv)try {
    require(argc==6,"Supply ELF, stop, text end, region descriptor and output");
    const auto stop=std::stoul(argv[2],nullptr,0),text_end=std::stoul(argv[3],nullptr,0);
    Child child;child.pid=fork();require(child.pid>=0,"Cannot create original audio reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{require(waitpid(child.pid,&status,0)==child.pid,"Original audio wait failed");};wait();
    require(WIFSTOPPED(status),"Original audio exec stop absent");
    request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original audio setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};std::uint32_t instructions{};std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t address,char* output,std::size_t size) {
        iovec local{output,size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};
        require(process_vm_readv(child.pid,&local,1,&remote,1,0)==ssize_t(size),"Cannot read original audio memory");
    };
    for(;instructions<200000;++instructions) {
        require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original audio execution fault");
        request(PTRACE_GETREGS,child.pid,nullptr,&regs);if(regs.rip==stop)break;
        require(regs.rip>=0x12000u&&regs.rip<text_end,"Original audio escaped authenticated text");
        sites.insert(std::uint32_t(regs.rip));
        Store store{};store.site=std::uint32_t(regs.rip);bool persistent=true;
        switch(regs.rip) {
        case 0x22836c:case 0x228377:store.address=std::uint32_t(regs.rax)-4u;break;
        case 0x228371:case 0x22837a:store.address=std::uint32_t(regs.rax);break;
        case 0x225d87:case 0x225da1:store.address=std::uint32_t(regs.rdi);persistent=std::uint32_t(regs.rcx)!=0;break;
        case 0x225dda:case 0x225e06:store.address=std::uint32_t(regs.rax);break;
        case 0x225e26:store.address=std::uint32_t(regs.rcx)-12u;break;
        case 0x225e2c:store.address=std::uint32_t(regs.rcx);break;
        default:persistent=false;
        }
        if(persistent)read(store.address,store.before,4);
        request(PTRACE_SINGLESTEP,child.pid);wait();
        if(persistent){require(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original audio store fault");read(store.address,store.after,4);stores.push_back(store);}
    }
    require(regs.rip==stop,"Original audio return boundary absent");user_fpregs_struct fp{};
    request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);std::ofstream output(argv[5],std::ios::binary);
    for(auto value:{std::uint32_t(fp.cwd),std::uint32_t(fp.swd),std::uint32_t(fp.mxcsr),instructions})output.write(reinterpret_cast<const char*>(&value),4);
    std::ifstream regions(argv[4]);std::uint32_t address{},size{};
    while(regions>>address>>size){std::vector<char> data(size);read(address,data.data(),size);output.write(data.data(),data.size());}
    auto count=std::uint32_t(stores.size());output.write(reinterpret_cast<const char*>(&count),4);
    for(const auto& store:stores)output.write(reinterpret_cast<const char*>(&store),sizeof(store));
    count=std::uint32_t(sites.size());output.write(reinterpret_cast<const char*>(&count),4);
    for(const auto site:sites)output.write(reinterpret_cast<const char*>(&site),4);
    require(bool(output),"Original audio output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
