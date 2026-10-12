#pragma once
#include "halo2/item_transform_math.hpp"

namespace halo2 {
struct SourceItemNodesStorage {
  std::span<std::byte> body;
  std::span<const std::byte> definition, model, render_model, render_nodes;
  std::span<std::byte> fallback_nodes;
};
struct SourceItemNodeStore {
  bool fallback;
  SourceMatrixMathStore write;
  friend bool operator==(const SourceItemNodeStore &, const SourceItemNodeStore &) = default;
};
using SourceItemNodeObserver = std::function<void(const SourceItemNodeStore &)>;
// Original BDF80 measured unparented, entity/physical-NONE world weapons.
// Current/local banks retain original descriptors; fallback node construction
// is native. Actual NULL callback entries and RCPSS remain explicit inputs.
void source_item_current_nodes(SourceItemNodesStorage,
    const SourceNodeMatrixInputs &, std::span<const std::uint32_t> notification_callbacks,
    const SourceItemNodeObserver & = {});
} // namespace halo2
