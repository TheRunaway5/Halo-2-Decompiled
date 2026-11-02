#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ComponentBodyMotionServices {
    // Actual1D8B90 writes scale, basis and translation (13 source words).
    // This is a semantic frame result, not any original register/stack image.
    std::function<std::array<std::uint32_t,13>(std::uint32_t component,std::uint32_t index,
        NativeTu15PhysicsFrameFp&)>component_frame;
};
struct NativeTu15ComponentBodyMotionStats {
    std::uint64_t calls{},position_queries{},component_frames{},body_motion_queries{},
        disabled_bodies{},stores{},unbound_services{};
};
// Actual1D8B20's position query and1D7260's nine-word retained motion snapshot.
// Current component/body/motion pointers and their source read order survive;
// original private vector temporaries become ordinary typed local values.
std::array<std::uint32_t,3> native_tu15_component_body_position(SourcePhysicsContactMemory&,
    std::uint32_t component,std::uint32_t index,NativeTu15PhysicsFrameFp&,
    const NativeTu15ComponentBodyMotionServices& = {},NativeTu15ComponentBodyMotionStats* = nullptr);
void native_tu15_publish_component_body_motion(SourcePhysicsContactMemory&,
    std::uint32_t component,std::uint32_t index,NativeTu15PhysicsFrameFp&,
    const NativeTu15ComponentBodyMotionServices& = {},
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ComponentBodyMotionStats* = nullptr);
}
