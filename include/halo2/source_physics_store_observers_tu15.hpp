#pragma once
#include "halo2/physics_contact_record.hpp"
#include <utility>
namespace halo2 {
// Each listener receives the same real store exactly once. Hold owning copies
// for this call so replacing a registration cannot invalidate an active tee.
inline SourcePhysicsContactRecordObserver source_physics_store_observers_tu15(
    SourcePhysicsContactRecordObserver caller,SourcePhysicsContactRecordObserver registered) {
    if(!caller)return registered;
    if(!registered)return caller;
    return [caller=std::move(caller),registered=std::move(registered)](const auto &store){
        caller(store);registered(store);
    };
}
}
