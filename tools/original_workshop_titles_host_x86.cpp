// Offline unchanged i386 XBE execution. HS parser/completion are protected
// execute pages handled at their exact ABI; no original code byte is patched.
#include <sys/mman.h>
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
std::uint32_t title_entry,title_host_stack,title_initial[8],title_result[8],title_flags,title_xmm[32];
std::uint16_t title_fp_status;
volatile std::uint32_t title_parsed,title_parser_calls,title_completion_calls;
}
static std::uint32_t word(const std::vector<unsigned char>& raw,unsigned at) {
    if(at>raw.size() || raw.size()-at<4)throw std::runtime_error("Truncated XBE field");
    std::uint32_t value;std::memcpy(&value,raw.data()+at,4);return value;
}
static void boundary(int, siginfo_t*,void* state) {
    auto* context=static_cast<ucontext_t*>(state);auto& r=context->uc_mcontext.gregs;
    const auto pc=std::uint32_t(r[REG_EIP]),sp=std::uint32_t(r[REG_ESP]);
    const auto* stack=reinterpret_cast<const std::uint32_t*>(sp);
    if(pc==0x212010) {
        if(stack[1]!=0x12340001 || stack[2]!=2 || stack[3]!=0x459662 || stack[4]!=0x23450002)_exit(121);
        title_parser_calls=title_parser_calls+1;r[REG_EAX]=title_parsed?0x2103000:0;
        r[REG_EIP]=stack[0];r[REG_ESP]=sp+20;return;
    }
    if(pc==0x211da0) {
        if(std::uint32_t(r[REG_EAX])!=0 || std::uint32_t(r[REG_ECX])!=0x12340001)_exit(122);
        title_completion_calls=title_completion_calls+1;r[REG_EIP]=stack[0];r[REG_ESP]=sp+4;return;
    }
    _exit(123);
}
__attribute__((noinline,noclone)) static void execute() {
    const std::uint16_t control=0x27f;const std::uint32_t mxcsr=0x1f80;
    asm volatile("fninit; fldcw %0; ldmxcsr %1"::"m"(control),"m"(mxcsr):"memory");
    asm volatile(
        "movl %%esp,title_host_stack\n\t"
        "movl title_initial+16,%%esp\n\taddl $4,%%esp\n\tpushl $0x202\n\tpopfl\n\t"
        "movups title_xmm,%%xmm0\n\tmovups title_xmm+16,%%xmm1\n\t"
        "movups title_xmm+32,%%xmm2\n\tmovups title_xmm+48,%%xmm3\n\t"
        "movups title_xmm+64,%%xmm4\n\tmovups title_xmm+80,%%xmm5\n\t"
        "movups title_xmm+96,%%xmm6\n\tmovups title_xmm+112,%%xmm7\n\t"
        "movl title_initial,%%eax\n\tmovl title_initial+4,%%ecx\n\t"
        "movl title_initial+8,%%edx\n\tmovl title_initial+12,%%ebx\n\t"
        "movl title_initial+20,%%ebp\n\tmovl title_initial+24,%%esi\n\tmovl title_initial+28,%%edi\n\t"
        "call *title_entry\n\t"
        "movl %%eax,title_result\n\tmovl %%ecx,title_result+4\n\t"
        "movl %%edx,title_result+8\n\tmovl %%ebx,title_result+12\n\t"
        "movl %%esp,title_result+16\n\tmovl %%ebp,title_result+20\n\t"
        "movl %%esi,title_result+24\n\tmovl %%edi,title_result+28\n\t"
        "pushfl\n\tpopl title_flags\n\tfnstsw title_fp_status\n\t"
        "movups %%xmm0,title_xmm\n\tmovups %%xmm1,title_xmm+16\n\t"
        "movups %%xmm2,title_xmm+32\n\tmovups %%xmm3,title_xmm+48\n\t"
        "movups %%xmm4,title_xmm+64\n\tmovups %%xmm5,title_xmm+80\n\t"
        "movups %%xmm6,title_xmm+96\n\tmovups %%xmm7,title_xmm+112\n\t"
        "movl title_host_stack,%%esp"
        :::"eax","ecx","edx","ebx","ebp","esi","edi","cc","memory",
        "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7");
}
int main(int argc,char** argv)try {
    if(argc!=2)throw std::runtime_error("Supply independently pinned original TU1.5 image");
    std::ifstream file(argv[1],std::ios::binary);const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file),{}};
    if(raw.empty())throw std::runtime_error("Original image unavailable");
    if(mmap(reinterpret_cast<void*>(0x10000),0x800000,PROT_READ|PROT_WRITE|PROT_EXEC,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=reinterpret_cast<void*>(0x10000))throw std::runtime_error("Original address bank unavailable");
    if(mmap(reinterpret_cast<void*>(0x2100000),0x100000,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=reinterpret_cast<void*>(0x2100000))throw std::runtime_error("Declared title inputs unavailable");
    const auto base=word(raw,0x104),count=word(raw,0x11c),table=word(raw,0x120)-base;
    for(unsigned i=0;i<count;++i) {
        const auto row=table+i*56,address=word(raw,row+4),offset=word(raw,row+12),size=word(raw,row+16);
        if(address<0x10000 || std::uint64_t(address)+size>0x810000 || offset>raw.size() || size>raw.size()-offset)
            throw std::runtime_error("Original section escapes declared address bank");
        std::memcpy(reinterpret_cast<void*>(address),raw.data()+offset,size);
        if(std::memcmp(reinterpret_cast<const void*>(address),raw.data()+offset,size))throw std::runtime_error("Original instruction/data bytes changed");
    }
    struct sigaction action{};action.sa_sigaction=boundary;action.sa_flags=SA_SIGINFO;sigemptyset(&action.sa_mask);
    if(sigaction(SIGSEGV,&action,nullptr) || mprotect(reinterpret_cast<void*>(0x211000),0x2000,PROT_READ|PROT_WRITE))
        throw std::runtime_error("Cannot bind exact HS boundary pages");
    std::string line;
    while(std::getline(std::cin,line)) {
        std::istringstream row(line);unsigned hs,parsed,free,title,delay;int tick_rate;std::string before;
        if(!(row>>hs>>parsed>>free>>tick_rate>>std::hex>>title>>delay>>before) || before.size()!=72 || hs>2 || parsed>1 || free>4)
            throw std::runtime_error("Malformed declared title ABI input");
        auto* bank=reinterpret_cast<unsigned char*>(0x2100000);
        for(unsigned n=0;n<36;++n)bank[n]=std::stoul(before.substr(n*2,2),nullptr,16);
        *reinterpret_cast<std::uint32_t*>(0x51df50)=0x2100000;
        *reinterpret_cast<std::uint32_t*>(0x51df54)=free==4?0:0x2102000;
        *reinterpret_cast<std::int16_t*>(0x2102002)=tick_rate;
        *reinterpret_cast<std::uint32_t*>(0x2103000)=title;*reinterpret_cast<std::uint32_t*>(0x2103004)=delay;
        if(hs==2) {
            // Lifecycle inputs: delay carries the literal seconds-per-tick
            // word and title the scenario title count. The three 36-byte
            // records are declared timing inputs, never original outcomes.
            *reinterpret_cast<std::uint32_t*>(0x51df54)=0x2102000;
            *reinterpret_cast<std::uint32_t*>(0x2102004)=delay;
            *reinterpret_cast<std::uint32_t*>(0x4ecc20)=0x2104000;
            *reinterpret_cast<std::uint32_t*>(0x21041f0)=title;
            *reinterpret_cast<std::uint32_t*>(0x21041f4)=0x2105000;
            for(unsigned n=0;n<3;++n) {
                std::uint32_t hold,fade;
                if(!(row>>hold>>fade))throw std::runtime_error("Missing declared title timing inputs");
                *reinterpret_cast<std::uint32_t*>(0x210501c+n*36)=hold;
                *reinterpret_cast<std::uint32_t*>(0x2105020+n*36)=fade;
            }
        }
        title_entry=hs==2?0x13f160:hs?0x2b3620:0x13ecc0;
        title_parsed=parsed;title_parser_calls=0;title_completion_calls=0;
        const std::uint32_t initial[]{0x11223344,0x22334455,0x33445566,0x44556677,0x21fff00,0x55667788,0x66778899,0x778899aa};
        std::memcpy(title_initial,initial,sizeof(initial));
        for(unsigned i=0;i<32;++i)title_xmm[i]=0x3f000000+i;
        auto* stack=reinterpret_cast<std::uint32_t*>(0x21fff00);
        if(hs==1){stack[1]=561;stack[2]=0x12340001;stack[3]=0x23450002;}
        else {stack[1]=title;stack[2]=delay;}
        execute();
        for(auto value:title_result)std::cout<<std::hex<<value<<' ';
        std::cout<<title_flags<<' '<<title_fp_status<<' ';
        for(auto value:title_xmm)std::cout<<value<<' ';
        for(unsigned i=0;i<9;++i)std::cout<<*reinterpret_cast<std::uint32_t*>(bank+i*4)<<' ';
        std::cout<<title_parser_calls<<' '<<title_completion_calls<<'\n';
    }
    // The entire mapped .text remains literal even across protected-page
    // interceptions. This verification excludes original mutable .data/BSS.
    for(unsigned i=0;i<count;++i) {
        const auto row=table+i*56,address=word(raw,row+4),offset=word(raw,row+12),size=word(raw,row+16);
        if(address<=0x13ecc0 && 0x2b3663<=std::uint64_t(address)+size &&
            std::memcmp(reinterpret_cast<const void*>(address),raw.data()+offset,size))throw std::runtime_error("Original .text bytes changed during execution");
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
