#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "gameplay/objects/native_tu15_object_body_frame.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
namespace{
struct Fault{};
struct Memory final:SourcePhysicsContactMemory{
    std::array<std::byte,16>bank{};State*state{};std::uint32_t pc{};std::string phase;unsigned ordinal{};
    mutable unsigned reads{};unsigned writes{},observations{};mutable J events=J::array();
    void event(const char*kind)const{const auto a=state->save();events.push_back(J{{"kind",kind},{"pc",pc},{"gp",a.gp},{"flags",a.eflags},{"cw",a.floating_point->x87_control_word},{"sw",a.x87_status},{"mxcsr",a.floating_point->mxcsr}});}
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        event("preimage");if(phase=="preimage"&&reads++==ordinal)throw Fault{};
        if(p<0x620000f0u||std::uint64_t(p-0x620000f0u)+out.size()>bank.size())throw std::runtime_error("Push read escaped bank");
        std::memcpy(out.data(),bank.data()+p-0x620000f0u,out.size());
    }
    void write(std::uint32_t p,std::span<const std::byte>raw)override{
        event("write");if(phase=="write"&&writes++==ordinal)throw Fault{};
        if(p<0x620000f0u||std::uint64_t(p-0x620000f0u)+raw.size()>bank.size())throw std::runtime_error("Push write escaped bank");
        std::memcpy(bank.data()+p-0x620000f0u,raw.data(),raw.size());
    }
};
J run(unsigned mode,const std::string&phase,unsigned ordinal){
    Memory memory;memory.phase=phase;memory.ordinal=ordinal;SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){memory.pc=pc;};
    SourcePhysicsContactRecordObserver observer=[&](const auto&){memory.event("observer");if(phase=="observer"&&memory.observations++==ordinal)throw Fault{};};
    State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;
    SourcePhysicsWorldRuntimeAbi a{};a.gp={1,2,3,4,0x62000100u,6,0xaabbccddu,0x11223344u};a.eflags=0x646u;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};state.load(a);
    bool stopped{};try{
        if(mode==0){
            // These are the exact first two operations of the retained body.
            // The real shared State::push and w/observer helpers execute.
            if(state.services.instruction)state.services.instruction(0xbcdf0u);state.push(0xbcdf0u,state.gp[6]);
            if(state.services.instruction)state.services.instruction(0xbcdf1u);state.push(0xbcdf1u,state.gp[7]);
        }else if(mode==1)native_object_body_detail::run_source_provider(state,[&](auto&,auto pc){memory.pc=pc;});
        else native_object_body_detail::run_private(state,[&](auto&,auto pc){memory.pc=pc;});
    }catch(const Fault&){stopped=true;}
    if(!stopped)throw std::runtime_error("Push failure fixture escaped its two-instruction boundary");
    a=state.save();std::vector<unsigned>bank;for(auto byte:memory.bank)bank.push_back(std::to_integer<unsigned>(byte));
    return J{{"gp",a.gp},{"flags",a.eflags},{"events",memory.events},{"bank",bank}};
}
}
int main()try{
    unsigned comparisons{},architectural_differences{};
    for(const auto&phase:{"preimage","write","observer"})for(unsigned ordinal:{0u,1u}){
        const auto source=run(0,phase,ordinal),native=run(1,phase,ordinal),raw=run(2,phase,ordinal);
        if(source!=native)throw std::runtime_error("Source PUSH provider publication differs "+std::string(phase)+" source="+source.dump()+" native="+native.dump());
        ++comparisons;architectural_differences+=source!=raw;
    }
    if(architectural_differences!=6u)throw std::runtime_error("Physical/source PUSH distinction was not witnessed");
    std::cout<<J{{"passed",true},{"source_provider_comparisons",comparisons},{"architectural_protocol_differences",architectural_differences},{"whole_source_or_default_qualified",false}}.dump()<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
