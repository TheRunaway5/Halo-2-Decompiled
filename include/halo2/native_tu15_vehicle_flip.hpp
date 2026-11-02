#pragma once
#include "halo2/native_tu15_object_motion.hpp"
namespace halo2 {
struct NativeTu15VehicleFlipServices {
    std::function<NativeTu15Velocities(std::uint32_t object,NativeTu15PhysicsFrameFp&)>get_velocities;
    std::function<void(std::uint32_t object,const Vec3&linear,const Vec3&angular,
        NativeTu15PhysicsFrameFp&)>set_velocities;
};
struct NativeTu15VehicleFlipStats {
    std::uint64_t calls{},flag_skips{},direction_skips{},timeouts{},orientation_skips{},
        get_calls{},set_calls{},completed{},unbound_getters{},unbound_setters{};
};
// Actual TU1.5 F1DD0. The result is its boolean AL contract; all work uses
// ordinary object/velocity data and native SSE/x87, never guest register state.
bool native_tu15_flip_vehicle(SourcePhysicsContactMemory&,std::uint32_t object,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15VehicleFlipServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15VehicleFlipStats* = nullptr);
} // namespace halo2
