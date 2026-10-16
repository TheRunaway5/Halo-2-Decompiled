#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace halo2::tu15_sound_crosstalk {
using Word24=halo2::tu15_sound::Word24;
struct PairState {
    std::array<std::array<Word24,11>,2> histories;
    std::array<Word24,2> previous;
    unsigned cursor{};
    bool operator==(const PairState&)const=default;
};
struct State {
    Word24 flags;
    std::array<PairState,2> pairs;
    // Cold source initialization overwrites both coefficient fields.
    std::array<Word24,2> coefficients;
    bool operator==(const State&)const=default;
};
enum class AddressingMode {linear,modulo,reverse_carry};
struct Controls {
    // Explicit entry bounds only; no complete native CCR producer is claimed.
    Word24 entry_status;
    // Flag/coefficient displacement accesses use M5 before source forces
    // M0..M5 linear. Output M6/M7 also remain caller-owned entry modes.
    AddressingMode state_relative,first_output,second_output;
};
struct ArithmeticTrace {
    std::array<std::int64_t,8> values;
    bool operator==(const ArithmeticTrace&)const=default;
};
struct Result {bool exit_arithmetic_saturation;};
using Input=std::span<const Word24,32>;
using Output=std::span<Word24,32>;
// Four disjoint original input blocks and four disjoint output blocks.
// Two sequential stereo pairs share the authored coefficients. Each pair
// retains two11-word histories, two filtered words and an explicit cursor.
Result process(const Controls&,State&,const std::array<Input,4>&,
               const std::array<Output,4>&,std::span<ArithmeticTrace,64>);
} // namespace halo2::tu15_sound_crosstalk
