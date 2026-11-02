#pragma once
#include "halo2/native_tu15_simplex_helpers_handoff.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
namespace halo2::physics_world_runtime_detail {
// Invoke only after one of the five genuine solver helper calls has pushed its
// authenticated return. A completed native typed call publishes only its typed EAX result and FP/DATA, with no GP/XMM postimage.
inline bool handoff_native_simplex_helpers(WorldState&s,std::uint32_t target,
    std::uint32_t tu15_instruction){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    auto actual=s.save();
    try{
        if(!native_tu15_handoff_simplex_helpers(s.memory,
            {tu15_instruction,target,false},actual,s.observe))return false;
    }catch(...){s.load(actual);throw;}
    s.load(actual);return true;
}
} // namespace halo2::physics_world_runtime_detail
