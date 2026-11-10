#pragma once
#include "halo2/native_tu15_simplex_helpers_handoff.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#if defined(HALO2_NATIVE_SIMPLEX_TRIANGLE_SOURCE_OWNER)
#include "physics/runtime/native_tu15_simplex_triangle_abi.hpp"
#endif
namespace halo2::physics_world_runtime_detail {
// Invoke only after one of the five genuine solver helper calls has pushed its
// authenticated return. A qualified whole triangle owner must enter through
// its real run_309900 body, preserving owner accounting and complete returned
// state. The residual scalar handoff publishes only EAX/FP/DATA.
template<class SolverState>
inline bool handoff_native_simplex_helpers(SolverState&s,std::uint32_t target,
    std::uint32_t tu15_instruction){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
#if defined(HALO2_NATIVE_SIMPLEX_TRIANGLE_SOURCE_OWNER)
    if(target==0x30d3f0u&&(tu15_instruction==0x30dad9u||tu15_instruction==0x30e3b6u||
        tu15_instruction==0x30eb85u||tu15_instruction==0x30ece2u)){
        if(native_simplex_triangle_detail::reference()){
            ++native_simplex_triangle_detail::activity().world_reference_deferrals;return false;
        }
        if(native_simplex_triangle_detail::admitted(s)){
            ++native_simplex_triangle_detail::activity().world_owner_deferrals;return false;
        }
        ++native_simplex_triangle_detail::activity().legacy_handoff_fallbacks;
    }
#endif
    auto actual=s.save();
    try{
        if(!native_tu15_handoff_simplex_helpers(s.memory,
            {tu15_instruction,target,false},actual,s.observe))return false;
    }catch(...){s.load(actual);throw;}
    s.load(actual);return true;
}
} // namespace halo2::physics_world_runtime_detail
