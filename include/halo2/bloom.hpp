#pragma once
#include "halo2/render_program.hpp"
namespace halo2 {
struct MenuBloom {
    PixelProgram threshold,blur,composite;
    std::array<float,5> radii{};
};
// Original 214F0/211A0/20F30 title path (no layer-17 HDR accumulation).
// Cinematic defaults from 22590/13870 give strength 0.375. The menu's
// rasterizer_bloom_override_threshold service supplies threshold 0.3.
MenuBloom menu_bloom(float threshold,float strength=.375f);
}
