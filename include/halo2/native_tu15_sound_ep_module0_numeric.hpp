#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace halo2::tu15_ep_module0_numeric {
using Word24=tu15_sound::Word24;
using Inputs=std::array<std::array<Word24,256>,5>;
enum class AddressingMode {linear,modulo,reverse_carry};
struct Controls {Word24 entry_status;std::array<AddressingMode,8> address_modes;};
struct Parameters {Word24 initialization_requested,gain_level;};
struct SectionHistory {
    // Source section order: previous/older output, previous/older input.
    std::array<Word24,4> samples{};
    Word24 residual_high{},residual_low{};
};
struct State {
    std::array<SectionHistory,4> left{},right{};
    std::array<SectionHistory,5> surround{};
};
struct Result {
    std::array<std::array<Word24,256>,2> output;
    std::array<Word24,2> peak,energy_root;
    Word24 scale_coefficient;
};
// Full fixed module0 matrix, histories and diagnostics. The caller owns
// actual input acquisition, persistent State, reset flag, FP-free arithmetic
// context, frame phase and transport. This supplies no installer/frame ACK.
Result process(const Controls&,const Parameters&,const Inputs&,State&);
std::array<std::byte,1024> pack_interleaved_s16(const Result&);
}
