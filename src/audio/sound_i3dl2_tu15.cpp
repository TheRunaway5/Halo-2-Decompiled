#include "halo2/sound_i3dl2_tu15.hpp"
#include "native_tu15_sound_graph_storage_literals.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
// Each step stores to a float where the original stores to a 32-bit float
// (FSTP dword) and truncates where it calls 379D4B (CVTTSS2SI).
std::int32_t truncate(float value) { return std::int32_t(value); }
double unsigned_value(std::int32_t word) { return double(std::uint32_t(word)); }

// The parameter block under construction, addressed by the byte offsets of
// the record's state that the original uses (0x118 = word 70).
class Block {
public:
  std::int32_t &operator[](unsigned byte) {
    if (byte < 0x118 || byte >= 0x220 || byte % 4) throw std::out_of_range("I3DL2 reverb word outside its block");
    return words_[(byte - 0x118) / 4];
  }
  // The record's fourteen history lines in 24 kHz words (3833E9 converts
  // the image's byte length - 1 at +0xAC).
  std::array<std::int32_t, 14> lines{6000, 6000, 6000, 6000, 680, 612, 640, 720, 1392, 1684, 2044, 2476, 24000, 24000};
  SoundI3dl2ReverbWordsTu15 words() const {
    SoundI3dl2ReverbWordsTu15 out{};
    for (unsigned n = 0; n < out.size(); ++n) out[n] = std::uint32_t(words_[n]) & 0xffffffu;
    return out;
  }

private:
  std::array<std::int32_t, 66> words_{};
};

// 383436: a one-pole low-pass y = b*y + a*x whose gain is the dc level at
// 0 Hz and is hf millibels lower at the reference frequency.
struct OnePole {
  double pole_exact;
  float pole, gain;
};
OnePole one_pole(std::int32_t dc, std::int32_t hf, float reference, float rate) {
  const double w = double(reference) / double(rate) * double(6.2831855f);
  if (dc <= -10000) return {0, 0, 0};
  if (hf == 0) return {0, 0, dc == 0 ? 1.f : sound_i3dl2_level_tu15(dc)};
  const double gain = sound_i3dl2_level_tu15(dc), squared = sound_i3dl2_level_tu15(2 * hf), c = std::cos(w);
  const double pole = ((1 - c * squared) - std::sqrt(2 * (1 - c) * squared - (1 - c * c) * squared * squared)) /
                      (1 - squared);
  return {pole, float(pole), float((1 - pole) * gain)};
}

// 3835E0: the damping filter of a recirculating line of `delay` 24 kHz
// words: 60 dB of loss over the decay time, and the HF ratio's extra loss
// at the HF reference when it is below 1.
std::array<std::int32_t, 2> damping(std::int32_t delay, float decay, float hf_ratio, float reference) {
  float loss = float(unsigned_value(delay) * double(4.16666662e-05f) / double(decay) * -6000.0);
  const std::int32_t dc = truncate(loss);
  if (hf_ratio < 1.f) loss = float(double(loss) / double(hf_ratio));
  const std::int32_t total = truncate(loss);
  const auto filter = one_pole(dc, total - dc, reference, 24000.f);
  return {truncate(float(double(filter.pole) * 8388608.0)), truncate(float(double(filter.gain) * 8388608.0))};
}

// 3869E8: the four reflections. Output gain, feedback sign (the image's
// fixed words 143..146), tap position as a fraction of the reverb delay,
// and feedback delay offset and slope.
struct Reflection {
  float gain, feedback, tap, offset, slope;
};
constexpr std::array<Reflection, 4> reflections{{{0.57f, -0.4f, 0.1052f, 0.003651f, 0.2469f},
                                                 {0.52f, 0.4f, 0.0673f, 0.003515f, 0.2209f},
                                                 {0.54f, -0.4f, 0.1737f, 0.003673f, 0.2311f},
                                                 {0.6f, -0.4f, 0.2404f, 0.003946f, 0.2608f}}};
