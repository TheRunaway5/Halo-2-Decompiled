#pragma once
#include "halo2/native_tu15_object_deletion_detaching_binding.hpp"
#include "halo2/native_tu15_object_spatial_packet.hpp"
#include <optional>
#include <stdexcept>
namespace halo2 {
struct NativeTu15ObjectDeletionSpatialBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15ObjectDeletionStats deletion;
    NativeTu15ObjectDetachmentStats detachment;
    NativeTu15ObjectSpatialPacketStats spatial;
};
// Native BA570/C0F60 with locally computed BA910/16E290 packet DATA and
// native1D30A0/29FFF0 updates. No numeric address represents a CPU frame.
// Remaining attachment/controller/AI/player children retain explicit refusal.
inline void native_tu15_delete_spatially_bound_object(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ObjectDeletionSpatialBindingActivity*activity=nullptr){
    NativeTu15ObjectDeletionSpatialBindingActivity local;auto&result=activity?*activity:local;
    ++result.attempts;
    try{
        NativeTu15ObjectDeletionServices services;
        std::optional<NativeTu15ObjectSpatialPacketData>packet;
        services.detach_object=[&](auto object,auto&state){
            native_tu15_detach_object_tree(memory,{object,1u,0u,0u,0u},state,{},observe,&result.detachment);
        };
        services.spatial_packet=[&](auto object,auto,auto&){
            packet=native_tu15_build_object_spatial_packet(memory,object,&result.spatial);
        };
        services.remove_spatial=[&](auto first_datum,auto object,auto,auto&){
            if(!packet)throw std::runtime_error("Native object deletion spatial packet lifetime is absent");
            native_tu15_update_object_spatial_chain(memory,0x4eabb0u,first_datum,object,*packet,observe,&result.spatial);
        };
        native_tu15_delete_object(memory,datum,{},fp,services,observe,&result.deletion);
        ++result.completed;
    }catch(...){++result.refusals;throw;}
}
}
