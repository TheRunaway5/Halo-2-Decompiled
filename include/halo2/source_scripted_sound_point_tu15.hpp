#pragma once
#include "halo2/math.hpp"
namespace halo2 {
class PhysicsLiveWorldTu15;
// Original18CC0C..18CC36 selects marker04000095 through wholeBAD60.
// Resolve its completed WORLD matrix, including relative animation and the
// original missing-marker fallback. The caller supplies a live full handle.
Vec3 source_scripted_sound_point_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object);
}
