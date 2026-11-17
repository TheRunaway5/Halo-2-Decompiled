#pragma once
#include <cstdint>
#include <cstdlib>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "gameplay/animation/native_tu15_node_function_progress_source_frame.hpp"
#endif

namespace halo2::native_node_function_progress_owner_detail {
inline bool reference() {
    static const bool enabled = [] {
        const auto* value = std::getenv("HALO2_REFERENCE_NODE_FUNCTION_PROGRESS");
        return value && *value;
    }();
    return enabled;
}

inline std::uint64_t& activity() {
    thread_local std::uint64_t count{};
    return count;
}

template<bool Literal, class State, class Dispatch>
bool bridge(State& state, Dispatch dispatch) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || reference()) return false;
    ++activity();
    native_node_function_progress_detail::run_frame<Literal>(state, dispatch);
    return true;
#else
    (void)state;
    (void)dispatch;
    return false;
#endif
}
} // namespace halo2::native_node_function_progress_owner_detail
