#pragma once
#include "gameplay/objects/native_tu15_object_body_raw_frame.hpp"
#include <cstdlib>

namespace halo2::native_vehicle_object_lookup_owner_detail {
inline bool reference(){
    static const bool enabled=[]{const auto*value=std::getenv("HALO2_REFERENCE_VEHICLE_OBJECT_LOOKUP");return value&&*value;}();
    return enabled;
}
inline std::uint64_t&activity(){thread_local std::uint64_t value{};return value;}
// The raw view preserves arbitrary callbacks and source provider side effects;
// it introduces no floating-point admission or observer capability contract.
template<class State>bool bridge(State&s){
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    ++activity();native_object_body_detail::run_raw_frame(s);return true;
}
} // namespace halo2::native_vehicle_object_lookup_owner_detail
