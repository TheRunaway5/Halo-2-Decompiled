#pragma once
#include "physics/runtime/native_tu15_packed_rotation_frame.hpp"
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include <cstdlib>

namespace halo2::native_packed_rotation_owner_detail {
inline bool reference(){static const bool enabled=[]{const auto*value=std::getenv("HALO2_REFERENCE_PACKED_ROTATION");return value&&*value;}();return enabled;}
inline std::uint64_t&activity(){thread_local std::uint64_t value{};return value;}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!s.services.reciprocal_sqrt||
       !native_model_marker_lookup_detail::protocol(s.services,s.observe)||!native_integer_fp_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction?s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor:
        !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;return false;
#endif
}
template<class State>bool bridge(State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s))return false;++activity();native_packed_rotation_detail::run_private(s);return true;
#else
    (void)s;return false;
#endif
}
} // namespace halo2::native_packed_rotation_owner_detail
