#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace halo2::tu15_sound_mono_distortion {
// Ordinary typed parameters and state; no source pointers or DSP banks.
using Word24=halo2::tu15_sound::Word24;
struct FilterCoefficients {
    Word24 current_input, previous_input, older_input, previous_output, older_output;
    bool operator==(const FilterCoefficients&) const = default;
};
struct Parameters {
    Word24 gain;
    FilterCoefficients before_clip, after_clip;
    bool operator==(const Parameters&) const = default;
};
struct FilterHistory {
    Word24 previous_input, older_input, previous_output, older_output;
    bool operator==(const FilterHistory&) const = default;
};
struct State {
    Word24 flags, last_dry_word;
    FilterHistory before_clip, after_clip;
    bool operator==(const State&) const = default;
};
enum class ScalingMode { none, down, up, reserved };
enum class AddressingMode { linear, modulo, reverse_carry };
struct Controls {
    ScalingMode entry_scaling;
    bool arithmetic_saturation, sixteen_bit_arithmetic, double_precision_multiply;
    AddressingMode coefficients, input, output, history;
    // Original R5-relative state reads require an explicitly owned linear M5.
    AddressingMode state_relative;
};

// Exactly 32 source samples. Exact alias and disjoint native spans are admitted;
// partial overlap is rejected before any mutation. flags bit 0 initialization
// follows P0198..019E; this is module state, not engine or DSP readiness.
// Only the traced arithmetic/linear-address modes are admitted. RM is irrelevant
// because this module executes no rounding instruction. MR scaling exits at zero.
void process(const Controls&, Parameters&, State&, std::span<const Word24,32> input,
             std::span<Word24,32> output);
Parameters authored_initial_parameters();
} // namespace tu15_candidate
