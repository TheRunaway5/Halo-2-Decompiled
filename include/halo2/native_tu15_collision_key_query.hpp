#pragma once
#include "halo2/native_tu15_collision_component.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
namespace halo2 {
struct NativeTu15CollisionKeyQueryServices {
    std::function<void(std::uint32_t shape,std::uint32_t bounds,
        std::uint32_t key_array,NativeTu15PhysicsFrameFp&)> query_shape; //2E9670
    std::function<void(std::uint32_t shape,std::uint32_t actual_owner,
        std::uint32_t transform,std::uint32_t half_extents,
        std::uint32_t tolerance_bits,std::uint32_t key_array,
        NativeTu15PhysicsFrameFp&)> query_instance; //actual virtual2C
};
// Caller-owned aligned256-byte semantic matrix/bounds workspace, not a CPU
// frame. Fields are initialized at their actual source phases.
struct NativeTu15CollisionKeyQueryScratch {std::uint32_t identity{};};
// Actual1F6FA0. Removes disabled keys in place, preserving source rereads and
// ordered shifts. The receiver's unused channel address is not fabricated.
void native_tu15_prune_collision_keys(SourcePhysicsContactMemory&,
    std::uint32_t key_array,const SourcePhysicsContactRecordObserver& = {});
// Actual1F6D10. Initial tree query and oriented instance query remain genuine
// typed services; production must bind native owners, never supplied results.
void native_tu15_query_collision_keys(SourcePhysicsContactMemory&,
    std::uint32_t shape,std::uint32_t bounds,std::uint32_t key_array,
    NativeTu15CollisionKeyQueryScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15CollisionKeyQueryServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
