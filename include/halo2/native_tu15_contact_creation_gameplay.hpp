#pragma once
#include "halo2/native_tu15_discrete_pair_binding.hpp"
#include <map>
namespace halo2 {
struct NativeTu15ContactBumperInput {
    std::uint32_t event{},first_body{},second_body{},radius_bits{},
        radius_difference_bits{},relative_velocity_bits{};
};
struct NativeTu15ContactCreationGameplayScratch {
    std::uint32_t result_byte{},radius_difference{},squared_speed{},
        scaled_velocity{},bumper_vector{},bumper_speed{},bumper_original_speed{};
    std::uint32_t bumper_relative_speed{};
};
struct NativeTu15ContactCreationGameplayServices {
    // Actual282FD0 and1D7FC0. Missing selected children refuse, never supply AL.
    std::function<bool(SourcePhysicsContactMemory&,std::uint32_t event,
        NativeTu15PhysicsFrameFp&)> test_event;
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t event,
        std::uint32_t component,std::uint32_t delta_bits,
        NativeTu15PhysicsFrameFp&)> damage;
    // Actual1E10F0: assign the source-computed linear velocity Vec4.
    std::function<void(SourcePhysicsContactMemory&,NativeTu15PhysicsBody,
        std::uint32_t velocity,NativeTu15PhysicsFrameFp&)> set_velocity;
};
struct NativeTu15ContactCreationGameplayStats {
    std::uint64_t world_calls{},bumper_calls{},limit_calls{},body_calls{},
        completed{},refusals{},class_queries{},motion_queries{},
        damage_requests{},event_tests{},velocity_requests{},point_rejections{},
        body_rejected_early_outs{};
    std::map<std::uint32_t,std::uint64_t> unqualified_owners;
    NativeTu15DiscretePairBindingActivity discrete;
};
// Actual283120. Source property/metadata lookups and all packed SSE/COMISS
// effects are retained. Scratch is initialized arithmetic DATA, no CPU frame.
bool native_tu15_test_contact_bumper(SourcePhysicsContactMemory&,
    NativeTu15ContactBumperInput,NativeTu15ContactCreationGameplayScratch,
    NativeTu15PhysicsFrameFp&,unsigned actual_x87_depth,
    const NativeTu15ContactCreationGameplayServices& = {},
    NativeTu15ContactCreationGameplayStats* = nullptr);
// Actual2E4B60. All selected providers receive this SAME guarded event view.
void native_tu15_world_contact_creation_listener(SourcePhysicsContactMemory&,
    std::uint32_t listener,std::uint32_t event,
    NativeTu15ContactCreationGameplayScratch,NativeTu15PhysicsFrameFp&,
    unsigned actual_x87_depth,const NativeTu15ContactCreationGameplayServices& = {},
    NativeTu15ContactCreationGameplayStats* = nullptr);
// Actual316510. Ordered-less rejects; equal/greater/unordered leave DATA alone.
void native_tu15_limit_created_contact(SourcePhysicsContactMemory&,
    std::uint32_t listener,std::uint32_t event,NativeTu15PhysicsFrameFp&,
    unsigned actual_x87_depth,NativeTu15ContactCreationGameplayStats* = nullptr);
// Current selected281710 flag1 early return. Wider body gameplay stays explicit.
void native_tu15_body_contact_creation_listener(SourcePhysicsContactMemory&,
    std::uint32_t listener,std::uint32_t event,NativeTu15PhysicsFrameFp&,
    unsigned actual_x87_depth,NativeTu15ContactCreationGameplayStats* = nullptr);
// Exact Root notify_listener(view,receiver,owner,event,currentFP) adapter target.
// Capture the actual depth and owned scratch, never the unguarded outer memory.
void native_tu15_dispatch_contact_creation_gameplay_listener(SourcePhysicsContactMemory&,
    std::uint32_t listener,NativeTu15ContactVirtualOwner,std::uint32_t event,
    NativeTu15ContactCreationGameplayScratch,NativeTu15PhysicsFrameFp&,
    unsigned actual_x87_depth,const NativeTu15ContactCreationGameplayServices& = {},
    NativeTu15ContactCreationGameplayStats* = nullptr);
} // namespace halo2
