#pragma once
#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Whole unchanged startup slots59..68 and their complete CPU constructor
// graph. Five selected slots are literal RET instructions in TU1.5. The real
// game arena, earlier renderer raw storage and platform allocator are inputs.
// This creates storage and source metadata; it does not admit gameplay.
void mount_native_tu15_final_startup_storage_data(SourceRuntimeMemory&);
bool source_final_startup_storage_tu15_owns_entry(std::uint32_t);
bool source_final_startup_storage_tu15_is_initializer(std::uint32_t);
void source_run_final_startup_storage_tu15(SourcePhysicsContactMemory&, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&, const SourcePhysicsContactRecordObserver& = {});
void source_run_final_startup_storage_with_native_storage(SourceRuntimeMemory&, std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&, SourcePhysicsPoolHostStorage&,
    const SourcePhysicsContactRecordObserver& = {});

struct NativeTu15FinalStartupStorageViews {
    struct Region {
        std::uint32_t binding{}, source{};
        std::uint64_t bytes{};
        NativeRuntimeAddress native{};
    };
    std::vector<Region> regions;
};
// Complete reached constructor extents, including genuine parent/heap-header
// overlap and the untouched bytes of arena allocations. Views borrow the real
// native64 bank and must be rebound after bank replacement.
NativeTu15FinalStartupStorageViews native_tu15_final_startup_storage_views(const SourceRuntimeMemory&);
}
