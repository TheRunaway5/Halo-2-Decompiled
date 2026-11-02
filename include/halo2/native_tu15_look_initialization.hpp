#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15LookInitializationStats {
    std::uint64_t calls{},completed{},unchanged{},cleared_records{},unit_directions{},refusals{};
};
// Whole 18A0C0. Source index and full unit datum are ordinary scalar inputs.
// Preserves each REP-STOSD store and every later redundant field publication.
// Positive unit direction math requires masked FP and incoming x87 depth<=4.
void native_tu15_initialize_look_controller(SourcePhysicsContactMemory&,
    std::uint32_t index,std::uint32_t unit_datum,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15LookInitializationStats* = nullptr);
}
