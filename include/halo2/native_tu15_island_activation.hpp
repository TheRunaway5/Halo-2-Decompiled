#pragma once
#include "halo2/native_tu15_island_transition.hpp"
namespace halo2 {
struct NativeTu15IslandActivationServices {
    std::function<void(NativeTu15PhysicsArrayHeader,std::uint32_t stride,
        NativeTu15PhysicsFrameFp&)>grow_array;
    std::function<void(NativeTu15PhysicsListener,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&)>island_activated;
    std::function<void(NativeTu15PhysicsListener,NativeTu15PhysicsVirtualOwner,
        NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&)>body_activated;
    std::function<void(NativeTu15PhysicsBody,NativeTu15PhysicsVirtualOwner,
        std::uint32_t disposing,NativeTu15PhysicsFrameFp&)>destroy_body;
};
struct NativeTu15IslandActivationStats {
    std::uint64_t body_calls{},island_calls{},growths{},deferred_removals{},
        protected_references{},destroy_calls{},island_notifications{},body_notifications{},
        native_default_notifications{},null_entries_removed{};
};
// Actual283450: signed original reservation-range predicate. It reads both
// real globals at their source phase, even when the enable byte is zero.
bool native_tu15_reserved_body_reference(SourcePhysicsContactMemory&,NativeTu15PhysicsBody);
// Actual322660: reverse world callbacks, null compaction, forward body cohort
// with reverse entity callbacks and live cohort/list/count rereads.
void native_tu15_notify_island_activation(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandActivationServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandActivationStats* = nullptr);
// Actual316D40. Genuine growth remains explicit until supplied by its already
// qualified native array owner. Notification callbacks own their real work.
void native_tu15_activate_physics_island(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandActivationServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandActivationStats* = nullptr);
// Actual3165D0 removes deferred references using the original forward scan,
// including its post-removal index increment and true virtual destruction.
void native_tu15_remove_deferred_physics_body(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandActivationServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandActivationStats* = nullptr);
// Actual315CB0. Reread the body's world after deferred removal; never invent a
// missing island or suppress the original invalid-address/failure boundary.
void native_tu15_activate_physics_body(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,
    const NativeTu15IslandActivationServices&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15IslandActivationStats* = nullptr);
} // namespace halo2
