#pragma once
#include "halo2/native_tu15_island_agent_phase.hpp"
namespace halo2 {
struct NativeTu15ContactCompletionPacket {std::uint32_t identity{};};
struct NativeTu15ContactCompletionListener {std::uint32_t identity{};};
struct NativeTu15ContactCompletionServices {
    std::function<void(NativeTu15ContactCompletionListener,NativeTu15PhysicsVirtualOwner,
        NativeTu15ContactCompletionPacket,NativeTu15PhysicsFrameFp&)> notify;
};
struct NativeTu15ContactCompletionStats {
    std::uint64_t calls{},copied_contacts{},world_notifications{},body_notifications{},
        custom_callbacks{},null_entries_removed{};
};
// Actual322560/31FF50: source subject publication, reverse notification and
// stable null compaction. Custom callback bodies remain genuine services.
void native_tu15_notify_world_contact_completion(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15ContactCompletionPacket,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCompletionServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ContactCompletionStats* = nullptr);
void native_tu15_notify_body_contact_completion(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody,NativeTu15ContactCompletionPacket,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCompletionServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ContactCompletionStats* = nullptr);
// Actual3230C0 contact-manager completion. The caller owns20 ordinary writable
// event bytes across every callback; no CPU frame or return state is retained.
void native_tu15_complete_contact_manager(SourcePhysicsContactMemory&,
    NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
    NativeTu15IslandContactOutput,NativeTu15ContactCompletionPacket,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCompletionServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ContactCompletionStats* = nullptr);
} // namespace halo2
