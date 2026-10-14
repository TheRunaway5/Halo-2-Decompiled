#pragma once
#include "halo2/render_program.hpp"
namespace halo2 {
// Literal52D30 CPU program stores. Bitmap, sampler, viewport and vertex
// declaration admission are separate native resource services.
PixelProgram source_contrail_pixel_program(std::uint16_t blend,
    std::uint8_t properties,std::uint32_t declaration_flags=0);
}
