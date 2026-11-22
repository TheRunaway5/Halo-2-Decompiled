#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
class SourceRuntimeMemory;
struct SourceCullDrawUnit;

struct SourceLiveProjectedModelPaletteTu15 {
    bool resident{};
    std::uint32_t object{};
    std::vector<std::array<float,12>> rows;
};

// Validate the completed native resource before resolving its object. The
// provider validates that object and may override its selected c50 rows with
// the presented pose. The selected World rows are owned before this callout.
SourceLiveProjectedModelPaletteTu15 source_live_projected_model_palette_tu15(
    SourceRuntimeMemory&,const SourceCullDrawUnit&,
    const std::function<std::span<const std::array<float,12>>(std::uint32_t,std::uint32_t)>&);
}
