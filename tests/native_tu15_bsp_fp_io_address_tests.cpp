#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_bsp_surface_2d.hpp"
#include <iostream>
#include <map>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
struct Stop{};
struct ObservedState:WorldState{using WorldState::WorldState;bool selected_observation_pcs=true;};
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct Memory:SourcePhysicsContactMemory{
    WorldState*state{};mutable unsigned reads{};unsigned writes{};
    mutable std::uint32_t read_address{};std::uint32_t write_address{};std::array<std::byte,4>written{};
    void read(std::uint32_t address,std::span<std::byte>data)const override{
        check(data.size()==4,"Unexpected preimage width");
        read_address=address;++reads;state->gp[4]+=0x40u;
        std::fill(data.begin(),data.end(),std::byte{0});
    }
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        check(data.size()==4,"Unexpected stack-store width");++writes;write_address=address;
        std::copy(data.begin(),data.end(),written.begin());
    }
};
auto execute(bool native){
    Memory memory;std::uint32_t position{};bool tracing{};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){position=pc;};
    services.instruction_cursor={&position,&tracing,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    SourcePhysicsContactRecordStore observed;unsigned observers{};
    SourcePhysicsContactRecordObserver observer=[&](const auto&event){observed=event;++observers;throw Stop{};};
    ObservedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;
    state.gp[0]=0u;state.gp[4]=0x61000800u;state.gp[6]=0x11223344u;
    bool stopped{};
    try{if(native){auto before=[](auto&,auto){};native_bsp_surface_2d_detail::bridge(state,before);}
        else state.push(0x2a0237u,state.gp[6]);}
    catch(const Stop&){stopped=true;}
    check(stopped&&observers==1&&memory.reads==1&&memory.writes==1,"Stack-store boundary/side effects changed");
    check(state.gp[4]==0x6100083cu,"Preimage callback SP mutation was lost");
    check(memory.read_address==0x610007fcu&&memory.write_address==0x610007fcu&&observed.address==0x610007fcu,
          "Stack write followed mutated SP instead of captured address");
    const std::array expected{std::byte{0x44},std::byte{0x33},std::byte{0x22},std::byte{0x11}};
    check(memory.written==expected,"Saved SI bytes changed");
    return std::tuple{memory.read_address,memory.write_address,memory.written,state.gp[4],observed.before,observed.written};
}
}
int main()try{check(execute(false)==execute(true),"Native push differs from the real retained State::push contract");
    std::cout<<"{\"passed\":true,\"retained_native_address_capture_comparisons\":1}\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