// 386A38: the short reverb's eight input signs.
constexpr std::array<float, 8> input_signs{1, -1, -1, 1, 1, 1, -1, -1};
// 386A68: the cascade's tap positions in seconds at full size, first taps
// then second taps of lines 0..3.
constexpr std::array<float, 8> cascade{0.040023f, 0.288186f, 0.603764f, 0.851905f,
                                       0.021224f, 0.37542f,  0.623583f, 0.759229f};

// 3831B5: the cascade's taps and the long reverb's feedback delay for its
// modal density term.
void cascade_taps(Block &block, float density) {
  const float scale = density >= 2.5f ? 1.f : float((double(density) - double(0.8f)) * double(0.58823526f));
  for (unsigned line = 0; line < 4; ++line)
    for (unsigned tap = 0; tap < 2; ++tap)
      block[0x130 + 8 * line + 4 * tap] =
          truncate(float(double(scale) * double(cascade[line + 4 * tap]) * 24000.0));
  block[0x160] = truncate(float(double(scale) * 24000.0));
}

// 3838A4's closing steps: round every delay to four words, make the
// reflection and short taps relative to the reflections delay, fit each
// cascade line's two taps into its line, clamp the feedback delays to
// their lines, clamp the input gains and convert delays to bytes.
void finish(Block &block) {
  const auto round4 = [](std::int32_t &word) { word = (word + 1) & ~3; };
  for (auto &line : block.lines) round4(line);
  for (unsigned byte = 0x118; byte < 0x12c; byte += 4) round4(block[byte]);
  for (unsigned byte = 0x130; byte < 0x150; byte += 4) round4(block[byte]);
  round4(block[0x12c]);
  for (unsigned byte = 0x150; byte < 0x160; byte += 4) round4(block[byte]);
  round4(block[0x160]);
  for (auto &line : block.lines) --line;
  for (unsigned byte = 0x11c; byte < 0x12c; byte += 4) block[byte] += block[0x118];
  block[0x12c] += block[0x118];
  // Each cascade line starts where the previous line's second tap left it.
  std::uint32_t start = 0;
  for (unsigned line = 0; line < 4; ++line) {
    auto &first = block[0x130 + 8 * line], &second = block[0x134 + 8 * line];
    const std::uint32_t original_second = std::uint32_t(second);
    const std::uint32_t span = std::max(std::uint32_t(first), std::uint32_t(second)) - start;
    const std::uint32_t length = std::uint32_t(block.lines[line]);
    if (length < span) {
      const float ratio = float(double(length) / double(span));
      first = truncate(float(double(std::uint32_t(first) - start) * double(ratio)));
      second = truncate(float(double(std::uint32_t(second) - start) * double(ratio)));
      round4(first);
      round4(second);
    } else {
      first -= std::int32_t(start);
      second -= std::int32_t(start);
    }
    start = original_second;
  }
  for (unsigned n = 0; n < 4; ++n)
    block[0x150 + 4 * n] = std::int32_t(std::min(std::uint32_t(block[0x150 + 4 * n]), std::uint32_t(block.lines[4 + n])));
  block[0x160] = std::int32_t(std::min(std::uint32_t(block[0x160]), std::uint32_t(block.lines[12])));
  for (unsigned byte = 0x164; byte < 0x184; byte += 4) block[byte] = std::clamp(block[byte], -0x7fffff, 0x7fffff);
  for (unsigned byte = 0x118; byte < 0x164; byte += 4) block[byte] = std::int32_t(std::uint32_t(block[byte]) << 2);
}
} // namespace

float sound_i3dl2_level_tu15(std::int32_t millibels) {
  const float exponent = float(millibels) * 0.0005f;
  return float(std::exp2(double(3.321928f) * double(exponent)));
}

