#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
struct SourceBitmapPendingHeaderMetadata {
    std::int16_t width{};
    std::int16_t height{};
    std::int16_t format{};
    std::int16_t mipmap_count{};
    std::int8_t depth{};
    std::uint16_t type{};
    std::uint16_t flags{};
};

// Complete pure12D6D0 with its actual359E0/1365A0/13E3A0 arithmetic paths.
// Admit format0..23 and partition0..2. Preserve signed narrow dimensions,
// wrapped maximum levels, zero reductions and literal format-table values.
// Output is Common/Data/Lock/Format/Size. No resource readiness or I/O is
// inferred. Invalid metadata rejects before any caller can mutate its banks.
std::array<std::uint32_t,5> source_bitmap_pending_header(
    const SourceBitmapPendingHeaderMetadata &metadata,std::uint32_t partition,
    std::uint32_t source_level_address);
} // namespace halo2
