#pragma once
#include "halo2/native_tu15_island_activation.hpp"
namespace halo2 {
struct NativeTu15InstancedRayLifetimeServices {
    std::function<void(std::uint32_t allocator,std::uint32_t owner,std::uint32_t code,
        NativeTu15PhysicsFrameFp&)>release_code;
    std::function<void(std::uint32_t collection,std::uint32_t owner,std::uint32_t disposing,
        NativeTu15PhysicsFrameFp&)>destroy_collection;
    std::function<void(std::uint32_t object,std::uint32_t mode,
        NativeTu15PhysicsFrameFp&)>allocation_mode;
};
// Actual30AA40/30AA50 reference32 and2E9790/2E96A0 object DATA initialization.
void native_tu15_reference_instanced_mopp_code(SourcePhysicsContactMemory&,std::uint32_t,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_release_instanced_mopp_code(SourcePhysicsContactMemory&,std::uint32_t,
    NativeTu15PhysicsFrameFp&,const NativeTu15InstancedRayLifetimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
std::uint32_t native_tu15_initialize_instanced_ray_shape(SourcePhysicsContactMemory&,
    std::uint32_t shape,std::uint32_t collection,std::uint32_t code,
    const SourcePhysicsContactRecordObserver& = {});
std::uint32_t native_tu15_initialize_instanced_ray_cache(SourcePhysicsContactMemory&,
    std::uint32_t cache,const SourcePhysicsContactRecordObserver& = {});
// Actual1F3600 and2E96B0. Source-native reservation predicate reused once;
// unknown real destructor/allocation owners remain explicit typed services.
void native_tu15_destroy_instanced_collection_base(SourcePhysicsContactMemory&,
    std::uint32_t shape,NativeTu15PhysicsFrameFp&,const NativeTu15InstancedRayLifetimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_destroy_instanced_ray_shape(SourcePhysicsContactMemory&,
    std::uint32_t shape,NativeTu15PhysicsFrameFp&,const NativeTu15InstancedRayLifetimeServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