// Both helpers run on the x87 at double precision with float literals: the
// sample period 1/48000, pi, 4096/ln 2 and -ln 10/20.
std::uint16_t sound_dls2_cutoff_tu15(float hz) {
  if (hz < 30.0f) return 0x8000;
  if (hz > 8000.0f) return 0;
  const double angle = double(hz) * double(std::bit_cast<float>(0x37aec33eu)) * double(std::bit_cast<float>(0x40490fdbu));
  const float word = float(std::log(2 * std::sin(angle)) * double(std::bit_cast<float>(0x45b8aa3bu)));
  return std::uint16_t(truncate(word));
}

std::uint16_t sound_dls2_resonance_tu15(float db) {
  if (db > 22.5f) db = 22.5f;
  const double scaled = std::exp2(double(db) * double(std::bit_cast<float>(0xbdebc8e3u)) * 1.4426950408889634) * 32768.0;
  // Its integer conversion keeps the low word of a truncated 64-bit
  // integer, which is 0 when the value is out of range.
  if (!(std::fabs(scaled) < 0x1p63)) return 0;
  const auto low = std::uint32_t(std::uint64_t(std::int64_t(scaled)));
  return std::uint16_t(std::min(low, 0xffffu));
}

double sound_dls2_frequency_tu15(std::uint16_t cutoff) {
  const auto step = int(std::floor(std::int16_t(cutoff) / 4.0)), octave = int(std::floor(step / 1024.0));
  const double value = std::ldexp(std::exp2((step - octave * 1024) / 1024.0) * 65536.0, octave);
  return std::min(std::round(value), 65535.0) / 65536.0;
}

std::uint16_t sound_route_attenuation_tu15(std::int32_t millibels) {
  if (millibels >= 0) return 0;
  const auto attenuation = std::uint64_t(-std::int64_t(millibels)) * 64u / 100u;
  return std::uint16_t(std::min<std::uint64_t>(attenuation, 0xfffu));
}

std::uint32_t sound_route_gain_tu15(std::uint16_t attenuation) {
  static const auto nodes = [] {
    std::array<std::uint32_t, 64> out{};
    for (unsigned k = 0; k <= 48; ++k) out[k] = std::uint32_t(std::floor(4095.0 * std::pow(10.0, -double(k) / 20.0) + 0.5));
    for (unsigned k = 49; k < 64; ++k) out[k] = out[48] * (63u - k) / 15u;
    return out;
  }();
  if (attenuation == 0) return 16384;
  if (attenuation >= 63 * 64) return 0;
  const auto low = nodes[attenuation >> 6], high = nodes[(attenuation >> 6) + 1];
  return (256u * low - 4u * (low - high) * (attenuation & 63u)) >> 6;
}

std::array<double, 8> sound_resampler_weights_tu15(std::uint16_t fraction) {
  static const auto rows = [] {
    std::array<std::array<double, 8>, 257> out{};
    for (int r = 0; r <= 256; ++r)
      for (int k = -3; k <= 4; ++k) {
        double weight = 1;
        for (int j = -3; j <= 4; ++j)
          if (j != k) weight *= (r / 256.0 - j) / double(k - j);
        out[r][k + 3] = std::round(weight * 65536.0) / 65536.0;
      }
    return out;
  }();
  const auto &low = rows[fraction >> 8], &high = rows[(fraction >> 8) + 1];
  const double between = (fraction & 0xffu) / 256.0;
  std::array<double, 8> out{};
  for (unsigned k = 0; k < 8; ++k) out[k] = low[k] + (high[k] - low[k]) * between;
  return out;
}

