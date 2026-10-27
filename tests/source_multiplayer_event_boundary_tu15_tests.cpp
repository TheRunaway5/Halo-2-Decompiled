#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <iostream>
#include <map>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        for(auto& b:out){const auto at=bytes.find(p++);if(at==bytes.end())throw std::runtime_error("Missing declared native event operand");b=at->second;}
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override{for(auto b:in)bytes.at(p++)=b;}
    void put(std::uint32_t p,std::uint32_t value,unsigned n=4){for(unsigned i=0;i<n;++i)bytes[p+i]=std::byte(value>>(8*i));}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4> value;read(p,value);return std::bit_cast<std::uint32_t>(value);}
};
struct Boundary {};
struct Result {unsigned native_calls{},required_calls{};std::array<std::uint32_t,8> words{};std::uint16_t data{};};
Result *result;
// Stop at the real native invocation boundary. No callee return, selected
// sound, source admission result, local-view mask or voice is supplied.
static void run_1a8740(WorldState& s) {
    ++result->native_calls;
    need(static_cast<Memory&>(s.memory).word(s.gp[4])==0x2c5dc5,"Original event CALL return address changed");
    for(unsigned i=0;i<8;++i)result->words[i]=static_cast<Memory&>(s.memory).word(s.gp[7]+i*4u);
    result->data=std::uint16_t(static_cast<Memory&>(s.memory).word(s.gp[7]+32u));
    throw Boundary{};
}
static void run_b8050(WorldState&){throw std::runtime_error("Unowned mode5 dependency reached the native mode1/3/4 probe");}
#include HALO2_MULTIPLAYER_EVENT_CALLER
Result run(unsigned mode) {
    Memory memory;constexpr std::uint32_t game=0x1000000,stack=0x2000100,player=0x12340007;
    memory.put(0x4f3218,game);memory.put(game+0xc,mode);
    for(unsigned i=0;i<0x200;++i)memory.put(stack-0x100+i,0,1);
    memory.put(stack,0x60300000);memory.put(stack+4,player);
    Result output;result=&output;
    SourcePhysicsContactRuntimeServices services;
    services.required_owner=[&](auto& m,const auto& call,auto& abi,const auto& stores) {
        need(&m==&memory && !call.tail && call.instruction==0x2c5dc0 && call.target==0x1a8740,
            "Native event observation escaped its actual CALL boundary");
        ++output.required_calls;WorldState callee(m,stores,services,SourcePhysicsContactImage::TitleUpdate15);
        callee.load(abi);run_1a8740(callee);
    };
    SourcePhysicsContactRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=2;
    const SourcePhysicsContactRecordObserver stores;
    WorldState state(memory,stores,services,SourcePhysicsContactImage::TitleUpdate15);state.load(abi);
    try{run_2c5d70(state);}catch(const Boundary&){}
    if(mode==4)need(!output.native_calls && state.save().gp[4]==stack+8,"Original mode4 event suppression or return ABI changed");
    else {
        need(output.native_calls==1 && output.data==0xffffu && output.words==std::array<std::uint32_t,8>{2,0,player,
            0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0},"Original Slayer activation packet changed");
    }
    return output;
}
}
int main()try {
    unsigned calls{},observed{};
    for(unsigned mode:{1u,3u,4u}){const auto row=run(mode);calls+=row.native_calls;observed+=row.required_calls;}
    std::cout<<"native_activation_calls="<<calls<<" observable_required_calls="<<observed<<'\n';
    need(calls==2 && observed==calls,"Actual native Slayer event bypassed its existing publication boundary");
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
