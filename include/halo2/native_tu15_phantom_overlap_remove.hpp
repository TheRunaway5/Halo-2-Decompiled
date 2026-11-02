#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"

namespace halo2 {
// Nine writable DATA bytes: phantom word, collidable word, was-present byte.
// All three fields are initialized before a listener can observe the packet.
struct NativeTu15PhantomOverlapEvent {std::uint32_t identity{};};
struct NativeTu15PhantomOverlapRemoveServices {
    std::function<void(std::uint32_t listener,NativeTu15ContactVirtualOwner,
        NativeTu15PhantomOverlapEvent,NativeTu15PhysicsFrameFp&)> removed;
};
// Actual314FE0, reverse listener dispatch. It rereads the listener array after
// each callback and keeps the initial reverse index, exactly as the source.
void native_tu15_notify_phantom_overlap_removed(SourcePhysicsContactMemory&,
    std::uint32_t phantom,NativeTu15ContactPayload other,std::uint8_t was_present,
    NativeTu15PhantomOverlapEvent,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhantomOverlapRemoveServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual315A70. Search backward, notify before removal, then reread current
// count/backing and swap the last entry into the original matching index.
void native_tu15_remove_phantom_overlap(SourcePhysicsContactMemory&,
    std::uint32_t phantom,NativeTu15ContactPayload other,NativeTu15PhantomOverlapEvent,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomOverlapRemoveServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
