// Offline unchanged original 27BB60 + 27BC00 + 27BCE0. No instruction patches,
// host time substitution, supplied return values or resumed unknown owners.
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
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void* a=nullptr,void* b=nullptr){errno=0;auto n=ptrace(op,pid,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child{pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int status{};waitpid(pid,&status,0);}}};
struct Read{std::uint32_t address,size,stores_before;char value[4];};
struct Store{std::uint32_t site,address,size;char before[4],after[4];};
}
int main(int argc,char** argv)try {
    check(argc==6||argc==7,"Supply ELF, stop, text end, regions, output and optional actual callee capture");const auto capture_entry=argc==7?std::stoul(argv[6],nullptr,0):0u;unsigned prefix_steps{};bool captured=false;const auto stop=std::stoul(argv[2],nullptr,0),end=std::stoul(argv[3],nullptr,0);
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical path heap reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical path heap wait failed");};wait();
    check(WIFSTOPPED(status),"Path heap exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Path heap input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original path heap input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original path heap execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Path heap escaped original text");
        if(capture_entry&&!captured&&regs.rip==capture_entry) {
            user_fpregs_struct state{};request(PTRACE_GETFPREGS,child.pid,nullptr,&state);
            std::ofstream capture(std::string(argv[5])+".capture",std::ios::binary);prefix_steps=instructions;
            for(auto n:{std::uint32_t(state.cwd),std::uint32_t(state.swd),std::uint32_t(state.mxcsr),prefix_steps})capture.write(reinterpret_cast<const char*>(&n),4);
            for(auto n:{regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi}){auto value=std::uint32_t(n);capture.write(reinterpret_cast<const char*>(&value),4);}
            capture.write(reinterpret_cast<const char*>(state.xmm_space),128);capture.write(reinterpret_cast<const char*>(state.st_space),128);
            std::ifstream regions(argv[4]);std::uint32_t at{},bytes{};
            while(regions>>at>>bytes){std::vector<char> values(bytes);read(at,values.data(),bytes);capture.write(values.data(),values.size());}
            auto count=std::uint32_t(sites.size());capture.write(reinterpret_cast<const char*>(&count),4);for(auto site:sites)capture.write(reinterpret_cast<const char*>(&site),4);
            check(bool(capture),"Actual path-search caller capture incomplete");captured=true;sites.clear();reads.clear();stores.clear();
        }
        sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x27bb65:input.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b4u);input.size=2;break;
        case 0x27bb74:input.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b2u);input.size=2;break;
        case 0x27bb8d:input.address=std::uint32_t(regs.rax+regs.rsi*4u+0x110b4u);input.size=2;break;
        case 0x27bb98:input.address=std::uint32_t(regs.rax+regs.rsi*4u+0x110b2u);input.size=2;break;
        case 0x27bc09:input.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b2u);input.size=2;break;
        case 0x27bc11:input.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b4u);input.size=2;break;
        case 0x27bc47:input.address=std::uint32_t(regs.rax+0x110b0u);input.size=2;break;
        case 0x27bc55:input.address=std::uint32_t(regs.rax+regs.rsi*4u+0x110b4u);input.size=2;break;
        case 0x27bc62:input.address=std::uint32_t(regs.rax+regs.rsi*4u+0x110b2u);input.size=2;break;
        case 0x27bce2:input.address=std::uint32_t(regs.rax+0x110b0u);input.size=2;break;
        case 0x27bba5:write.address=std::uint32_t(regs.rax+regs.rsi*4u+0x110b2u);writing=true;break;
        case 0x27bbad:write.address=std::uint32_t(regs.rax+regs.rsi*4u+0x110b4u);writing=true;break;
        case 0x27bbbf:write.address=std::uint32_t(regs.rax+regs.rsi+0xf0u);writing=true;break;
        case 0x27bbd4:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b2u);writing=true;break;
        case 0x27bbdc:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b4u);writing=true;break;
        case 0x27bbeb:write.address=std::uint32_t(regs.rax+regs.rcx+0xf0u);writing=true;break;
        case 0x27bc8c:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b4u);writing=true;break;
        case 0x27bc9a:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b2u);writing=true;break;
        case 0x27bca2:write.address=std::uint32_t(regs.rax+regs.rdx+0xf0u);writing=true;break;
        case 0x27bcb7:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b4u);writing=true;break;
        case 0x27bcbf:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b2u);writing=true;break;
        case 0x27bcd0:write.address=std::uint32_t(regs.rax+regs.rcx+0xf0u);writing=true;break;
        case 0x27bcf3:write.address=std::uint32_t(regs.rax+0x110b0u);writing=true;break;
        case 0x27bd03:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b2u);writing=true;break;
        case 0x27bd10:write.address=std::uint32_t(regs.rax+regs.rcx*4u+0x110b4u);writing=true;break;
        }
        write.size=2;

        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original path heap store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original path heap did not return");check(!capture_entry||captured,"Actual path-search caller did not reach required heap owner");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
    std::ofstream output(argv[5],std::ios::binary);for(auto n:{std::uint32_t(fp.cwd),std::uint32_t(fp.swd),std::uint32_t(fp.mxcsr),instructions-prefix_steps})output.write(reinterpret_cast<const char*>(&n),4);
    for(auto n:{regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi}) {
        auto value=std::uint32_t(n);output.write(reinterpret_cast<const char*>(&value),4);
    }
    output.write(reinterpret_cast<const char*>(fp.xmm_space),128);
    output.write(reinterpret_cast<const char*>(fp.st_space),128);
    std::ifstream regions(argv[4]);std::uint32_t address{},size{};
    while(regions>>address>>size){std::vector<char> bytes(size);read(address,bytes.data(),size);output.write(bytes.data(),bytes.size());}
    auto count=std::uint32_t(reads.size());output.write(reinterpret_cast<const char*>(&count),4);for(const auto& event:reads)output.write(reinterpret_cast<const char*>(&event),sizeof(event));
    count=std::uint32_t(stores.size());output.write(reinterpret_cast<const char*>(&count),4);for(const auto& event:stores)output.write(reinterpret_cast<const char*>(&event),sizeof(event));
    count=std::uint32_t(sites.size());output.write(reinterpret_cast<const char*>(&count),4);for(auto site:sites)output.write(reinterpret_cast<const char*>(&site),4);
    check(bool(output),"Physical path heap output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
