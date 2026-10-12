#pragma once
#include "halo2/physics_narrowphase_owners.hpp"

namespace halo2 {
struct SourcePhysicsGjkOwnerCall {
    std::uint32_t instruction,entry,ecx,entry_esp;
    std::optional<std::uint32_t> eax,edx,esi,edi;
    std::vector<std::uint32_t> arguments;
    //309AE0 consumes EBX as well as EAX; these are original register inputs.
    std::optional<std::uint32_t> ebx;
};
struct SourcePhysicsGjkServices {
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsGjkOwnerCall &,const SourcePhysicsContactRecordObserver &)> required_owner;
};
// Complete actual convex/wrapper, capsule and BSP triangle retained-key
// retrieve owners. Unknown real virtuals remain required original producers.
void source_retrieve_physics_support_points_1_1(SourcePhysicsContactMemory &,
    std::uint32_t shape,const std::array<std::uint32_t,3> &arguments,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsGjkServices & = {},const SourcePhysicsContactRecordObserver & = {},
    std::uint32_t original_call_instruction = 0);
// Complete30EB30 source segment-pair arithmetic; consumes original points and
// directions, not a chosen closest point/contact or replacement shape.
std::uint32_t source_physics_gjk_segment_pair_1_1(SourcePhysicsContactMemory &,
    const std::array<std::uint32_t,5> &arguments,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
// Complete309900 source triangle-side/simplex owner and ordered swaps.
std::uint32_t source_physics_gjk_triangle_side_1_1(SourcePhysicsContactMemory &,
    std::uint32_t workspace,const std::array<std::uint32_t,3> &arguments,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete30AE40 orchestration. Actual shape virtuals and x86 estimate are
// explicit providers; unported simplex fallback owners cannot return success
// by default. No workspace, support point or finished contact is replayed.
std::uint32_t source_physics_gjk_1_1(SourcePhysicsContactMemory &,
    std::uint32_t workspace,const std::array<std::uint32_t,5> &arguments,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsReciprocalSqrtEstimate &,
    const SourcePhysicsGjkServices & = {},const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
