#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
struct ParticleVertexSelection {
  std::uint32_t template_datum{},pass_datum{},vertex_datum{},classification{};
  std::vector<std::byte> program;
};
} // namespace halo2
