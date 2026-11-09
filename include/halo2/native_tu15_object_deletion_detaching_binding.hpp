#pragma once
#include "halo2/native_tu15_object_deletion_binding.hpp"
#include "halo2/native_tu15_object_detachment.hpp"
namespace halo2 {
struct NativeTu15ObjectDeletionDetachingBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15ObjectDeletionStats deletion;
    NativeTu15ObjectDetachmentStats detachment;
};
// Native BA570/C0F60/C5250 and its ancestor predicate. The remaining selected
// attachment deletion/spatial/controller/AI/player callbacks are dependencies.
inline void native_tu15_delete_detaching_bound_object(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ObjectDeletionDetachingBindingActivity*activity=nullptr){
    NativeTu15ObjectDeletionDetachingBindingActivity local;auto&result=activity?*activity:local;
    ++result.attempts;
    try{
        NativeTu15ObjectDeletionServices services;
        services.detach_object=[&](auto object,auto&state){
            NativeTu15ObjectDetachmentServices detach;
            detach.detach_attachment=[&](auto attachment,auto&){
                native_tu15_detach_light_attachment(memory,attachment,observe,&result.detachment.attachments);
            };
            native_tu15_detach_object_tree(memory,{object,1u,0u,0u,0u},state,detach,observe,&result.detachment);
        };
        native_tu15_delete_object(memory,datum,{},fp,services,observe,&result.deletion);
        ++result.completed;
    }catch(...){++result.refusals;throw;}
}
}
