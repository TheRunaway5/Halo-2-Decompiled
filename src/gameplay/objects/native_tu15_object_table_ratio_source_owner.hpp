#pragma once
#include "gameplay/objects/native_tu15_object_table_ratio_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_object_table_ratio_owner_detail {
template<bool Parent> inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv(Parent?"HALO2_REFERENCE_OBJECT_TABLE_RATIO_MODE":"HALO2_REFERENCE_OBJECT_TABLE_RATIO");
        return value&&*value;
    }();
    return enabled;
}
template<bool Parent> inline std::uint64_t& activity() {thread_local std::uint64_t count{};return count;}

template<bool Literal,class State> bool bridge_leaf(State& state) {
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference<false>())return false;
    ++activity<false>();
    native_object_table_ratio_detail::run_source_frame<Literal>(state);
    return true;
}
template<bool Literal,class State,class Child> bool bridge_parent(State& state,Child child) {
    if(state.image!=SourcePhysicsContactImage::TitleUpdate15||reference<true>())return false;
    ++activity<true>();
    native_object_table_ratio_detail::run_mode_owner<Literal>(state,child);
    return true;
}
} // namespace halo2::native_object_table_ratio_owner_detail