SoundI3dl2ReverbWordsTu15 sound_i3dl2_reverb_words_tu15(const SoundI3dl2Parameters &p) {
  for (const float value : {p.decay_time, p.decay_hf_ratio, p.reflections_delay, p.reverb_delay, p.diffusion, p.density,
                            p.hf_reference})
    if (!std::isfinite(value)) throw std::invalid_argument("I3DL2 listener values must be finite");
  if (p.decay_time <= 0 || p.decay_hf_ratio <= 0 || p.hf_reference <= 0)
    throw std::invalid_argument("I3DL2 listener decay and HF reference must be positive");
  Block block;
  const float diffusion = float(double(p.diffusion) * double(0.01f));
  float density = float((double(p.decay_time) - 1.0) * 5.0 * (2.0 - double(p.density) * double(0.01f)));
  if (density < 1.f) density = 1.f;
  if (density > 60.f) density = 60.f;
  // 3833E9: four output channels and cleared filter histories.
  block[0x1a4] = 4;
  for (unsigned n = 0; n < 10; ++n) block[0x1a8 + 12 * n] = 0;
  // 38357F: the room HF filter on the 48 kHz input.
  const auto room = one_pole(0, p.room_hf, p.hf_reference, 48000.f);
  block[0x1ac] = truncate(float(double(room.pole) * 8388608.0));
  block[0x1b0] = truncate(float(double(room.gain) * 8388608.0));
  // 383243: the reflections' output gains.
  const float reflection_level = sound_i3dl2_level_tu15(p.reflections + p.room);
  for (unsigned n = 0; n < 4; ++n)
    block[0x18c + 4 * n] = truncate(float(double(reflection_level) * double(reflections[n].gain) * 65536.0));
  // 38327C: the reflections delay.
  block[0x118] = truncate(float(double(p.reflections_delay) * 24000.0));
  // 38329A: the short and long reverb output gains.
  const float reverb_level = sound_i3dl2_level_tu15(p.reverb + p.room);
  block[0x19c] = block[0x1a0] = truncate(float(double(reverb_level) * 65536.0));
  // 3832D2: the short reverb delay, the reflection taps within it and the
  // reflection feedback delays.
  block[0x12c] = truncate(float(double(p.reverb_delay) * 24000.0));
  for (unsigned n = 0; n < 4; ++n)
    block[0x11c + 4 * n] = truncate(float(unsigned_value(block[0x12c]) * double(reflections[n].tap)));
  for (unsigned n = 0; n < 4; ++n)
    block[0x150 + 4 * n] = truncate(
        float((double(p.reverb_delay) * double(reflections[n].slope) + double(reflections[n].offset)) * 24000.0));
  cascade_taps(block, density);
  // 3837BA: damping of the cascade (a quarter of the long delay each), the
  // long lines and the short lines.
  const auto quarter = damping(std::int32_t(std::uint32_t(block[0x160]) >> 2), p.decay_time, p.decay_hf_ratio, p.hf_reference);
  for (unsigned n = 0; n < 3; ++n) {
    block[0x1b8 + 12 * n] = quarter[0];
    block[0x1bc + 12 * n] = quarter[1];
  }
  const auto long_lines = damping(block[0x160], p.decay_time, p.decay_hf_ratio, p.hf_reference);
  for (unsigned n = 0; n < 2; ++n) {
    block[0x20c + 12 * n] = long_lines[0];
    block[0x210 + 12 * n] = long_lines[1];
  }
  for (unsigned n = 0; n < 4; ++n) {
    const auto filter = damping(block.lines[8 + n], p.decay_time, p.decay_hf_ratio, p.hf_reference);
    block[0x1dc + 12 * n] = filter[0];
    block[0x1e0 + 12 * n] = filter[1];
  }
  // 38336C: diffusion's input gains and the long reverb's input split.
  for (unsigned n = 0; n < 8; ++n)
    block[0x164 + 4 * n] = truncate(float(double(diffusion) * double(input_signs[n]) * 8388608.0));
  block[0x188] = truncate(float(double(diffusion) * double(1677721.625f)));
  block[0x184] = truncate(float((1.0 - double(diffusion)) * 8388608.0));
  finish(block);
  return block.words();
}

