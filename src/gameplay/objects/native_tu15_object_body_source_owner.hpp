#pragma once
#include "gameplay/objects/native_tu15_object_body_source_frame.hpp"
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include <cstdlib>

namespace halo2::native_object_body_owner_detail {
// The exact TU1.5 owner hands off only after a provider instruction retires.
inline bool reference(){
    static const bool enabled=[]{const auto*value=std::getenv("HALO2_REFERENCE_OBJECT_BODY");return value&&*value;}();
    return enabled;
}
inline std::uint64_t&activity(){thread_local std::uint64_t value{};return value;}
template<bool Literal=true,class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_raw_x87_detail::supported(s)||
       !native_model_marker_lookup_detail::protocol(s.services,s.observe))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction?s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor:
        !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;return false;
#endif
}
// 0 declines an untouched entry, FFFFFFFF completes, and every other result
// identifies the next actual TU1.5 instruction in the retained suffix.
template<bool Literal=true,class State,class Sites=native_object_body_detail::ActualSites>std::uint32_t bridge(State&s,Sites sites={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted<Literal>(s))return 0u;
    ++activity();
    const auto next=native_object_body_detail::run_source_frame(s,[](const auto&current){return admitted<Literal>(current);},native_object_body_detail::Noop{},sites);
    return next?next:0xffffffffu;
#else
    (void)s;return 0u;
#endif
}
} // namespace halo2::native_object_body_owner_detail
