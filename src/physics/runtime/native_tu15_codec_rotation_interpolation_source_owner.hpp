#pragma once
#include "physics/runtime/native_tu15_codec_rotation_interpolation_arithmetic.hpp"
#include "physics/runtime/native_tu15_codec_rotation_interpolation_source_frame.hpp"
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include <cstdlib>
namespace halo2::native_codec_rotation_interpolation_owner_detail {
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_CODEC_ROTATION_INTERPOLATION");return p&&*p;}();return enabled;}
struct Activity{std::uint64_t calls{},completed{},continuations{};};
inline Activity&activity(){thread_local Activity value{};return value;}
template<bool Literal=true,class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!s.services.reciprocal_sqrt||!native_integer_fp_detail::supported(s)||
       !native_model_marker_lookup_detail::protocol(s.services,s.observe))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction?s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor:
        !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;return false;
#endif
}
template<bool Literal=true,class State>bool continuation_admitted(const State&s,std::uint32_t next){
    if(!admitted<Literal>(s))return false;
    std::uint32_t address{};
    switch(next){
    case 0x296299u:address=s.gp[0];break;
    case 0x29629cu:address=s.gp[1];break;
    case 0x2962ccu:address=s.gp[2];break;
    default:break;
    }
    return !(address&15u);
}
template<bool Literal=true,class State>std::uint32_t bridge(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted<Literal>(s))return 0u;++activity().calls;
    const auto resume=native_codec_rotation_interpolation_detail::run_source(s,[](const auto&current,std::uint32_t next){return continuation_admitted<Literal>(current,next);});
    if(resume){++activity().continuations;return resume;}++activity().completed;return 1u;
#else
    (void)s;return 0u;
#endif
}
} // namespace halo2::native_codec_rotation_interpolation_owner_detail
