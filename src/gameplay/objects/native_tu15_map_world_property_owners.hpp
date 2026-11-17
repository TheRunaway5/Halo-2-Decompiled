#pragma once
#include "gameplay/objects/native_tu15_map_world_property_caller_source_frame.hpp"
#include "gameplay/objects/native_tu15_map_world_property_query_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_map_world_property_owner_detail {
template<bool Caller> bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv(Caller?"HALO2_REFERENCE_MAP_WORLD_PROPERTY_CALLER":
            "HALO2_REFERENCE_MAP_WORLD_PROPERTY_QUERY");
        return value&&*value;
    }();
    return enabled;
}
template<bool Caller> std::uint64_t& activity() {
    thread_local std::uint64_t calls{};return calls;
}

template<bool Literal,class State> bool bridge_query(State& state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference<false>())return false;
    ++activity<false>();native_map_world_property_query_detail::run_source_frame<Literal>(state);return true;
#else
    (void)state;return false;
#endif
}
template<bool Literal,class State,class Child> bool bridge_caller(State& state,Child child) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference<true>())return false;
    ++activity<true>();native_map_world_property_caller_detail::run_source_frame<Literal>(state,child);return true;
#else
    (void)state;(void)child;return false;
#endif
}
} // namespace halo2::native_map_world_property_owner_detail
