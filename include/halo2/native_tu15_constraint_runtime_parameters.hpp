#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintRuntimeParameterUpdate {
    std::uint32_t position{},bounds{},frame{}; // Optional ordinary DATA addresses.
    std::uint32_t primary_scalar_bits{},secondary_scalar_bits{};
};
struct NativeTu15ConstraintRuntimeParameterStats {
    std::uint64_t calls{},none_skips{},index_skips{},salt_skips{},valid_handles{},
        positions{},bounds{},frame_lists{},frame_nodes{},link_flag_updates{},
        link_creation_calls{},link_creation_none{},links_created{},allocation_failures{};
};
// Actual17DEC0. The thirteen-word frame copy admits the ordinary forward
// copy caller profile; source and destination must not overlap.
bool native_tu15_update_constraint_runtime_parameters(SourcePhysicsContactMemory&,
    std::uint32_t handle,NativeTu15ConstraintRuntimeParameterUpdate,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintRuntimeParameterStats* = nullptr);
// Actual18D510 and18D4B0 with native16E6A0. No guest register state or floating
// point computation belongs to either native interface.
void native_tu15_set_constraint_runtime_link_flag(SourcePhysicsContactMemory&,
    std::uint32_t link,bool enabled,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintRuntimeParameterStats* = nullptr);
std::uint32_t native_tu15_create_constraint_runtime_link(SourcePhysicsContactMemory&,
    std::uint32_t handle,std::uint32_t reference,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintRuntimeParameterStats* = nullptr);
} // namespace halo2
