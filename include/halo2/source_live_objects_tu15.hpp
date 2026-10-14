#pragma once
#include "halo2/source_model_regions.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
struct SourceLiveObjectTu15 {
    std::uint32_t object{},definition{},parent{};
    std::uint8_t type{};
};
// Occupied full source generations with constructed bodies. Native header
// reservations with no body remain with their own constructor, not this list.
std::vector<SourceLiveObjectTu15> source_live_objects_tu15(const SourceRuntimeMemory &,
    NativeRuntimeAddress object_pool);
// Actual BC570/BC6C0/1700E0 region banks and full looping-effect handles.
// Reads completed state; does not select a variant or consume another RNG draw.
SourceModelRegionBank source_live_model_regions_tu15(const SourceRuntimeMemory &,
    NativeRuntimeAddress object_pool,std::uint32_t object);
// BCD80's current half of the two RGB banks. Absent channels stay absent.
std::vector<Vec3> source_live_object_colors_tu15(const SourceRuntimeMemory &,
    NativeRuntimeAddress object_pool,std::uint32_t object);
}
