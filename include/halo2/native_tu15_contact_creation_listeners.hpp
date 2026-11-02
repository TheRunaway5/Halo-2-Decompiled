#pragma once
#include "halo2/native_tu15_contact_producer.hpp"

namespace halo2 {
struct NativeTu15ContactCreationListenerServices {
    // Actual current virtual method; event/constraint is retained caller DATA.
    std::function<void(std::uint32_t listener,NativeTu15ContactVirtualOwner,
        std::uint32_t argument,NativeTu15PhysicsFrameFp&)> notify;
};
struct NativeTu15ContactCreationListenerStats {
    std::uint64_t attempts{},completed{},refusals{},notifications{},removed_nulls{};
};
// Actual322440/31FE70/322360. Notification walks backwards; null compaction
// preserves order and rereads backing/count at every source phase, including
// after callbacks. These functions never read or invent event padding.
void native_tu15_notify_world_contact_creation(SourcePhysicsContactMemory&,
    std::uint32_t world,std::uint32_t event,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCreationListenerServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactCreationListenerStats* = nullptr);
void native_tu15_notify_body_contact_creation(SourcePhysicsContactMemory&,
    std::uint32_t body,std::uint32_t event,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCreationListenerServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactCreationListenerStats* = nullptr);
void native_tu15_notify_world_constraint_addition(SourcePhysicsContactMemory&,
    std::uint32_t world,std::uint32_t constraint,NativeTu15PhysicsFrameFp&,
    const NativeTu15ContactCreationListenerServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactCreationListenerStats* = nullptr);
} // namespace halo2
