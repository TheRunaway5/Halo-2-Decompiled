#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <set>

namespace {
using namespace halo2;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& m,unsigned a,unsigned value,unsigned bytes=4) {
    std::array<std::byte,4> out{};for(unsigned n=0;n<bytes;++n)out[n]=std::byte(value>>(n*8));
    m.write(a,std::span(out).first(bytes));
}
unsigned get(const SourceRuntimeMemory& m,unsigned a){return Bytes(m.view(a,4)).u32(0);}
template<class F>void rejected(F&& f){bool bad=false;try{f();}catch(const FormatError&){bad=true;}require(bad,"Invalid MCC builtin receipt admitted");}
class Unmapped final:public SourcePhysicsContactMemory {
public:
    void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("Unbound getter read memory");}
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Unbound getter wrote memory");}
};
struct Fixture {
    static constexpr unsigned scenario=0x90001000,thread_pool=0x90002000,syntax_pool=0x90002100,
        thread=0x90003000,nodes=0x90004000,strings=0x90005000,clock=0x90006000,
        output=0x90007000,stack=0x60008000,datum=0x92340000,frame=thread+0x40;
    SourceRuntimeMemory memory;
    std::string name;
    MccScriptLayout layout;
    std::unique_ptr<NativeMccScriptBuiltinProfile> profile;
    Fixture(unsigned opcode,unsigned context):name(opcode==0x38f?"game_tick_rate":"game_tick_rate_scalar") {
        for(const auto [address,size]:{std::pair(0x90000000u,0x10000u),std::pair(0x60000000u,0x10000u),
            std::pair(0x4ecc20u,4u),std::pair(0x4f3218u,4u),std::pair(0x51df54u,4u),
            std::pair(0x505e5cu,0x14u),std::pair(0x502248u,0x3c10u)})
            memory.mount(address,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,"explicit HS unit fixture");
        put(memory,0x4ecc20,scenario);put(memory,0x505e5c,thread_pool);put(memory,0x505e60,1);
        put(memory,0x505e6c,syntax_pool);put(memory,0x51df54,clock);
        put(memory,scenario+0x238,2);put(memory,scenario+0x23c,nodes);
        put(memory,scenario+0x1b0,name.size()+1);put(memory,scenario+0x1b4,strings);
        put(memory,syntax_pool+0x20,2);put(memory,syntax_pool+0x24,20);put(memory,syntax_pool+0x44,nodes);
        put(memory,thread_pool+0x44,thread);
        put(memory,nodes,datum>>16,2);put(memory,nodes+2,opcode,2);put(memory,nodes+4,context,2);
        put(memory,nodes+6,8,2);put(memory,nodes+8,0xffffffff);put(memory,nodes+16,0x92350001);
        put(memory,nodes+20,0x9235,2);put(memory,nodes+22,2,2);put(memory,nodes+24,2,2);
        put(memory,nodes+26,9,2);put(memory,nodes+28,0xffffffff);
        const auto names=std::as_bytes(std::span(name.c_str(),name.size()+1));memory.write(strings,names);
        layout=project_mcc_script_layout(memory.view(nodes,40),names,{}, {true});
        profile=std::make_unique<NativeMccScriptBuiltinProfile>(layout,names);
        require(profile->calls()==1 && profile->owns(layout.requirements.at(0)),"Genuine clock receipt not owned");
        // Construct the original scalar-cast table by executing212470, not by
        // supplying cast callbacks or writing preconstructed source results.
        auto abi=call_input();SourcePhysicsWorldRuntimeServices services;
        source_run_campaign_runtime_tu15(memory,0x212470,abi,{0x37f,0x1f80},services);
        require(get(memory,0x502930)==0x2123e0 && get(memory,0x502838)==0x212360,
            "Original scalar-cast constructor did not publish real callbacks");
    }
    SourcePhysicsWorldRuntimeAbi call_input() {
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,0x23456789,0x3456789a,
            0x456789ab,stack,0x56789abc,0x6789abcd,0xa0010000};abi.eflags=0x202;
        put(memory,stack,0xfeedcafe);return abi;
    }
    std::uint32_t run(int rate,bool ai=false) {
        put(memory,clock+2,std::uint16_t(rate),2);put(memory,thread+2,1,1);put(memory,thread+3,1,1);
        put(memory,thread+8,0);put(memory,thread+0x10,frame);put(memory,frame,thread+0x18);
        put(memory,frame+4,datum);put(memory,thread+0x20,output);put(memory,output,0xdeadbeef);
        auto abi=call_input();const auto before=abi;std::set<unsigned> seen;
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](unsigned pc){seen.insert(pc);};
        services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe) {
            if(call.target!=0x212360 && call.target!=0x2123e0)
                throw std::runtime_error("Synthetic HS thread reached an unexpected required owner");
            source_run_campaign_runtime_tu15(bank,call.target,state,{0x37f,0x1f80},services,observe);
        };
        if(ai)source_run_ai_frame_tu15(memory,0x211b10,abi,{0x37f,0x1f80},services);
        else source_run_campaign_runtime_tu15(memory,0x211b10,abi,{0x37f,0x1f80},services);
        require(abi.gp[4]==stack+4,"Native builtin wrapper/completion RET12 stack differs");
        for(const auto n:{3,5,6,7})require(abi.gp[n]==before.gp[n],"Native getter changed a nonvolatile caller register");
        require(get(memory,thread+0x10)==thread+0x18 && get(memory,0x505e64)==0xffffffff,
            "Original completion did not retire its actual thread frame");
        require(seen.contains(0x211da0) && !seen.contains(0x211c0d),"Getter did not bypass the Xbox descriptor collision through real completion");
        const auto type=Bytes(memory.view(nodes,20)).u16(4);
        if(type==7)require(seen.contains(0x2123e0),"Original LONG to SHORT body was bypassed");
        if(type==6)require(seen.contains(0x212360),"Original LONG to REAL body was bypassed");
        return get(memory,output);
    }
};
}
int main()try {
    Unmapped unbound;
    require(!native_mcc_script_builtin_value(unbound,0xffffffff) &&
        !native_mcc_script_builtin_return_type(unbound,0xffffffff),"Unbound bank did not retain ordinary dispatch");
    require(source_ai_frame_tu15_owns_entry(0x211da0) && source_campaign_runtime_tu15_owns_entry(0x212360) &&
        source_campaign_runtime_tu15_owns_entry(0x2123e0),"Actual completion/cast ownership is not exported");
    unsigned completions=0;
    for(const unsigned opcode:{0x38fu,0x390u})for(const unsigned context:{6u,7u,8u}) {
        Fixture f(opcode,context);NativeMccScriptBuiltinBinding binding(f.memory,*f.profile);
        require(f.memory.native_address(Fixture::nodes,40).value>UINT32_MAX,"HS fixture did not exercise real host64 backing");
        for(const bool ai:{false,true})for(const int rate:{-32768,-61,-31,-30,-1,0,1,29,30,31,60,61,120,32767}) {
            const auto integer=opcode==0x38f?rate:rate/30;
            const auto expected=context==6?std::bit_cast<std::uint32_t>(float(integer)):std::uint32_t(integer);
            require(f.run(rate,ai)==expected,"Dynamic original clock getter/cast result differs");++completions;
        }
        {NativeMccScriptBuiltinBinding nested(f.memory,*f.profile);require(f.run(60)==(context==6?
            std::bit_cast<std::uint32_t>(opcode==0x38f?60.f:2.f):opcode==0x38f?60u:2u),"Nested binding lost ownership");}
        const auto snapshot=f.memory.checkpoint();SourceRuntimeMemory replacement;replacement.restore(snapshot);
        f.memory.swap_backing(replacement);require(f.run(30)==(context==6?std::bit_cast<std::uint32_t>(opcode==0x38f?30.f:1.f):opcode==0x38f?30u:1u),
            "Clock binding retained stale native pointers after backing swap");
        put(f.memory,Fixture::nodes+20,0x1111,2);
        rejected([&]{native_mcc_script_builtin_return_type(f.memory,Fixture::nodes);});
        put(f.memory,Fixture::nodes+20,0x9235,2);
        put(f.memory,Fixture::strings,'X',1);
        rejected([&]{native_mcc_script_builtin_value(f.memory,Fixture::nodes);});
        put(f.memory,Fixture::strings,'g',1);
        put(f.memory,Fixture::syntax_pool+0x44,Fixture::nodes+20);
        rejected([&]{native_mcc_script_builtin_value(f.memory,Fixture::nodes);});
        put(f.memory,Fixture::syntax_pool+0x44,Fixture::nodes);
        put(f.memory,Fixture::nodes,0x1111,2);rejected([&]{native_mcc_script_builtin_value(f.memory,Fixture::nodes);});
    }
    std::cout<<"PASS native MCC clock getters: "<<completions<<" original thread completions; dynamic signed clock, scalar division, real casts, RET12, identity, nested binding and backing restore\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
