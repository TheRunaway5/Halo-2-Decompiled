// Offline unchanged original 12A810 + 12A930 + 2287B0 + 253070 + 2531E0. No instruction patches,
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
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical listener reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical listener wait failed");};wait();
    check(WIFSTOPPED(status),"Listener exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Listener input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original listener input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original listener execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Listener escaped original text");sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x12a812:input.address=std::uint32_t(regs.rsp*1u+(12));input.size=4;break;
        case 0x12a816:input.address=std::uint32_t(regs.rbp*1u+(3));input.size=1;break;
        case 0x12a822:input.address=std::uint32_t((5199088));input.size=4;break;
        case 0x12a830:input.address=std::uint32_t(regs.rcx*1u+(0));input.size=4;break;
        case 0x12a850:input.address=std::uint32_t((4548956));input.size=4;break;
        case 0x12a85a:input.address=std::uint32_t((5188688));input.size=4;break;
        case 0x12a871:input.address=std::uint32_t(regs.rdi*1u+(0));input.size=1;break;
        case 0x12a898:input.address=std::uint32_t(regs.rsp*1u+(20));input.size=4;break;
        case 0x12a8a3:input.address=std::uint32_t(regs.rsp*1u+(24));input.size=4;break;
        case 0x12a8a7:input.address=std::uint32_t(regs.rsp*1u+(28));input.size=4;break;
        case 0x12a8ab:input.address=std::uint32_t(regs.rsp*1u+(0));input.size=4;break;
        case 0x12a8be:input.address=std::uint32_t(regs.rbp*1u+(0));input.size=2;break;
        case 0x12a8ce:input.address=std::uint32_t(regs.rsp*1u+(16));input.size=4;break;
        case 0x12a8ed:input.address=std::uint32_t((5199088));input.size=4;break;
        case 0x12a900:input.address=std::uint32_t(regs.rcx*1u+(0));input.size=4;break;
        case 0x12a912:input.address=std::uint32_t(regs.rsp*1u+(12));input.size=4;break;
        case 0x12a918:input.address=std::uint32_t(regs.rsp*1u+(20));input.size=4;break;
        case 0x12a931:input.address=std::uint32_t(regs.rcx*1u+(3));input.size=1;break;
        case 0x12a949:input.address=std::uint32_t(regs.rcx*1u+(20));input.size=4;break;
        case 0x12a94c:input.address=std::uint32_t(regs.rcx*1u+(16));input.size=4;break;
        case 0x12a94f:input.address=std::uint32_t(regs.rcx*1u+(12));input.size=4;break;
        case 0x12a96a:input.address=std::uint32_t(regs.rsp*1u+(0));input.size=4;break;
        case 0x12a96d:input.address=std::uint32_t(regs.rsp*1u+(0));input.size=4;break;
        case 0x12a974:input.address=std::uint32_t((5188688));input.size=4;break;
        case 0x12a97c:input.address=std::uint32_t(regs.rax*1u+regs.rdx*8u+(192));input.size=4;break;
        case 0x12a985:input.address=std::uint32_t(regs.rcx*1u+(20));input.size=4;break;
        case 0x12a991:input.address=std::uint32_t(regs.rcx*1u+(2));input.size=1;break;
        case 0x12a9a2:input.address=std::uint32_t(regs.rcx*1u+(48));input.size=4;break;
        case 0x12a9a8:input.address=std::uint32_t(regs.rcx*1u+(48));input.size=4;break;
        case 0x12a9bb:input.address=std::uint32_t(regs.rcx*1u+(16));input.size=4;break;
        case 0x12a9c0:input.address=std::uint32_t(regs.rdx*1u+(52));input.size=4;break;
        case 0x12a9c5:input.address=std::uint32_t(regs.rcx*1u+(12));input.size=4;break;
        case 0x12a9ca:input.address=std::uint32_t(regs.rdx*1u+(48));input.size=4;break;
        case 0x12a9e0:input.address=std::uint32_t((4548956));input.size=4;break;
        case 0x12a9ea:input.address=std::uint32_t(regs.rdx*1u+(52));input.size=4;break;
        case 0x12a9ef:input.address=std::uint32_t(regs.rcx*1u+(16));input.size=4;break;
        case 0x12a9f4:input.address=std::uint32_t(regs.rdx*1u+(48));input.size=4;break;
        case 0x12a9f9:input.address=std::uint32_t(regs.rcx*1u+(12));input.size=4;break;
        case 0x2287bb:input.address=std::uint32_t(regs.rdx*1u+(0));input.size=1;break;
        case 0x2287c0:input.address=std::uint32_t(regs.rdx*1u+(3));input.size=1;break;
        case 0x2287ce:input.address=std::uint32_t((5188688));input.size=4;break;
        case 0x2287db:input.address=std::uint32_t(regs.rdx*1u+(44));input.size=2;break;
        case 0x2287e6:input.address=std::uint32_t(regs.rcx*1u+(4));input.size=2;break;
        case 0x2287f0:input.address=std::uint32_t(regs.rcx*1u+(0));input.size=4;break;
        case 0x2287f2:input.address=std::uint32_t(regs.rdx*1u+(40));input.size=4;break;
        case 0x2287fc:input.address=std::uint32_t((5164056));input.size=4;break;
        case 0x22880d:input.address=std::uint32_t(regs.rsp*1u+(12));input.size=4;break;
        case 0x228813:input.address=std::uint32_t((4629100));input.size=4;break;
        case 0x253075:input.address=std::uint32_t((4628808));input.size=4;break;
        case 0x253087:input.address=std::uint32_t(regs.rdi*1u+(548));input.size=4;break;
        case 0x25309f:input.address=std::uint32_t(regs.rdi*1u+(552));input.size=4;break;
        case 0x2530ae:input.address=std::uint32_t(regs.rdi*1u+(156));input.size=4;break;
        case 0x2530ba:input.address=std::uint32_t(regs.rbx*1u+(40));input.size=4;break;
        case 0x2530c1:input.address=std::uint32_t(regs.rdx*1u+regs.rcx*1u+(-1));input.size=1;break;
        case 0x2530d4:input.address=std::uint32_t((4629100));input.size=4;break;
        case 0x2530e0:input.address=std::uint32_t(regs.rbx*1u+(4));input.size=4;break;
        case 0x2530e5:input.address=std::uint32_t(regs.rbx*1u+(8));input.size=4;break;
        case 0x2530f3:input.address=std::uint32_t((4629208));input.size=4;break;
        case 0x25311c:input.address=std::uint32_t((4629204));input.size=4;break;
        case 0x2531fc:input.address=std::uint32_t(regs.rdi*1u+(156));input.size=4;break;
        case 0x25320a:input.address=std::uint32_t(regs.rdi*1u+(232));input.size=4;break;
        case 0x253213:input.address=std::uint32_t(regs.rdx*1u+regs.rax*1u+(-1));input.size=1;break;
        case 0x12a920:write.address=std::uint32_t(regs.rax);writing=true;break;
        }
        const bool mutable_input=(input.address>=0x1000000u&&input.address<0x1010000u)||
            input.address==0x4f2c50u||input.address==0x4f54f0u||input.address==0x4ecc18u;
        if(!mutable_input)input.address=0;

        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original listener store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original listener did not return");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
    std::ofstream output(argv[5],std::ios::binary);for(auto n:{std::uint32_t(fp.cwd),std::uint32_t(fp.swd),std::uint32_t(fp.mxcsr),instructions})output.write(reinterpret_cast<const char*>(&n),4);
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
    check(bool(output),"Physical listener output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
