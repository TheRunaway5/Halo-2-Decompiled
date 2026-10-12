#pragma once
#include "halo2/physics_pair_manager.hpp"
#include "halo2/physics_contact_lifecycle.hpp"
#include <optional>

namespace halo2 {
struct SourcePhysicsAgentOwnerCall {
    std::uint32_t instruction,entry,receiver,entry_esp;
    std::vector<std::uint32_t> arguments;
    bool cdecl;
};
struct SourcePhysicsAgentSchedulerServices {
    SourcePhysicsBroadphaseServices scratch;
    std::optional<SourcePhysicsFactoryFloatingPoint> floating_point;
    // Real lower agent/terrain/solver/listener ownership, never recorded
    // contact output or a successful admission default. Missing throws.
    std::function<std::uint32_t(SourcePhysicsContactMemory &,
        const SourcePhysicsAgentOwnerCall &,const SourcePhysicsContactRecordObserver &)> required_owner;
    // Optional actual platform RDTSC provider. Source profiling-disabled
    // inputs need no counter; positive profiling requires real ownership.
    std::function<std::uint64_t()> timestamp_counter;
};
std::uint32_t source_construct_physics_agent_output_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,std::uint32_t count,
    const SourcePhysicsAgentSchedulerServices & = {},const SourcePhysicsContactRecordObserver & = {});
void source_destroy_physics_agent_output_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t descriptor,const SourcePhysicsAgentSchedulerServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
// Complete retained manager31F550. Its four arguments are the actual two
// collidables, source step packet and source-produced output packet. Scratch
// is the original entry ESP; no output count/contact is manufactured.
void source_deliver_physics_agent_output_1_1(SourcePhysicsContactMemory &,
    std::uint32_t manager,const std::array<std::uint32_t,4> &arguments,
    std::uint32_t entry_esp,const SourcePhysicsAgentSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Exact descending listener dispatch/ordered null compaction. Packet+10
// ownership differs for the world and entity paths. Callbacks are required.
void source_deliver_physics_world_contact_listeners_1_1(SourcePhysicsContactMemory &,
    std::uint32_t world,std::uint32_t packet,std::uint32_t entry_esp,
    const SourcePhysicsAgentSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_deliver_physics_entity_contact_listeners_1_1(SourcePhysicsContactMemory &,
    std::uint32_t entity,std::uint32_t packet,std::uint32_t entry_esp,
    const SourcePhysicsAgentSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete actual listener27E3A0 and matching contact-bank iterator1D4160.
// Refresh consumes only real emitted rows and existing retained contacts; it
// cannot manufacture a contact. Current lower positive wake/character/impulse
// branches retain the existing contact-refresh API's hard boundaries.
void source_refresh_physics_entity_contact_bank_1_1(SourcePhysicsContactMemory &,
    std::uint32_t packet,const std::array<std::uint32_t,5> &arguments,
    std::uint32_t entry_esp,const SourcePhysicsAgentSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
void source_deliver_physics_object_contact_listener_1_1(SourcePhysicsContactMemory &,
    std::uint32_t packet,std::uint32_t entry_esp,const SourcePhysicsAgentSchedulerServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete retained-row scheduler31DEE0. entry_esp owns its
// original temporary packet/descriptor aliases. Agent virtual+10 and any
// positive refinement31CA20 remain mandatory real providers.
void source_process_physics_island_agents_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,std::uint32_t step,std::uint32_t entry_esp,
    SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsAgentSchedulerServices &,const SourcePhysicsContactRecordObserver & = {});
std::uint32_t source_physics_agent_minimum_distance_1_1(const SourcePhysicsContactMemory &,
    std::uint32_t output_packet);
// Complete original island owner31E2C0, including real heap accounting calls,
// current broadphase update then stored world+23E scheduler selection.
void source_schedule_physics_island_agents_1_1(SourcePhysicsContactMemory &,
    std::uint32_t island,std::uint32_t backend,std::uint32_t step,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsAgentSchedulerServices &,const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
