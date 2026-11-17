#pragma once
#include "gameplay/objects/native_tu15_object_ratio_caller_ordered.hpp"
#include <cstdint>
#include <cstdlib>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "gameplay/objects/native_tu15_object_ratio_caller_source_frame.hpp"
#endif

namespace halo2::native_object_ratio_caller_owner_detail {
template<class ParentMode,class RecordRatio,class Combat>
struct Routes {
    ParentMode object_parent_mode;
    RecordRatio record_ratio;
    Combat combat;
};

inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_OBJECT_RATIO_CALLER");
        return value&&*value;
    }();
    return enabled;
}

inline std::uint64_t& activity() {thread_local std::uint64_t count{};return count;}

template<bool Literal,class State,class Children> bool bridge(State& state,Children children) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    ++activity();
    native_object_ratio_caller_detail::run_source_frame<Literal,true>(state,children);
    return true;
#else
    (void)state;
    (void)children;
    return false;
#endif
}
} // namespace halo2::native_object_ratio_caller_owner_detail
