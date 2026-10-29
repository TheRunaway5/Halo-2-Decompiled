// Offline unchanged original 129500 + 33B590 + 12E9A0. No instruction patches,
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
struct Store{std::uint32_t site,address;char before[4],after[4];};
}
int main(int argc,char** argv)try {
    check(argc==6,"Supply ELF, stop, text end, regions and output");const auto stop=std::stoul(argv[2],nullptr,0),end=std::stoul(argv[3],nullptr,0);
    Child child;child.pid=fork();check(child.pid>=0,"Cannot create physical ducking reference");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
    int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Physical ducking wait failed");};wait();
    check(WIFSTOPPED(status),"Ducking exec stop absent");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
    request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Ducking input setup failed");
    request(PTRACE_SINGLESTEP,child.pid);wait();user_regs_struct regs{};unsigned instructions{};
    std::vector<Read> reads;std::vector<Store> stores;std::set<std::uint32_t> sites;
    auto read=[&](std::uint32_t p,char* out,unsigned size){iovec local{out,size},remote{reinterpret_cast<void*>(std::uintptr_t(p)),size};check(process_vm_readv(child.pid,&local,1,&remote,1,0)==size,"Original ducking input read failed");};
    for(;instructions<10000;++instructions) {
        check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original ducking execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);
        if(regs.rip==stop)break;check(regs.rip>=0x12000u&&regs.rip<end,"Ducking escaped original text");sites.insert(std::uint32_t(regs.rip));
        Read input{};input.size=4;Store write{};write.site=std::uint32_t(regs.rip);bool writing=false;
        switch(regs.rip) {
        case 0x33b590:input.address=0x41b774u;break;case 0x33b595:input.address=std::uint32_t(regs.rax);break;
        case 0x129507:input.address=0x4f2c50u;break;case 0x12950d:input.address=std::uint32_t(regs.rdx)+0x80u;break;
        case 0x12951b:input.address=std::uint32_t(regs.rdx)+0x1fcu;break;case 0x12952d:input.address=std::uint32_t(regs.rdx)+0x84u;break;
        case 0x12954a:input.address=0x4f3218u;break;case 0x129559:input.address=std::uint32_t(regs.rdx)+0x204u;break;
        case 0x12957f:input.address=std::uint32_t(regs.rax)+0x1120u;input.size=1;break;
        case 0x12958d:input.address=std::uint32_t(regs.rax)+8u;break;case 0x129597:input.address=std::uint32_t(regs.rax)+0x1121u;input.size=1;break;
        case 0x1295a5:case 0x1295c8:case 0x1295e4:input.address=std::uint32_t(regs.rdx)+0x1e8u;break;
        case 0x1295ae:input.address=std::uint32_t(regs.rdx)+0x1ecu;break;
        case 0x1295f2:case 0x1295fb:input.address=std::uint32_t(regs.rdx)+0x1f0u;break;
        case 0x12960d:case 0x129621:case 0x12963c:case 0x129679:case 0x12969e:input.address=std::uint32_t(regs.rdx)+0x1e4u;break;
        case 0x129658:case 0x129661:input.address=std::uint32_t(regs.rdx)+0x1f4u;break;
        case 0x129669:input.address=std::uint32_t(regs.rdx)+0x1e0u;break;
        case 0x12e9a0:input.address=0x4f9c6du;input.size=1;break;case 0x12e9ae:input.address=0x556e03u;input.size=1;break;
        case 0x12e9b8:input.address=0x556dfeu;input.size=1;break;case 0x12e9c2:input.address=0x556dffu;input.size=1;break;
        case 0x12e9cc:input.address=0x556e02u;input.size=1;break;case 0x12e9d6:input.address=0x556e00u;input.size=1;break;
        case 0x12e9e0:input.address=0x556e01u;input.size=1;break;case 0x12e9ea:input.address=0x556e06u;input.size=1;break;
        case 0x12e9f4:input.address=0x4f2d40u;break;case 0x12e9fe:input.address=0x4f2d70u;break;
        case 0x129527:write.address=std::uint32_t(regs.rdx)+0x80u;writing=true;break;
        case 0x129544:write.address=std::uint32_t(regs.rdx)+0x84u;writing=true;break;
        case 0x129551:write.address=std::uint32_t(regs.rdx)+0x1fcu;writing=true;break;
        case 0x129569:write.address=std::uint32_t(regs.rdx)+0x1e0u;writing=true;break;
        case 0x129571:write.address=std::uint32_t(regs.rdx)+0x204u;writing=true;break;
        case 0x1295c2:case 0x1295de:case 0x1296b3:write.address=std::uint32_t(regs.rdx)+0x1e8u;writing=true;break;
        case 0x12962a:case 0x129645:case 0x1296ab:case 0x1296c7:write.address=std::uint32_t(regs.rdx)+0x1e4u;writing=true;break;
        }
        if(input.address){input.stores_before=stores.size();read(input.address,input.value,input.size);reads.push_back(input);}
        if(writing)read(write.address,write.before,4);request(PTRACE_SINGLESTEP,child.pid);wait();
        if(writing){check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original ducking store fault");read(write.address,write.after,4);stores.push_back(write);}
    }
    check(regs.rip==stop,"Original ducking did not return");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
    std::ofstream output(argv[5],std::ios::binary);for(auto n:{std::uint32_t(fp.cwd),std::uint32_t(fp.swd),std::uint32_t(fp.mxcsr),instructions})output.write(reinterpret_cast<const char*>(&n),4);
    std::ifstream regions(argv[4]);std::uint32_t address{},size{};
    while(regions>>address>>size){std::vector<char> bytes(size);read(address,bytes.data(),size);output.write(bytes.data(),bytes.size());}
    auto count=std::uint32_t(reads.size());output.write(reinterpret_cast<const char*>(&count),4);for(const auto& event:reads)output.write(reinterpret_cast<const char*>(&event),sizeof(event));
    count=std::uint32_t(stores.size());output.write(reinterpret_cast<const char*>(&count),4);for(const auto& event:stores)output.write(reinterpret_cast<const char*>(&event),sizeof(event));
    count=std::uint32_t(sites.size());output.write(reinterpret_cast<const char*>(&count),4);for(auto site:sites)output.write(reinterpret_cast<const char*>(&site),4);
    check(bool(output),"Physical ducking output incomplete");return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
