#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Actual18A2A0: publish yaw/pitch from the current direction into the selected
// retained 0x94-byte player input. Reads remain lazy, including when the input
// aliases the direction. Native x87 owns atan2/sqrt and original spill rounding.
// This leaf does not advance the18A300 look controller.
void native_tu15_look_direction(SourcePhysicsContactMemory&,std::uint32_t direction,
    std::uint32_t input_index,NativeTu15PhysicsFrameFp&,unsigned actual_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
}
