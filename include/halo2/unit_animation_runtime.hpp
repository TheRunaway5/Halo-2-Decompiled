#pragma once
#include "halo2/physics_body_factory.hpp"
#include "halo2/source_runtime_call.hpp"

namespace halo2 {
// Complete1E13C0/1E1230 resource admission. The live initialization bytes,
// inherited graph, linked definitions and descriptor readiness own demand.
// Reached124E40 requests retain their actual ABI and require a real owner.
void source_demand_unit_animation_1_1(
    SourcePhysicsContactMemory &, std::uint32_t graph_address,
    std::uint32_t packed, std::uint32_t entry_esp, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1150D0/1DCD10 packed transition clocks and10F430/1102B0
// retained animation-weight updates. Constants and RNG state belong to the
// same source bank; no presentation-time blend or random stream is substituted.
bool source_advance_unit_transition_clocks_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    const SourcePhysicsContactRecordObserver & = {});
bool source_decay_unit_animation_weights_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
bool source_blend_unit_animation_weights_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1105E0/E7E30 completion gate over the retained manager channels.
bool source_unit_animation_completed_1_1(const SourcePhysicsContactMemory &,
                                         std::uint32_t manager);
// Complete1D0A60 selection/installation caller on the raw retained manager.
// Graph resolution, secondary/third selection, admitted variants and resource
// demand execute natively. Root sampling and admitted cache IO require owners.
bool source_select_unit_animation_1_1(
    SourcePhysicsContactMemory &, std::uint32_t manager,
    std::array<std::uint32_t, 4> identifiers, std::uint32_t selection_flags,
    std::uint32_t installation_flags, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1D06C0 third-channel activity mapping. Output pointers are the
// actual caller's packets;1CFDE0 resolution remains required when reached.
std::uint32_t source_select_third_unit_animation_1_1(
    SourcePhysicsContactMemory &, std::uint32_t manager,
    std::array<std::uint32_t, 4> identifiers, std::uint32_t packed_output,
    std::uint32_t activity_output, std::uint32_t entry_esp,
    const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
struct SourceUnitAnimationRequest {
  std::uint32_t object;
  // Actual source mode, weapon class, weapon and activity, in stack order.
  std::array<std::uint32_t, 4> identifiers;
  std::uint32_t minimum_transition_word;
  std::uint8_t force;
};
// Complete1CA5A0 inherited graph lookup and1C9E30 current definition request.
// Complete1DE890 definition resolution, including native1E13C0 demand.
std::uint32_t source_unit_animation_definition_1_1(
    SourcePhysicsContactMemory &, std::uint32_t graph_address,
    std::uint32_t packed, std::uint32_t entry_esp, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint32_t
source_unit_animation_channel_graph_1_1(const SourcePhysicsContactMemory &,
                                        std::uint32_t channel);
std::uint32_t source_unit_animation_channel_definition_1_1(
    SourcePhysicsContactMemory &, std::uint32_t channel,
    std::uint32_t entry_esp, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1CA090,1C9F50 and1CA3A0 frame/event scheduling. Event traversal and
// admitted RNG/resource changes run through their actual owners on the same
// bank; a callback can replace the packed animation before the next check.
void source_set_unit_animation_frame_1_1(
    SourcePhysicsContactMemory &, std::uint32_t channel,
    std::uint32_t frame_word, std::uint32_t manager, std::uint32_t callback,
    std::uint32_t context, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
void source_clamp_unit_animation_frame_1_1(
    SourcePhysicsContactMemory &, std::uint32_t channel,
    std::uint32_t frame_word, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1CAE40 timing/sound/effect traversal and20-byte event delivery.
// The original callback owns effects and can mutate the current animation.
std::uint32_t source_traverse_unit_animation_events_1_1(
    SourcePhysicsContactMemory &, std::uint32_t graph_address,
    std::uint32_t definition, std::uint32_t options, std::uint32_t from_word,
    std::uint32_t to_word, std::uint32_t callback, std::uint32_t context,
    std::uint32_t entry_esp, SourcePhysicsFactoryFloatingPoint,
    const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1CA2B0 retained channel clock. It clears current event masks before
// the delay gate, then derives the exact requested frame from the live clock.
void source_advance_unit_animation_channel_1_1(
    SourcePhysicsContactMemory &, std::uint32_t channel, std::uint32_t manager,
    std::uint32_t callback, std::uint32_t context, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete1CF100/1CF070 manager clock scheduling, completion cleanup and
// layer synchronization on the retained raw bank. Channel clocks, definition
// resources and local-transition capture must be real reached owners.
bool source_update_unit_animation_manager_1_1(
    SourcePhysicsContactMemory &, std::uint32_t manager, std::uint32_t callback,
    std::uint32_t context, std::uint32_t node_count, std::uint32_t saved_locals,
    std::uint32_t current_locals, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete114F70 transition-duration arbitration. Constants are read from
// the authenticated selected image; no duration or clip defaults are added.
std::uint32_t source_unit_animation_transition_1_1(
    const SourcePhysicsContactMemory &, std::uint32_t previous_activity,
    std::uint32_t activity, std::uint8_t mode_changed, std::uint8_t flag,
    std::uint32_t minimum_word, SourcePhysicsFactoryFloatingPoint);
// Complete114F00 overlay refresh with the three actual1CEC20 requests.
void source_refresh_unit_animation_channels_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object, std::uint32_t entry_esp,
    const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete110BF0 request installation and transition scheduling. Reached
// 1D0A60/1CFA90/BB240/114BD0 owners must mutate the same retained manager,
// local banks and resource state. Missing owners never admit an animation.
bool source_request_unit_animation_1_1(
    SourcePhysicsContactMemory &, const SourceUnitAnimationRequest &,
    std::uint32_t flags, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete112E10 per-unit animation phase, including action completion,
// conditional death requests, channel advancement, events and resources.
// It consumes the current prephysics action packet and retained manager.
std::uint8_t source_update_unit_animation_1_1(
    SourcePhysicsContactMemory &, std::uint32_t object,
    std::uint32_t action_packet, std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint, const SourceRuntimeOwner &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
