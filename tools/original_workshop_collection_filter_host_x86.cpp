// Offline unchanged TU1.5 i386 execution, with declared ordinary DATA only.
// No instruction patches, callee interception or supplied dependency results.
#include <sys/mman.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
extern "C" {
std::uint32_t collection_entry,collection_host_stack,collection_initial[8],collection_result[8],collection_flags,collection_xmm[32],collection_initial_flags,collection_mxcsr;
std::uint16_t collection_status,collection_control;
}
static std::uint32_t word(const std::vector<unsigned char>& raw,unsigned at){
    if(at>raw.size()||raw.size()-at<4)throw std::runtime_error("Truncated original image");std::uint32_t value;std::memcpy(&value,raw.data()+at,4);return value;
}
__attribute__((noinline,noclone)) static void execute(){
    const std::uint16_t control=0x27f;const std::uint32_t mxcsr=0x1f80;
    asm volatile("fninit; fldcw %0; ldmxcsr %1"::"m"(control),"m"(mxcsr):"memory");
    asm volatile(
        "movl %%esp,collection_host_stack\n\t"
        "movl collection_initial+16,%%esp\n\taddl $4,%%esp\n\tpushl collection_initial_flags\n\tpopfl\n\t"
        "movups collection_xmm,%%xmm0\n\tmovups collection_xmm+16,%%xmm1\n\t"
        "movups collection_xmm+32,%%xmm2\n\tmovups collection_xmm+48,%%xmm3\n\t"
        "movups collection_xmm+64,%%xmm4\n\tmovups collection_xmm+80,%%xmm5\n\t"
        "movups collection_xmm+96,%%xmm6\n\tmovups collection_xmm+112,%%xmm7\n\t"
        "movl collection_initial,%%eax\n\tmovl collection_initial+4,%%ecx\n\t"
        "movl collection_initial+8,%%edx\n\tmovl collection_initial+12,%%ebx\n\t"
        "movl collection_initial+20,%%ebp\n\tmovl collection_initial+24,%%esi\n\tmovl collection_initial+28,%%edi\n\t"
        "call *collection_entry\n\t"
        "movl %%eax,collection_result\n\tmovl %%ecx,collection_result+4\n\t"
        "movl %%edx,collection_result+8\n\tmovl %%ebx,collection_result+12\n\t"
        "movl %%esp,collection_result+16\n\tmovl %%ebp,collection_result+20\n\t"
        "movl %%esi,collection_result+24\n\tmovl %%edi,collection_result+28\n\t"
        "pushfl\n\tpopl collection_flags\n\tfnstsw collection_status\n\tfnstcw collection_control\n\tstmxcsr collection_mxcsr\n\t"
        "movups %%xmm0,collection_xmm\n\tmovups %%xmm1,collection_xmm+16\n\t"
        "movups %%xmm2,collection_xmm+32\n\tmovups %%xmm3,collection_xmm+48\n\t"
        "movups %%xmm4,collection_xmm+64\n\tmovups %%xmm5,collection_xmm+80\n\t"
        "movups %%xmm6,collection_xmm+96\n\tmovups %%xmm7,collection_xmm+112\n\t"
        "movl collection_host_stack,%%esp"
        :::"eax","ecx","edx","ebx","ebp","esi","edi","cc","memory","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7");
}
static void hex_input(unsigned char* out,const std::string& text,std::size_t size){
    if(text.size()!=size*2)throw std::runtime_error("Declared bank size differs");
    for(unsigned n=0;n<size;++n)out[n]=std::stoul(text.substr(n*2,2),nullptr,16);
}
static void hex_output(const unsigned char* data,std::size_t size){
    constexpr char hex[]="0123456789abcdef";for(unsigned n=0;n<size;++n){std::cout.put(hex[data[n]>>4]);std::cout.put(hex[data[n]&15]);}
}
int main(int argc,char** argv)try{
    if(argc!=2)throw std::runtime_error("Supply independently pinned original TU1.5 image");
    std::ifstream file(argv[1],std::ios::binary);const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file),{}};
    if(raw.empty())throw std::runtime_error("Original image unavailable");
    for(const auto& range:std::vector<std::pair<std::uint32_t,std::size_t>>{{0x10000,0x800000},{0x2100000,0x100000},{0x90000000,4096}})
        if(mmap(reinterpret_cast<void*>(range.first),range.second,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=reinterpret_cast<void*>(range.first))throw std::runtime_error("Original/declared address bank unavailable");
    const auto base=word(raw,0x104),count=word(raw,0x11c),table=word(raw,0x120)-base;
    for(unsigned n=0;n<count;++n){const auto row=table+n*56,address=word(raw,row+4),offset=word(raw,row+12),size=word(raw,row+16);
        if(address<0x10000||std::uint64_t(address)+size>0x810000||offset>raw.size()||size>raw.size()-offset)throw std::runtime_error("Original section outside bank");
        std::memcpy(reinterpret_cast<void*>(address),raw.data()+offset,size);if(std::memcmp(reinterpret_cast<void*>(address),raw.data()+offset,size))throw std::runtime_error("Original bytes changed");}
    std::string line;
    while(std::getline(std::cin,line)){
        std::istringstream input(line);std::string bank,negative;std::uint32_t receiver,global;std::uint32_t args[6];
        if(!(input>>std::hex>>collection_entry>>receiver>>collection_initial_flags>>global))throw std::runtime_error("Invalid original call input");
        for(auto& arg:args)if(!(input>>arg))throw std::runtime_error("Missing ordinary argument");
        if(!(input>>bank>>negative))throw std::runtime_error("Missing declared banks");
        hex_input(reinterpret_cast<unsigned char*>(0x2100000),bank,0x4000);hex_input(reinterpret_cast<unsigned char*>(0x90000000),negative,4096);
        *reinterpret_cast<std::uint32_t*>(0x48b860)=global;
        const std::uint32_t initial[]{0x11223344,receiver,0x33445566,0x44556677,0x21fff00,0x55667788,0x66778899,0x778899aa};
        std::memcpy(collection_initial,initial,sizeof(initial));for(unsigned n=0;n<32;++n)collection_xmm[n]=0x3f000000+n;
        auto* stack=reinterpret_cast<std::uint32_t*>(0x21fff00);for(unsigned n=0;n<6;++n)stack[n+1]=args[n];
        execute();
        for(auto value:collection_result)std::cout<<std::hex<<value<<' ';
        std::cout<<collection_flags<<' '<<collection_control<<' '<<collection_status<<' '<<collection_mxcsr<<' ';
        for(auto value:collection_xmm)std::cout<<value<<' ';
        hex_output(reinterpret_cast<unsigned char*>(0x2100000),0x4000);std::cout<<' ';hex_output(reinterpret_cast<unsigned char*>(0x90000000),4096);std::cout<<'\n';
    }
    // Every original section remains byte-identical except the declared
    // mutable global input48B860. No function or jump-table byte is changed.
    for(unsigned n=0;n<count;++n){const auto row=table+n*56,address=word(raw,row+4),offset=word(raw,row+12),size=word(raw,row+16);
        for(unsigned k=0;k<size;++k)if(address+k<0x48b860||address+k>=0x48b864)
            if(*reinterpret_cast<const unsigned char*>(address+k)!=raw[offset+k])throw std::runtime_error("Original section mutated during execution");}
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
