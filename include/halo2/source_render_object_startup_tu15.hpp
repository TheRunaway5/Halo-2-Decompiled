#pragma once
#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Startup slots33..44, including literal RET slots34/39/42/43. Every original
// CPU constructor store runs; only raw platform allocation crosses to a host
// owner. This does not create a map, admit tags or submit rendered gameplay.
void mount_native_tu15_render_object_startup_data(SourceRuntimeMemory&);
bool source_render_object_startup_tu15_owns_entry(std::uint32_t);
bool source_render_object_startup_tu15_is_initializer(std::uint32_t);
void source_run_render_object_startup_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
void source_run_render_object_startup_with_native_storage(SourceRuntimeMemory&,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,SourcePhysicsPoolHostStorage&,
    const SourcePhysicsContactRecordObserver& = {});
struct NativeTu15RenderObjectStartupStorage {
    struct Region { std::uint32_t binding{},source{};std::uint64_t bytes{};NativeRuntimeAddress native{}; };
    std::vector<Region> regions;
};
// Full source-owned extents, represented with actual native64 backing. Views
// borrow this bank and must be rebound after bank replacement.
NativeTu15RenderObjectStartupStorage native_tu15_render_object_startup_views(const SourceRuntimeMemory&);
}
