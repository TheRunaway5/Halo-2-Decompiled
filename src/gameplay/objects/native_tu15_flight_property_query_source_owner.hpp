#pragma once
#include "gameplay/objects/native_tu15_flight_property_query_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_flight_property_query_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_FLIGHT_PROPERTY_QUERY");
        return value&&*value;
    }();
    return enabled;
}
inline std::uint64_t& activity() {thread_local std::uint64_t count{};return count;}

template<bool Literal,bool Fresh,class State,class Spill> bool bridge(State& state,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    ++activity();
    native_flight_property_query_detail::run_source_frame<Literal,Fresh>(state,spill);
    return true;
#else
    (void)state;(void)spill;
    return false;
#endif
}
} // namespace halo2::native_flight_property_query_owner_detail
