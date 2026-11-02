#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>

namespace halo2 {
struct NativeTu15GameplayContactRemovalServices {
    // Actual22E1B0: current160-byte instance and the complete cached datum.
    std::function<void(std::uint32_t instance,std::uint32_t datum,
        NativeTu15PhysicsFrameFp&)> release_constraint_reference;
    // Actual1E0A60, RET12(slot DATA,signed part,current component).
    std::function<void(std::uint32_t slot,std::int32_t part,std::uint32_t component,
        NativeTu15PhysicsFrameFp&)> release_part_contact;
};
struct NativeTu15GameplayContactRemovalStats {
    std::uint64_t notifications{},active_components{},searches{},matches{},rows{},
        constraint_releases{},part_releases{};
};
// Actual281D40, receiver-independent RET4(event). Reads event+8/+C/+10 only.
void native_tu15_notify_gameplay_contact_removed(SourcePhysicsContactMemory&,
    std::uint32_t event,NativeTu15PhysicsFrameFp&,const NativeTu15GameplayContactRemovalServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15GameplayContactRemovalStats* = nullptr);
// Actual1D8880. EAX/ECX in the original are typed other/subject body inputs.
// Event ID is read as16bits only; no event pointer or padding escapes to a child.
void native_tu15_remove_matching_gameplay_contact(SourcePhysicsContactMemory&,
    std::uint32_t other_body,std::uint32_t subject_body,std::uint32_t component,
    std::uint32_t event,NativeTu15PhysicsFrameFp&,const NativeTu15GameplayContactRemovalServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15GameplayContactRemovalStats* = nullptr);
// Actual1D8970. Current72-byte last row is copied forward even for self-copy.
void native_tu15_remove_gameplay_contact_row(SourcePhysicsContactMemory&,
    std::uint32_t component,std::uint32_t index,NativeTu15PhysicsFrameFp&,
    const NativeTu15GameplayContactRemovalServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15GameplayContactRemovalStats* = nullptr);
} // namespace halo2
