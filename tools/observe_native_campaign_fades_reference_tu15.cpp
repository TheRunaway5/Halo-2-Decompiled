// Offline unchanged original 129900 + 12D320 + 12DB00 + 220000 + 220200. No instruction patches,
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
    check(argc==6,"Supply ELF, stop, text end, regions and output");const auto stop=std::stoul(argv[2],nullptr,0),end=std::stoul(argv[3],nullptr,0);
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical fade reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical fade wait failed");};wait();
    check(WIFSTOPPED(status),"Fade exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Fade input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original fade input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original fade execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Fade escaped original text");sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x129904:case 0x12d327:input.address=0x4f2c4cu;break;
        case 0x12990d:case 0x129974:input.address=std::uint32_t(regs.rbp)+0x44u;break;
        case 0x129920:input.address=std::uint32_t(regs.rsi)+4u;input.size=1;break;
        case 0x12d339:input.address=std::uint32_t(regs.rax)+0x44u;break;
        case 0x12d33c:input.address=std::uint32_t(regs.rsi+regs.rdi)+4u;input.size=1;break;
        case 0x12d350:case 0x12d38f:input.address=std::uint32_t(regs.rsi)+0xb4u;break;
        case 0x12d359:input.address=0x4f2c50u;break;
        case 0x12d361:case 0x12d380:case 0x12d39c:input.address=std::uint32_t(regs.rsi)+0xb8u;break;
        case 0x12d36a:case 0x12d37a:case 0x12d395:input.address=std::uint32_t(regs.rdx)+0x84u;break;
        case 0x12d3f2:case 0x12d404:if(regs.rax>=0x1000000u)input.address=std::uint32_t(regs.rax);break;
        case 0x12d406:input.address=std::uint32_t(regs.rsi)+0xaeu;input.size=2;break;
        case 0x12d374:write.address=std::uint32_t(regs.rsi)+0xb4u;writing=true;break;
        case 0x12d389:write.address=std::uint32_t(regs.rsi)+0xb8u;writing=true;break;
        case 0x129930:case 0x129938:case 0x12999c:write.address=std::uint32_t(regs.rsi)+0xb0u;writing=true;break;
        case 0x129954:case 0x1299a6:write.address=std::uint32_t(regs.rsi)+0xaeu;write.size=2;writing=true;break;
        case 0x12995b:case 0x1299ad:write.address=std::uint32_t(regs.rsi)+0xb4u;writing=true;break;
        case 0x129965:case 0x1299b3:write.address=std::uint32_t(regs.rsi)+0xb8u;writing=true;break;
        case 0x12996b:case 0x1299bd:
            input.address=write.address=std::uint32_t(regs.rsi)+4u;input.size=write.size=1;writing=true;break;
        }
        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original fade store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original fade did not return");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
    std::ofstream output(argv[5],std::ios::binary);for(auto n:{std::uint32_t(fp.cwd),std::uint32_t(fp.swd),std::uint32_t(fp.mxcsr),instructions})output.write(reinterpret_cast<const char*>(&n),4);
    for(auto n:{regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi}) {
        auto value=std::uint32_t(n);output.write(reinterpret_cast<const char*>(&value),4);
    }
    output.write(reinterpret_cast<const char*>(fp.xmm_space),16);
    std::ifstream regions(argv[4]);std::uint32_t address{},size{};
    while(regions>>address>>size){std::vector<char> bytes(size);read(address,bytes.data(),size);output.write(bytes.data(),bytes.size());}
    auto count=std::uint32_t(reads.size());output.write(reinterpret_cast<const char*>(&count),4);for(const auto& event:reads)output.write(reinterpret_cast<const char*>(&event),sizeof(event));
    count=std::uint32_t(stores.size());output.write(reinterpret_cast<const char*>(&count),4);for(const auto& event:stores)output.write(reinterpret_cast<const char*>(&event),sizeof(event));
    count=std::uint32_t(sites.size());output.write(reinterpret_cast<const char*>(&count),4);for(auto site:sites)output.write(reinterpret_cast<const char*>(&site),4);
    check(bool(output),"Physical fade output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
