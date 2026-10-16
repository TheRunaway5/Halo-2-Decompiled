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
// TU1.5 214D0 reads these fields only while interface+5 is true. -1 is
// the original per-field sentinel, not an ordinary threshold/strength.
// Opening scripts leave blur/box/max/alpha controls at their source defaults.
struct CinematicBloomOverride {
    bool enabled{};
    float threshold=-1.f,brightness=-1.f;
};
struct CinematicBloomParameters {float threshold{},strength{};};
CinematicBloomParameters cinematic_bloom_parameters(float threshold,float strength,
    bool cinematic,const CinematicBloomOverride &);
}
