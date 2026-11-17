#pragma once
#include <cstdint>
#include <cstdlib>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "native_tu15_object_function_query_source_frame.hpp"
#endif

namespace halo2::native_object_function_query_owner_detail {
inline bool reference() {
    static const bool enabled = [] {
        const auto* value = std::getenv("HALO2_REFERENCE_OBJECT_FUNCTION_QUERY");
        return value && *value;
    }();
    return enabled;
}
inline std::uint64_t& activity() {
    thread_local std::uint64_t count{};
    return count;
}

template<class Effect, class Progress, class Inherited>
struct Children {
    Effect effect_ratio;
    Progress progress;
    Inherited inherited;
};

template<bool Literal, class State, class Routes>
bool bridge(State& state, Routes children) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || reference()) return false;
    ++activity();
    native_object_function_query_detail::run_source_frame<Literal>(state, children);
    return true;
#else
    (void)state;
    (void)children;
    return false;
#endif
}
} // namespace halo2::native_object_function_query_owner_detail
