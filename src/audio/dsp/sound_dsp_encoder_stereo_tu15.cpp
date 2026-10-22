#include "halo2/sound_dsp_encoder_stereo_tu15.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::int32_t signed_word(std::uint32_t word) {
  if(word>0xffffffu)throw std::invalid_argument("EP stereo input exceeds 24 bits");
  return std::bit_cast<std::int32_t>(word<<8)>>8;
}
// The source MPYR/MACR rounds a doubled 24x24 product in the 56-bit
// accumulator to its 24-bit grid, with ties to even. MOVE then limits it.
std::int32_t rounded_word(std::int64_t accumulator) {
  constexpr std::int64_t grid=std::int64_t{1}<<24;
  auto quotient=accumulator>>24;
  const auto remainder=accumulator-quotient*grid;
  if(remainder>grid/2 || (remainder==grid/2 && (quotient&1)))++quotient;
  return static_cast<std::int32_t>(std::clamp(quotient,std::int64_t{-0x800000},std::int64_t{0x7fffff}));
}
std::uint32_t encoded_word(std::int32_t word) {return static_cast<std::uint32_t>(word)&0xffffffu;}
std::int16_t fifo_word(std::uint32_t word) {
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(word>>8));
}
}

SoundDspEncoderStereoOutputTu15 source_sound_dsp_encoder_stereo_tu15(
    const SoundDspEncoderStereoInputTu15 &input) {
  // Literal Y:0..9 values written by the original overlay at P:032E..0348.
  std::array<std::array<std::int32_t,2>,5> coefficients{{
      {0x7fffff,0},{0x5a8279,0x5a8279},{0,0x7fffff},
      {0x5a8279,0},{0,0x5a8279}}};
  const auto scale=signed_word(input.scale_word);
  (void)signed_word(input.scale_enabled);
  if(input.scale_enabled)for(auto &pair:coefficients)for(auto &coefficient:pair)
    coefficient=rounded_word(std::int64_t{coefficient}*scale*2);
  SoundDspEncoderStereoOutputTu15 output{};
  for(std::size_t i=0;i<256;++i) {
    std::int64_t left{},right{};
    for(std::size_t channel=0;channel<5;++channel) {
      const auto sample=signed_word(input.channels[channel][i]);
      left+=std::int64_t{sample}*coefficients[channel][0]*2;
      right+=std::int64_t{sample}*coefficients[channel][1]*2;
    }
    // Keep the complete six-channel input contract checked even though the
    // original stereo program does not request the sixth ring's samples.
    (void)signed_word(input.channels[5][i]);
    output.left[i]=encoded_word(rounded_word(left));
    output.right[i]=encoded_word(rounded_word(right));
    output.interleaved[2*i]=fifo_word(output.left[i]);
    output.interleaved[2*i+1]=fifo_word(output.right[i]);
  }
  return output;
}
} // namespace halo2
