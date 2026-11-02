#pragma once
#include "halo2/native_tu15_contact_point_create.hpp"
#include "halo2/native_tu15_contact_point_storage.hpp"
#include "halo2/native_tu15_contact_constraint_registration.hpp"
#include "halo2/native_tu15_contact_collector_info.hpp"
#include "halo2/native_tu15_contact_body_update.hpp"
#include "halo2/native_tu15_contact_point_removal.hpp"
#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
#include <string>

namespace halo2 {
struct NativeTu15ContactPointCreationNativeServices {
    // Each provider receives the SAME guarded DATA view as322D20. A provider
    // owns real native allocation/activation/removal, never a recorded result.
    std::function<NativeTu15ArrayGrowthServices(SourcePhysicsContactMemory&)> allocation;
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t world,
        std::uint32_t island,NativeTu15PhysicsFrameFp&)> activate_island;
    std::function<NativeTu15ContactPointRemovalServices(SourcePhysicsContactMemory&)> removal;
    // Preferred existing whole native removal owner, including its genuine
    // gameplay/listener/constraint closure. The raw service factory above
    // remains available for the separately qualified component domain.
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t manager,
        std::uint16_t point,NativeTu15PhysicsFrameFp&)> remove_bound_point;
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t listener,
        NativeTu15ContactVirtualOwner,std::uint32_t event,
        NativeTu15PhysicsFrameFp&)> notify_listener;
    // Optional read-only diagnostics see the guarded initialized event at its
    // real callback phase. Padding remains unavailable through this view.
    std::function<void(const SourcePhysicsContactMemory&,std::uint32_t listener,
        NativeTu15ContactVirtualOwner,std::uint32_t event)> observe_listener;
};
struct NativeTu15ContactPointCreationBindingScratch {
    NativeTu15ContactPointCreateScratch creation;
    NativeTu15ContactPointRemovalScratch removal;
};
struct NativeTu15ContactPointCreationBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15ContactPointCreateStats creation;
    NativeTu15ContactPointStorageStats storage;
    NativeTu15ContactConstraintRegistrationStats registration;
    NativeTu15ContactCreationListenerStats listeners;
    NativeTu15PairDeletionNativeScopeActivity native_only;
};
// Sole native322D20 composition. Unknown listener/velocity/metadata owners
// refuse at their true service phase. No generic source executor is accepted.
inline std::uint16_t native_tu15_create_bound_contact_point(
    SourcePhysicsContactMemory&memory,std::uint32_t manager,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext context,std::uint32_t packet,
    NativeTu15ContactPointCreationBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const NativeTu15ContactPointCreationNativeServices&native,
    NativeTu15PairDeletionNativeScope&scope,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ContactPointCreationBindingActivity*activity=nullptr){
    NativeTu15ContactPointCreationBindingActivity local;auto&out=activity?*activity:local;
    ++out.attempts;NativeTu15PairDeletionNativeScopeGuard guard(scope,&out.native_only);
    const auto allocation=[&](SourcePhysicsContactMemory&view){
        if(!native.allocation)throw std::runtime_error("Native contact creation requires native allocation services");
        return native.allocation(view);
    };
    const auto metadata=[](SourcePhysicsContactMemory&view,std::uint32_t island,
        NativeTu15ContactVirtualOwner owner,std::uint32_t info){
        if(owner.identity!=0x3200b0u)
            throw std::runtime_error("Native contact creation requires its island metadata owner="+std::to_string(owner.identity));
        native_tu15_add_island_constraint_info(view,{island},{info});
    };
    const auto listeners=[&](SourcePhysicsContactMemory&view){
        NativeTu15ContactCreationListenerServices services;
        services.notify=[&view,&native](auto listener,auto owner,auto event,auto&state){
            // Actual2E4D60 is RET4 with no DATA/FP side effect.
            if(owner.identity!=0x2e4d60u&&!native.notify_listener)
                throw std::runtime_error("Native contact creation requires its listener owner="+std::to_string(owner.identity));
            if(native.observe_listener)native.observe_listener(view,listener,owner,event);
            if(owner.identity!=0x2e4d60u)native.notify_listener(view,listener,owner,event,state);
        };return services;
    };
    NativeTu15ContactPointCreateServices services;
    services.register_constraint=[&](auto&view,auto world,auto collector,auto info,auto&current){
        NativeTu15ContactConstraintRegistrationServices registration;
        registration.allocation=allocation(view);
        registration.get_info=[&](auto constraint,auto owner,auto output,auto&){
            if(owner.identity!=0x325e90u)
                throw std::runtime_error("Native contact creation requires its collector info owner="+std::to_string(owner.identity));
            native_tu15_get_contact_collector_info(view,constraint,{output});
        };
        registration.add_info=[&](auto island,auto owner,auto,auto output,auto&){metadata(view,island,owner,output);};
        registration.activate_island=[&](auto w,auto island,auto&state){
            if(!native.activate_island)throw std::runtime_error("Native contact creation requires native island activation");
            native.activate_island(view,w,island,state);
        };
        registration.listeners=listeners(view);
        native_tu15_register_world_contact_constraint(view,world,collector,info,current,
            registration,{},&out.registration);
    };
    services.allocate_point=[&](auto&view,auto collector,auto point,auto properties,auto info,auto&current){
        NativeTu15ContactPointStorageServices storage;storage.allocation=allocation(view);
        storage.add_info=[&](auto island,auto owner,auto,auto output,auto&){metadata(view,island,owner,output);};
        return native_tu15_allocate_contact_point_storage(view,collector,point,properties,
            {info},current,storage,{},&out.storage);
    };
    services.point_velocity=[](auto&view,auto motion,auto owner,auto point,auto output,auto&current){
        if(owner.identity!=0x323a10u)
            throw std::runtime_error("Native contact creation requires its point velocity owner="+std::to_string(owner.identity));
        native_tu15_contact_motion_point_velocity(view,motion,point,output,current);
    };
    services.notify_world=[&](auto&view,auto world,auto event,auto&current){
        native_tu15_notify_world_contact_creation(view,world,event,current,listeners(view),{},&out.listeners);
    };
    services.notify_body=[&](auto&view,auto body,auto event,auto&current){
        native_tu15_notify_body_contact_creation(view,body,event,current,listeners(view),{},&out.listeners);
    };
    services.remove_point=[&](auto&view,auto receiver,auto owner,auto id,auto&current){
        if(owner.identity==0x2e4d60u)return;
        if(owner.identity!=0x322f70u)
            throw std::runtime_error("Native contact creation requires its point removal owner="+std::to_string(owner.identity));
        if(native.remove_bound_point){native.remove_bound_point(view,receiver,id,current);return;}
        if(!native.removal)throw std::runtime_error("Native contact creation requires native point removal services");
        native_tu15_remove_contact_point(view,receiver,id,scratch.removal,current,native.removal(view));
    };
    try{
        const auto result=native_tu15_create_contact_point(memory,manager,first,second,context,
            packet,scratch.creation,fp,depth,services,observe,&out.creation);
        ++out.completed;return result;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2
