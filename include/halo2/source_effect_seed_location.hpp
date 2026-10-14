#pragma once
#include "halo2/transform.hpp"

namespace halo2 {
// Complete TU1.5 17B580 / retail1786F0 arithmetic for a declared seed marker.
// The constructor179540 supplies1791C0's up/gravity packet at the actual object
// origin. The result is retained locally and composed with current node zero.
Transform source_effect_seed_location(const Transform *node,Vec3 position,Vec3 forward);
}
