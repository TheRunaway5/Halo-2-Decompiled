#pragma once
#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
void mount_native_tu15_startup_sound_storage_data(SourceRuntimeMemory&);
bool source_startup_sound_storage_tu15_owns_entry(std::uint32_t);
bool source_startup_sound_storage_tu15_is_initializer(std::uint32_t);
void source_run_startup_sound_storage_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
void source_run_startup_sound_storage_with_native_storage(SourceRuntimeMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,SourcePhysicsPoolHostStorage&,
    const SourcePhysicsContactRecordObserver& = {});
// Literal main12008..1201F prefix, withEBX=0 from source12001. Allocates the
// original0x23000 sound arena through the actual33B3AC/kernel/native owner.
// Ends before source12025; it does not complete main or initialize audio.
void source_prepare_native_tu15_sound_arena(SourceRuntimeMemory&,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
    SourcePhysicsPoolHostStorage&,const SourcePhysicsContactRecordObserver& = {});
struct NativeTu15StartupSoundStorage {
    struct Region {std::uint32_t binding{},source{},bytes{};NativeRuntimeAddress native{};};
    std::vector<Region> regions;
};
// Slots46..50 only. Slot45 additionally requires the actual DirectSound
// replacement owner at224EC0; this module supplies no successful audio result.
NativeTu15StartupSoundStorage native_tu15_startup_sound_storage_views(const SourceRuntimeMemory&);
}
