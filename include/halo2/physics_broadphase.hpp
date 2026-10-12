#pragma once
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
// Exact original source-address storage. Scratch comes from the caller's real
// frame/arena; it is never used as a body identity or a recorded-pair provider.
struct SourcePhysicsBroadphaseScratch {
    std::uint32_t bitset_descriptor;
};
struct SourcePhysicsBroadphaseServices {
    // Actual allocator +24/+28 fallback owners, required when the source
    // scratch bank cannot satisfy a request or delegates its retirement.
    std::function<std::uint32_t(SourcePhysicsContactMemory &,std::uint32_t,
        std::uint32_t,std::uint32_t,const SourcePhysicsContactRecordObserver &)> scratch_allocate;
    std::function<void(SourcePhysicsContactMemory &,std::uint32_t,
        std::uint32_t,std::uint32_t,std::uint32_t,const SourcePhysicsContactRecordObserver &)> scratch_free;
};
std::uint32_t source_construct_physics_pair_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t count,
    const SourcePhysicsBroadphaseServices & = {},const SourcePhysicsContactRecordObserver & = {});
void source_destroy_physics_pair_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,const SourcePhysicsBroadphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_construct_physics_overlap_bitset_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t count,
    const SourcePhysicsBroadphaseServices & = {},const SourcePhysicsContactRecordObserver & = {});
void source_destroy_physics_overlap_bitset_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,const SourcePhysicsBroadphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete2E7500 insertion and2E6240 MMX signed-word index repair.
std::uint32_t source_insert_physics_broadphase_axis_1_1(SourcePhysicsContactMemory &,
    std::uint32_t axis,const std::array<std::uint32_t,6> &arguments,
    const SourcePhysicsContactRecordObserver & = {});
void source_repair_physics_broadphase_indices_1_1(SourcePhysicsContactMemory &,
    std::uint32_t body_bank,std::uint32_t body_index,std::uint32_t new_record,
    const SourcePhysicsContactRecordObserver & = {});
// Complete2E8A00 uses original constants, retained sorted endpoints, real
// allocator/growth and actual constructed pairs. It makes no solver contact.
void source_insert_physics_broadphase_body_1_1(SourcePhysicsContactMemory &,
    std::uint32_t backend,std::uint32_t handle,std::uint32_t bounds,
    std::uint32_t pairs,const SourcePhysicsBroadphaseScratch &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsBroadphaseServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete2E2380 selected resident convex callback and source velocity
// expansion. Unknown underlying shape virtual targets are hard boundaries.
void source_physics_wrapper_bounds_1_1(SourcePhysicsContactMemory &,
    std::uint32_t wrapper,std::uint32_t transform,std::uint32_t margin,
    std::uint32_t bounds,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});

struct SourcePhysicsWorldBroadphaseScratch {
    std::uint32_t frame; // Actual3144D0 aligned owning frame, at least0x8C.
    SourcePhysicsBroadphaseScratch backend;
};
struct SourcePhysicsWorldPairDeliveryServices {
    // Original378EB0 manager call: bank,count,0,0,optional(world+D0+8).
    // The original temporary manager+24 list is already installed. This
    // provider must execute actual pair/solver ownership; no success default.
    std::function<void(SourcePhysicsContactMemory &,std::uint32_t,
        const std::array<std::uint32_t,5> &,const SourcePhysicsContactRecordObserver &)> static_pairs;
    // Original31D980 island call: bank,count,0,0. No replayed contacts.
    std::function<void(SourcePhysicsContactMemory &,std::uint32_t,
        const std::array<std::uint32_t,4> &,const SourcePhysicsContactRecordObserver &)> dynamic_pairs;
};
// Complete3144D0 orchestration with actual supported shape/backend owners.
// Stops at the required real pair manager after original preceding stores.
void source_insert_physics_world_broadphase_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t body,const SourcePhysicsWorldBroadphaseScratch &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsBroadphaseServices &,
    const SourcePhysicsWorldPairDeliveryServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
