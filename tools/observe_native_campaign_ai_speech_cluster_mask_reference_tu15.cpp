// Offline unchanged original 203C00/252F30/253070/2531E0 actual BSP speech cluster mask. No instruction patches,
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
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical speech cluster mask reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical speech cluster mask wait failed");};wait();
    check(WIFSTOPPED(status),"Speech choice exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Speech choice input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original speech cluster mask input read failed");};
    for(;instructions<1000000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original speech cluster mask execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Speech choice escaped original text");
        if(capture_entry&&!captured&&regs.rip==capture_entry) {
            user_fpregs_struct state{};request(PTRACE_GETFPREGS,child.pid,nullptr,&state);
            std::ofstream capture(std::string(argv[5])+".capture",std::ios::binary);prefix_steps=instructions;
            for(auto n:{std::uint32_t(state.cwd),std::uint32_t(state.swd),std::uint32_t(state.mxcsr),prefix_steps})capture.write(reinterpret_cast<const char*>(&n),4);
            for(auto n:{regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi}){auto value=std::uint32_t(n);capture.write(reinterpret_cast<const char*>(&value),4);}
            capture.write(reinterpret_cast<const char*>(state.xmm_space),128);capture.write(reinterpret_cast<const char*>(state.st_space),128);
            std::ifstream regions(argv[4]);std::uint32_t at{},bytes{};
            while(regions>>at>>bytes){std::vector<char> values(bytes);read(at,values.data(),bytes);capture.write(values.data(),values.size());}
            std::uint32_t actual_arguments[1]{};read(std::uint32_t(regs.rsp)+4u,reinterpret_cast<char*>(actual_arguments),4);capture.write(reinterpret_cast<const char*>(actual_arguments),4);
            auto count=std::uint32_t(sites.size());capture.write(reinterpret_cast<const char*>(&count),4);for(auto site:sites)capture.write(reinterpret_cast<const char*>(&site),4);
            auto stored=std::uint32_t(stores.size());capture.write(reinterpret_cast<const char*>(&stored),4);for(const auto &event:stores)capture.write(reinterpret_cast<const char*>(&event),sizeof(event));
            check(bool(capture),"Actual cancellation caller capture incomplete");captured=true;sites.clear();reads.clear();stores.clear();
        }
        sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x203e8f:input.address=std::uint32_t(regs.rax)+4u;input.size=2;break;
        case 0x203c05:input.address=std::uint32_t(5251648u);input.size=2;break;
        case 0x203c13:input.address=std::uint32_t(5164056u);input.size=4;break;
        case 0x203c19:input.address=std::uint32_t(regs.rbx*1u+156u);input.size=4;break;
        case 0x203c37:write.address=std::uint32_t(regs.rdi*1u+0u);write.size=4;writing=true;break;
        case 0x203c3e:write.address=std::uint32_t(regs.rdi*1u+0u);write.size=1;writing=true;break;
        case 0x203c40:input.address=std::uint32_t(regs.rbx*1u+156u);input.size=4;break;
        case 0x203c4c:input.address=std::uint32_t(4628904u);input.size=4;break;
        case 0x203c90:input.address=std::uint32_t(regs.rax*4u+5251584u);input.size=4;break;
        case 0x203ca0:write.address=std::uint32_t(regs.rax*1u+0u);write.size=4;writing=true;break;
        case 0x203ca2:input.address=std::uint32_t(regs.rbx*1u+156u);input.size=4;break;
        case 0x203cb8:write.address=std::uint32_t(5251648u);write.size=2;writing=true;break;
        case 0x252f39:input.address=std::uint32_t(regs.rdi*1u+548u);input.size=4;break;
        case 0x252f43:input.address=std::uint32_t(regs.rdi*1u+552u);input.size=4;break;
        case 0x252f4c:input.address=std::uint32_t(regs.rdx*1u+32u);input.size=4;break;
        case 0x252f63:input.address=std::uint32_t(regs.rdi*1u+156u);input.size=4;break;
        case 0x252f84:input.address=std::uint32_t(regs.rbx*1u+regs.rax*4u+0u);input.size=4;break;
        case 0x253075:input.address=std::uint32_t(4628808u);input.size=4;break;
        case 0x253087:input.address=std::uint32_t(regs.rdi*1u+548u);input.size=4;break;
        case 0x25309f:input.address=std::uint32_t(regs.rdi*1u+552u);input.size=4;break;
        case 0x2530ae:input.address=std::uint32_t(regs.rdi*1u+156u);input.size=4;break;
        case 0x2530ba:input.address=std::uint32_t(regs.rbx*1u+40u);input.size=4;break;
        case 0x2530c1:input.address=std::uint32_t(regs.rdx*1u+regs.rcx*1u+-1u);input.size=1;break;
        case 0x2530d4:input.address=std::uint32_t(4629100u);input.size=4;break;
        case 0x2530e0:input.address=std::uint32_t(regs.rbx*1u+4u);input.size=4;break;
        case 0x2530e5:input.address=std::uint32_t(regs.rbx*1u+8u);input.size=4;break;
        case 0x2530f3:input.address=std::uint32_t(4629208u);input.size=4;break;
        case 0x25311c:input.address=std::uint32_t(4629204u);input.size=4;break;
        case 0x2531fc:input.address=std::uint32_t(regs.rdi*1u+156u);input.size=4;break;
        case 0x25320a:input.address=std::uint32_t(regs.rdi*1u+232u);input.size=4;break;
        case 0x253213:input.address=std::uint32_t(regs.rdx*1u+regs.rax*1u+-1u);input.size=1;break;
        }

        if(regs.rip>=0x253070u&&regs.rip<0x253126u&&input.address>=0x46a000u&&input.address<0x46b000u)input.address=0;
        if((regs.rip==0x203c37u||regs.rip==0x203c3eu)&&regs.rcx==0)writing=false;
        if(input.address>=0x2000000u&&input.address<0x2010000u)input.address=0;
        if(writing&&write.address>=0x2000000u&&write.address<0x2010000u)writing=false;
        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original speech cluster mask store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original speech cluster mask did not return");check(!capture_entry||captured,"Actual speech publication block did not reach queue admission");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
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
    check(bool(output),"Physical speech cluster mask output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
