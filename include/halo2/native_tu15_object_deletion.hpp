#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ObjectDeletionScratch { std::uint32_t spatial_packet{}; };
struct NativeTu15ObjectDeletionServices {
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> detach_object; //C0F60(datum,1,0,0,0)
    std::function<void(std::uint32_t datum,std::uint32_t packet,NativeTu15PhysicsFrameFp&)> spatial_packet; //BA910
    std::function<void(std::uint32_t actor,std::uint32_t datum,std::uint32_t packet,NativeTu15PhysicsFrameFp&)> remove_spatial; //1D30A0(system4EABB0,bytes20)
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> release_projectile_controller; //1CE340
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> release_physics_component; //1D9F10
    std::function<void(std::uint32_t actor,std::uint32_t flags,NativeTu15PhysicsFrameFp&)> actor_flags; //B7E50
    std::function<void(std::uint32_t datum,std::uint32_t packet,NativeTu15PhysicsFrameFp&)> release_player_state; //1F5620
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> release_player_control; //E3E20(datum,1)
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> unregister_actor; //B7F00
    std::function<void(std::uint32_t datum,std::uint32_t owner,NativeTu15PhysicsFrameFp&)> actor_notification; //10DF30 dynamic owner
};
struct NativeTu15ObjectDeletionStats {
    std::uint64_t calls{},ancestor_queries{},root_queries{},spatial_queries{},
        detach_calls{},spatial_packets{},spatial_removals{},projectile_calls{},
        tracking_clears{},player_clears{},schedule_updates{},reference_clears{},
        dependent_calls{},actor_calls{},unbound_services{};
};
bool native_tu15_object_or_ancestor_deleting(SourcePhysicsContactMemory&,std::uint32_t datum,
    NativeTu15ObjectDeletionStats* = nullptr); //BBD10
bool native_tu15_object_root_attached(SourcePhysicsContactMemory&,std::uint32_t datum,
    NativeTu15ObjectDeletionStats* = nullptr); //BBD50
void native_tu15_clear_object_tracking(SourcePhysicsContactMemory&,std::uint32_t datum,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectDeletionStats* = nullptr); //C10C0
void native_tu15_clear_object_player(SourcePhysicsContactMemory&,std::uint32_t datum,
    std::int32_t player,const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectDeletionStats* = nullptr); //15DC40
void native_tu15_update_object_delete_schedule(SourcePhysicsContactMemory&,std::uint32_t datum,
    bool enabled,std::uint32_t ticks,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectDeletionStats* = nullptr); //BD980
void native_tu15_clear_object_references(SourcePhysicsContactMemory&,std::uint32_t datum,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectDeletionStats* = nullptr); //10FD90
// ActualBA570. Every remaining child is an explicit typed owner, never a
// virtual register adapter, replayed result or original-execution fallback.
void native_tu15_delete_object(SourcePhysicsContactMemory&,std::uint32_t datum,
    NativeTu15ObjectDeletionScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ObjectDeletionServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectDeletionStats* = nullptr);
}
