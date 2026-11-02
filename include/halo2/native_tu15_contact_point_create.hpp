#pragma once
#include "halo2/native_tu15_contact_producer.hpp"

namespace halo2 {
struct NativeTu15ContactPointCreateScratch {std::uint32_t identity{};}; // aligned16,0x70 bytes
struct NativeTu15ContactPointCreateServices {
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t world,std::uint32_t collector,
        std::uint32_t info,NativeTu15PhysicsFrameFp&)> register_constraint;
    std::function<std::uint32_t(SourcePhysicsContactMemory&,std::uint32_t collector,
        std::uint32_t point_cell,std::uint32_t property_cell,std::uint32_t info,
        NativeTu15PhysicsFrameFp&)> allocate_point;
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t motion,NativeTu15ContactVirtualOwner,
        std::uint32_t packet,std::uint32_t output,NativeTu15PhysicsFrameFp&)> point_velocity;
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t owner,std::uint32_t event,
        NativeTu15PhysicsFrameFp&)> notify_world,notify_body;
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t manager,NativeTu15ContactVirtualOwner,
        std::uint16_t id,NativeTu15PhysicsFrameFp&)> remove_point;
};
struct NativeTu15ContactPointCreateStats {
    std::uint64_t attempts{},completed{},refusals{},capacity_rejections{},registrations{},
        allocations{},velocity_queries{},world_notifications{},body_notifications{},rejected_points{},
        unprovided_event_reads{};
};
// Actual322D20. Source event padding+2..3 is UNPROVIDED; the same read-guarded
// memory is passed explicitly to every native service. Event+10 is unavailable
// until the genuine world notification initializes it. No private/control
// before-byte image enters this API. Generic raw store observers are currently
// refused; retained DATA/order/FP qualification is a separate component domain.
std::uint16_t native_tu15_create_contact_point(SourcePhysicsContactMemory&,
    std::uint32_t manager,NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext,std::uint32_t packet,NativeTu15ContactPointCreateScratch,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15ContactPointCreateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactPointCreateStats* = nullptr);
} // namespace halo2
