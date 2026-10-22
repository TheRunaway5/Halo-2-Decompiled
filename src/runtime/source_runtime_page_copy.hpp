#pragma once
#include <algorithm>
#include <cstring>
#include <span>
#include <stdexcept>

namespace halo2::source_runtime_detail {
// Both spans cover complete copy units (source pages for virtual reservations).
// The destination is a fresh zero-filled
// native allocation whose pages have already been made writable. Copy every
// nonzero unit, preserving all bytes without writing unused zero units.
inline void restore_zero_filled_pages(std::span<std::byte> destination,
    std::span<const std::byte> source, std::size_t page_bytes) {
    if (!page_bytes || destination.size() != source.size() || source.size() % page_bytes)
        throw std::invalid_argument("Source snapshot page-copy extents differ");
    for (std::size_t offset = 0; offset < source.size(); offset += page_bytes) {
        const auto page = source.subspan(offset, page_bytes);
        if (std::any_of(page.begin(), page.end(), [](auto byte) { return byte != std::byte{}; }))
            std::memcpy(destination.data() + offset, page.data(), page.size());
    }
}
} // namespace halo2::source_runtime_detail
