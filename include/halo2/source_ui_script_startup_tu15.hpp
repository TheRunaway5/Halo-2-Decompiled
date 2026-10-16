#pragma once
#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/native_tu15_object_storage.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Authenticated prior381390 CRT constructor and startup slots51..58. All CPU
// stores/virtual dispatch/arena allocations execute. Only raw host allocation
// and the authored CRT formatting contract cross native resource boundaries.
void mount_native_tu15_ui_script_startup_data(SourceRuntimeMemory&);
bool source_ui_script_startup_tu15_owns_entry(std::uint32_t);
bool source_ui_script_startup_tu15_is_initializer(std::uint32_t);
void source_run_ui_script_startup_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
// Original CRT registration46DA0C names381390. Invoke once on loader-cold
// declaration storage before the68 initializer chain; mounting cannot invent
// its19 virtual objects or their constructor results.
void source_run_ui_script_startup_prior_constructor(SourceRuntimeMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
void source_run_ui_script_startup_with_native_storage(SourceRuntimeMemory&,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,SourcePhysicsPoolHostStorage&,
    NativeTu15StorageFormatting&,const SourcePhysicsContactRecordObserver& = {});
struct NativeTu15UiScriptStartupStorage {
    struct Region {std::uint32_t binding{},source{};std::uint64_t bytes{};NativeRuntimeAddress native{};};
    std::vector<Region> regions;
};
// Actual wide storage views. The script-node definition intentionally has no
// scenario payload at cold startup; its real allocation owns header+bitmap.
// These borrowed views must be rebound following memory-bank replacement.
NativeTu15UiScriptStartupStorage native_tu15_ui_script_startup_views(const SourceRuntimeMemory&);
}
