#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "gameplay/objects/native_tu15_object_body_source_frame.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::byte,16>bank{};State*state{};
    std::uint32_t pc{};unsigned selected{},phase{},foreign_pc{};
    const void*foreign_owner{};mutable unsigned reads{};unsigned writes{},observations{};
    mutable J events=J::array();
    void event(const char*kind)const{events.push_back(J{{"kind",kind},{"pc",pc},{"gp",state->gp},{"flags",state->flags.bits()}});}
    void stop(unsigned at_phase,unsigned ordinal)const{
        if(at_phase==phase&&ordinal==selected)
            throw native_object_body_detail::SourceContinuation{foreign_pc,foreign_owner};
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        event("read");stop(0u,reads++);
        if(address<0x620000f0u||std::uint64_t(address-0x620000f0u)+out.size()>bank.size())throw std::runtime_error("Foreign read escaped bank");
        std::memcpy(out.data(),bank.data()+address-0x620000f0u,out.size());
    }
    void write(std::uint32_t address,std::span<const std::byte>value)override{
        event("write");const auto ordinal=writes++;stop(1u,ordinal);
        if(address<0x620000f0u||std::uint64_t(address-0x620000f0u)+value.size()>bank.size())throw std::runtime_error("Foreign write escaped bank");
        std::memcpy(bank.data()+address-0x620000f0u,value.data(),value.size());stop(2u,ordinal);
    }
};
J run(bool native,bool observed,unsigned phase,unsigned ordinal,unsigned foreign_pc,bool identified){
    Memory memory;memory.phase=phase;memory.selected=ordinal;memory.foreign_pc=foreign_pc;
    static const unsigned foreign_token{};memory.foreign_owner=identified?&foreign_token:nullptr;
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    if(observed)observer=[&](const auto&){memory.event("observer");memory.stop(3u,memory.observations++);};
    State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,0x62000100u,6,0xaabbccddu,0x11223344u};
    abi.eflags=0x646u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};state.load(abi);
    bool foreign{};unsigned caught_pc{},resume{};
    try{
        if(native){
            unsigned retired{};
            resume=native_object_body_detail::run_source_frame(state,[&](auto&){return ++retired<2u;},[&](auto&,auto pc){memory.pc=pc;});
        }else{
            memory.pc=0xbcdf0u;state.push(0xbcdf0u,state.gp[6]);
            memory.pc=0xbcdf1u;state.push(0xbcdf1u,state.gp[7]);
        }
    }catch(const native_object_body_detail::SourceContinuation&error){
        if(error.owner!=memory.foreign_owner)throw std::runtime_error("Foreign continuation owner identity changed");
        foreign=true;caught_pc=error.pc;
    }
    std::vector<unsigned>bank;for(auto value:memory.bank)bank.push_back(std::to_integer<unsigned>(value));
    // The native-only handoff is checked separately and is not a source result.
    if(!foreign&&native&&resume!=0xbcdf2u)throw std::runtime_error("Foreign fixture missed exact completed two-PUSH boundary");
    return J{{"gp",state.gp},{"flags",state.flags.bits()},{"events",memory.events},{"bank",bank},{"foreign",foreign},{"caught_pc",caught_pc}};
}
}
int main()try{
    unsigned comparisons{},exceptions{};
    for(bool observed:{false,true})for(unsigned phase=0;phase<4u;++phase)for(unsigned ordinal:{0u,1u})for(unsigned pc:{0u,0xbcdf1u,0xbce48u,0xdeadbeefu})for(bool identified:{false,true}){
        const auto expected=run(false,observed,phase,ordinal,pc,identified),actual=run(true,observed,phase,ordinal,pc,identified);
        if(expected!=actual)throw std::runtime_error("Object lookup foreign exception differs expected="+expected.dump()+" actual="+actual.dump());
        ++comparisons;exceptions+=expected.at("foreign").get<bool>();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"actual_provider_exceptions",exceptions},{"reference_uses_actual_State_push_and_w",true}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
