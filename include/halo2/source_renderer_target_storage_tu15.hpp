#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
void mount_native_tu15_renderer_target_storage_data(SourceRuntimeMemory&);
bool source_renderer_target_storage_tu15_owns_entry(std::uint32_t);
void source_run_renderer_target_storage_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
struct NativeTu15TargetStorageMip {
    std::uint32_t width{},height{},format_word{},size_word{};
    NativeRuntimeAddress pixels{};
    std::uint64_t bytes{};
};
struct NativeTu15TargetStorageDefinition {
    std::uint32_t index{},width{},height{},format_word{},size_word{},pitch{};
    bool linear{};
    NativeRuntimeAddress pixels{};
    std::uint64_t bytes{};
    std::vector<NativeTu15TargetStorageMip> surfaces;
};
// Constructed source-ABI words remain a transitional shadow. Native resource
// consumers receive actual64-bit host addresses into the retained physical
// arena, including overlapping views and mip offsets. No GPU upload, image
// binding or cache-coherency/readiness claim follows from CPU construction.
// Views expire when this bank/backing is replaced; rebind after restoration.
std::vector<NativeTu15TargetStorageDefinition> native_tu15_renderer_target_definitions(const SourceRuntimeMemory&);
}
