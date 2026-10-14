#pragma once
#include "halo2/source_effect_location.hpp"
#include <cstdint>
#include <vector>
namespace halo2 {
class SourceRuntimeMemory;
// Complete original17BA50 resolves every actual linked location, including
//encoded FP graph nodes. Writes are restricted to the native caller's scratch;
//no source effect, pose, allocator, random stream or pool is changed.
std::vector<SourceEffectLocation> source_effect_world_locations_tu15(
    const SourceRuntimeMemory &,std::uint32_t instance,std::uint32_t expected_definition);
}
