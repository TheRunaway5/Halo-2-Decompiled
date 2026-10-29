// Offline unchanged original 1BD9D0 +2787F0/278720/278A50. No instruction patches,
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
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical query start reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical query start wait failed");};wait();
    check(WIFSTOPPED(status),"Query start exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Query start input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original query start input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original query start execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Query start escaped original text");sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        write.size=4;
        switch(regs.rip) {
        case 0x1bd9d1:input.address=std::uint32_t((5251272));input.size=4;break;
        case 0x1bd9e9:input.address=std::uint32_t(regs.rbp*1u+(68));input.size=4;break;
        case 0x1bd9ec:input.address=std::uint32_t(regs.rsi*1u+regs.rdi*1u+(124));input.size=4;break;
        case 0x1bd9fd:input.address=std::uint32_t((5304056));input.size=4;break;
        case 0x1bda03:input.address=std::uint32_t(regs.rcx*1u+(68));input.size=4;break;
        case 0x1bda12:input.address=std::uint32_t(regs.rax*1u+regs.rdx*1u+(24));input.size=4;break;
        case 0x1bda1b:input.address=std::uint32_t(regs.rbp*1u+(68));input.size=4;break;
        case 0x1bda31:input.address=std::uint32_t(regs.rax*1u+(128));input.size=4;break;
        case 0x27880a:input.address=std::uint32_t((5304060));input.size=4;break;
        case 0x27880f:input.address=std::uint32_t(regs.rax*1u+(68));input.size=4;break;
        case 0x278820:input.address=std::uint32_t(regs.rsi*1u+(4));input.size=4;break;
        case 0x27883d:input.address=std::uint32_t(regs.rsi*1u+regs.rcx*4u+(4));input.size=4;break;
        case 0x278859:write.address=std::uint32_t(regs.rax*1u+(4));write.size=4;writing=true;break;
        case 0x27885d:write.address=std::uint32_t(regs.rax*1u+(12));write.size=4;writing=true;break;
        case 0x278862:write.address=std::uint32_t(regs.rax*1u+(10));write.size=2;writing=true;break;
        case 0x278721:input.address=std::uint32_t((5251272));input.size=4;break;
        case 0x278726:input.address=std::uint32_t(regs.rax*1u+(68));input.size=4;break;
        case 0x278756:input.address=std::uint32_t(regs.rsi*1u+regs.rcx*4u+(404));input.size=2;break;
        case 0x278768:input.address=std::uint32_t(regs.rdx*1u+(8));input.size=4;break;
        case 0x27876b:input.address=std::uint32_t((5365588));input.size=4;break;
        case 0x278771:input.address=std::uint32_t(regs.rcx*1u+(8));input.size=4;break;
        case 0x278776:input.address=std::uint32_t(regs.rdx*1u+(4));input.size=4;break;
        case 0x278779:input.address=std::uint32_t(regs.rdx*1u+(2));input.size=2;break;
        case 0x27877d:input.address=std::uint32_t((5304060));input.size=4;break;
        case 0x27878e:input.address=std::uint32_t(regs.rax*1u+(68));input.size=4;break;
        case 0x278794:input.address=std::uint32_t(regs.rcx*1u+regs.rdx*4u+(10));input.size=2;break;
        case 0x278a50:input.address=std::uint32_t((5251272));input.size=4;break;
        case 0x278a62:input.address=std::uint32_t(regs.rcx*1u+(68));input.size=4;break;
        case 0x278a76:input.address=std::uint32_t(regs.rcx*1u+(4));input.size=4;break;
        case 0x278a79:input.address=std::uint32_t((5304060));input.size=4;break;
        case 0x278a7f:input.address=std::uint32_t(regs.rdx*1u+(68));input.size=4;break;
        case 0x278a82:input.address=std::uint32_t(regs.rcx*1u+(2));input.size=2;break;
        case 0x278a96:write.address=std::uint32_t(regs.rax*1u+regs.rdx*4u+(4));write.size=4;writing=true;break;
        case 0x278a9e:input.address=std::uint32_t(regs.rcx*1u+(2));input.size=2;break;
        case 0x278aa5:write.address=std::uint32_t(regs.rax*1u+regs.rdx*4u+(8));write.size=2;writing=true;break;
        case 0x278aac:write.address=std::uint32_t(regs.rcx*1u+(0));write.size=2;writing=true;break;
        }

        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,write.size);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original query start store fault");read(write.address,write.after,write.size);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original query start did not return");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
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
    check(bool(output),"Physical query start output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
