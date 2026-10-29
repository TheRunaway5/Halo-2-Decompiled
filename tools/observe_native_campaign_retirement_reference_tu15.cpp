// Offline unchanged original 129EA0 + 220690 + 16E8A0. No instruction patches,
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
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical retirement reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical retirement wait failed");};wait();
    check(WIFSTOPPED(status),"Retirement exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Retirement input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original retirement input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original retirement execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Retirement escaped original text");sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x129eaf:input.address=std::uint32_t((5188684));input.size=4;break;
        case 0x129eb5:input.address=std::uint32_t(regs.rbx*1u+(68));input.size=4;break;
        case 0x129eb8:input.address=std::uint32_t(regs.rcx*1u+(168));input.size=4;break;
        case 0x129ece:input.address=std::uint32_t((5428840));input.size=4;break;
        case 0x129ed4:input.address=std::uint32_t(regs.rsi*1u+(68));input.size=4;break;
        case 0x129ed7:input.address=std::uint32_t(regs.rcx*1u+(3));input.size=1;break;
        case 0x129edc:input.address=std::uint32_t(regs.rax*1u+(4));input.size=4;break;
        case 0x129ee1:input.address=std::uint32_t(regs.rax*1u+(3));input.size=1;break;
        case 0x129eec:input.address=std::uint32_t(regs.rcx*1u+(168));input.size=4;break;
        case 0x220695:input.address=std::uint32_t((5428840));input.size=4;break;
        case 0x2206a7:input.address=std::uint32_t(regs.rdx*1u+(68));input.size=4;break;
        case 0x2206aa:input.address=std::uint32_t(regs.rcx*1u+regs.rbx*1u+(2));input.size=1;break;
        case 0x16e8a4:input.address=std::uint32_t(regs.rdx*1u+(68));input.size=4;break;
        case 0x16e8a8:input.address=std::uint32_t(regs.rdx*1u+(36));input.size=4;break;
        case 0x16e8b8:input.address=std::uint32_t(regs.rdx*1u+(42));input.size=1;break;
        case 0x16e8db:input.address=std::uint32_t(regs.rdx*1u+(72));input.size=4;break;
        case 0x16e8f2:input.address=std::uint32_t(regs.rax*1u+(0));input.size=4;break;
        case 0x16e8ff:input.address=std::uint32_t(regs.rdx*1u+(52));input.size=4;break;
        case 0x16e907:input.address=std::uint32_t(regs.rdx*1u+(56));input.size=4;break;
        case 0x16e90f:input.address=std::uint32_t(regs.rdx*1u+(36));input.size=4;break;
        case 0x16e912:input.address=std::uint32_t(regs.rdx*1u+(56));input.size=4;break;
        case 0x16e921:input.address=std::uint32_t(regs.rsi*1u+(0));input.size=2;break;
        case 0x16e927:input.address=std::uint32_t(regs.rdx*1u+(60));input.size=4;break;
        case 0x129ee9:write.address=std::uint32_t(regs.rax)+3u;write.size=1;writing=true;break;
        case 0x2206bb:write.address=std::uint32_t(regs.rcx)+2u;write.size=1;writing=true;break;
        case 0x16e8d1:write.address=std::uint32_t(regs.rdi);writing=regs.rcx!=0;break;
        case 0x16e8d8:write.address=std::uint32_t(regs.rdi);write.size=1;writing=regs.rcx!=0;break;
        case 0x16e8f8:write.address=std::uint32_t(regs.rax);writing=true;break;
        case 0x16e8fa:write.address=std::uint32_t(regs.rsi);write.size=2;writing=true;break;
        case 0x16e904:write.address=std::uint32_t(regs.rdx)+0x34u;writing=true;break;
        case 0x16e91c:write.address=std::uint32_t(regs.rdx)+0x38u;writing=true;break;
        case 0x16e92d:write.address=std::uint32_t(regs.rdx)+0x3cu;writing=true;break;
        }
        const bool mutable_input=(input.address>=0x1000000u&&input.address<0x1010000u)||input.address==0x4f2c4cu||input.address==0x52d668u;
        if(!mutable_input)input.address=0;

        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original retirement store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original retirement did not return");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
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
    check(bool(output),"Physical retirement output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
