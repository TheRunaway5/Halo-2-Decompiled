#include "halo2/source_damage_runtime.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include <stdexcept>

namespace halo2 {
bool source_damage_owns_entry_1_1(std::uint32_t entry) {
    return source_damage_delivery_owns_entry(entry);
}
void source_run_damage_1_1(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourceUnitAnimationRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!source_damage_owns_entry_1_1(entry))
        throw std::runtime_error("Required original damage entry unavailable");
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &o) {
        if(source_damage_owns_entry_1_1(call.target))
            source_run_damage_1_1(bank,call.target,state,state.floating_point.value_or(fp),joined,o);
        else {
            if(!services.required_owner)
                throw std::runtime_error("Required original damage callee unavailable");
            services.required_owner(bank,call,state,o);
        }
    };
    SourcePhysicsContactRuntimeServices raw;
    raw.instruction=joined.instruction;raw.timestamp_counter=joined.timestamp_counter;
    if(joined.estimate)raw.reciprocal_sqrt=[&](auto x) {
        return joined.estimate(SourceUnitAnimationEstimate::ReciprocalSquareRoot,x);
    };
    raw.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &o) {
        auto full=abi;static_cast<SourcePhysicsContactRuntimeAbi &>(full)=state;
        try {joined.required_owner(bank,call,full,o);}
        catch(...) {state=static_cast<const SourcePhysicsContactRuntimeAbi &>(full);abi.mmx=full.mmx;throw;}
        state=static_cast<const SourcePhysicsContactRuntimeAbi &>(full);abi.mmx=full.mmx;
    };
    const auto controls=abi.floating_point.value_or(fp);
    source_run_damage_delivery_runtime(memory,entry,abi,
        SourcePhysicsContactImage::MapPack11,controls,raw,observe);
}
} // namespace halo2
