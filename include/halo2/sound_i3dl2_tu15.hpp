#pragma once
#include "halo2/native_tu15_sound_ring_effect.hpp"
#include "halo2/sound_i3dl2_reverb.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// DirectSound's I3DL2 code in the TU1.5 executable (its DSOUND section; the
// retail image's addresses are named), feeding the DSP's I3DL2 reverb
// records 8 and 9 (tu15_sound_ring_effect) and the voice processor's
// per-voice I3DL2 filters.

// The reverb record's parameter words: X words 70..135 of its state, which
// XAudioSetEffectData (37BA6F) builds with 3838A4 for an I3DL2 listener and
// writes at byte 0x118 of the record:
//   70..74   reflection tap delays (bytes of 24 kHz words)
//   75       short reverb input delay
//   76..83   the four cascade lines' two taps each
//   84..87   reflection feedback delays
//   88       long reverb feedback delay
//   89..96   short reverb input gains (diffusion, signed)
//   97, 98   long reverb input (1 - diffusion) and cross-feed (0.2 x diffusion)
//   99..104  reflection, short and long output gains (x 2^-7)
//   105      output channels (4)
//   106..135 ten one-pole filters: history, pole, gain
using SoundI3dl2ReverbWordsTu15 = std::array<std::uint32_t, 66>;
SoundI3dl2ReverbWordsTu15 sound_i3dl2_reverb_words_tu15(const SoundI3dl2Parameters &);

// SetEffectData of those words and of the record's flags (bit 2), then
// CommitEffectData: the record takes them at its next block, gliding its
// output gains and crossfading its moved taps.
void sound_i3dl2_reverb_commit_tu15(tu15_sound_ring_effect::State &, const SoundI3dl2ReverbWordsTu15 &);

// Record 8 or 9 as XAudioDownloadEffectsImage installs it from the TU1.5
// DSPImage, with zeroed histories. Record 8 reads mixbin 12 and writes
// mixbins 19..22; record 9 reads mixbin 11 and writes 15..18. Effects 2
// and 3 copy the I3DL2 send (mixbin 10) into both inputs.
tu15_sound_ring_effect::State sound_i3dl2_reverb_installed_tu15(unsigned record);

// DSI3DL2BUFFER: a voice's I3DL2 source, as SetI3DL2Source stores it.
struct SoundI3dl2SourceTu15 {
  std::int32_t direct = 0, direct_hf = 0, room = 0, room_hf = 0;
  float room_rolloff = 0;
  std::int32_t obstruction = 0;
  float obstruction_lf_ratio = 0;
  std::int32_t occlusion = 0;
  float occlusion_lf_ratio = 0.25f;
};

// 38368B's results for one voice: its direct and room (mixbin 10) levels
// in millibels, and the poles of the voice processor's one-pole low-passes
// on each path, y = b*y + (1-b)*x with b = pole/65536 (0 leaves the path
// open). DirectSound designs them from the HF loss alone; the level is the
// path's volume. Its room HF filter reads lRoom, never lRoomHF.
struct SoundI3dl2VoiceTu15 {
  std::int32_t direct = 0, room = 0;
  std::uint16_t direct_pole = 0, room_pole = 0;
  friend bool operator==(const SoundI3dl2VoiceTu15 &, const SoundI3dl2VoiceTu15 &) = default;
};

// 38368B (from CDirectSound3DCalculator::GetVoiceData, 37AE57).
// rolloff_distance is the 3D calculator's distance term; DirectSound's
// listener keeps its default room rolloff (0) and 5000 Hz HF reference.
SoundI3dl2VoiceTu15 sound_i3dl2_voice_tu15(const SoundI3dl2SourceTu15 &, float rolloff_distance = 0,
                                           float listener_rolloff = 0, float hf_reference = 5000);

// 3834F3: the 16-bit pole of a unity-DC one-pole low-pass that loses hf
// millibels at the reference frequency.
std::uint16_t sound_i3dl2_pole_tu15(std::int32_t dc, std::int32_t hf, float reference, float rate);

