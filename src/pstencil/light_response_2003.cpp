#include "halo2/pstencil/light_response_2003.hpp"
#include "halo2/cache.hpp"
#include <cmath>

namespace halo2::pstencil {
namespace {
#include "light_response_2003_programs.inc"
}
std::uint32_t pack_light_rgb_2003(const std::array<float, 3> &color) {
    std::uint32_t result{};
    for (unsigned i = 0; i < 3; ++i) {
        if (!std::isfinite(color[i]) || color[i] < 0 || color[i] > 1)
            throw FormatError("2003 light RGB requires completed finite unit-range colors");
        const double scaled = double(color[i]) * 255.0;
        auto byte = static_cast<std::uint32_t>(scaled);
        const double fraction = scaled - double(byte);
        if (fraction > .5 || (fraction == .5 && (byte & 1u)))
            ++byte;
        result |= byte << ((2u - i) * 8u);
    }
    return result;
}
PixelProgram light_utility_2003(unsigned geometry, unsigned diffuse, unsigned specular, int quality,
                                bool dull) {
    if (geometry > 3 || diffuse > 3 || specular > 3 || quality < -1 || quality > 2)
        throw FormatError("2003 light utility selector outside verified constructor domain");
    const auto index = unsigned(dull) * 64u + unsigned(geometry != 0) * 32u + unsigned(quality != 0) * 16u +
                       diffuse * 4u + specular;
    return {utility_2003_programs[index], {}};
}
PixelProgram dependent_light_2003(unsigned geometry, unsigned mask, unsigned shift, std::uint32_t diffuse,
                                  std::uint32_t specular) {
    if (geometry > 3 || mask > 3 || shift > 2 || (diffuse & 0xff000000u) || (specular & 0xff000000u))
        throw FormatError("2003 dependent light requires verified selectors and packed RGB colors");
    PixelProgram result{dependent_2003_programs[unsigned(geometry != 0) * 12u + mask * 3u + shift], {}};
    result.words[11] = diffuse;
    result.words[13] = specular;
    return result;
}
} // namespace halo2::pstencil
