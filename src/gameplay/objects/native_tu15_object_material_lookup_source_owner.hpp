#pragma once
#include "gameplay/objects/native_tu15_object_body_source_owner.hpp"

namespace halo2::native_object_material_owner_detail {
// Distinct authenticated source owner, sharing the proved childless kernel.
template<bool Literal=true,class State>std::uint32_t bridge(State&state){
    return native_object_body_owner_detail::bridge<Literal>(state);
}
} // namespace halo2::native_object_material_owner_detail
