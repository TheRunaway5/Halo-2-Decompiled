#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>

// Native object node owners (track "nodes"). See docs/NATIVE_OBJECT_FACTORY.md.
namespace halo2::native_object_factory_tu15 {
// C15D0: the object has an animation manager (body word +12A is not -1).
bool object_has_animation(const SourcePhysicsContactMemory&,std::uint32_t object);
// BAC00: node matrix `node` (sign-extended word) of the object: body +
// [+116] + node * 0x34.
std::uint32_t object_node_matrix(const SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t node);
}
