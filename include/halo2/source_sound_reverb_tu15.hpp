#pragma once
#include "halo2/sound_i3dl2_reverb.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace halo2 {
// A sound environment's reverb, as its snde record stores it from +8:
// levels in dB, times in seconds, diffusion and density as fractions.
// The defaults are the original's reverb of no environment (21E330's
// g_44A2A0).
struct SourceSoundReverbTu15 {
  float room = -100, room_hf = -100, room_rolloff = 0, decay_time = 1, decay_hf_ratio = 1;
  float reflections = -100, reflections_delay = 0, reverb = -100, reverb_delay = 0;
  float diffusion = 1, density = 1, hf_reference = 5000;
};

// One of the listener's two sound environments as native 18E970/129170
// leave them in the sound system (+0x1A8, 0x1C each): its reverb, its
// portal span (minimum and maximum angle, offset, scale) and its fade.
struct SourceSoundListenerEnvironmentTu15 {
  SourceSoundReverbTu15 reverb;
  std::array<float, 4> occlusion{0, 0, 0, 0};
  float fade = 0;
};

// set_global_sound_environment's values (HS 878) in the loop-sound
// globals (+0x228): the four global gains, the DLS2 filter cutoff in Hz and
// its resonance in dB. The original's defaults change nothing.
struct SourceSoundGlobalEnvironmentTu15 {
  std::array<float, 4> gains{1, 1, 1, 1};
  std::int32_t filter_frequency = 8000;
  float filter_gain = 0;
  friend bool operator==(const SourceSoundGlobalEnvironmentTu15 &, const SourceSoundGlobalEnvironmentTu15 &) = default;
};

namespace source_sound_reverb_detail {
// 12AFF0 with clamping: value's position between minimum and maximum.
inline float position(float minimum, float maximum, float value) {
  return std::clamp((value - minimum) / (maximum - minimum), 0.f, 1.f);
}
} // namespace source_sound_reverb_detail

// 21ECE0: the I3DL2 reverb effect description for an environment's reverb
// (the global reverb gain, g_468834, is 1: 0 dB).
inline SoundI3dl2Parameters source_sound_reverb_i3dl2_tu15(const SourceSoundReverbTu15 &r) {
  using source_sound_reverb_detail::position;
  SoundI3dl2Parameters p;
  p.room = std::int32_t(position(-64, 0, r.room) * 6400.f - 6400.f);
  p.room_hf = std::int32_t(position(-64, 0, r.room_hf) * 6400.f - 6400.f);
  p.room_rolloff = r.room_rolloff;
  p.decay_time = r.decay_time;
  p.decay_hf_ratio = r.decay_hf_ratio;
  p.reflections = std::int32_t(position(-64, 10, r.reflections) * 7400.f - 6400.f);
  p.reflections_delay = r.reflections_delay;
  p.reverb = std::int32_t(position(-64, 20, r.reverb) * 8400.f - 6400.f);
  p.reverb_delay = r.reverb_delay;
  p.diffusion = r.diffusion * 100.f;
  p.density = r.density * 100.f;
  p.hf_reference = r.hf_reference;
  return p;
}

// 21EE80: each speaker submix's gain for an environment's reverb (front
// left, front right, back left, back right). A speaker covers a half circle
// of listener angles (counterclockwise from ahead); its share of the
// environment's portal span, over pi, maps through the span's offset and
// scale, times the environment's fade. The original sets the submix volume
// in millibels with a -6400 floor.
inline std::array<float, 4> source_sound_reverb_speaker_gains_tu15(const SourceSoundListenerEnvironmentTu15 &e) {
  constexpr float q = 0.785398185f;
  using Range = std::array<std::array<float, 2>, 3>;
  const std::array<Range, 4> speakers{{
      {{{-q, 3 * q}, {7 * q, 8 * q}, {-8 * q, -5 * q}}},   // front left
      {{{-3 * q, q}, {5 * q, 8 * q}, {-8 * q, -7 * q}}},   // front right
      {{{q, 5 * q}, {-7 * q, -3 * q}, {0, 0}}},            // back left
      {{{3 * q, 7 * q}, {-5 * q, -q}, {0, 0}}},            // back right
  }};
  const float minimum = e.occlusion[0], maximum = e.occlusion[1], offset = e.occlusion[2], scale = e.occlusion[3];
  std::array<float, 4> gains{};
  for (unsigned speaker = 0; speaker < 4; ++speaker) {
    float covered = 0;
    for (const auto &range : speakers[speaker]) {
      const float upper = maximum > range[1] ? range[1] : maximum;
      const float lower = minimum > range[0] ? minimum : range[0];
      covered += upper - lower > 0.f ? upper - lower : 0.f;
    }
    const float fraction = covered * 0.318309873f;
    float gain = std::clamp(((fraction - offset) * scale + offset) * e.fade, 0.f, 1.f);
    if (gain > 0.f && std::log10(gain) * 2000.f < -6400.f) gain = std::pow(10.f, -3.2f);
    gains[speaker] = gain;
  }
  return gains;
}

// 227BF0 (retail 2211E0): a 3D voice's room send in millibels for its
// reverb gain in dB (its sound class's +0x10, plus any obstruction gain),
// with the global gains at their default of 1 (0 dB). Its room HF level
// is the same value.
inline std::int32_t source_sound_room_send_tu15(float decibels) {
  const float attenuation = source_sound_reverb_detail::position(-64.f, 0.f, decibels);
  return std::clamp(-std::int32_t(6400.f - attenuation * 6400.f), -10000, 0);
}
} // namespace halo2
