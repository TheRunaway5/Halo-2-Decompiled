#pragma once
#include "gameplay/objects/native_tu15_object_body_source_owner.hpp"

namespace halo2::native_object_lookup_copies_detail {
// Complete Scene/Sandbox/Particle owners are authenticated independently.
template<bool Literal=true,class State>std::uint32_t bridge(State&state){
    return native_object_body_owner_detail::bridge<Literal>(state);
}
} // namespace halo2::native_object_lookup_copies_detail
