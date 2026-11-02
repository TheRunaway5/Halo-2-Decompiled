#include "halo2/source_workshop_unit_controls_tu15.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include <iostream>
#include <set>

namespace {
using namespace halo2;
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void put(SourceRuntimeMemory &m,unsigned at,unsigned value,unsigned bytes=4) {
    std::array<std::byte,4> out{};
    for(unsigned n=0;n<bytes;++n)out[n]=std::byte(value>>(8*n));
    m.write(at,std::span(out).first(bytes));
}
unsigned get(const SourceRuntimeMemory &m,unsigned at){return Bytes(m.view(at,4)).u32(0);}
void node(SourceRuntimeMemory &m,unsigned index,unsigned opcode,unsigned type,unsigned flags,
          unsigned next,unsigned value) {
    const auto at=0x90004000u+index*20u;
    put(m,at,0x9000u+index,2);put(m,at+2,opcode,2);put(m,at+4,type,2);
    put(m,at+6,flags,2);put(m,at+8,next);put(m,at+16,value);
}
void exercise(bool jump,unsigned object,unsigned enabled,unsigned flags,bool reset=false) {
    constexpr unsigned pool=0x90002000,syntax_pool=0x90002100,thread=0x90003000,
        nodes=0x90004000,output=0x90007000,headers=0x90008000,entries=0x90009000,
        unit=0x9000a000,actions=0x9000b000,stack=0x60008000,frame=thread+0x40;
    const unsigned opcode=reset?479u:jump?480u:210u,descriptor=reset?0x45909cu:jump?0x4590acu:0x457b58u,
        evaluator=reset?0x2b2d80u:jump?0x2b2da0u:0x2ad5b0u,count=(jump||reset)?2u:4u;
    SourceRuntimeMemory m;
    for(const auto [at,size]:{std::pair(0x90000000u,0x10000u),std::pair(0x60000000u,0x10000u),
        std::pair(0x505e5cu,0x14u),std::pair(0x480c00u+opcode*4u,4u),
        std::pair(descriptor,20u),std::pair(0x502248u,0x3c10u),std::pair(0x4f3218u,4u),
        std::pair(0x4ecbd0u,4u),std::pair(0x4f9b54u,4u)})
        m.mount(at,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,
            "explicit original HS/unit-input composition fixture");
    put(m,0x505e5c,pool);put(m,0x505e60,1);put(m,0x505e6c,syntax_pool);
    put(m,pool+0x44,thread);put(m,syntax_pool+0x20,count);
    put(m,syntax_pool+0x24,20);put(m,syntax_pool+0x44,nodes);
    put(m,0x480c00u+opcode*4u,descriptor);put(m,descriptor,jump?5u:4u);
    put(m,descriptor+4,evaluator);put(m,descriptor+12,(jump||reset)?0u:2u,2);
    if(!jump&&!reset){put(m,descriptor+14,51,2);put(m,descriptor+16,5,2);}
    put(m,0x4ecbd0,headers);put(m,headers+0x44,entries);put(m,entries+2u*12u+8u,unit);
    put(m,unit+0x134,flags);put(m,0x4f9b54,actions);put(m,actions+4,flags);
    put(m,actions+8,flags^0xa5a5a5a5u);put(m,actions+12,flags^0x5a5a5a5au);
    node(m,0,opcode,jump?5u:4u,8,0xffffffffu,0x90010001u);
    node(m,1,2,2,9,(jump||reset)?0xffffffffu:0x90020002u,0);
    if(!jump&&!reset) {
        node(m,2,51,51,9,0x90030003u,object);
        node(m,3,5,5,9,0xffffffffu,enabled);
    }
    put(m,thread+2,1,1);put(m,thread+3,1,1);put(m,thread+0x10,frame);
    put(m,frame,thread+0x18);put(m,frame+4,0x90000000);
    put(m,thread+0x20,output);put(m,output,0xdeadbeef);
    put(m,thread+0xc,0x100,2);put(m,frame+0xc,0,2);put(m,stack,0xfeedcafe);
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp={0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0xa0010000};
    abi.eflags=0x202;const auto before=abi;
    SourcePhysicsWorldRuntimeServices services;std::set<unsigned> seen,dependencies;
    services.instruction=[&](unsigned pc){seen.insert(pc);};
    services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe) {
        dependencies.insert(call.target);
        if(source_workshop_unit_controls_tu15_owns_entry(call.target))
            source_run_workshop_unit_controls_tu15(memory,call.target,state,{0x37f,0x1f80},services,observe);
        else if(call.target==0x212010u || source_ai_frame_tu15_owns_entry(call.target))
            source_run_ai_frame_tu15(memory,call.target,state,{0x37f,0x1f80},services,observe);
        else if(source_campaign_runtime_tu15_owns_entry(call.target))
            source_run_campaign_runtime_tu15(memory,call.target,state,{0x37f,0x1f80},services,observe);
        else throw std::runtime_error("HS unit/input composition reached an unowned dependency: "+std::to_string(call.target));
    };
    source_run_campaign_runtime_tu15(m,0x212470,abi,{0x37f,0x1f80},services);
    abi=before;
    source_run_campaign_runtime_tu15(m,0x211b10,abi,{0x37f,0x1f80},services);
    require(dependencies.contains(evaluator),"Original HS dispatch did not invoke the native evaluator");
    require(seen.contains(0x211da0) && get(m,thread+0x10)==thread+0x18 && abi.gp[4]==stack+4,
        "Original HS completion/frame retirement/RET12 differs");
    for(unsigned r:{3u,5u,6u,7u})require(abi.gp[r]==before.gp[r],"HS composition changed a nonvolatile register");
    if(reset)require(get(m,actions+4)==0 && get(m,actions+8)==0 &&
        get(m,actions+12)==(flags^0x5a5a5a5au) && get(m,output)==0 && get(m,unit+0x134)==flags &&
        !seen.contains(0x212010),"Reset did not preserve its exact action/armed fields and no-argument completion");
    else if(jump)require(get(m,output)==((flags>>1u)&1u),"Jump test did not return its actual action bit");
    else {
        require(seen.contains(0x212010),"Unit control bypassed actual HS argument parsing");
        const auto expected=object==0xffffffffu?flags:
            (enabled&255u)?flags|2u:flags&~2u;
        require(get(m,unit+0x134)==expected && get(m,output)==0,
            "Parsed unit action changed unrelated flags or its void completion");
    }
}
}
int main()try {
    unsigned cases=0;
    for(unsigned flags:{0u,2u,0xffffffffu,0x12345678u,0x87654321u}) {
        exercise(true,0xffffffffu,0,flags);++cases;
        exercise(false,0xffffffffu,0,flags,true);++cases;
        for(unsigned object:{0xffffffffu,0x23450002u})
            for(unsigned enabled:{0u,1u,0x100u,0xffffff00u,0xffffffffu}) {
                exercise(false,object,enabled,flags);++cases;
            }
    }
    std::cout<<"PASS "<<cases<<" complete original HS dispatch/parser/completion and native unit/input evaluator compositions\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
