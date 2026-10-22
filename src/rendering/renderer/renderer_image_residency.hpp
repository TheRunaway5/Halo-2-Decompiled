#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <string_view>

namespace halo2::detail {
// This chooses an equality bucket, not an image identity. Full byte and mip
// equality below always decides sharing, including deliberately colliding keys.
inline std::size_t image_residency_bucket(std::span<const std::byte> pixels) {
    const auto hash = [](const std::byte *data, std::size_t size) {
        return std::hash<std::string_view>{}(size ? std::string_view{
            reinterpret_cast<const char *>(data), size} : std::string_view{});
    };
    if (pixels.size() <= 256)
        return hash(pixels.data(), pixels.size());
    std::array<std::byte, 256> samples;
    const auto extent = pixels.size() - 16;
    for (unsigned i = 0; i < 16; ++i) {
        const auto offset = (extent / 15) * i + (extent % 15) * i / 15;
        std::memcpy(samples.data() + i * 16, pixels.data() + offset, 16);
    }
    return hash(samples.data(), samples.size());
}
inline bool same_resident_image(const Texture &a, const Texture &b) {
    return a.width == b.width && a.height == b.height && a.format == b.format &&
        a.layers == b.layers && a.depth == b.depth && a.pixels == b.pixels &&
        a.mips.size() == b.mips.size() &&
        std::equal(a.mips.begin(), a.mips.end(), b.mips.begin(), [](const auto &x, const auto &y) {
            return x.width == y.width && x.height == y.height && x.offset == y.offset &&
                x.size == y.size && x.depth == y.depth;
        });
}
} // namespace halo2::detail
