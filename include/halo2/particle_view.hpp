#pragma once
#include "halo2/cache.hpp"
#include <span>

namespace halo2 {
using ParticleViewConstants=std::array<ShaderVector,22>;
// 23E460's upload at physical vertex registers 148..169 (game 52..73).
// Input preserves the original nine-float 4856C4 orientation layout. The
// caller owns the original dirty flag, camera state and upload scheduling.
ParticleViewConstants particle_view_constants(std::span<const float,9> orientation);
}