void sound_i3dl2_reverb_commit_tu15(tu15_sound_ring_effect::State &s, const SoundI3dl2ReverbWordsTu15 &w) {
  for (unsigned n = 0; n < 19; ++n) s.delays[n] = w[n];
  for (unsigned n = 0; n < 8; ++n) s.reflection_gains[n] = w[19 + n];
  s.cross_gain = w[27];
  s.mono_gain = w[28];
  for (unsigned n = 0; n < 6; ++n) s.target_gains[n] = w[29 + n];
  s.output_mode = w[35];
  for (unsigned n = 0; n < 10; ++n) s.filters[n] = {w[36 + 3 * n], w[37 + 3 * n], w[38 + 3 * n]};
  s.flags |= 4u;
}

tu15_sound_ring_effect::State sound_i3dl2_reverb_installed_tu15(unsigned record) {
  if (record != 8 && record != 9) throw std::invalid_argument("The I3DL2 reverbs are DSP records 8 and 9");
  constexpr std::array<unsigned, 14> words{6000, 6000, 6000, 6000, 680, 612, 640, 720, 1392, 1684, 2044, 2476, 24000, 24000};
  tu15_sound_ring_effect::State state;
  tu15_sound_graph_storage::literal_reverb_parameters(state, record == 8 ? 0 : 1);
  for (unsigned n = 0; n < 14; ++n) state.rings[n].words.assign(words[n], 0);
  return state;
}

std::uint16_t sound_i3dl2_pole_tu15(std::int32_t dc, std::int32_t hf, float reference, float rate) {
  if (dc <= -10000 || hf == 0) return 0;
  const double c = std::cos(double(reference) / double(rate) * double(6.2831855f));
  const double squared = sound_i3dl2_level_tu15(2 * hf);
  const double pole = ((1 - c * squared) - std::sqrt(2 * (1 - c) * squared - (1 - c * c) * squared * squared)) /
                      (1 - squared);
  if (!(pole < 1.0)) return 0xffff;
  return std::uint16_t(std::clamp(truncate(float(pole * 65536.0)), 0, 0xffff));
}

SoundI3dl2VoiceTu15 sound_i3dl2_voice_tu15(const SoundI3dl2SourceTu15 &s, float rolloff_distance, float listener_rolloff,
                                           float hf_reference) {
  const float rolloff = float(double(rolloff_distance) * double(s.room_rolloff) * double(listener_rolloff));
  // Each of obstruction and occlusion takes its LF ratio of its loss below
  // the HF reference (the path's level) and the rest above it.
  const double obstruction_lf = double(s.obstruction) * double(s.obstruction_lf_ratio);
  const double occlusion_lf_exact = double(s.occlusion) * double(s.occlusion_lf_ratio);
  const float occlusion_lf = float(occlusion_lf_exact);
  SoundI3dl2VoiceTu15 out;
  out.direct = truncate(float(occlusion_lf_exact + obstruction_lf)) + s.direct;
  const double obstruction_hf = (1.0 - double(s.obstruction_lf_ratio)) * double(s.obstruction);
  const double occlusion_hf_exact = (1.0 - double(s.occlusion_lf_ratio)) * double(s.occlusion);
  const float occlusion_hf = float(occlusion_hf_exact);
  const std::int32_t direct_hf =
      std::clamp(truncate(float(occlusion_hf_exact + obstruction_hf)) + s.direct_hf, -10000, 0);
  // The room path loses its rolloff above unit distance term.
  double room = occlusion_lf;
  if (rolloff > 1.f) {
    const std::int32_t attenuation = rolloff > 1e-05f ? truncate(float(std::log10(double(rolloff)) * 2000.0)) : -10000;
    room = double(attenuation) + double(occlusion_lf);
  }
  out.room = truncate(float(room)) + s.room;
  const std::int32_t room_hf = std::clamp(truncate(occlusion_hf) + s.room, -10000, 0);
  out.direct_pole = sound_i3dl2_pole_tu15(0, direct_hf, hf_reference, 48000.f);
  out.room_pole = sound_i3dl2_pole_tu15(0, room_hf, hf_reference, 48000.f);
  return out;
}
} // namespace halo2
