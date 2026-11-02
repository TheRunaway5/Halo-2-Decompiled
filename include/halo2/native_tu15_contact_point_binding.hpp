#pragma once
#include "halo2/native_tu15_constraint_reference_input_observer.hpp"
#include "halo2/native_tu15_contact_point_removal.hpp"
#include "halo2/native_tu15_contact_world_update.hpp"
#include "halo2/native_tu15_gameplay_contact_removal.hpp"
#include "halo2/native_tu15_constraint_reference_release.hpp"
#include "halo2/native_tu15_part_contact_binding_extended.hpp"
#include <bit>
#include "halo2/native_tu15_island_constraint_detach.hpp"
#include <stdexcept>
#include <string>

namespace halo2 {
struct NativeTu15ContactPointBindingScratch {
    std::uint32_t event{},removal_info{},result_byte{},detach_info{};
};
struct NativeTu15ContactPointBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},unsupported_profiles{},unqualified_services{},
        noop_listeners{},gameplay_listeners{},collector_updates{};
    NativeTu15GameplayContactRemovalStats gameplay;
    NativeTu15ConstraintReferenceReleaseStats reference;
    NativeTu15PartContactExtendedBindingActivity part;
    NativeTu15ContactWorldUpdateStats world;
    NativeTu15IslandConstraintDetachStats detach;
};
// Complete qualified actual1470 point pipeline. All callbacks below are native.
// Unknown/runtime-effect/zero-reference owners and unsupported authored numerical profiles refuse; there is no CPU fallback.
// The caller owns each DATA range. detach_info may equal removal_info: the
// point metadata consumer has returned before325E90 fully rewrites that16B.
// Selected PART callbacks require removal_info to own20B (including its4B gap)
// and result_byte to own4B. Their temporary values die before the later consumers.
inline void native_tu15_remove_bound_contact_point(SourcePhysicsContactMemory& memory,
    std::uint32_t manager,std::uint32_t point,NativeTu15ContactPointBindingScratch scratch,
    NativeTu15PhysicsFrameFp& fp,unsigned incoming_x87_depth,
    const NativeTu15ArrayGrowthServices& allocation={},
    const SourcePhysicsContactRecordObserver& observe={},NativeTu15ContactPointBindingActivity* activity=nullptr){
    NativeTu15ContactPointBindingActivity local;auto& result=activity?*activity:local;++result.attempts;
    try{
        const auto bad_profile=[&](const char* why){++result.unsupported_profiles;throw std::runtime_error(why);};
        const auto valid=[](std::uint32_t p,unsigned bytes){return p&&std::uint64_t(p)+bytes<=0x100000000ull;};
        const auto overlap=[](std::uint32_t a,unsigned an,std::uint32_t b,unsigned bn){return std::uint64_t(a)<std::uint64_t(b)+bn&&std::uint64_t(b)<std::uint64_t(a)+an;};
        if(incoming_x87_depth)bad_profile("Native TU1.5 point binding requires its qualified empty live x87 profile");
        if(!valid(scratch.event,20)||!valid(scratch.removal_info,16)||!valid(scratch.result_byte,1)||!valid(scratch.detach_info,16)||
           overlap(scratch.event,20,scratch.removal_info,16)||overlap(scratch.event,20,scratch.result_byte,1)||overlap(scratch.removal_info,16,scratch.result_byte,1)||
           (scratch.detach_info!=scratch.removal_info&&(overlap(scratch.detach_info,16,scratch.event,20)||overlap(scratch.detach_info,16,scratch.removal_info,16)||overlap(scratch.detach_info,16,scratch.result_byte,1))))
            bad_profile("Native TU1.5 point binding requires owned disjoint event/info/result DATA lifetimes");
        const auto unqualified=[&](const char* service,std::uint32_t owner=0){++result.unqualified_services;
            throw std::runtime_error(std::string("Native TU1.5 point binding requires qualified ")+service+" owner="+std::to_string(owner));};
        NativeTu15GameplayContactRemovalServices gameplay;
        gameplay.release_constraint_reference=[&](auto instance,auto datum,auto&state){
            native_tu15_observe_constraint_reference_input(memory,{instance,datum,state,incoming_x87_depth});
            // Empty runtime service preserves the source conditional child
            // boundary: the native core counts/refuses22FEA0 when selected.
            native_tu15_release_constraint_reference(memory,instance,datum,state,
                incoming_x87_depth,{},observe,&result.reference);
        };
        gameplay.release_part_contact=[&](auto slot,auto signed_part,auto component,auto& state){
            // Source invokes the gameplay listener before325D90 initializes
            // removal_info, before the world result, and before detach_info.
            // These owned DATA aliases therefore have disjoint live phases.
            const NativeTu15PartContactResponseScratch part_scratch{
                scratch.removal_info,scratch.removal_info+12u,
                {scratch.removal_info+16u,scratch.result_byte}};
            if(!valid(scratch.removal_info,20)||!valid(scratch.result_byte,4)||
               overlap(scratch.removal_info,20,scratch.event,20)||
               overlap(scratch.result_byte,4,scratch.event,20)||
               overlap(scratch.removal_info,20,scratch.result_byte,4))
                bad_profile("Native TU1.5 PART binding requires owned20B info/gap and4B result DATA");
            native_tu15_update_bound_part_contact_response_extended(memory,slot,
                std::bit_cast<std::int32_t>(std::uint32_t(signed_part)),component,
                part_scratch,state,incoming_x87_depth,allocation,observe,&result.part);
        };
        NativeTu15IslandConstraintDetachServices detach;
        detach.get_info=[&](auto,auto owner,auto,auto&){unqualified("constraint info",owner.identity);};
        detach.remove_info=[&](auto,auto owner,auto,auto,auto&){unqualified("island metadata",owner.identity);};
        detach.destroy_constraint=[&](auto,auto owner,auto,auto&){unqualified("zero-reference destruction",owner.identity);};
        NativeTu15ContactWorldUpdateServices world;
        world.allocation.allocate=[&](const auto& request,auto& state){
            if(!allocation.allocate){unqualified("native queue allocation",request.owner);return std::uint32_t{};}
            return allocation.allocate(request,state);
        };
        world.allocation.release=[&](const auto& request,auto& state){
            if(!allocation.release)unqualified("native queue release",request.owner);allocation.release(request,state);
        };
        world.listener=[&](auto,auto owner,auto,auto&){
            if(owner.identity!=0x2e4d60u)unqualified("world listener",owner.identity);
            ++result.noop_listeners;
        };
        world.remove_from_island=[&](auto island,auto owner,auto collector,auto& state){
            if(owner.identity!=0x3201b0u)unqualified("island detach",owner.identity);
            native_tu15_detach_island_constraint(memory,{island},collector,{scratch.detach_info},state,detach,observe,&result.detach);
        };
        NativeTu15ContactPointRemovalServices services;
        services.listener_removed=[&](auto,auto owner,auto event,auto& state){
            if(owner.identity==0x2e4d60u){++result.noop_listeners;return;}
            if(owner.identity!=0x281d40u)unqualified("removed-point listener",owner.identity);
            ++result.gameplay_listeners;
            native_tu15_notify_gameplay_contact_removed(memory,event,state,gameplay,observe,&result.gameplay);
        };
        services.collector_removed=[&](auto island,auto owner,auto,auto info,auto&){
            if(owner.identity!=0x320100u)unqualified("point metadata",owner.identity);
            ++result.collector_updates;native_tu15_remove_island_constraint_info(memory,{island},{info},observe);
        };
        services.update_world=[&](auto receiver,auto output,auto collector,auto& state){
            native_tu15_update_contact_world(memory,receiver,output,collector,state,world,observe,&result.world);
        };
        native_tu15_remove_contact_point(memory,manager,point,{scratch.event,scratch.removal_info,scratch.result_byte},fp,services,observe);
        ++result.completed;
    }catch(...){++result.refusals;throw;}
}
} // namespace halo2
