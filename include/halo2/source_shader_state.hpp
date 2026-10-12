#pragma once
#include "halo2/render_program.hpp"

namespace halo2 {
// Original 1A170's seven render-state descriptor rows at 467020. The caller
// supplies the actual state words after ordinary pass states have run. This
// returns every ordered write (including equal values) and updates that bank.
// Graphics packet publication and deferred-device dirty masks are separate.
// Native admission checks compiled row indices; the original does not check
// them. Byte +2 is ignored, and byte +1 == 1 selects scalar, otherwise RGB.
std::vector<ShaderState> remap_source_shader_states(
    std::span<std::uint32_t> state_words,std::span<const float> scalars,
    std::span<const ShaderVector> colors,std::span<const ShaderRemap>,
    std::uint16_t x87_control_word);
}
