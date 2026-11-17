#pragma once
#include "gameplay/animation/native_tu15_node_progress_query_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_node_progress_query_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_NODE_PROGRESS_QUERY");
        return value&&*value;
    }();
    return enabled;
}
inline std::uint64_t& activity() {thread_local std::uint64_t count{};return count;}

template<bool Literal,class State,class Child> bool bridge(State& state,Child lookup) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    ++activity();
    native_node_progress_query_detail::run_source_frame<Literal>(state,lookup);
    return true;
#else
    (void)state;(void)lookup;
    return false;
#endif
}
} // namespace halo2::native_node_progress_query_owner_detail
