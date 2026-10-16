#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstdint>
#include <span>
namespace halo2::tu15_sound_controller_arithmetic {
using Word24=halo2::tu15_sound::Word24;
struct Parameters {Word24 threshold,gain;bool operator==(const Parameters&)const=default;};
// This state projects only the source arithmetic fields. flags.bit0 records
// arithmetic counter initialization here; it does not establish that the
// separate source DMA descriptor has been initialized or is ready to consume.
struct State {Word24 flags,counter;bool operator==(const State&)const=default;};
enum class AddressingMode {linear,modulo,reverse_carry};
struct Controls {
    Word24 entry_status;
    AddressingMode storage_read,storage_write,state_relative;
};
struct Emission {
    std::uint32_t input_index{};
    std::int64_t fractional_product{};
    Word24 limited_product;
    std::array<std::int64_t,8> addition_results{};
    Word24 output;
    bool operator==(const Emission&)const=default;
};
struct Result {
    std::uint32_t produced_count{};
    std::array<Emission,32> emissions{};
};
// Authentic input/output views alias exactly. The caller owns this one native
// block for the whole synchronous call. Writes pack a produced prefix behind
// the consumed index; the remaining words retain their original values.
// This is only the arithmetic prefix. The returned count is not a DMA receipt,
// scratch-offset advance, device clock, PCM format or playback completion.
Result process(const Controls&,const Parameters&,State&,std::span<Word24,32> storage);
Parameters authored_parameters();
} // namespace halo2::tu15_sound_controller_arithmetic
