#include "halo2/source_damage_model_children_tu15.hpp"
#include "gameplay/objects/source_damage_model_children_native_tu15_internal.hpp"

namespace halo2 {
void source_run_damage_model_children_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    using namespace physics_world_runtime_detail;
    FpScope scope(fp);WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try {
        damage_model_children_native_tu15(state,[&](std::uint32_t pc,bool publish) {
            if(publish && services.instruction)services.instruction(pc);return pc;
        },[&]{state.required(0xdc619u,0xd9110u,false);});
    } catch(...) {abi=state.save();throw;}
    abi=state.save();
}
std::uint64_t source_damage_model_children_native_activity() {
    return physics_world_runtime_detail::damage_model_children_native_calls.load(std::memory_order_relaxed);
}
}
