#pragma once
#include "halo2/particle_vertex.hpp"

namespace halo2 {
// Pixel implementation is supplied by the original render dispatch. The
// declaration override remains the separate 2243C0/23EA80 style selection.
struct ParticleMaterial {
  ParticleVertexSelection vertex;
  MaterialPass pass;
  std::array<std::optional<ShaderBitmap>,4> bitmaps;
  std::array<std::optional<Texture>,4> textures;
};
} // namespace halo2
