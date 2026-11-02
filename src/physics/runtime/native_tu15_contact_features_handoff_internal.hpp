#pragma once
#include "halo2/native_tu15_contact_features_handoff.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
namespace halo2::physics_world_runtime_detail {
// Invoke only after one of the two genuine virtual34 calls has pushed its
// authenticated return. A completed native void call needs no GP/XMM postimage.
inline bool handoff_native_contact_features(WorldState&s,std::uint32_t target,
    std::uint32_t tu15_instruction){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    auto actual=s.save();
    try{
        if(!native_tu15_handoff_contact_features(s.memory,
            {tu15_instruction,target,false},actual,s.observe))return false;
    }catch(...){s.load(actual);throw;}
    s.load(actual);return true;
}
} // namespace halo2::physics_world_runtime_detail
