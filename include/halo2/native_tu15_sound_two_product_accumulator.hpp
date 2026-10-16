#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstdint>
#include <span>
namespace halo2::tu15_sound_two_product_accumulator {
using Word24=halo2::tu15_sound::Word24;
struct Parameters {Word24 primary_gain,secondary_gain;bool operator==(const Parameters&)const=default;};
struct State {Word24 flags;bool operator==(const State&)const=default;};
enum class AddressingMode {linear,modulo,reverse_carry};
struct Controls {
    // This word is a numeric source input on flags.bit1==0. It is explicit
    // observed original entry context, not a register-machine runtime.
    // Reject SC13 and unowned incoming LF15/FV16, scaling, SA, DM and reserved bits.
    Word24 entry_status;
    AddressingMode primary,secondary,output;
    // Original R5-relative state reads require an explicitly owned linear M5.
    AddressingMode state_relative;
};
struct ArithmeticTrace {std::int64_t primary_product,first_sum,secondary_product,final_sum;bool operator==(const ArithmeticTrace&)const=default;};
struct Result {Word24 captured_status_operand;bool exit_arithmetic_saturation;};
// Source bodies are independently proved equivalent under their state/output
// relocation. Three disjoint native spans; no partial/exact overlap admitted.
Result process(const Controls&,const Parameters&,State&,std::span<const Word24,32> primary,
               std::span<const Word24,32> secondary,std::span<Word24,32> output,
               std::span<ArithmeticTrace,32> arithmetic);
Parameters authored_parameters();
} // namespace halo2::tu15_sound_two_product_accumulator