// 383167: DirectSound's millibel-to-linear conversion.
float sound_i3dl2_level_tu15(std::int32_t millibels);

// A voice's DLS2 filter as the voice processor takes it: coefficient words
// FC0 and FC1 (TAR_FCA). 2211E0 sets it on every 3D voice in DLS2 mode,
// which on a 3D voice keeps the I3DL2 filters (coefficient words 2 and 3)
// as well. The words are a Chamberlin state-variable low-pass's
//   f = 2 sin(pi fc / 48000) = 2^(int16(cutoff) / 4096), at most 1 (8 kHz),
//   q = 1 / Q = resonance / 32768, the damping; its gain at fc is 1 / q.
// At 8 kHz and 0 dB (f = 65535/65536, q = 1) it is flat within 0.001 dB.
struct SoundDls2FilterTu15 {
  std::uint16_t cutoff = 0, resonance = 0x8000;
  friend bool operator==(const SoundDls2FilterTu15 &, const SoundDls2FilterTu15 &) = default;
};

// 21E270 (TU1.5 220380): a cutoff in Hz to FC0. Below 30 Hz it is 0x8000,
// above 8000 Hz it is 0.
std::uint16_t sound_dls2_cutoff_tu15(float hz);

// 21E2D0 (TU1.5 2203E0): a resonance in dB, at most 22.5, to FC1:
// 32768 x 10^(-dB/20), saturated at 0xFFFF.
std::uint16_t sound_dls2_resonance_tu15(float db);

// SetFilter's mode, Q field and four coefficient words, as built by 226C00.
struct SoundPlaybackFilterTu15 {
  std::uint8_t mode{}, q{};
  std::array<std::uint16_t, 4> coefficients{};
  friend bool operator==(const SoundPlaybackFilterTu15 &, const SoundPlaybackFilterTu15 &) = default;
};
std::optional<SoundPlaybackFilterTu15> source_sound_playback_filter_tu15(
    std::span<const std::uint32_t, 18> authored, float control, std::uint32_t seed, bool stereo);

// The voice processor's f for a cutoff word: 2^(int16(word) / 4096) taken
// to a 1024th of an octave, then to the nearest 1/65536, at most FFFFh.
// Retail-console measurements (xemu pull request 3105): word D97Eh gave
// 3043h, and word 0 passed 1 - 2^-16 of an impulse.
double sound_dls2_frequency_tu15(std::uint16_t cutoff);

// The voice processor keeps its filters' values to 1/16 of a 24-bit sample,
// rounding halves up (the same measurements).
inline double sound_filter_round_tu15(double value) { return std::floor(value * 524288.0 + 0.5) / 524288.0; }

// A voice route's attenuation register as DirectSound's 380B97 writes it:
// the route's total level in millibels (voice, mixbin and 3D terms, at
// most 0) times 64 over 100, truncated, at most FFFh. Units are 1/64 dB.
std::uint16_t sound_route_attenuation_tu15(std::int32_t millibels);

// The voice processor's gain for an attenuation register, in 1/16384
// (retail-console measurements, xemu pull request 3105): straight lines
// between whole decibels through round(4095 x 10^(-k/20)) x 4, rounded down;
// from 48 dB a straight line to nothing at 63 dB. Register 0 is 16384.
std::uint32_t sound_route_gain_tu15(std::uint16_t attenuation);

// The voice processor's resampler (the same measurements): 8-point Lagrange
// interpolation around the cursor, sample 0 the one at or before it, with
// each weight rounded to 1/65536 at 256ths of a sample and linear between
// them. fraction is the cursor's 16-bit fraction; weights are for samples
// -3..+4.
std::array<double, 8> sound_resampler_weights_tu15(std::uint16_t fraction);
const std::array<std::array<double, 8>, 257> &sound_resampler_table_tu15() noexcept;
} // namespace halo2
