#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <array>
#include <functional>
#include <map>
namespace halo2 {
// The original successful driver-entry branch owns this event. Native remote
// input records need the same reset even without an Xbox local-view index.
struct SourceVehicleInputResetTu15 {
    std::uint32_t unit{},player{};
    std::array<std::byte,12> forward{};
};
using SourceVehicleInputResetHandlerTu15=std::function<void(const SourceVehicleInputResetTu15 &)>;
inline thread_local std::map<const SourcePhysicsContactMemory *,SourceVehicleInputResetHandlerTu15>
    source_vehicle_input_reset_handlers_tu15;
class SourceVehicleInputResetBindingTu15 {
    const SourcePhysicsContactMemory *memory_;
    SourceVehicleInputResetHandlerTu15 previous_;
public:
    SourceVehicleInputResetBindingTu15(const SourcePhysicsContactMemory &memory,SourceVehicleInputResetHandlerTu15 handler):memory_(&memory) {
        auto &slot=source_vehicle_input_reset_handlers_tu15[memory_];
        previous_=std::move(slot);slot=std::move(handler);
    }
    ~SourceVehicleInputResetBindingTu15() {
        if(previous_)source_vehicle_input_reset_handlers_tu15[memory_]=std::move(previous_);
        else source_vehicle_input_reset_handlers_tu15.erase(memory_);
    }
    SourceVehicleInputResetBindingTu15(const SourceVehicleInputResetBindingTu15 &)=delete;
};
inline void source_vehicle_input_reset_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t unit,std::uint32_t player,std::uint32_t forward) {
    const auto found=source_vehicle_input_reset_handlers_tu15.find(&memory);
    if(found==source_vehicle_input_reset_handlers_tu15.end())return;
    SourceVehicleInputResetTu15 event{unit,player,{}};memory.read(forward,event.forward);found->second(event);
}
}
