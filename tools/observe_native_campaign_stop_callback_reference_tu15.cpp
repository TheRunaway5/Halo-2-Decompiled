// Offline unchanged original 129EA0 + 220690 + 16E8A0. No instruction patches,
// host time substitution, supplied return values or resumed unknown owners.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <array>
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
    check(argc==6||argc==7,"Supply ELF, stop, text end, regions, output and optional real callee entry");const auto stop=std::stoul(argv[2],nullptr,0),end=std::stoul(argv[3],nullptr,0);
    const std::uint32_t capture_entry=argc==7?std::stoul(argv[6],nullptr,0):0u;
    bool tracking=!capture_entry;unsigned measured{};user_regs_struct captured_regs{};user_fpregs_struct captured_fp{};
    std::array<std::uint32_t,4> captured_frame{};std::vector<char> captured_banks;std::set<std::uint32_t> prefix_sites;
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical stop callback reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical stop callback wait failed");};wait();
    check(WIFSTOPPED(status),"Stop callback exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Stop callback input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original stop callback input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original stop callback execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Stop callback escaped original text");
        if(!tracking) {
            if(regs.rip==capture_entry) {
                tracking=true;captured_regs=regs;request(PTRACE_GETFPREGS,child.pid,nullptr,&captured_fp);
                read(std::uint32_t(regs.rsp),reinterpret_cast<char*>(captured_frame.data()),16);
                std::ifstream input_regions(argv[4]);std::uint32_t p{},length{};
                while(input_regions>>p>>length){auto offset=captured_banks.size();captured_banks.resize(offset+length);read(p,captured_banks.data()+offset,length);}
            } else {prefix_sites.insert(std::uint32_t(regs.rip));request(PTRACE_SINGLESTEP,child.pid);wait();continue;}
        }
        ++measured;sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x18f6b0:input.address=std::uint32_t((5217116));input.size=4;break;
        case 0x18f6b5:input.address=std::uint32_t(regs.rax*1u+(41));input.size=1;break;
        case 0x18f6bd:input.address=std::uint32_t(regs.rsp*1u+(8));input.size=4;break;
        case 0x18f6ce:input.address=std::uint32_t(regs.rsp*1u+(12));input.size=4;break;
        case 0x18f6d2:input.address=std::uint32_t((5188684));input.size=4;break;
        case 0x18f6d8:input.address=std::uint32_t(regs.rdx*1u+(68));input.size=4;break;
        case 0x18f6e7:input.address=std::uint32_t(regs.rcx*1u+regs.rax*1u+(12));input.size=4;break;
        case 0xbcdfb:input.address=std::uint32_t((5163984));input.size=4;break;
        case 0xbce01:input.address=std::uint32_t(regs.rdx*1u+(56));input.size=4;break;
        case 0xbce10:input.address=std::uint32_t(regs.rdx*1u+(36));input.size=4;break;
        case 0xbce16:input.address=std::uint32_t(regs.rdx*1u+(68));input.size=4;break;
        case 0xbce19:input.address=std::uint32_t(regs.rax*1u+(0));input.size=2;break;
        case 0xbce33:input.address=std::uint32_t(regs.rdi*1u+(3));input.size=1;break;
        case 0xbce43:input.address=std::uint32_t(regs.rdi*1u+(8));input.size=4;break;
        case 0xd3300:input.address=std::uint32_t((5163984));input.size=4;break;
        case 0xd3306:input.address=std::uint32_t(regs.rcx*1u+(68));input.size=4;break;
        case 0xd3311:input.address=std::uint32_t(regs.rdx*1u+regs.rax*4u+(8));input.size=4;break;
        case 0xd3315:input.address=std::uint32_t(regs.rax*1u+(830));input.size=2;break;
        case 0xd331c:input.address=std::uint32_t(regs.rcx*1u+regs.rax*1u+(56));input.size=4;break;
        case 0xd3324:input.address=std::uint32_t(regs.rsp*1u+(4));input.size=4;break;
        case 0xd332a:write.address=std::uint32_t(regs.rax)+8u;writing=true;break;
        }
        const bool mutable_input=(input.address>=0x1000000u&&input.address<0x1020000u)||input.address==0x4f2c4cu||input.address==0x4ecbd0u||input.address==0x4f9b5cu;
        if(!mutable_input)input.address=0;

        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original stop callback store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original stop callback did not return");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
    std::ofstream output(argv[5],std::ios::binary);for(auto n:{std::uint32_t(fp.cwd),std::uint32_t(fp.swd),std::uint32_t(fp.mxcsr),measured})output.write(reinterpret_cast<const char*>(&n),4);
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
    const auto caller=std::uint32_t(capture_entry!=0);output.write(reinterpret_cast<const char*>(&caller),4);
    if(caller) {
        check(tracking,"Actual caller never entered the stop callback");
        auto prefix_steps=instructions-measured;output.write(reinterpret_cast<const char*>(&prefix_steps),4);
        for(auto n:{captured_regs.rax,captured_regs.rcx,captured_regs.rdx,captured_regs.rbx,captured_regs.rsp,captured_regs.rbp,captured_regs.rsi,captured_regs.rdi}){auto value=std::uint32_t(n);output.write(reinterpret_cast<const char*>(&value),4);}
        for(auto n:{std::uint32_t(captured_fp.cwd),std::uint32_t(captured_fp.swd),std::uint32_t(captured_fp.mxcsr)})output.write(reinterpret_cast<const char*>(&n),4);
        output.write(reinterpret_cast<const char*>(captured_fp.xmm_space),128);output.write(reinterpret_cast<const char*>(captured_fp.st_space),128);
        output.write(reinterpret_cast<const char*>(captured_frame.data()),16);output.write(captured_banks.data(),captured_banks.size());
        count=prefix_sites.size();output.write(reinterpret_cast<const char*>(&count),4);for(auto site:prefix_sites)output.write(reinterpret_cast<const char*>(&site),4);
    }
    check(bool(output),"Physical stop callback output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
