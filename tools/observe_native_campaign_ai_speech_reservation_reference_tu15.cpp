// Offline unchanged original 116C00/116B80/213890/21FE40/140EA0 actual unit speech reservation. No instruction patches,
// host time substitution, supplied return values or resumed unknown owners.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <signal.h>
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
    check(argc==6||argc==7,"Supply ELF, stop, text end, regions, output and optional actual callee capture");const bool fault_mode=argc==7&&std::string(argv[6])=="fault";const auto capture_entry=argc==7&&!fault_mode?std::stoul(argv[6],nullptr,0):0u;unsigned prefix_steps{};bool captured=false;std::uint32_t observed_output{};const auto stop=std::stoul(argv[2],nullptr,0),end=std::stoul(argv[3],nullptr,0);
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical speech reservation reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical speech reservation wait failed");};wait();
    check(WIFSTOPPED(status),"Speech choice exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Speech choice input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original speech reservation input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original speech reservation execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop) {
            if(fault_mode) {
                request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGSEGV,"Expected original sound-owner fault absent");
                siginfo_t information{};request(PTRACE_GETSIGINFO,child.pid,nullptr,&information);const auto at=std::uint32_t(reinterpret_cast<std::uintptr_t>(information.si_addr));
                std::ofstream fault(std::string(argv[5])+".fault",std::ios::binary);fault.write(reinterpret_cast<const char*>(&at),4);check(bool(fault),"Original sound-owner fault receipt incomplete");++instructions;
            }
            break;
        }check(regs.rip>=0x12000u&&regs.rip<end,"Speech choice escaped original text");
        if(capture_entry&&!captured&&regs.rip==capture_entry) {
            user_fpregs_struct state{};request(PTRACE_GETFPREGS,child.pid,nullptr,&state);
            std::ofstream capture(std::string(argv[5])+".capture",std::ios::binary);prefix_steps=instructions;
            for(auto n:{std::uint32_t(state.cwd),std::uint32_t(state.swd),std::uint32_t(state.mxcsr),prefix_steps})capture.write(reinterpret_cast<const char*>(&n),4);
            for(auto n:{regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi}){auto value=std::uint32_t(n);capture.write(reinterpret_cast<const char*>(&value),4);}
            capture.write(reinterpret_cast<const char*>(state.xmm_space),128);capture.write(reinterpret_cast<const char*>(state.st_space),128);
            std::ifstream regions(argv[4]);std::uint32_t at{},bytes{};
            while(regions>>at>>bytes){std::vector<char> values(bytes);read(at,values.data(),bytes);capture.write(values.data(),values.size());}
            std::uint32_t actual_arguments[3]{};read(std::uint32_t(regs.rsp)+4u,reinterpret_cast<char*>(actual_arguments),12);capture.write(reinterpret_cast<const char*>(actual_arguments),12);observed_output=actual_arguments[2];
            auto count=std::uint32_t(sites.size());capture.write(reinterpret_cast<const char*>(&count),4);for(auto site:sites)capture.write(reinterpret_cast<const char*>(&site),4);
            auto stored=std::uint32_t(stores.size());capture.write(reinterpret_cast<const char*>(&stored),4);for(const auto &event:stores)capture.write(reinterpret_cast<const char*>(&event),sizeof(event));
            check(bool(capture),"Actual cancellation caller capture incomplete");captured=true;sites.clear();reads.clear();stores.clear();
        }
        sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x116c04:input.address=std::uint32_t(5163984u);input.size=4;break;
        case 0x116c0a:input.address=std::uint32_t(regs.rsi*1u+68u);input.size=4;break;
        case 0x116c18:input.address=std::uint32_t(regs.rsi*1u+regs.rcx*4u+8u);input.size=4;break;
        case 0x116c1c:input.address=std::uint32_t(regs.rsi*1u+834u);input.size=2;break;
        case 0x116c37:input.address=std::uint32_t(regs.rcx*1u+0u);input.size=4;break;
        case 0x116c5a:input.address=std::uint32_t(regs.rsi*1u+266u);input.size=1;break;
        case 0x116c73:input.address=std::uint32_t(regs.rbp*1u+12u);input.size=2;break;
        case 0x116c9d:input.address=std::uint32_t(regs.rbp*1u+73u);input.size=1;break;
        case 0x116cb6:write.address=std::uint32_t(regs.rax*1u+0u);write.size=4;writing=true;break;
        case 0x116cc0:input.address=std::uint32_t(regs.rbp*1u+68u);input.size=4;break;
        case 0x116cc3:write.address=std::uint32_t(regs.rax*1u+0u);write.size=4;writing=true;break;
        case 0x116b80:input.address=std::uint32_t(5163984u);input.size=4;break;
        case 0x116b86:input.address=std::uint32_t(regs.rcx*1u+68u);input.size=4;break;
        case 0x116b91:input.address=std::uint32_t(regs.rcx*1u+regs.rax*4u+8u);input.size=4;break;
        case 0x116b95:input.address=std::uint32_t(regs.rax*1u+834u);input.size=2;break;
        case 0x116b9e:input.address=std::uint32_t(regs.rcx*1u+0u);input.size=4;break;
        case 0x116bb2:input.address=std::uint32_t(5178388u);input.size=4;break;
        case 0x116bbe:input.address=std::uint32_t(regs.rcx*1u+regs.rbx*1u+8u);input.size=4;break;
        case 0x116bc4:input.address=std::uint32_t(regs.rcx*1u+12u);input.size=4;break;
        case 0x116bce:input.address=std::uint32_t(regs.rcx*1u+16u);input.size=4;break;
        case 0x116bd4:input.address=std::uint32_t(regs.rdx*1u+regs.rcx*1u+12u);input.size=4;break;
        case 0x116beb:input.address=std::uint32_t(regs.rdi*1u+0u);input.size=4;break;
        case 0x116bf0:write.address=std::uint32_t(regs.rdi*1u+0u);write.size=4;writing=true;break;
        case 0x21389c:input.address=std::uint32_t(5178388u);input.size=4;break;
        case 0x2138aa:input.address=std::uint32_t(regs.rax*1u+regs.rcx*1u+8u);input.size=4;break;
        case 0x2138ae:input.address=std::uint32_t(regs.rax*1u+10u);input.size=1;break;
        case 0x2138b4:input.address=std::uint32_t(regs.rax*1u+8u);input.size=2;break;
        case 0x2138bc:input.address=std::uint32_t(5428836u);input.size=4;break;
        case 0x2138c2:input.address=std::uint32_t(regs.rcx*1u+36u);input.size=4;break;
        case 0x2138d7:input.address=std::uint32_t(regs.rsi*1u+10u);input.size=2;break;
        case 0x2138e2:input.address=std::uint32_t(5422736u);input.size=4;break;
        case 0x213902:input.address=std::uint32_t(regs.rsi*1u+regs.rcx*4u+0u);input.size=4;break;
        case 0x21fe40:input.address=std::uint32_t(5164060u);input.size=4;break;
        case 0x21fe45:input.address=std::uint32_t(regs.rax*1u+192u);input.size=4;break;
        case 0x21fe4f:input.address=std::uint32_t(regs.rax*1u+196u);input.size=4;break;
        case 0x21fe59:input.address=std::uint32_t(regs.rax*1u+4u);input.size=4;break;
        case 0x21fe5c:input.address=std::uint32_t(5178388u);input.size=4;break;
        case 0x21fe6a:input.address=std::uint32_t(regs.rax*1u+regs.rcx*1u+8u);input.size=4;break;
        case 0x21fe72:input.address=std::uint32_t(regs.rax*1u+2u);input.size=1;break;
        case 0x21fe76:input.address=std::uint32_t(regs.rdx*1u+4u);input.size=4;break;
        case 0x21fe7c:input.address=std::uint32_t(regs.rdx*1u+regs.rcx*1u+10u);input.size=1;break;
        case 0x21fe8b:input.address=std::uint32_t(regs.rcx*1u+6u);input.size=2;break;
        }

        if(input.address>=0x2000000u&&input.address<0x2010000u&&input.address!=observed_output)input.address=0;
        if(writing&&write.address>=0x2000000u&&write.address<0x2010000u&&write.address!=observed_output)writing=false;
        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original speech reservation store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original speech reservation did not return");check(!capture_entry||captured,"Actual117240 did not reach reservation");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
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
    check(bool(output),"Physical speech reservation output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
