#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
struct ParticleVertexSelection {
  // vertex_datum is null for a code-owned custom Vulkan signature when the
  // imported level has no preserved source vertex-tag/declaration namespace.
  std::uint32_t template_datum{},pass_datum{},vertex_datum{},classification{};
  std::vector<std::byte> program;
};
} // namespace halo2
