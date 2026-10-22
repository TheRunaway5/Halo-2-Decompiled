#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
unsigned retail_mip_count(unsigned width, unsigned height, unsigned depth,
                          unsigned format, bool linear, unsigned maximum) {
    // Retail 000359E0 and texture-header caller 0012D6D0. Supported inputs
    // are positive cache dimensions (1..4096).
    if (!width || !height || !depth || width > 4096 || height > 4096 || depth > 4096)
        throw FormatError("invalid bitmap dimensions for retail mip count");
    unsigned count = 0;
    if (!linear && std::has_single_bit(width) && std::has_single_bit(height) && std::has_single_bit(depth)) {
        const auto largest = format >= 14 && format <= 16
            ? std::max({width / 4, height / 4, depth}) : std::max({width, height, depth});
        count = static_cast<unsigned>(std::bit_width(largest)) - 1;
    }
    return std::min(count, maximum);
}
}

// Instruction-oracle testing only; the native renderer never executes x86.
extern "C" unsigned halo2_bitmap_mip_count(unsigned width, unsigned height, unsigned depth,
                                          unsigned format, unsigned linear, unsigned maximum) {
    return halo2::retail_mip_count(width, height, depth, format, linear != 0, maximum);
}
