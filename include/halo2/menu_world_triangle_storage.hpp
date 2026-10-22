#pragma once
#include "halo2/menu_world_source_storage.hpp"

namespace halo2 {
// Wire payload for triangle ABI1 set1/binding3: 16 uvec4 stream descriptors,
// then source-ordered uint indices. Offsets address the caller's raw_storage
// slice; no vertex values or floating computations are supplied by the host.
// Rejects address overflow, nonmatching resources/layouts and any fetch beyond
// an actual source resource. Returned words own no source or Vulkan lifetime.
std::vector<std::uint32_t> menu_world_triangle_storage(const MenuWorldSourceStoredDraw &);
}
