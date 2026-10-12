#pragma once
#include "halo2/atmosphere.hpp"

namespace halo2 {
// Post130DE0 inputs. Interpolation is bank+11C, distinct from the planar
// transition at+118. Supply14B360's low byte and retained bank+4/+8/+C RGB.
struct SourceFogClearInput {
    Vec3 retained_color{};
    std::uint8_t sky_model_control{};
    Vec3 sky_color_premultiplied{};
    float sky_opacity{};
    AtmosphericFog primary{},secondary{};
    float planar_density{},interpolation{};
};
// Original12E68F..12E88B RGB selection only. Requires finite inputs and
// nearest-even, gradual binary32 arithmetic. The authored override follows
// this operation; caller admission, sky draw flags and depth clears are separate.
Vec3 source_fog_clear_color(const SourceFogClearInput &);
// Original131ED0, including x87 precision/rounding and byte wrap. Exceptions
// must be masked and PC must be24/53/64. No live caller FP state is inferred.
std::uint32_t source_clear_color_rgb8(Vec3,std::uint16_t x87_control_word);
}
