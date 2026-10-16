#pragma once
#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Original1E0F0 constructs transparent-plane storage and an idle hash/free-list
// container. Its retained callbacks are identities; construction never runs
// them. Native graphics submission remains a separate owner.
void mount_native_tu15_renderer_transparent_storage_data(SourceRuntimeMemory&);
bool source_renderer_transparent_storage_tu15_owns_entry(std::uint32_t);
void source_run_renderer_transparent_storage_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
void source_run_renderer_transparent_storage_with_native_storage(SourceRuntimeMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,SourcePhysicsPoolHostStorage&,
    const SourcePhysicsContactRecordObserver& = {});

struct NativeTu15RendererTransparentStorage {
    NativeRuntimeAddress planes{};        //0x1C000 source data bytes.
    NativeRuntimeAddress indices{};       //0x800 source index bytes.
    NativeRuntimeAddress container{};     //0x1203C source header/payload bytes.
    NativeRuntimeAddress buckets{};       //4096 source hash buckets.
    NativeRuntimeAddress free_nodes{};    //2048 source0x1C-byte nodes.
    std::uint32_t bucket_count{},node_count{},node_payload_bytes{};
};
// Initial constructor admission, including the complete free chain. Views
// borrow actual native64 backing and must be rebound after bank replacement.
NativeTu15RendererTransparentStorage native_tu15_renderer_transparent_storage_initial_views(
    const SourceRuntimeMemory&);
}
