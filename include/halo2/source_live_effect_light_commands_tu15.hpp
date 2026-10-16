#pragma once
#include "halo2/source_draw_unit.hpp"
#include "halo2/source_object_admission.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/native_runtime_address.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <functional>

namespace halo2 {
class PhysicsLiveWorldTu15;
class SourceRuntimeMemory;

struct SourceLiveEffectLightQueueTu15 {
    std::uint32_t header{};
    std::uint16_t capacity{};
    std::array<std::uint32_t,4> streams{};
    NativeRuntimeAddress header_native;
    std::array<NativeRuntimeAddress,4> streams_native{
        NativeRuntimeAddress{},NativeRuntimeAddress{},NativeRuntimeAddress{},NativeRuntimeAddress{}};
};
struct SourceLiveEffectLightStateTu15 {
    SourceDrawUnitState state;
    SourceBspRangeArena main_arena;
    std::uint32_t bsp{},current_context{};
    std::array<SourceLiveEffectLightQueueTu15,3> queues;
    NativeRuntimeAddress bsp_native,draws_native,secondary_native;
};
// Read the entire retained shared draw/override/section banks before a light
// collection. Secondary2CB10 resets only draw_count; all other bytes survive.
// The caller copies state into its typed collector before resetting the actual
// query-owned secondary arena. No source memory is written by this operation.
SourceLiveEffectLightStateTu15 source_live_effect_light_state_tu15(const SourceRuntimeMemory&);

struct SourceLiveEffectLightBindingsTu15 {
    std::uint32_t light_arena_token{}; // Explicit caller-owned2048-byte arena.
    std::uint32_t instance_token{}; //0 requires already-native instance+1C.
    std::span<const Transform> instance_transforms;
    std::uint32_t query_mode{}; // Completed light query, not inferred from mode.
};
using SourceLiveEffectLightInputWriterTu15=
    std::function<void(std::uint32_t,std::span<const std::byte>)>;
struct SourceLiveEffectLightHandoffTu15 {
    unsigned main_prefix{},draw_count{},mask_relocations{},instance_relocations{};
    std::array<unsigned,3> queue_counts{};
};
// Publish completed secondary inputs after culling and before31172's40E80.
// Existing retained source banks must still equal the pre-collection snapshot.
// Main prefix units/coverage/slots and native main mask arena are never written.
// Only the declared light arena and authored instance token can be relocated;
// all queue allocation identities and salted model owners are validated first.
SourceLiveEffectLightHandoffTu15 source_live_effect_light_handoff_tu15(
    const SourceRuntimeMemory&,const SourceLiveEffectLightStateTu15&,
    const SourceDrawUnitState&,const SourceBspAdmission&,const SourceObjectAdmission&,
    const SourceLiveEffectLightBindingsTu15&,const SourceLiveEffectLightInputWriterTu15&);

struct SourceLiveEffectLightCommandsTu15 {
    SourceLiveEffectLightHandoffTu15 handoff;
    SourceBspPartCommandState legacy; // Private copy; native callbacks unchanged.
    std::uint32_t special_bank{};
    std::array<std::byte,0x406> special_source{};
};
// Read all actual command backing, including retained unused records, and the
// actual selected cached special bank. Normalize only active4CBE0/4D0A0 fields
// in the private copy for the existing typed opaque/range consumer.
SourceLiveEffectLightCommandsTu15 source_live_effect_light_private_commands_tu15(
    const SourceRuntimeMemory&);
using SourceLiveEffectLightCommandObserverTu15=std::function<void(bool,
    const SourcePhysicsWorldRuntimeAbi&,const SourceRuntimeMemory&)>;
// Execute the complete actual producer once with EAX0. This does not install
// a light/shadow definition or feature overlay, sort/submit GPU work, or join
// the later cached402A0 owner. It retains the actual31172 caller globals.
SourceLiveEffectLightCommandsTu15 source_live_effect_light_commands_tu15(
    PhysicsLiveWorldTu15&,const SourceLiveEffectLightStateTu15&,
    const SourceDrawUnitState&,const SourceBspAdmission&,const SourceObjectAdmission&,
    const SourceLiveEffectLightBindingsTu15&,const SourceLiveEffectLightCommandObserverTu15& = {});
}
