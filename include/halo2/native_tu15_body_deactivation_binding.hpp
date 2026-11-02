#pragma once
#include "halo2/native_tu15_body_deactivation_listener.hpp"
namespace halo2 {
struct NativeTu15BodyDeactivationBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15BodyDeactivationListenerStats listener;
};
inline void native_tu15_notify_bound_body_deactivation(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsListener listener,NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15BodyDeactivationBindingActivity*activity=nullptr){
    NativeTu15BodyDeactivationBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15BodyDeactivationListenerServices services;services.allocation=allocation;
    // Native1D7260 is an explicit remaining child. The core counts/refuses at
    // its actual selected boundary; this binding supplies no result/fallback.
    try{native_tu15_body_deactivation_listener(memory,listener,body,fp,services,observe,&out.listener);++out.completed;}
    catch(...){++out.refusals;throw;}
}
}
