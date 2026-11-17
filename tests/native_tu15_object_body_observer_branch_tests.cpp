#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "gameplay/objects/native_tu15_object_body_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_body_canonical_sites.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::byte,16>bank{};State*state{};
    SourcePhysicsContactRecordObserver*observer{};
    std::uint32_t pc{};unsigned phase{},selected{},action{};mutable unsigned reads{};unsigned writes{},observations{};
    mutable J events=J::array();mutable unsigned mutations{};
    void event(const char*kind)const{events.push_back(J{{"kind",kind},{"pc",pc},{"gp",state->gp},{"flags",state->flags.bits()}});}
    void change(unsigned at_phase,unsigned ordinal)const{
        if(at_phase!=phase||ordinal!=selected)return;
        ++mutations;
        if(action==0u)*observer=[this](const auto&value){event("installed-observer");events.push_back(J{{"instruction",value.instruction},{"address",value.address},{"before",value.before.size()},{"written",value.written.size()}});};
        if(action==1u)*observer={};
        if(action==2u)*observer=[this](const auto&value){event("rebound-observer");events.push_back(J{{"instruction",value.instruction},{"address",value.address},{"before",value.before.size()},{"written",value.written.size()}});};
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        event("read");if(address<0x620000f0u||std::uint64_t(address-0x620000f0u)+out.size()>bank.size())throw std::runtime_error("Observer read escaped bank");
        std::memcpy(out.data(),bank.data()+address-0x620000f0u,out.size());change(0u,reads++);
    }
    void write(std::uint32_t address,std::span<const std::byte>value)override{
        event("write");if(address<0x620000f0u||std::uint64_t(address-0x620000f0u)+value.size()>bank.size())throw std::runtime_error("Observer write escaped bank");
        std::memcpy(bank.data()+address-0x620000f0u,value.data(),value.size());change(1u,writes++);
    }
};
J run(bool native,bool initially_observed,unsigned phase,unsigned ordinal,unsigned action,bool canonical){
    Memory memory;memory.phase=phase;memory.selected=ordinal;memory.action=action;
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    if(initially_observed)observer=[&](const auto&value){memory.event("original-observer");memory.events.push_back(J{{"instruction",value.instruction},{"address",value.address},{"before",value.before.size()},{"written",value.written.size()}});};
    State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;memory.observer=&observer;
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,0x62000100u,6,0xaabbccddu,0x11223344u};
    abi.eflags=0x646u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};state.load(abi);
    if(native){
        unsigned retired{};const auto continuation=[&](auto&){return ++retired<2u;};const auto before=[&](auto&,auto pc){memory.pc=pc;};
        const auto next=canonical?native_object_body_detail::run_source_frame(state,continuation,before,native_object_body_detail::CanonicalSites{}):
            native_object_body_detail::run_source_frame(state,continuation,before);
        if(next!=(canonical?0xbbcb2u:0xbcdf2u))throw std::runtime_error("Observer fixture failed exact two-PUSH boundary");
    }else{
        memory.pc=canonical?0xbbcb0u:0xbcdf0u;state.push(memory.pc,state.gp[6]);
        memory.pc=canonical?0xbbcb1u:0xbcdf1u;state.push(memory.pc,state.gp[7]);
    }
    std::vector<unsigned>bank;for(auto value:memory.bank)bank.push_back(std::to_integer<unsigned>(value));
    return J{{"gp",state.gp},{"flags",state.flags.bits()},{"events",memory.events},{"bank",bank},{"mutations",memory.mutations}};
}
}
int main()try{
    unsigned comparisons{},mutations{};
    for(bool canonical:{false,true})for(bool observed:{false,true})for(unsigned phase:{0u,1u})for(unsigned ordinal:{0u,1u})for(unsigned action=0;action<3u;++action){
        const auto expected=run(false,observed,phase,ordinal,action,canonical),actual=run(true,observed,phase,ordinal,action,canonical);
        if(expected!=actual)throw std::runtime_error("Object lookup observer branch differs expected="+expected.dump()+" actual="+actual.dump());
        ++comparisons;mutations+=expected.at("mutations").get<unsigned>();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"real_observer_replacements",mutations},
        {"reference_uses_actual_State_push_and_w",true},{"actual_and_canonical_sites",true}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
