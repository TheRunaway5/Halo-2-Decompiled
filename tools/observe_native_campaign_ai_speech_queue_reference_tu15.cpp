// Offline unchanged original 217970/217870/218070/2180F0 plus actual16E8A0 queue admission and retirement. No instruction patches,
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
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical speech queue reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical speech queue wait failed");};wait();
    check(WIFSTOPPED(status),"Speech choice exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Speech choice input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original speech queue input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original speech queue execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Speech choice escaped original text");
        if(capture_entry&&!captured&&regs.rip==capture_entry) {
            user_fpregs_struct state{};request(PTRACE_GETFPREGS,child.pid,nullptr,&state);
            std::ofstream capture(std::string(argv[5])+".capture",std::ios::binary);prefix_steps=instructions;
            for(auto n:{std::uint32_t(state.cwd),std::uint32_t(state.swd),std::uint32_t(state.mxcsr),prefix_steps})capture.write(reinterpret_cast<const char*>(&n),4);
            for(auto n:{regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi}){auto value=std::uint32_t(n);capture.write(reinterpret_cast<const char*>(&value),4);}
            capture.write(reinterpret_cast<const char*>(state.xmm_space),128);capture.write(reinterpret_cast<const char*>(state.st_space),128);
            std::ifstream regions(argv[4]);std::uint32_t at{},bytes{};
            while(regions>>at>>bytes){std::vector<char> values(bytes);read(at,values.data(),bytes);capture.write(values.data(),values.size());}
            std::uint32_t actual_arguments[2]{};read(std::uint32_t(regs.rsp)+4u,reinterpret_cast<char*>(actual_arguments),8);capture.write(reinterpret_cast<const char*>(actual_arguments),8);
            auto count=std::uint32_t(sites.size());capture.write(reinterpret_cast<const char*>(&count),4);for(auto site:sites)capture.write(reinterpret_cast<const char*>(&site),4);
            auto stored=std::uint32_t(stores.size());capture.write(reinterpret_cast<const char*>(&stored),4);for(const auto &event:stores)capture.write(reinterpret_cast<const char*>(&event),sizeof(event));
            check(bool(capture),"Actual cancellation caller capture incomplete");captured=true;sites.clear();reads.clear();stores.clear();
        }
        sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x217814:input.address=std::uint32_t(regs.rcx)+0x138u;input.size=2;break;
        case 0x217820:input.address=0x505e74u;break;
        case 0x217973:input.address=std::uint32_t(5267056u);input.size=4;break;
        case 0x21798b:input.address=std::uint32_t(regs.rax*1u+68u);input.size=4;break;
        case 0x21798e:input.address=std::uint32_t(regs.rcx*1u+2008u);input.size=4;break;
        case 0x2179a0:input.address=std::uint32_t(regs.rdi*1u+4u);input.size=4;break;
        case 0x2179a5:input.address=std::uint32_t(regs.rdi*1u+40u);input.size=4;break;
        case 0x2179af:input.address=std::uint32_t(regs.rcx*1u+2000u);input.size=4;break;
        case 0x2179d0:input.address=std::uint32_t(5267056u);input.size=4;break;
        case 0x2179d6:input.address=std::uint32_t(regs.rcx*1u+68u);input.size=4;break;
        case 0x2179eb:input.address=std::uint32_t(regs.rdi*1u+20u);input.size=4;break;
        case 0x2179ee:input.address=std::uint32_t(regs.rsi*1u+20u);input.size=4;break;
        case 0x2179f7:input.address=std::uint32_t(regs.rsi*1u+80u);input.size=4;break;
        case 0x217a0d:input.address=std::uint32_t(regs.rdi*1u+16u);input.size=4;break;
        case 0x217a10:input.address=std::uint32_t(regs.rdi*1u+20u);input.size=4;break;
        case 0x217a15:input.address=std::uint32_t(regs.rdi*1u+30u);input.size=2;break;
        case 0x217a21:input.address=std::uint32_t(regs.rdi*1u+14u);input.size=1;break;
        case 0x217a28:input.address=std::uint32_t(regs.rdi*1u+68u);input.size=4;break;
        case 0x217a3d:input.address=std::uint32_t(5267056u);input.size=4;break;
        case 0x217a43:input.address=std::uint32_t(regs.rdx*1u+68u);input.size=4;break;
        case 0x217a56:write.address=std::uint32_t(regs.rax*1u+80u);write.size=4;writing=true;break;
        case 0x217a5f:write.address=std::uint32_t(regs.rcx*1u+2000u);write.size=4;writing=true;break;
        case 0x217a69:write.address=std::uint32_t(regs.rdi*1u+80u);write.size=4;writing=true;break;
        case 0x217a6c:input.address=std::uint32_t(regs.rcx*1u+2004u);input.size=2;write.address=input.address;write.size=2;writing=true;break;
        case 0x217a7c:input.address=std::uint32_t(5267056u);input.size=4;break;
        case 0x217a82:input.address=std::uint32_t(regs.rcx*1u+68u);input.size=4;break;
        case 0x217a85:input.address=std::uint32_t(4628844u);input.size=4;break;
        case 0x217a8f:input.address=std::uint32_t(5365588u);input.size=4;break;
        case 0x217aa4:input.address=std::uint32_t(5267056u);input.size=4;break;
        case 0x217aaa:input.address=std::uint32_t(regs.rdx*1u+68u);input.size=4;break;
        case 0x217ab8:input.address=std::uint32_t(regs.rbx*1u+2u);input.size=2;break;
        case 0x217ad2:input.address=std::uint32_t(regs.rsi*1u+20u);input.size=4;break;
        case 0x217ad9:input.address=std::uint32_t(regs.rdi*1u+20u);input.size=4;break;
        case 0x217ae2:input.address=std::uint32_t(regs.rbx*1u+2u);input.size=2;break;
        case 0x217afc:input.address=std::uint32_t(regs.rsi*1u+20u);input.size=4;break;
        case 0x217b03:write.address=std::uint32_t(regs.rdi*1u+20u);write.size=4;writing=true;break;
        case 0x217b0b:input.address=std::uint32_t(regs.rdi*1u+72u);input.size=1;break;
        case 0x217b12:input.address=std::uint32_t(regs.rdi*1u+20u);input.size=4;break;
        case 0x217b15:input.address=std::uint32_t(regs.rdi*1u+16u);input.size=4;break;
        case 0x217b18:input.address=std::uint32_t(regs.rdi*1u+30u);input.size=2;break;
        case 0x217b22:input.address=std::uint32_t(regs.rdi*1u+14u);input.size=1;break;
        case 0x217b29:input.address=std::uint32_t(regs.rdi*1u+68u);input.size=4;break;
        case 0x217b4c:input.address=std::uint32_t(regs.rdi*1u+80u);input.size=4;break;
        case 0x217870:input.address=std::uint32_t(5164060u);input.size=4;break;
        case 0x21787f:input.address=std::uint32_t(regs.rax*1u+200u);input.size=4;break;
        case 0x217889:input.address=std::uint32_t(regs.rax*1u+204u);input.size=4;break;
        case 0x21788f:input.address=std::uint32_t(regs.rax*1u+100u);input.size=4;break;
        case 0x217897:input.address=std::uint32_t(5178388u);input.size=4;break;
        case 0x2178a5:input.address=std::uint32_t(regs.rax*1u+regs.rcx*1u+8u);input.size=4;break;
        case 0x2178a9:input.address=std::uint32_t(regs.rdi*1u+20u);input.size=4;break;
        case 0x2178ac:input.address=std::uint32_t(regs.rsi*1u+20u);input.size=4;break;
        case 0x2178b7:input.address=std::uint32_t(regs.rsi*1u+28u);input.size=2;break;
        case 0x2178c7:input.address=std::uint32_t(regs.rbx*1u+4u);input.size=4;break;
        case 0x2178d2:input.address=std::uint32_t(regs.rdi*1u+2u);input.size=2;break;
        case 0x2178e1:input.address=std::uint32_t(regs.rsi*1u+64u);input.size=1;break;
        case 0x2178e8:input.address=std::uint32_t(regs.rsi*1u+4u);input.size=4;break;
        case 0x2178eb:input.address=std::uint32_t(regs.rdi*1u+4u);input.size=4;break;
        case 0x2178f0:input.address=std::uint32_t(regs.rdi*1u+34u);input.size=2;break;
        case 0x2178f7:input.address=std::uint32_t(regs.rsi*1u+34u);input.size=2;break;
        case 0x2178fb:input.address=std::uint32_t(regs.rax*1u+26u);input.size=2;break;
        case 0x217905:input.address=std::uint32_t(regs.rdi*1u+34u);input.size=2;break;
        case 0x217913:input.address=std::uint32_t(regs.rdi*1u+34u);input.size=2;break;
        case 0x21791d:input.address=std::uint32_t(regs.rdi*1u+68u);input.size=4;break;
        case 0x217923:input.address=std::uint32_t(regs.rsi*1u+68u);input.size=4;break;
        case 0x217929:input.address=std::uint32_t(5365588u);input.size=4;break;
        case 0x21792e:input.address=std::uint32_t(regs.rax*1u+2u);input.size=2;break;
        case 0x217936:input.address=std::uint32_t(4628844u);input.size=4;break;
        case 0x21794c:input.address=std::uint32_t(regs.rsi*1u+28u);input.size=2;break;
        case 0x217950:input.address=std::uint32_t(regs.rsi*1u+20u);input.size=4;break;
        case 0x21795b:write.address=std::uint32_t(regs.rdi*1u+20u);write.size=4;writing=true;break;
        case 0x218079:input.address=std::uint32_t(5267056u);input.size=4;break;
        case 0x21807f:input.address=std::uint32_t(regs.rbp*1u+68u);input.size=4;break;
        case 0x218084:input.address=std::uint32_t(regs.rcx*1u+80u);input.size=4;break;
        case 0x21808a:write.address=std::uint32_t(regs.rcx*1u+72u);write.size=1;writing=true;break;
        case 0x218092:input.address=std::uint32_t(regs.rbp*1u+68u);input.size=4;break;
        case 0x21809d:input.address=std::uint32_t(regs.rax*1u+regs.rdi*1u+68u);input.size=4;break;
        case 0x2180a8:input.address=std::uint32_t(regs.rbp*1u+56u);input.size=4;break;
        case 0x2180b7:input.address=std::uint32_t(regs.rbp*1u+36u);input.size=4;break;
        case 0x2180bd:input.address=std::uint32_t(regs.rcx*1u+regs.rdi*1u+0u);input.size=2;break;
        case 0x2180d2:input.address=std::uint32_t(regs.rcx*1u+72u);input.size=1;break;
        case 0x2180d9:write.address=std::uint32_t(regs.rax*1u+72u);write.size=1;writing=true;break;
        case 0x2180dd:input.address=std::uint32_t(regs.rax*1u+80u);input.size=4;break;
        case 0x2180f5:input.address=std::uint32_t(regs.rbp*1u+2000u);input.size=4;break;
        case 0x21810b:input.address=std::uint32_t(5267056u);input.size=4;break;
        case 0x218113:input.address=std::uint32_t(regs.rdi*1u+0u);input.size=4;break;
        case 0x218115:input.address=std::uint32_t(regs.rdx*1u+68u);input.size=4;break;
        case 0x218123:input.address=std::uint32_t(regs.rcx*1u+regs.rsi*1u+72u);input.size=1;break;
        case 0x21812d:input.address=std::uint32_t(regs.rcx*1u+80u);input.size=4;break;
        case 0x21813d:input.address=std::uint32_t(regs.rcx*1u+4u);input.size=4;break;
        case 0x218145:input.address=std::uint32_t(5163984u);input.size=4;break;
        case 0x21814b:input.address=std::uint32_t(regs.rsi*1u+68u);input.size=4;break;
        case 0x218157:input.address=std::uint32_t(regs.rsi*1u+regs.rcx*4u+8u);input.size=4;break;
        case 0x21815b:input.address=std::uint32_t(regs.rsi*1u+834u);input.size=2;break;
        case 0x218164:input.address=std::uint32_t(regs.rcx*1u+28u);input.size=4;break;
        case 0x218169:write.address=std::uint32_t(regs.rcx*1u+28u);write.size=4;writing=true;break;
        case 0x218175:write.address=std::uint32_t(regs.rdi*1u+0u);write.size=4;writing=true;break;
        case 0x218177:input.address=std::uint32_t(regs.rbp*1u+2004u);input.size=2;write.address=input.address;write.size=2;writing=true;break;
        case 0x218183:input.address=std::uint32_t(regs.rdi*1u+0u);input.size=4;break;
        case 0x16e8a4:input.address=std::uint32_t(regs.rdx*1u+68u);input.size=4;break;
        case 0x16e8a8:input.address=std::uint32_t(regs.rdx*1u+36u);input.size=4;break;
        case 0x16e8b8:input.address=std::uint32_t(regs.rdx*1u+42u);input.size=1;break;
        case 0x16e8d1:write.address=std::uint32_t(regs.rdi*1u+0u);write.size=4;writing=true;break;
        case 0x16e8d8:write.address=std::uint32_t(regs.rdi*1u+0u);write.size=1;writing=true;break;
        case 0x16e8db:input.address=std::uint32_t(regs.rdx*1u+72u);input.size=4;break;
        case 0x16e8f2:input.address=std::uint32_t(regs.rax*1u+0u);input.size=4;break;
        case 0x16e8f8:write.address=std::uint32_t(regs.rax*1u+0u);write.size=4;writing=true;break;
        case 0x16e8fa:write.address=std::uint32_t(regs.rsi*1u+0u);write.size=2;writing=true;break;
        case 0x16e8ff:input.address=std::uint32_t(regs.rdx*1u+52u);input.size=4;break;
        case 0x16e904:write.address=std::uint32_t(regs.rdx*1u+52u);write.size=4;writing=true;break;
        case 0x16e907:input.address=std::uint32_t(regs.rdx*1u+56u);input.size=4;break;
        case 0x16e90f:input.address=std::uint32_t(regs.rdx*1u+36u);input.size=4;break;
        case 0x16e912:input.address=std::uint32_t(regs.rdx*1u+56u);input.size=4;break;
        case 0x16e91c:write.address=std::uint32_t(regs.rdx*1u+56u);write.size=4;writing=true;break;
        case 0x16e921:input.address=std::uint32_t(regs.rsi*1u+0u);input.size=2;break;
        case 0x16e927:input.address=std::uint32_t(regs.rdx*1u+60u);input.size=4;break;
        case 0x16e92d:write.address=std::uint32_t(regs.rdx*1u+60u);write.size=4;writing=true;break;
        }

        if((regs.rip==0x16e8d1u||regs.rip==0x16e8d8u)&&regs.rcx==0)writing=false;
        if(input.address>=0x2000000u&&input.address<0x2010000u)input.address=0;
        if(writing&&write.address>=0x2000000u&&write.address<0x2010000u)writing=false;
        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original speech queue store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original speech queue did not return");check(!capture_entry||captured,"Actual speech publication block did not reach queue admission");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
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
    check(bool(output),"Physical speech queue output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
