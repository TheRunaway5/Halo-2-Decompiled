#pragma once
#include "halo2/native_tu15_object_deletion.hpp"
namespace halo2 {
struct NativeTu15ObjectDeletionBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15ObjectDeletionStats deletion;
};
// Sole production builder. All selected child owners without qualified native
// bindings refuse; there is no residual executor or supplied child result.
// No spatial packet is allocated here: its BA910 branch refuses before using
// packet DATA. Nonspatial completions do not access that storage.
inline void native_tu15_delete_bound_object(SourcePhysicsContactMemory& memory,
    std::uint32_t datum,NativeTu15PhysicsFrameFp& fp,
    const SourcePhysicsContactRecordObserver& observe={},
    NativeTu15ObjectDeletionBindingActivity* activity=nullptr){
    NativeTu15ObjectDeletionBindingActivity local;auto&result=activity?*activity:local;
    ++result.attempts;
    try{
        native_tu15_delete_object(memory,datum,{},fp,{},observe,&result.deletion);
        ++result.completed;
    }catch(...){++result.refusals;throw;}
}
}
