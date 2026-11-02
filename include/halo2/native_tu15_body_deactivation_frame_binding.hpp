#pragma once
#include "halo2/native_tu15_body_deactivation_listener.hpp"
#include "halo2/native_tu15_component_body_motion_snapshot.hpp"
#include "halo2/native_tu15_body_constraint_update.hpp"
namespace halo2 {
struct NativeTu15BodyDeactivationFrameBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15BodyDeactivationListenerStats listener;
    NativeTu15ComponentBodyMotionStats snapshots;
    NativeTu15ModelConstraintUpdateStats frames;
};
inline void native_tu15_notify_bound_body_deactivation_with_frames(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsListener listener,NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15BodyDeactivationFrameBindingActivity*activity=nullptr){
    NativeTu15BodyDeactivationFrameBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15ComponentBodyMotionServices motion;
    motion.component_frame=[&](auto component,auto index,auto&current){
        const auto frame=native_tu15_model_constraint_body_frame(memory,{component},index,current,&out.frames);
        std::array<std::uint32_t,13>words{};words[0]=frame.scale;
        for(unsigned n=0;n<3;++n){words[1+n]=frame.forward[n];words[4+n]=frame.side[n];words[7+n]=frame.up[n];words[10+n]=frame.position[n];}
        return words;
    };
    NativeTu15BodyDeactivationListenerServices services;services.allocation=allocation;
    services.publish_component_body_motion=[&](auto component,auto index,auto&current){
        native_tu15_publish_component_body_motion(memory,component,index,current,motion,observe,&out.snapshots);
    };
    try{native_tu15_body_deactivation_listener(memory,listener,body,fp,services,observe,&out.listener);++out.completed;}
    catch(...){++out.refusals;throw;}
}
}
