#pragma once
#include "halo2/render_program.hpp"

namespace halo2 {
struct SourceShadowFilterPass {
    unsigned source_target{},destination_target{};
    PixelProgram program;
    // Literal 34100 offsets in source pixels. Both source targets are128².
    std::array<ShaderVector,4> taps{};
};
// Fixed 222A0→211A0(2,2,.5,11,NONE,10,1,1,0,0).
// This is the required color-shadow filter, independent of presentation size.
std::array<SourceShadowFilterPass,2> source_shadow_filter_passes();
}
