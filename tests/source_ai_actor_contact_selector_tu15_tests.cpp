#include "halo2/source_ai_actor_contact_selector_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& memory,unsigned address,unsigned value,unsigned width=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8));
    memory.write(address,std::span(bytes).first(width));
}
unsigned word(const SourceRuntimeMemory& memory,unsigned address) {
    unsigned value{};const auto bytes=memory.view(address,4);
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8);
    return value;
}
void run_case(unsigned kind,unsigned rounding) {
    constexpr unsigned actor=0x64000200u,stack=0x68000800u;
    const unsigned target_base=kind==4?0xe6000000u:kind==5?0xfffffedcu:0x66000000u;
    const unsigned target_row=target_base+0x124u;
    const unsigned contact_datum=kind==0?0xffffffffu:0xabcd0001u;
    const unsigned target_datum=kind==1?0xffffffffu:0xbeef0001u;
    const unsigned selected=kind==2?0u:kind==3?0xffffu:0x6789u;
    SourceRuntimeMemory memory;
    const auto mount=[&](unsigned at,unsigned bytes,const char* owner) {
        memory.mount(at,std::vector<std::byte>(bytes),SourceRuntimeMemory::Access::ReadWrite,owner);
    };
    mount(0x64000000u,0x1000,"AI selector actor fixture");
    mount(0x68000000u,0x1000,"AI selector actual caller stack");
    mount(0x50eef0u,4,"AI selector contact header binding");
    mount(0x50eeecu,4,"AI selector target header binding");
    mount(0x63000000u,0x48,"AI selector contact array header");
    mount(0x63100000u,0x48,"AI selector target array header");
    mount(0x65000000u,0x78,"AI selector contact rows");
    if(kind!=5)mount(target_base,0x200,"AI selector target rows");
    put(memory,actor+0x338u,contact_datum);
    put(memory,0x50eef0u,0x63000000u);put(memory,0x63000044u,0x65000000u);
    put(memory,0x65000050u,target_datum);
    put(memory,0x50eeecu,0x63100000u);put(memory,0x63100044u,target_base);
    if(kind!=5)put(memory,target_row+0x70u,selected,2);
    put(memory,stack,0x12345678u);
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp={actor,0x22222222u,0x33333333u,0x44444444u,stack,0x55555555u,0x66666666u,0x77777777u};
    for(unsigned n=0;n<8;++n)for(unsigned lane=0;lane<4;++lane)
        abi.xmm[n][lane]=0x7fa00123u+n*17+lane;
    abi.eflags=0xed7u;abi.fs_base=0x69000000u;
    abi.x87[0]=3.25L;abi.x87[1]=-0.0L;abi.x87_depth=2;abi.x87_status=0x1820;
    const SourcePhysicsFactoryFloatingPoint fp{std::uint16_t(0x27fu|(rounding<<10)),0x1f80u|(rounding<<13)};
    abi.floating_point=fp;const auto before=abi;
    std::vector<unsigned> instructions;
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsWorldRuntimeServices services{};
    services.instruction=[&](unsigned pc){instructions.push_back(pc);};
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Selector delegated an unowned service");};
    source_run_ai_actor_contact_selector_tu15(memory,0x272200u,abi,fp,services,
        [&](const auto& store){stores.push_back(store);});
    check(abi.gp[4]==stack+4,"Original selector did not return RET0");
    for(const unsigned n:{3u,5u,6u,7u})check(abi.gp[n]==before.gp[n],"Selector clobbered a source nonvolatile register");
    check(abi.xmm==before.xmm&&abi.fs_base==before.fs_base&&abi.x87_depth==before.x87_depth&&
        abi.x87_status==before.x87_status&&abi.floating_point&&before.floating_point&&
        abi.floating_point->x87_control_word==before.floating_point->x87_control_word&&
        abi.floating_point->mxcsr==before.floating_point->mxcsr&&
        !abi.active_mmx,"Selector changed unrelated SIMD/FP/control state");
    for(unsigned n=0;n<abi.x87_depth;++n)check(abi.x87[n]==before.x87[n]&&
        std::signbit(abi.x87[n])==std::signbit(before.x87[n]),"Selector changed a live x87 operand");
    const bool absent=kind==0,null_lookup=kind==1||kind==5;
    const unsigned pointer=target_row+0x70u;
    const unsigned expected_eax=absent?(actor&0xffff0000u):null_lookup?0u:(pointer&0xffff0000u)|selected;
    check(abi.gp[0]==expected_eax,"Original MOV/XOR AX lost upper EAX bits or returned a fabricated contact value");
    check(abi.gp[1]==(absent||kind==1?0xffffffffu:target_row),"Original contact lookup ECX result differs");
    check(abi.gp[2]==(absent?before.gp[2]:kind==1?0x63000000u:0x63100000u),"Original contact lookup EDX clobber differs");
    unsigned expected_flags=before.eflags&~0x8d5u;
    if(absent||null_lookup)expected_flags|=0x44u;
    else {
        if(std::popcount(pointer&255u)%2==0)expected_flags|=4u;
        if(pointer&0x80000000u)expected_flags|=0x80u;
    }
    check((abi.eflags&~0x10u)==(expected_flags&~0x10u),"Selector flags follow loaded AX instead of original TEST/XOR instructions");
    check(word(memory,stack)==0x12345678u,"Selector overwrote the caller return identity");
    const bool helper=std::ranges::find(instructions,0x267410u)!=instructions.end();
    check(helper!=absent,"Selector bypassed the existing native lookup or called it on an absent contact");
    if(absent)check(stores.empty(),"Absent contact selector fabricated a call stack write");
    else {
        check(stores.size()==2&&stores[0].instruction==0x27220bu&&stores[0].address==stack-4&&
            stores[1].instruction==0x26741fu&&stores[1].address==stack-8,
            "Selector did not preserve the original CALL/PUSH ESI stack writes");
        check(word(memory,stack-4)==0x272210u&&word(memory,stack-8)==before.gp[6],
            "Selector stack footprint differs from the original nested lookup");
    }
}
void missing_lookup_bank(bool target_header) {
    constexpr unsigned actor=0x64000200u,stack=0x68000800u;
    SourceRuntimeMemory memory;
    memory.mount(0x64000000u,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,
        "AI selector missing-lookup actor");
    memory.mount(0x68000000u,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,
        "AI selector missing-lookup stack");
    put(memory,actor+0x338u,1);put(memory,stack,0x12345678u);
    if(target_header) {
        memory.mount(0x50eef0u,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,
            "AI selector retained contact header binding");
        memory.mount(0x63000000u,std::vector<std::byte>(0x48),SourceRuntimeMemory::Access::ReadWrite,
            "AI selector retained contact header");
        memory.mount(0x65000000u,std::vector<std::byte>(0x78),SourceRuntimeMemory::Access::ReadWrite,
            "AI selector retained contact rows");
        put(memory,0x50eef0u,0x63000000u);put(memory,0x63000044u,0x65000000u);
    }
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp={actor,0,0x33333333u,0,stack,0,0x66666666u,0};abi.eflags=0x202;
    std::vector<unsigned> instructions;
    SourcePhysicsWorldRuntimeServices services{};
    services.instruction=[&](unsigned pc){instructions.push_back(pc);};
    bool rejected=false;
    try{source_run_ai_actor_contact_selector_tu15(memory,0x272200u,abi,{0x27f,0x1f80},services);}
    catch(const std::exception&){rejected=true;}
    check(rejected,"Selector fabricated a missing nested lookup bank");
    check(abi.gp[4]==stack-(target_header?8u:4u)&&word(memory,stack-4)==0x272210u&&
        word(memory,stack)==0x12345678u,"Nested lookup failure lost its real partial call stack");
    check(!instructions.empty()&&instructions.back()==(target_header?0x26742du:0x267410u),
        "Selector failed at an unrelated owner instead of its missing bank");
    if(target_header)check(abi.gp[0]==0&&abi.gp[1]==0&&abi.gp[2]==0x63000000u&&
        abi.gp[6]==0x66666666u&&word(memory,stack-8)==0x66666666u,
        "Nested target lookup failure discarded original partial register/ESI state");
    else check(abi.gp[0]==actor&&abi.gp[1]==1&&abi.gp[2]==0x33333333u,
        "Nested contact lookup failure fabricated a result");
}
}
int main()try {
    check(source_ai_actor_contact_selector_tu15_owns_entry(0x272200u)&&
        !source_ai_actor_contact_selector_tu15_owns_entry(0x267410u),"Selector owner admits an unrelated entry");
    for(unsigned rounding=0;rounding<4;++rounding)for(unsigned kind=0;kind<6;++kind)run_case(kind,rounding);
    SourceRuntimeMemory missing;
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x64000200u;abi.gp[4]=0x68000800u;
    bool rejected=false;
    try{source_run_ai_actor_contact_selector_tu15(missing,0x272200u,abi,{0x27f,0x1f80},{});}
    catch(const std::exception&){rejected=true;}
    check(rejected,"Selector fabricated a missing actor bank");
    missing_lookup_bank(false);missing_lookup_bank(true);
    std::cout<<"PASS 24 full-ABI original-selector cases, existing lookup reuse, 3missing-bank guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
