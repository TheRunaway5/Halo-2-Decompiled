#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15CameraBasisStats {
    std::uint64_t calls{},completed{},fallbacks{},refusals{};
};
// Exact E3310 scalar return. The camera consumes its low boolean byte;
// the remaining return bits are derived from the same current object index.
std::uint32_t native_tu15_unit_camera_transition(const SourcePhysicsContactMemory&,
    std::uint32_t object_datum);
// Whole 11FA50. Direction and destination are ordinary three-word DATA;
// the current direction is reread before the three ordered output stores.
// Masked x87/SIMD profiles with at most six incoming x87 values are admitted.
std::uint32_t native_tu15_camera_perpendicular(SourcePhysicsContactMemory&,
    std::uint32_t direction,std::uint32_t destination,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15CameraBasisStats* = nullptr);
}
