#pragma once
#include "gameplay/objects/native_tu15_object_type_query_source_frame.hpp"
#include "gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include <cstdlib>

namespace halo2::native_object_type_query_owner_detail {
// Entry admission preserves untouched refusal and retired-provider suffixes.
inline bool reference() {
    static const bool enabled=[]{
        const auto*value=std::getenv("HALO2_REFERENCE_OBJECT_TYPE_QUERY");
        return value&&*value;
    }();
    return enabled;
}
inline std::uint64_t&activity(){thread_local std::uint64_t value{};return value;}
template<bool Literal=true,class State>bool admitted(const State&s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(!Literal)if(s.services.instruction)return false;
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))return false;
    // HALO2_CENSUS_NATIVE_BRIDGES: full census callbacks without a cursor.
    // 10DE40..10DEB2 holds no live service PC, the frame writes the cursor
    // only when there is one, and the virtual callee keeps its own admission.
    if constexpr(Literal)if(native_census_detail::uncursored(s,0x10de40u,0x10deb2u)) {
        if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
        return true;
    }
    if(!native_model_marker_lookup_detail::protocol(s.services,s.observe))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    return s.services.instruction?s.literal_service_sites&&s.instruction_cursor==&s.services.instruction_cursor:
        !s.instruction_cursor&&!s.literal_service_sites;
#else
    (void)s;return false;
#endif
}
// Zero declines an untouched entry; FFFFFFFF completes; all other results
// identify the first untouched source statement after a retired provider/child.
template<bool Literal=true,class State,class Call>std::uint32_t bridge(State&s,Call call) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted<Literal>(s))return 0u;
    ++activity();
    const auto next=native_object_type_query_detail::run_source_frame(s,call,
        [](const auto&current){return admitted<Literal>(current);});
    return next?next:0xffffffffu;
#else
    (void)s;(void)call;return 0u;
#endif
}
} // namespace halo2::native_object_type_query_owner_detail
