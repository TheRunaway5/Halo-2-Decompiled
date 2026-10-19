#pragma once
#include "halo2/source_effect_location.hpp"
#include <cstdint>
#include <vector>
namespace halo2 {
class SourceRuntimeMemory;
// Original17DCE0(mode0) admits WORLD/unbound linked locations; complete
//17BA50 resolves each admitted transform. Encoded FP records sharing the same
//marker name are separate owners, not extra WORLD part dispatches. Writes stay
//in caller scratch; no source effect, pose, allocator, RNG or pool is changed.
std::vector<SourceEffectLocation> source_effect_world_locations_tu15(
    const SourceRuntimeMemory &,std::uint32_t instance,std::uint32_t expected_definition);
}
