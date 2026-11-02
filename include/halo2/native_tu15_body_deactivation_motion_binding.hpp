#pragma once
#include "halo2/native_tu15_body_deactivation_listener.hpp"
#include "halo2/native_tu15_component_body_motion_snapshot.hpp"
namespace halo2 {
struct NativeTu15BodyDeactivationMotionBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15BodyDeactivationListenerStats listener;
    NativeTu15ComponentBodyMotionStats snapshots;
};
inline void native_tu15_notify_bound_body_deactivation_with_motion(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsListener listener,NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15BodyDeactivationMotionBindingActivity*activity=nullptr){
    NativeTu15BodyDeactivationMotionBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15BodyDeactivationListenerServices services;services.allocation=allocation;
    services.publish_component_body_motion=[&](auto component,auto index,auto&current){
        native_tu15_publish_component_body_motion(memory,component,index,current,{},observe,&out.snapshots);
    };
    try{native_tu15_body_deactivation_listener(memory,listener,body,fp,services,observe,&out.listener);++out.completed;}
    catch(...){++out.refusals;throw;}
}
}
