#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
namespace halo2::tu15_sound_gain_fanout {
using Word24=halo2::tu15_sound::Word24;
struct Parameters {std::array<Word24,8> gains;bool operator==(const Parameters&)const=default;};
// A logical source selection, never an address or readiness flag. The original
// publishes its first gain field; its external consumer remains unresolved.
enum class GainSource { sequence_begin };
struct State {
    Word24 flags;
    std::optional<GainSource> published_gain_source;
    bool operator==(const State&)const=default;
};
enum class ScalingMode { none,down,up,reserved };
enum class AddressingMode { linear,modulo,reverse_carry };
struct Controls {
    ScalingMode entry_scaling;
    bool arithmetic_saturation,sixteen_bit_arithmetic,double_precision_multiply;
    AddressingMode input,destination_table,gain_sequence,output;
    // Original R5-relative state reads require an explicitly owned linear M5.
    AddressingMode state_relative;
};
using Input=std::span<const Word24,32>;
using Output=std::span<Word24,32>;
// Eight ordered routes. Output spans may repeat exactly (the authored last four
// do); partial output overlap and any input/output overlap are unsupported.
void process(const Controls&,const Parameters&,State&,Input,const std::array<Output,8>&);
Parameters authored_parameters();
} // namespace halo2::tu15_sound_gain_fanout
