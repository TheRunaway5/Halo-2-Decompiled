#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp"
#include <iostream>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;
namespace reference {
static void run_313390(WorldState&){throw std::runtime_error("Unexpected child in first-PUSH witness");}
#include "edge_observer_reference.inc"
}
namespace {
struct Stop{};
struct Memory:SourcePhysicsContactMemory {
    SourcePhysicsContactRecordObserver&observer;unsigned&events;
    explicit Memory(SourcePhysicsContactRecordObserver&o,unsigned&n):observer(o),events(n){}
    void read(std::uint32_t,std::span<std::byte>)const override{throw Stop{};}
    void write(std::uint32_t,std::span<const std::byte>)override{observer=[this](const auto&){++events;};}
};
unsigned run(bool native){
    unsigned events{};SourcePhysicsContactRecordObserver observer;Memory memory{observer,events};
    SourcePhysicsWorldRuntimeServices services;std::uint32_t position{};
    services.instruction=[&](std::uint32_t pc){position=pc;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&position;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.gp[4]=0x6100e000u;
    const auto native_before=native_solver_hull_edge_owner_detail::activity().calls;bool stopped{};
    try{
        if(native)native_solver_hull_edge_owner_detail::bridge(state,[](auto&,auto,auto){throw std::runtime_error("Unexpected child");});
        else reference::owner(state);
    }catch(const Stop&){stopped=true;}
    if(!stopped||(native&&native_solver_hull_edge_owner_detail::activity().calls!=native_before+1u))throw std::runtime_error("First-PUSH witness did not enter native code and reach the real provider stop");
    return events;
}
}
int main(){const auto expected=run(false),actual=run(true);std::cout<<"{\"expected_completed_events\":"<<expected<<",\"actual_completed_events\":"<<actual<<"}\n";return actual==expected?0:1;}
