#pragma once
#include "gameplay/objects/native_tu15_object_record_ratio_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_object_record_ratio_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_OBJECT_RECORD_RATIO");
        return value&&*value;
    }();
    return enabled;
}
inline std::uint64_t& activity() {thread_local std::uint64_t count{};return count;}

template<bool Literal,class State> bool bridge(State& state) {
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference())return false;
    ++activity();
    native_object_record_ratio_detail::run_source_frame<Literal,true>(state);
    return true;
}
} // namespace halo2::native_object_record_ratio_owner_detail
