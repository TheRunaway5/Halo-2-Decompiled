#pragma once
#include "halo2/native_tu15_island_collision.hpp"
#include "halo2/native_tu15_constraint_step.hpp"
namespace halo2 {
struct NativeTu15IslandContactBuffer {std::uint32_t identity{};};
struct NativeTu15IslandContactOutput {std::uint32_t identity{};};
struct NativeTu15IslandCollisionAgent {std::uint32_t identity{};};
struct NativeTu15IslandAgentEntry {std::uint32_t identity{};};
struct NativeTu15IslandAgentPhaseScratch {
    NativeTu15IslandContactBuffer buffer; //20writable caller-owned bytes.
    NativeTu15IslandContactOutput output; //Actual pointer-holder packet.
};
struct NativeTu15IslandAgentPhaseServices {
    std::function<std::uint64_t()>timestamp_counter;
    std::function<std::uint32_t(NativeTu15ConstraintStackAllocator,NativeTu15PhysicsVirtualOwner,
        std::uint32_t bytes,NativeTu15PhysicsFrameFp&)>allocate_stack_block;
    std::function<void(NativeTu15ConstraintStackAllocator,NativeTu15PhysicsVirtualOwner,
        std::uint32_t block,std::uint32_t bytes,NativeTu15PhysicsFrameFp&)>release_stack_block;
    std::function<void(NativeTu15ConstraintStackAllocator,NativeTu15PhysicsVirtualOwner,
        std::uint32_t block,std::uint32_t bytes,std::uint32_t memory_class,NativeTu15PhysicsFrameFp&)>release_owned_contacts;
    // Actual agent vtable+10 and contact-manager vtable+C, both RET16.
    // The output holder points to the actual mutable contact-buffer header.
    // No contact count, collision result or success value is supplied.
    std::function<void(NativeTu15IslandCollisionAgent,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
        NativeTu15IslandContactOutput,NativeTu15PhysicsFrameFp&)>process_agent;
    std::function<void(NativeTu15IslandCollisionAgent,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration,
        NativeTu15IslandContactOutput,NativeTu15PhysicsFrameFp&)>finish_agent;
    // Actual3205B0, receiver=island, arguments(config,entry,output,time_bits),
    // RET16. This owner remains a genuine void dependency; no result is used.
    std::function<void(NativeTu15PhysicsIsland,NativeTu15PhysicsStepConfiguration,
        NativeTu15IslandAgentEntry,NativeTu15IslandContactOutput,std::uint32_t time_bits,
        NativeTu15PhysicsFrameFp&)>advance_contact_time;
};
struct NativeTu15IslandAgentPhaseStats {
    std::uint64_t flag_clear_calls{},flag_set_calls{},constructed_buffers{},destroyed_buffers{},
        allocated_stack_blocks{},released_stack_blocks{},released_owned_buffers{},process_calls{},finish_calls{},
        native_minimum_calls{},advance_time_calls{};
};
struct NativeTu15IslandContactMinimum {std::array<std::byte,10>extended_value;};
// Actual320510's real extended scalar result, encoded without invoking the
// host C++ floating-return ABI. fp contains the source post-return status;
// the phase kernel keeps this same result directly in its native expression.
NativeTu15IslandContactMinimum native_tu15_minimum_island_contact_distance(
    SourcePhysicsContactMemory&,NativeTu15IslandContactOutput,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth);
// Actual321180/321220 own the initial stack buffer and any subsequently grown
// contact allocation independently. Actual header/allocator words are reread
// after real services; no captured allocator pointer/result is substituted.
void native_tu15_construct_island_contact_buffer(SourcePhysicsContactMemory&,
    NativeTu15IslandContactBuffer,std::uint32_t initial_capacity,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandAgentPhaseServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandAgentPhaseStats* = nullptr);
void native_tu15_destroy_island_contact_buffer(SourcePhysicsContactMemory&,
    NativeTu15IslandContactBuffer,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandAgentPhaseServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandAgentPhaseStats* = nullptr);
// Actual321D20, using the actual mutable output/header packets through every
// agent callback. The caller owns their lifetime outside residual call frames.
void native_tu15_collide_island_flag_clear(SourcePhysicsContactMemory&,
    NativeTu15PhysicsIsland,NativeTu15PhysicsStepConfiguration,NativeTu15IslandAgentPhaseScratch,
    NativeTu15PhysicsFrameFp&,const NativeTu15IslandAgentPhaseServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15IslandAgentPhaseStats* = nullptr);
// Actual321A70, including the closed native320510 minimum-distance helper.
void native_tu15_collide_island_flag_set(SourcePhysicsContactMemory&,
    NativeTu15PhysicsIsland,NativeTu15PhysicsStepConfiguration,NativeTu15IslandAgentPhaseScratch,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15IslandAgentPhaseServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandAgentPhaseStats* = nullptr);
} // namespace halo2
