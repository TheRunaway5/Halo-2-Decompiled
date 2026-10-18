#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Original EP stereo overlay, P:032E..0379, in its unscaled/convergent
// rounding mode. Channels follow the actual GP/EP rings: FL,C,FR,RL,RR,LFE.
// Each sample and the optional coefficient scale are raw signed 24-bit words.
// The original stereo overlay reads the first five rings; LFE is not added.
struct SoundDspEncoderStereoInputTu15 {
  std::array<std::array<std::uint32_t,256>,6> channels;
  std::uint32_t scale_enabled;
  std::uint32_t scale_word;
};
struct SoundDspEncoderStereoOutputTu15 {
  std::array<std::uint32_t,256> left,right;
  // The original FIFO's 16-bit conversion drops the low eight bits, after
  // the DSP's rounded/limited 24-bit stores. It does not round again.
  std::array<std::int16_t,512> interleaved;
};
// Numerical downmix/packing only. This does not initialize DSPs, route voices,
// perform GP effects or EP peak/RMS metering, or establish output timing.
SoundDspEncoderStereoOutputTu15 source_sound_dsp_encoder_stereo_tu15(
    const SoundDspEncoderStereoInputTu15 &);
} // namespace halo2
