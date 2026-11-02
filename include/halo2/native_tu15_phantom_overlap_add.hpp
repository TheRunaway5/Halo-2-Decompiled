#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"
#include "halo2/native_tu15_array_growth.hpp"

namespace halo2 {
// Sixteen caller-owned DATA bytes. Fields0/+4/+8/+C are initialized by
// actual314F80; padding+9..B retains its caller-owned contents.
struct NativeTu15PhantomOverlapAddedEvent {std::uint32_t identity{};};
struct NativeTu15PhantomOverlapAddServices {
    NativeTu15ArrayGrowthServices allocation;
    std::function<void(std::uint32_t listener,NativeTu15ContactVirtualOwner,
        NativeTu15PhantomOverlapAddedEvent,NativeTu15PhysicsFrameFp&)> added;
};
// Actual314F80. Listeners run in reverse, may change the current decision,
// and may replace the backing array; the original reverse index is retained.
std::uint32_t native_tu15_notify_phantom_overlap_added(SourcePhysicsContactMemory&,
    std::uint32_t phantom,NativeTu15ContactPayload other,std::uint8_t was_present,
    NativeTu15PhantomOverlapAddedEvent,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhantomOverlapAddServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual315B00 and315300. Genuine listeners and native2E3250 growth own
// their outcomes. The complex and simple append stores retain source order.
void native_tu15_add_phantom_overlap(SourcePhysicsContactMemory&,std::uint32_t phantom,
    NativeTu15ContactPayload other,std::uint32_t flag_word,NativeTu15PhantomOverlapAddedEvent,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomOverlapAddServices&,
    const SourcePhysicsContactRecordObserver& = {});
void native_tu15_add_simple_phantom_overlap(SourcePhysicsContactMemory&,std::uint32_t phantom,
    NativeTu15ContactPayload other,std::uint32_t flag_word,NativeTu15PhantomOverlapAddedEvent,
    NativeTu15PhysicsFrameFp&,const NativeTu15PhantomOverlapAddServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
