#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
// Literal object-table slot and current object+14/+4 fields. 3DC90 and BAF80
// index by the low16; neither verifies the handle salt or occupancy here.
struct SourceShadowLightingOwner {
    std::uint16_t slot{};
    std::optional<std::uint32_t> parent,object_flags;
};
enum class SourceShadowLightingStorage : std::uint32_t { absent,default_bank,owner_cache };
struct SourceShadowLightingSelection {
    SourceShadowLightingStorage storage=SourceShadowLightingStorage::absent;
    std::uint32_t owner=0xffffffffu,token=0xffffffffu;
};
// Complete 3DC90/BAF80 selection. The callback is actual 3DDD0 (including
// allocation/rebind side effects), not a geometry/material visibility test.
// valid_bytes is literal cache-record+3, indexed by the callback token low16.
// default_bank_present asserts the actual4DE2F4 bank, whose selected offset
// is +1C. A selected owner-cache record's lighting starts at +54.
SourceShadowLightingSelection source_shadow_lighting_selection(
    std::span<const SourceShadowLightingOwner>,std::uint32_t owner,
    bool default_bank_present,
    const std::function<std::uint32_t(std::uint32_t)>& acquire,
    std::span<const std::optional<std::uint8_t>> valid_bytes);
}
