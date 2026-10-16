#pragma once
#include "halo2/native_tu15_sound_mono_distortion.hpp"
#include <array>
#include <cstdint>
namespace halo2::tu15_mono_parameter_conversion {
using Parameters=tu15_sound_mono_distortion::Parameters;
using Word24=tu15_sound::Word24;
// Explicit supplied conversion environment, not a claim about the live caller.
enum class FpMode:std::uint16_t {source_pretable_53=0x023f,double_53=0x027f,extended_64=0x037f};
Parameters convert_bounded_values(const std::array<std::uint32_t,7>& float_bits,FpMode);
Word24 quantize_finite_fraction(double,FpMode);
}
