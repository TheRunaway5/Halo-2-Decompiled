#pragma once
#include "halo2/native_tu15_phantom_added_input_observer.hpp"
#include "halo2/native_tu15_phantom_added_listener.hpp"
#include <stdexcept>
#include <string>
namespace halo2 {
struct NativeTu15PhantomAddedBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},unqualified_deletions{};
    NativeTu15PhantomAddedListenerStats listener;
};
inline void native_tu15_notify_bound_gameplay_phantom_added(SourcePhysicsContactMemory&memory,
    std::uint32_t listener,NativeTu15PhantomOverlapAddedEvent event,
    NativeTu15PhysicsFrameFp&fp,unsigned actual_x87_depth,
    const NativeTu15PhantomAddedListenerServices&object_deletion={},
    const SourcePhysicsContactRecordObserver&observer={},NativeTu15PhantomAddedBindingActivity*activity=nullptr) {
    NativeTu15PhantomAddedBindingActivity local;auto&result=activity?*activity:local;++result.attempts;
    try {
        native_tu15_observe_phantom_added_input(memory,{NativeTu15PhantomAddedInputPhase::ListenerEntry,0x282700u,listener,event.identity,0u,fp,actual_x87_depth});
        NativeTu15PhantomAddedListenerServices services;
        services.delete_object=[&](auto datum,auto&state){
            native_tu15_observe_phantom_added_input(memory,{NativeTu15PhantomAddedInputPhase::BeforeObjectDeletion,0xba570u,listener,event.identity,datum,state,actual_x87_depth});
            if(!object_deletion.delete_object){++result.unqualified_deletions;throw std::runtime_error("Native TU1.5 phantom-added binding requires qualifiedBA570 object="+std::to_string(datum));}
            object_deletion.delete_object(datum,state);
        };
        native_tu15_notify_gameplay_phantom_added(memory,event,fp,services,observer,&result.listener);
        ++result.completed;
    }catch(...){++result.refusals;throw;}
}
} // namespace halo2
