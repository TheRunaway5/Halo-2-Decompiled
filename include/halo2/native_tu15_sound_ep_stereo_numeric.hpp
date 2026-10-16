#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace halo2::tu15_ep_stereo_numeric {
using Word24=tu15_sound::Word24;
using Inputs=std::array<std::array<Word24,256>,5>;
enum class AddressingMode {linear,modulo,reverse_carry};
struct Controls {Word24 entry_status;std::array<AddressingMode,8> address_modes;};
struct Parameters {Word24 scale_enabled,scale;};
struct Result {
 std::array<std::array<Word24,256>,2> output;
 std::array<Word24,2> peak,energy_root;
 std::array<Word24,10> coefficients;
};
// P032E..03B1 numeric fragment only. Inputs already belong to the caller.
// No loader, input acquisition, processor status receipt, FIFO or clock.
Result process(const Controls&,const Parameters&,const Inputs&);
// Source format1 word>>8 packing, observed from the original loader request
// under the bounded reference DMA model. This creates bytes, no FIFO receipt.
std::array<std::byte,1024> pack_interleaved_s16(const Result&);
}
