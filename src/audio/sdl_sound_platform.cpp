#include "halo2/sdl_sound_platform.hpp"
#include "halo2/sound_i3dl2_tu15.hpp"

#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <thread>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace halo2 {
namespace {
constexpr unsigned pcm_rate = 48000;
constexpr unsigned pcm_channels = 2;
// The speakers a voice plays into: front left, front right, back left and
// back right (mixbins 0, 1, 4 and 5; an HRTF voice's 6, 7, 8 and 9). A
// route is one PCM channel (or HRTF ear) into one speaker.
constexpr unsigned speakers = 4, route_count = speakers * pcm_channels;
constexpr unsigned attached_packets = 2;
// The voice processor works in frames of 32 samples.
constexpr unsigned vp_frame = 32;
// HRTF stage: each voice keeps its last 128 mono input frames, enough for
// 31 taps and a fractional neighbour behind the largest delay the voice
// processor's 7-bit field holds.
constexpr unsigned hrtf_history = 128, hrtf_max_delay = 63;
static_assert(SdlSoundHrtf::taps < 32 && 32 + hrtf_max_delay + 1 <= hrtf_history);

struct NativeParameters {
  double relative_rate;
  bool paused;
  // The voice's attenuation in millibels and as a linear amplitude.
  std::int32_t millibels{};
  double amplitude{};
  // Each route's gain, [speaker * 2 + PCM channel].
  std::array<double, route_count> routes{};
  std::optional<tu15_sound_mono_distortion::Parameters> effect;
  std::optional<SdlSoundHrtf> hrtf;
  std::optional<SdlSoundI3dl2Path> room;
  std::optional<SdlSoundI3dl2Path> direct;
  // The DLS2 state-variable filter's frequency and damping (f, q).
  std::optional<std::array<double, 2>> dls2;
  std::optional<SdlSoundNativeRate> native;
};

// An HRTF that passes the mono input unchanged to both ears.
SdlSoundHrtf unfiltered_hrtf() noexcept {
  SdlSoundHrtf out;
  out.left[0] = out.right[0] = 1.f;
  return out;
}
bool same_filter(const SdlSoundHrtf &a, const SdlSoundHrtf &b) noexcept {
  return a.left == b.left && a.right == b.right && a.left_delay == b.left_delay && a.right_delay == b.right_delay;
}

// A linear gain in millibels as DirectSound takes it. The producers' gains
// are millibel levels, which rounding recovers; silence is -10000.
std::int32_t gain_millibels(double gain) noexcept {
  if (!(gain > 0)) return -10000;
  return std::int32_t(std::clamp(std::lround(2000.0 * std::log10(gain)), -10000L, 0L));
}
double millibel_gain(std::int32_t millibels) noexcept {
  return millibels <= -10000 ? 0.0 : std::pow(10.0, millibels / 2000.0);
}
// A route's voice processor level in 1/16384 for its total millibels.
double route_units(std::int32_t millibels) noexcept {
  return double(sound_route_gain_tu15(sound_route_attenuation_tu15(millibels)));
}

// Native-rate rebuilding: a sound's sample m sits at 48 kHz position
// m x 48000 / rate. Its value is a Blackman-windowed sinc low-pass at the
// native Nyquist over the 32 PCM frames around it, normalised to unity DC.
// The fractional positions repeat with period rate / gcd(48000, rate) (147
// for 22050 and 44100 Hz, 2 for 32000 Hz), so each rate keeps one table.
constexpr unsigned rebuild_taps = 32, rebuild_half = 15;
struct RebuildFilter {
  std::uint32_t rate{}, step{}, phases{};
  std::array<std::array<double, rebuild_taps>, 147> weights{};
};
const RebuildFilter *rebuild_filter(std::uint32_t rate) noexcept {
  const auto make = [](std::uint32_t r) {
    RebuildFilter f;
    const auto g = std::gcd(48000u, r);
    f.rate = r;
    f.step = 48000u / g;
    f.phases = r / g;
    const double ratio = r / 48000.0, pi = std::acos(-1.0);
    for (unsigned phase = 0; phase < f.phases; ++phase) {
      double sum = 0;
      for (unsigned k = 0; k < rebuild_taps; ++k) {
        const double t = double(phase) / f.phases + double(rebuild_half) - k, x = ratio * t;
        const double sinc = x == 0 ? 1.0 : std::sin(pi * x) / (pi * x);
        const double window = 0.42 + 0.5 * std::cos(pi * t / 16.0) + 0.08 * std::cos(2 * pi * t / 16.0);
        sum += f.weights[phase][k] = sinc * window;
      }
      for (auto &w : f.weights[phase]) w /= sum;
    }
    return f;
  };
  static const RebuildFilter r22050 = make(22050), r44100 = make(44100), r32000 = make(32000);
  switch (rate) {
  case 22050: return &r22050;
  case 44100: return &r44100;
  case 32000: return &r32000;
  default: return nullptr;
  }
}

// Mixbin samples are 24-bit fractions; float PCM [-1, 1) maps directly and
// saturates like the mixbin accumulators.
tu15_sound::Word24 mixbin_word(double sample) noexcept {
  const auto scaled=std::clamp(std::nearbyint(sample*8388608.0),-8388608.0,8388607.0);
  return tu15_sound::Word24(std::uint32_t(std::int32_t(scaled))&0xffffffu);
}
double mixbin_sample(tu15_sound::Word24 word) noexcept {
  const auto value=std::int32_t(word.bits<<8)>>8;
  return double(value)/8388608.0;
}

// Where a device with this many channels (SDL's layouts) plays the back
// pair; none when it has no back pair and the EP folds it into the sides.
std::optional<std::array<unsigned, 2>> back_channels(unsigned channels) noexcept {
  switch (channels) {
  case 4: return std::array<unsigned, 2>{2, 3}; // FL FR BL BR
  case 5: return std::array<unsigned, 2>{3, 4}; // FL FR LFE BL BR
  case 6: return std::array<unsigned, 2>{4, 5}; // FL FR FC LFE SL SR
  case 7: return std::array<unsigned, 2>{5, 6}; // FL FR FC LFE BC SL SR
  case 8: return std::array<unsigned, 2>{6, 7}; // FL FR FC LFE BL BR SL SR: 5.1's surrounds are the sides
  default: return std::nullopt;
  }
}

NativeParameters native_parameters(const SdlSoundVoiceParameters &input) {
  const float rate = std::bit_cast<float>(input.relative_rate_bits);
  if (!std::isfinite(rate) || rate <= 0)
    throw std::invalid_argument("SDL sound requires an explicit positive relative rate");
  if (input.volume_hundredths_db < -10000 || input.volume_hundredths_db > 0)
    throw std::invalid_argument("SDL sound volume is outside the driver attenuation range");
  if (input.routes.empty())
    throw std::invalid_argument("SDL sound requires supported explicit output routes");
  NativeParameters output{double(rate), input.paused};
  output.millibels = input.volume_hundredths_db;
  output.amplitude = std::pow(10.0, double(input.volume_hundredths_db) / 2000.0);
  if (input.effect) output.effect = input.effect->parameters;
  if (input.hrtf) {
    for (const auto &taps : {input.hrtf->left, input.hrtf->right})
      for (const auto tap : taps)
        if (!std::isfinite(tap)) throw std::invalid_argument("SDL sound HRTF has a non-finite tap");
    if (input.hrtf->left_delay > hrtf_max_delay || input.hrtf->right_delay > hrtf_max_delay)
      throw std::invalid_argument("SDL sound HRTF delay exceeds its voice history");
    if (input.hrtf->front > 0 || input.hrtf->rear > 0)
      throw std::invalid_argument("SDL sound HRTF front/rear levels must be attenuations");
    output.hrtf = input.hrtf;
  }
  for (const auto &path : {input.room, input.direct})
    if (path && (!std::isfinite(path->gain) || path->gain < 0))
      throw std::invalid_argument("SDL sound I3DL2 path levels must be finite and non-negative");
  output.room = input.room;
  output.direct = input.direct;
  if (input.dls2)
    output.dls2 = std::array<double, 2>{sound_dls2_frequency_tu15(input.dls2->cutoff), input.dls2->resonance / 32768.0};
  if (input.native) {
    if (!rebuild_filter(input.native->sample_rate) || input.native->pitch < -32768 || input.native->pitch > 32767)
      throw std::invalid_argument("SDL sound native rate must be 22050, 32000 or 44100 Hz with a 16-bit pitch");
    output.native = input.native;
  }
  for (const auto &route : input.routes) {
    if (route.pcm_channel >= pcm_channels || route.output_channel >= speakers ||
        !std::isfinite(route.linear_gain) || route.linear_gain < 0)
      throw std::invalid_argument("Unsupported SDL sound output route");
    output.routes[route.output_channel * pcm_channels + route.pcm_channel] += double(route.linear_gain);
  }
  return output;
}

void validate_packet(const SdlSoundPacket &packet) {
  if(packet.volume_hundredths_db && (*packet.volume_hundredths_db< -10000 || *packet.volume_hundredths_db>0))
    throw std::invalid_argument("SDL packet volume is outside the driver attenuation range");
  if (!packet.stereo || packet.stereo->size() % pcm_channels ||
      !packet.frame_count || !packet.packet_context)
    throw std::invalid_argument("SDL sound requires an owned complete PCM packet and context");
  const auto available = packet.stereo->size() / pcm_channels;
  if (packet.first_frame > available || packet.frame_count > available - packet.first_frame)
    throw std::out_of_range("SDL sound PCM packet exceeds its retained extent");
  const auto first = packet.first_frame * pcm_channels;
  const auto end = first + packet.frame_count * pcm_channels;
  for (auto i = first; i != end; ++i)
    if (!std::isfinite((*packet.stereo)[i]))
      throw std::invalid_argument("SDL sound PCM packet contains a non-finite sample");
}

class DeviceLock {
public:
  explicit DeviceLock(SDL_AudioDeviceID device) : device_(device) {
    SDL_LockAudioDevice(device_);
  }
  ~DeviceLock() { SDL_UnlockAudioDevice(device_); }
  DeviceLock(const DeviceLock &) = delete;
private:
  SDL_AudioDeviceID device_;
};
} // namespace

struct SdlSoundPlatform::State {
  struct Packet {
    SdlSoundPacket pcm;
    std::uint64_t submission;
    std::optional<SdlSoundCompletionStatus> completed;
    std::shared_ptr<const unsigned char> record;
    // Its own attenuation as a linear amplitude, when it carries one.
    std::optional<double> amplitude;
    void latch_volume() {
      amplitude.reset();
      if (pcm.volume_hundredths_db) amplitude = std::pow(10.0, double(*pcm.volume_hundredths_db) / 2000.0);
    }
  };
  // A voice processor level in 1/16384 moving from the last frame's value
  // to this frame's across one frame, each sample's rounded down.
  struct Ramp {
    double from{}, to{};
    double at(unsigned n) const noexcept { return std::floor(from + (to - from) * n / double(vp_frame)) / 16384.0; }
  };
  // A voice's levels: each route's (an HRTF voice's left ear on routes 0
  // and 4, its right on 3 and 7), its room send's and its effect send's.
  struct Levels {
    std::array<double, route_count> route{};
    double room{}, effect{};
  };
  struct Stream {
    std::uint64_t channel_context{};
    std::shared_ptr<const unsigned char> record;
    NativeParameters parameters{};
    std::array<std::optional<Packet>, attached_packets> packets;
    std::size_t retained = 0;
    double cursor = 0;
    std::uint64_t next_submission = 1;
    SdlSoundPlaybackPosition playback;
    std::optional<std::uint64_t> start_frame;
    // The PCM frames before the current packet's first (zero before the
    // voice's first packet), for the resampler and native rebuilding.
    static constexpr unsigned tail_frames = 24;
    std::array<std::array<double, pcm_channels>, tail_frames> tail{};
    // 48 kHz frames of the packets already played.
    std::uint64_t consumed = 0;
    // Native-rate playback: the voice processor's cursor in the sound's own
    // samples, and its interpolation window, the eight native samples
    // around it (native_next - 8 .. native_next - 1).
    std::uint32_t native_rate = 0;
    double native_cursor = 0;
    std::int64_t native_next = 0;
    std::array<std::array<double, pcm_channels>, 8> native_window{};
    // The voice processor runs each voice in 32-sample frames: the position
    // in the current one and what moves across it. A frame takes the
    // parameters current at its start.
    unsigned frame_position = 0;
    bool started = false;
    std::array<Ramp, route_count + 2> ramps{}; // routes, room, effect
    std::array<double, 2> pitch{};             // 4096 log2 of the rate: from, to
    double frame_rate = 1;
    // The DLS2 filter's frequency (from, to) and damping, while it runs.
    std::optional<std::array<double, 3>> dls2;
    // HRTF stage: the voice's recent mono input and its filter. When the
    // filter changes, the voice processor moves each tap and the signed
    // delay linearly across one frame, from the old value at its first
    // sample to 31/32 of the way at its last, reading a fractional delay
    // between neighbouring samples. A voice's first frame starts at its
    // filter, and a voice that has used an HRTF keeps an unfiltered one
    // rather than switching paths. The input is written twice (at slot and
    // slot + 128) so every ear's window is contiguous; taps are held
    // oldest-first, padded to 32, for the steady filter.
    struct PreparedHrtf {
      SdlSoundHrtf source;
      std::array<float, 32> left{}, right{};
      explicit PreparedHrtf(const SdlSoundHrtf &h) : source(h) {
        for (unsigned n = 0; n < SdlSoundHrtf::taps; ++n) {
          left[31 - n] = h.left[n];
          right[31 - n] = h.right[n];
        }
      }
      double delay() const noexcept { return double(source.left_delay) - double(source.right_delay); }
    };
    std::array<float, 2 * hrtf_history> history{};
    unsigned history_head = 0;
    // The I3DL2 filter's room and direct one-pole histories.
    double room_lowpass = 0;
    std::array<double, pcm_channels> direct_lowpass{};
    // The DLS2 filter's band-pass and low-pass histories.
    std::array<std::array<double, 2>, pcm_channels> dls2_state{};
    std::optional<PreparedHrtf> hrtf, hrtf_from;
    bool hrtf_moving = false;

    std::array<double, 2> filter(const PreparedHrtf &h, unsigned slot) const noexcept {
      const auto ear = [&](const std::array<float, 32> &taps, unsigned delay) {
        // The 32 frames ending delay frames before the newest input.
        const float *window = history.data() + slot + hrtf_history - delay - 31;
        std::array<float, 8> partial{};
        for (unsigned block = 0; block < 32; block += 8)
          for (unsigned lane = 0; lane < 8; ++lane) partial[lane] += taps[block + lane] * window[block + lane];
        double out = 0;
        for (const auto value : partial) out += value;
        return out;
      };
      return {ear(h.left, h.source.left_delay), ear(h.right, h.source.right_delay)};
    }
    // Sample n of a frame that moves from one filter to another.
    std::array<double, 2> moving_filter(const PreparedHrtf &from, const PreparedHrtf &to, unsigned slot,
                                        unsigned n) const noexcept {
      const double w = n / double(vp_frame), delay = from.delay() + (to.delay() - from.delay()) * w;
      const auto ear = [&](const std::array<float, SdlSoundHrtf::taps> &a, const std::array<float, SdlSoundHrtf::taps> &b,
                           double held) {
        held = std::max(held, 0.0);
        const auto whole = unsigned(held);
        const double fraction = held - whole;
        const float *newest = history.data() + slot + hrtf_history - whole;
        double out = 0;
        for (unsigned k = 0; k < SdlSoundHrtf::taps; ++k) {
          double x = newest[-std::ptrdiff_t(k)];
          if (fraction > 0) x = x * (1 - fraction) + newest[-std::ptrdiff_t(k) - 1] * fraction;
          out += (a[k] + (b[k] - a[k]) * w) * x;
        }
        return out;
      };
      return {ear(from.source.left, to.source.left, delay), ear(from.source.right, to.source.right, -delay)};
    }
    // Both ears' output for this frame's input sample, sample n of its frame.
    std::array<double, 2> hrtf_sample(const std::array<double, pcm_channels> &input, unsigned n) noexcept {
      const unsigned slot = history_head % hrtf_history;
      history[slot] = history[slot + hrtf_history] = float(0.5 * (input[0] + input[1]));
      const auto ears = hrtf_moving ? moving_filter(*hrtf_from, *hrtf, slot, n) : filter(*hrtf, slot);
      ++history_head;
      return ears;
    }

    // The levels its parameters and pending packet give: exact linear
    // products, or each route's DirectSound register through the voice
    // processor's curve in 1/16384. The direct I3DL2 level, and an HRTF
    // voice's front and rear levels, are part of its routes' registers.
    Levels levels(bool voice_processor) const noexcept {
      const auto *packet = pending();
      const auto &p = parameters;
      const auto millibels = packet && packet->pcm.volume_hundredths_db ? *packet->pcm.volume_hundredths_db : p.millibels;
      const double amplitude = packet && packet->amplitude ? *packet->amplitude : p.amplitude;
      const double direct = p.direct ? double(p.direct->gain) : 1.0;
      const auto direct_millibels = p.direct ? gain_millibels(direct) : 0;
      const auto level = [&](double gain, std::int32_t extra) {
        if (!voice_processor) return amplitude * direct * gain * millibel_gain(extra);
        return route_units(millibels + gain_millibels(gain) + direct_millibels + extra);
      };
      Levels out;
      if (hrtf) {
        const auto split = p.hrtf ? *p.hrtf : SdlSoundHrtf{};
        const double left = p.routes[0] + p.routes[1], right = p.routes[2] + p.routes[3];
        out.route[0] = level(left, split.front);
        out.route[3] = level(right, split.front);
        out.route[4] = level(left, split.rear);
        out.route[7] = level(right, split.rear);
      } else {
        for (unsigned n = 0; n < route_count; ++n) out.route[n] = level(p.routes[n], 0);
      }
      if (p.room)
        out.room = voice_processor ? route_units(millibels + gain_millibels(p.room->gain)) : amplitude * p.room->gain;
      if (p.effect) out.effect = voice_processor ? route_units(millibels + direct_millibels) : amplitude * direct;
      return out;
    }
    // A frame's start: it takes the current filter, levels and pitch.
    void begin_frame(bool voice_processor) {
      hrtf_moving = false;
      if (parameters.hrtf || hrtf) {
        const auto target = parameters.hrtf ? *parameters.hrtf : unfiltered_hrtf();
        if (!hrtf) {
          hrtf.emplace(target);
        } else if (!same_filter(hrtf->source, target)) {
          hrtf_from = std::move(hrtf);
          hrtf.emplace(target);
          hrtf_moving = true;
        } else {
          hrtf->source = target;
        }
      }
      if (const auto &d = parameters.dls2) dls2 = std::array<double, 3>{dls2 ? (*dls2)[1] : (*d)[0], (*d)[0], (*d)[1]};
      else dls2.reset();
      if (voice_processor) {
        const auto target = levels(true);
        for (unsigned n = 0; n < route_count; ++n) ramps[n] = {started ? ramps[n].to : 0.0, target.route[n]};
        ramps[route_count] = {started ? ramps[route_count].to : 0.0, target.room};
        ramps[route_count + 1] = {started ? ramps[route_count + 1].to : 0.0, target.effect};
        // Native playback steps by the pitch register itself; otherwise by
        // the 48 kHz rate in the same units.
        const auto &native = parameters.native;
        const double log_rate = native ? double(native->pitch) : 4096.0 * std::log2(parameters.relative_rate);
        pitch = {started && native_rate == (native ? native->sample_rate : 0) ? pitch[1] : log_rate, log_rate};
        frame_rate = native ? std::exp2(native->pitch / 4096.0) : parameters.relative_rate;
        if (native && native_rate != native->sample_rate) {
          native_rate = native->sample_rate;
          native_cursor = std::floor((double(consumed) + cursor) * native_rate / 48000.0 * 65536.0) / 65536.0;
          native_next = std::int64_t(std::floor(native_cursor)) - 3;
        } else if (!native) {
          native_rate = 0;
        }
      }
      started = true;
    }
    // The voice processor's cursor step for sample n of the frame, in 1/65536.
    double step(unsigned n) const noexcept {
      const double rate = pitch[0] == pitch[1] ? frame_rate
                                               : std::exp2((pitch[0] + (pitch[1] - pitch[0]) * n / double(vp_frame)) / 4096.0);
      return std::floor(rate * 65536.0) / 65536.0;
    }

    Packet *pending(std::size_t start = 0) noexcept {
      for (auto i = start; i < retained; ++i)
        if (!packets[i]->completed) return &*packets[i];
      return nullptr;
    }
    const Packet *pending() const noexcept {
      for (std::size_t i = 0; i < retained; ++i)
        if (!packets[i]->completed) return &*packets[i];
      return nullptr;
    }
    std::size_t pending_index() const noexcept {
      for (std::size_t i = 0; i < retained; ++i)
        if (!packets[i]->completed) return i;
      return retained;
    }
    void cancel(SdlSoundCompletionStatus reason) noexcept {
      for (std::size_t i = 0; i < retained; ++i)
        if (!packets[i]->completed) packets[i]->completed = reason;
      cursor = 0;
      tail = {};
      consumed = 0;
      native_cursor = 0;
      native_next = -3;
      native_window = {};
    }
    void advance() noexcept {
      while (auto *packet = pending()) {
        if (cursor < double(packet->pcm.frame_count)) break;
        cursor -= double(packet->pcm.frame_count);
        consumed += packet->pcm.frame_count;
        const auto &pcm = packet->pcm;
        for (auto n = pcm.frame_count > tail_frames ? pcm.frame_count - tail_frames : 0; n < pcm.frame_count; ++n) {
          const auto offset = (pcm.first_frame + n) * pcm_channels;
          std::copy(tail.begin() + 1, tail.end(), tail.begin());
          tail.back() = {(*pcm.stereo)[offset], (*pcm.stereo)[offset + 1]};
        }
        packet->completed = SdlSoundCompletionStatus::Success;
      }
    }
    double remaining_frames() const noexcept {
      double frames = -cursor;
      for (std::size_t i = 0; i < retained; ++i)
        if (!packets[i]->completed) frames += double(packets[i]->pcm.frame_count);
      return std::max(0.0,frames);
    }
    std::array<double, pcm_channels> sample() const noexcept {
      const auto index = pending_index();
      if (index == retained) return {};
      const auto &packet = packets[index]->pcm;
      const auto frame = std::size_t(cursor);
      const double fraction = cursor - double(frame);
      const auto offset = (packet.first_frame + frame) * pcm_channels;
      std::array<double, pcm_channels> first{
        (*packet.stereo)[offset], (*packet.stereo)[offset + 1]};
      // Linear interpolation (voice processor off). Crossing an already
      // supplied packet uses that packet's first decoded PCM frame; an
      // underrun holds the final supplied frame for this sample only.
      std::array<double, pcm_channels> next = first;
      if (frame + 1 < packet.frame_count) {
        next = {(*packet.stereo)[offset + 2], (*packet.stereo)[offset + 3]};
      } else if (index + 1 < retained && !packets[index + 1]->completed) {
        const auto &following = packets[index + 1]->pcm;
        const auto start = following.first_frame * pcm_channels;
        next = {(*following.stereo)[start], (*following.stereo)[start + 1]};
      }
      for (unsigned channel = 0; channel < pcm_channels; ++channel)
        first[channel] += fraction * (next[channel] - first[channel]);
      return first;
    }
    // The PCM frame offset frames from the cursor's whole frame.
    std::array<double, pcm_channels> frame_at(std::ptrdiff_t offset) const noexcept {
      return frame(std::ptrdiff_t(cursor) + offset);
    }
    // The PCM frame at a position from the current packet's first frame:
    // earlier ones from the tail, later ones across the queued packets,
    // holding the last supplied frame past them.
    std::array<double, pcm_channels> frame(std::ptrdiff_t position) const noexcept {
      if (position < 0)
        return position >= -std::ptrdiff_t(tail_frames) ? tail[std::size_t(std::ptrdiff_t(tail_frames) + position)]
                                                        : std::array<double, pcm_channels>{};
      std::array<double, pcm_channels> held{};
      for (auto index = pending_index(); index < retained; ++index) {
        const auto &pcm = packets[index]->pcm;
        const auto read = [&](std::size_t n) {
          const auto at = (pcm.first_frame + n) * pcm_channels;
          return std::array<double, pcm_channels>{(*pcm.stereo)[at], (*pcm.stereo)[at + 1]};
        };
        if (std::size_t(position) < pcm.frame_count) return read(std::size_t(position));
        position -= std::ptrdiff_t(pcm.frame_count);
        held = read(pcm.frame_count - 1);
      }
      return held;
    }
    // Native sample m rebuilt from the PCM (zero before the voice's first).
    std::array<double, pcm_channels> rebuild(const RebuildFilter &filter, std::int64_t m) const noexcept {
      if (m < 0) return {};
      const auto numerator = std::uint64_t(m) * 48000u;
      const auto whole = std::int64_t(numerator / filter.rate);
      const auto phase = std::size_t((numerator % filter.rate) * filter.phases / filter.rate);
      const auto first = whole - std::int64_t(rebuild_half) - std::int64_t(consumed);
      const auto &weights = filter.weights[phase];
      std::array<double, pcm_channels> out{};
      // Usually every tap lies in the current packet.
      const auto index = pending_index();
      if (index < retained && first >= 0 && std::size_t(first) + rebuild_taps <= packets[index]->pcm.frame_count) {
        const auto &pcm = packets[index]->pcm;
        const float *x = pcm.stereo->data() + (pcm.first_frame + std::size_t(first)) * pcm_channels;
        for (unsigned k = 0; k < rebuild_taps; ++k) {
          out[0] += weights[k] * x[2 * k];
          out[1] += weights[k] * x[2 * k + 1];
        }
        return out;
      }
      for (unsigned k = 0; k < rebuild_taps; ++k) {
        const auto x = frame(std::ptrdiff_t(first) + k);
        out[0] += weights[k] * x[0];
        out[1] += weights[k] * x[1];
      }
      return out;
    }
    // The voice processor at its native cursor: the window brought up to
    // the cursor, then the 8-point filter (an exact sample at a whole one).
    std::array<double, pcm_channels> native_resampled() noexcept {
      if (pending_index() == retained) return {};
      const auto &filter = *rebuild_filter(native_rate);
      const auto whole = std::int64_t(std::floor(native_cursor));
      if (native_next < whole - 3) native_next = whole - 3;
      while (native_next <= whole + 4) {
        std::copy(native_window.begin() + 1, native_window.end(), native_window.begin());
        native_window.back() = rebuild(filter, native_next++);
      }
      const auto fraction = std::uint16_t((native_cursor - double(whole)) * 65536.0);
      if (!fraction) return native_window[3];
      const auto weights = sound_resampler_weights_tu15(fraction);
      std::array<double, pcm_channels> out{};
      for (unsigned k = 0; k < 8; ++k) {
        out[0] += weights[k] * native_window[k][0];
        out[1] += weights[k] * native_window[k][1];
      }
      return out;
    }
    // The voice processor's resampler at the cursor (an exact frame when
    // the cursor's fraction is zero).
    std::array<double, pcm_channels> resampled() const noexcept {
      if (pending_index() == retained) return {};
      const auto fraction = std::uint16_t((cursor - std::floor(cursor)) * 65536.0);
      if (!fraction) return frame_at(0);
      const auto weights = sound_resampler_weights_tu15(fraction);
      std::array<double, pcm_channels> out{};
      for (int k = 0; k < 8; ++k) {
        const auto x = frame_at(k - 3);
        out[0] += weights[std::size_t(k)] * x[0];
        out[1] += weights[std::size_t(k)] * x[1];
      }
      return out;
    }
  };

  SDL_AudioDeviceID device = 0;
  SdlSoundPlatformConfiguration::Delivery delivery;
  unsigned channels=2,layout=5;
  std::uint16_t callback_frames{};std::optional<std::string> device_name;
  bool initialized = false;
  bool removed = false;
  std::uint64_t next_voice = 1;
  std::uint64_t frames = 0;
  std::uint64_t elapsed_frames = 0;
  const std::thread::id executor = std::this_thread::get_id();
  std::atomic_bool alive{true},continuable{false},held{false};
  bool foreign_producer = false;
  std::uint64_t revision = 0,device_generation = 1;
  std::atomic_uint64_t epoch{1};

  void require(bool raw=false) const {
    if (!alive.load() || (continuable.load() && executor!=std::this_thread::get_id()))
      throw std::logic_error("SDL sound requires its live producer executor");
    if (held.load()) throw std::logic_error("SDL sound callback exclusion is already held");
    if (raw && epoch!=1) throw std::logic_error("SDL sound raw voice authority retired by publication");
  }
  void change() {
    require(); // Recheck after SDL lock: enrollment may have won a legacy race.
    if (revision==std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("SDL sound producer revision exhausted");
    if(executor!=std::this_thread::get_id())foreign_producer=true;
    ++revision;
  }
  void enroll() {
    require();
    if(executor!=std::this_thread::get_id() || foreign_producer)
      throw std::logic_error("SDL sound continuation requires exclusive creating producer");
    continuable.store(true);
  }
  bool accepts(const VoiceReceipt& token) const noexcept {
    if(!alive.load() || executor!=std::this_thread::get_id() || token.epoch_!=epoch)
      return false;
    const auto owner=token.owner_.lock();
    if(owner.get()!=this)return false;
    const auto found=streams.find(token.id_);
    return found!=streams.end() && found->second.record==token.record_ &&
      !found->second.record.owner_before(token.record_) &&
      !token.record_.owner_before(found->second.record);
  }
  VoiceReceipt receipt(Voice id) {
    VoiceReceipt out;out.owner_=shared_owner;out.id_=id;out.epoch_=epoch;
    out.record_=stream(id).record;return out;
  }
  std::weak_ptr<State> shared_owner;

  std::uint64_t presentation_frames = 0;
  bool paused = false;
  std::map<Voice, Stream> streams;
  // Mixbin 13 and DSP record 1, active once an effect voice has existed.
  // Frames are the original 32-sample DSP blocks: one block of latency.
  struct EffectBus {
    bool active{};
    tu15_sound_mono_distortion::Parameters parameters=tu15_sound_mono_distortion::authored_initial_parameters();
    tu15_sound_mono_distortion::State state{};
    std::array<double,32> input{};
    std::array<double,32> output{};
    unsigned position{};
  } effect_bus;
  // The two listener reverbs: DSP records 9 and 8, fed by every voice's
  // room send through mixbin 10, and their four return voices' volumes:
  // 21EE80 sets each with SetVolume in millibels (ftol of 2000 log10 of its
  // gain, at least -6400), so they move like any voice's levels.
  struct Reverb {
    explicit Reverb(unsigned record) : record(sound_i3dl2_reverb_installed_tu15(record)) {}
    tu15_sound_ring_effect::State record;
    std::optional<SoundI3dl2Parameters> parameters;
    std::array<std::array<double, 32>, 4> output{};
    std::array<double, 4> targets{};
    std::array<Ramp, 4> ramps{};
  };
  static double return_units(double gain) noexcept {
    const auto millibels = gain > 0 ? std::clamp(std::int32_t(std::log10(gain) * 2000.0), -6400, 0) : -6400;
    return route_units(millibels);
  }
  std::array<Reverb, 2> reverbs{Reverb(9), Reverb(8)};
  tu15_sound_ring_effect::Workspace reverb_workspace{};
  std::array<double, 32> reverb_send{};
  unsigned reverb_position = 0;
  bool reverb_active = false;
  // DSP record 10 on mixbins 6..9 (P:0CEC..0D5E), per sample as the record
  // runs it: each side adds a one-pole low-pass of the other side's output
  // eleven samples earlier (pole 0x7645A1, gain 0xF88313). Its flags word
  // keeps processing on (bit 1); DirectSound never changes it.
  struct Crosstalk {
    std::array<std::array<double, 11>, 2> history{};
    std::array<double, 2> filtered{};
    unsigned cursor = 0;
  };
  std::array<Crosstalk, 2> crosstalks{};
  bool crosstalk_enabled = true;
  bool voice_processor = true;
  static constexpr double crosstalk_pole = 0x7645a1 / 8388608.0, crosstalk_gain = -0x077ced / 8388608.0;
  std::array<double, 2> crosstalk(Crosstalk &c, double left, double right) const noexcept {
    const unsigned n = c.cursor;
    c.filtered[0] = crosstalk_pole * c.filtered[0] + crosstalk_gain * c.history[0][n];
    c.filtered[1] = crosstalk_pole * c.filtered[1] + crosstalk_gain * c.history[1][n];
    // Mixbin words saturate at full scale.
    const double processed_left = std::clamp(left + c.filtered[1], -1.0, 1.0);
    const double processed_right = std::clamp(right + c.filtered[0], -1.0, 1.0);
    c.history[0][n] = processed_left;
    c.history[1][n] = processed_right;
    c.cursor = n + 1 < 11 ? n + 1 : 0;
    if (!crosstalk_enabled) return {left, right};
    return {processed_left, processed_right};
  }
  // The EP stereo encoder's back-channel coefficient (0x5A8279).
  static constexpr double back_fold = 0x5a8279 / 8388608.0;
  void reverb_block() noexcept {
    reverb_position = 0;
    // Effects 2 and 3 add mixbin 10 at 0x7FFFFF into each record's input.
    std::array<std::uint32_t, 32> send{};
    for (unsigned n = 0; n < 32; ++n) {
      const auto word = std::int64_t(std::int32_t(mixbin_word(reverb_send[n]).bits << 8) >> 8);
      send[n] = std::uint32_t(std::clamp<std::int64_t>((2 * word * 0x7fffff) >> 24, -0x800000, 0x7fffff)) & 0xffffffu;
    }
    reverb_send.fill(0);
    try {
      // The GP runs record 8 (the second slot) before record 9.
      for (auto *slot : {&reverbs[1], &reverbs[0]}) {
        auto &reverb = *slot;
        auto input = send;
        std::array<std::array<std::uint32_t, 32>, 4> words{};
        tu15_sound_ring_effect::process_admitted(reverb.record, reverb_workspace, input,
                                                 {words[0], words[1], words[2], words[3]});
        for (unsigned channel = 0; channel < 4; ++channel)
          for (unsigned n = 0; n < 32; ++n) reverb.output[channel][n] = mixbin_sample(tu15_sound::Word24(words[channel][n]));
      }
    } catch (...) {
      // A record outside its admitted domain silences both returns.
      for (auto &reverb : reverbs) reverb.output = {};
      reverb_active = false;
    }
  }
  void effect_block() noexcept {
    std::array<tu15_sound::Word24,32> block{};
    for(unsigned i=0;i<32;++i)block[i]=mixbin_word(effect_bus.input[i]);
    try {
      constexpr tu15_sound_mono_distortion::Controls controls{tu15_sound_mono_distortion::ScalingMode::none,false,false,false,
          tu15_sound_mono_distortion::AddressingMode::linear,tu15_sound_mono_distortion::AddressingMode::linear,
          tu15_sound_mono_distortion::AddressingMode::linear,tu15_sound_mono_distortion::AddressingMode::linear,
          tu15_sound_mono_distortion::AddressingMode::linear};
      tu15_sound_mono_distortion::process(controls,effect_bus.parameters,effect_bus.state,block,block);
      for(unsigned i=0;i<32;++i)effect_bus.output[i]=mixbin_sample(block[i]);
    } catch(...) {
      // Parameters are validated when installed; a failure silences the bus.
      effect_bus.output.fill(0);effect_bus.active=false;
    }
    effect_bus.input.fill(0);effect_bus.position=0;
  }

  explicit State(const SdlSoundPlatformConfiguration &configuration)
      : delivery(configuration.delivery),callback_frames(configuration.callback_frames),device_name(configuration.device_name),
        crosstalk_enabled(configuration.crosstalk && !std::getenv("HALO2_REFERENCE_NO_CROSSTALK")),
        voice_processor(configuration.voice_processor && !std::getenv("HALO2_REFERENCE_IDEAL_VOICES")) {
    if ((delivery != SdlSoundPlatformConfiguration::Delivery::DeviceCallback &&
         delivery != SdlSoundPlatformConfiguration::Delivery::Manual) ||
        !configuration.callback_frames ||
        (configuration.callback_frames & (configuration.callback_frames - 1)))
      throw std::invalid_argument("Unsupported SDL sound callback configuration");
    if (configuration.device_name && configuration.device_name->empty())
      throw std::invalid_argument("An explicit SDL sound device name cannot be empty");
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
      throw std::runtime_error(std::string("SDL sound initialization failed: ") + SDL_GetError());
    initialized = true;
    SDL_AudioSpec desired{};
    desired.freq = pcm_rate;
    desired.channels = pcm_channels;
    desired.format = AUDIO_F32SYS;
    desired.samples = configuration.callback_frames;
    desired.callback = callback;
    desired.userdata = this;
    SDL_AudioSpec obtained{};
    device = SDL_OpenAudioDevice(configuration.device_name ? configuration.device_name->c_str() : nullptr,
                                 0, &desired, &obtained, 0);
    if (!device || obtained.freq != int(pcm_rate) || obtained.channels != pcm_channels ||
        obtained.format != AUDIO_F32SYS) {
      const auto message = std::string("SDL sound device creation failed: ") + SDL_GetError();
      if (device) SDL_CloseAudioDevice(device);
      device = 0;
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
      initialized = false;
      throw std::runtime_error(message);
    }
    if (delivery == SdlSoundPlatformConfiguration::Delivery::DeviceCallback)
      SDL_PauseAudioDevice(device, 0);
  }
  ~State() {
    // SDL_CloseAudioDevice joins the callback before releasing stream storage.
    // Normal source teardown must flush/account packets before destroying
    // voices; final platform teardown also releases abandoned native storage.
    if (device) SDL_CloseAudioDevice(device);
    if (initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
  Stream &stream(Voice voice) {
    auto found = streams.find(voice);
    if (found == streams.end()) throw std::out_of_range("Unknown native SDL sound voice");
    return found->second;
  }
  const Stream &stream(Voice voice) const {
    auto found = streams.find(voice);
    if (found == streams.end()) throw std::out_of_range("Unknown native SDL sound voice");
    return found->second;
  }
  void fail() noexcept {
    removed = true;
    if(device_generation!=std::numeric_limits<std::uint64_t>::max())++device_generation;
    for (auto &[id, stream] : streams) {
      (void)id;
      stream.cancel(SdlSoundCompletionStatus::Failure);
    }
  }
  void mix(float *output, std::size_t count) noexcept {
    // The SDL callback already holds the device lock. No allocation, release
    // of retained PCM or source callbacks are allowed on this thread.
    std::fill(output,output+count*channels,0.f);
    const auto back = back_channels(channels);
    for (std::size_t frame = 0; frame < count; ++frame) {
      // Mixbins 0, 1, 4 and 5: the speakers.
      std::array<double, speakers> speaker{};
      // Mixbins 6..9: the 3D voices' and reverb returns' front left, front
      // right, back left and back right, which reach the speakers through
      // DSP record 10.
      std::array<double, speakers> spatial{};
      double effect_send=0,room_bus=0;
      if (!removed && !paused) for (auto &[id, stream] : streams) {
        (void)id;
        if (stream.parameters.paused) continue;
        if (stream.start_frame && presentation_frames < *stream.start_frame) continue;
        stream.advance();
        if (!stream.pending()) continue;
        if (stream.frame_position == 0) stream.begin_frame(voice_processor);
        const unsigned n = stream.frame_position;
        const bool native = voice_processor && stream.native_rate;
        auto sample = native ? stream.native_resampled() : voice_processor ? stream.resampled() : stream.sample();
        if (const auto &dls2 = stream.dls2) {
          // The DLS2 filter on the whole voice, ahead of its I3DL2 paths,
          // rounding each step as the voice processor does; a changed
          // frequency moves across the frame. The bound only keeps an
          // unstable pair (negative resonance near 8 kHz, which no script
          // sets) finite; stable ones stay below it.
          const double f = (*dls2)[0] + ((*dls2)[1] - (*dls2)[0]) * n / double(vp_frame), q = (*dls2)[2];
          for (unsigned channel = 0; channel < pcm_channels; ++channel) {
            auto &[band, low] = stream.dls2_state[channel];
            const double high = sound_filter_round_tu15(sample[channel] - low - q * band);
            band = std::clamp(sound_filter_round_tu15(band + f * high), -32.0, 32.0);
            low = std::clamp(sound_filter_round_tu15(low + f * band), -32.0, 32.0);
            sample[channel] = low;
          }
        }
        const auto dry = sample;
        if (const auto &direct = stream.parameters.direct) {
          const double pole = direct->pole / 65536.0;
          for (unsigned channel = 0; channel < pcm_channels; ++channel) {
            auto &low = stream.direct_lowpass[channel];
            low = sound_filter_round_tu15(dry[channel] + (low - dry[channel]) * pole);
            sample[channel] = low;
          }
        }
        if (!stream.playback.first_device_frame) {
          stream.playback.first_device_frame = frames + frame;
          stream.playback.first_presentation_frame = presentation_frames;
        }
        if(epoch!=1 && !stream.playback.first_resumed_device_frame)
          stream.playback.first_resumed_device_frame=frames+frame;
        // This sample's levels: moving across the frame, or exact.
        const auto exact = voice_processor ? Levels{} : stream.levels(false);
        const auto level = [&](unsigned route) {
          if (voice_processor) return stream.ramps[route].at(n);
          return route < route_count ? exact.route[route] : route == route_count ? exact.room : exact.effect;
        };
        // Each PCM channel (or HRTF ear) into its speakers.
        auto input = sample;
        auto *bus = &speaker;
        if (stream.hrtf) {
          input = stream.hrtf_sample(sample, n);
          bus = &spatial;
        }
        for (unsigned target = 0; target < speakers; ++target)
          for (unsigned channel = 0; channel < pcm_channels; ++channel)
            if (const double gain = level(target * pcm_channels + channel); gain != 0)
              (*bus)[target] += input[channel] * gain;
        if (stream.parameters.room) {
          // The voice's send to mixbin 10 through its I3DL2 room filter.
          const double mono = 0.5 * (dry[0] + dry[1]), pole = stream.parameters.room->pole / 65536.0;
          stream.room_lowpass = sound_filter_round_tu15(mono + (stream.room_lowpass - mono) * pole);
          room_bus += stream.room_lowpass * level(route_count);
        }
        if (stream.parameters.effect) effect_send += 0.5 * (sample[0] + sample[1]) * level(route_count + 1);
        // The cursor in 48 kHz frames; native playback derives it from the
        // voice processor's own cursor.
        double step = voice_processor ? stream.step(n) : stream.parameters.relative_rate;
        if (native) {
          stream.native_cursor += step;
          step = stream.native_cursor * 48000.0 / stream.native_rate - double(stream.consumed) - stream.cursor;
        }
        ++stream.playback.output_frames;
        stream.playback.source_frames += std::min(stream.remaining_frames(), step);
        stream.cursor += step;
        stream.frame_position = (n + 1) % vp_frame;
        stream.advance();
      }
      if (reverb_active && !removed && !paused) {
        // Mixbin 10 collects this block's room sends; each return voice
        // plays its reverb's previous block into mixbins 6..9.
        reverb_send[reverb_position] = room_bus;
        for (auto &reverb : reverbs)
          for (unsigned n = 0; n < 4; ++n) {
            auto &ramp = reverb.ramps[n];
            if (voice_processor && reverb_position == 0) ramp = {ramp.to, return_units(reverb.targets[n])};
            const double gain = voice_processor ? ramp.at(reverb_position) : reverb.targets[n];
            spatial[n] += reverb.output[n][reverb_position] * gain;
          }
        if (++reverb_position == 32) reverb_block();
      }
      if (!removed && !paused) {
        // Record 10 writes the front pair to mixbins 0/1 and the back pair to
        // 4/5.
        const auto front = crosstalk(crosstalks[0], spatial[0], spatial[1]);
        const auto rear = crosstalk(crosstalks[1], spatial[2], spatial[3]);
        speaker[0] += front[0];
        speaker[1] += front[1];
        speaker[2] += rear[0];
        speaker[3] += rear[1];
      }
      if (effect_bus.active && !removed && !paused) {
        // Mixbin 13 accumulates this frame's sends; the mix-in return plays
        // the previous processed block to both front speakers at 0 dB.
        effect_bus.input[effect_bus.position] += effect_send;
        const double wet = effect_bus.output[effect_bus.position];
        speaker[0] += wet;
        speaker[1] += wet;
        if (++effect_bus.position == 32) effect_block();
      }
      // A device with a back pair plays it; on stereo the EP's encoder adds
      // each back channel to its side (0x5A8279, -3 dB).
      auto *out = output + frame * channels;
      const auto clamp = [](double value) { return float(std::clamp(value, -1.0, 1.0)); };
      if (back) {
        out[0] = clamp(speaker[0]);
        out[1] = clamp(speaker[1]);
        out[(*back)[0]] = clamp(speaker[2]);
        out[(*back)[1]] = clamp(speaker[3]);
      } else {
        out[0] = clamp(speaker[0] + back_fold * speaker[2]);
        out[1] = clamp(speaker[1] + back_fold * speaker[3]);
      }
      if (!paused) ++presentation_frames;
    }
    frames += count;
    elapsed_frames += count;
  }
  static void callback(void *userdata, Uint8 *bytes, int length) noexcept {
    auto &state = *static_cast<State *>(userdata);
    if (length < 0) { state.fail(); return; }
    if (length % int(sizeof(float) * state.channels)) {
      std::memset(bytes, 0, std::size_t(length));
      state.fail();
      return;
    }
    state.mix(reinterpret_cast<float *>(bytes), std::size_t(length) / (sizeof(float) * state.channels));
  }
};

SdlSoundPlatform::SdlSoundPlatform(const SdlSoundPlatformConfiguration &configuration)
    : state_(std::make_shared<State>(configuration)) { state_->shared_owner=state_; }
SdlSoundPlatform::~SdlSoundPlatform() {
  if(state_->continuable.load() && state_->executor!=std::this_thread::get_id())std::terminate();
  state_->alive.store(false);
}

std::uint32_t SdlSoundPlatform::device_id() const { state_->require();return state_->device; }

unsigned SdlSoundPlatform::output_channels() const {state_->require();return state_->channels;}
std::string SdlSoundPlatform::speaker_layout(unsigned layout) {
  state_->require();
  if(state_->executor!=std::this_thread::get_id())throw std::logic_error("SDL endpoint configuration requires its creator");
  if(layout>4)return "Invalid speaker configuration";
  if(layout==state_->layout)return {};
  unsigned channels=layout==2?4:layout==3?6:2;
#if SDL_VERSION_ATLEAST(2,24,0)
  if(layout==4) {SDL_AudioSpec actual{};if(SDL_GetDefaultAudioInfo(nullptr,&actual,0)==0 && actual.channels>=2 && actual.channels<=8)channels=actual.channels;}
#endif
  {
    DeviceLock lock(state_->device);state_->require();
    if(state_->revision==std::numeric_limits<std::uint64_t>::max() || state_->device_generation==std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("SDL endpoint generation exhausted");
    if(channels==state_->channels){state_->change();state_->layout=layout;++state_->device_generation;return {};}
    state_->held.store(true); // Serialize even a legacy producer during replacement.
  }
  SDL_AudioSpec desired{};desired.freq=pcm_rate;desired.format=AUDIO_F32SYS;desired.channels=channels;
  desired.samples=state_->callback_frames;desired.callback=State::callback;desired.userdata=state_.get();
  const auto replacement=SDL_OpenAudioDevice(state_->device_name?state_->device_name->c_str():nullptr,0,&desired,nullptr,0);
  if(!replacement){state_->held.store(false);return std::string("Cannot open selected speaker configuration: ")+SDL_GetError();}
  SDL_PauseAudioDevice(state_->device,1);SDL_CloseAudioDevice(state_->device);
  ++state_->revision;++state_->device_generation;
  state_->device=replacement;state_->channels=channels;state_->layout=layout;state_->removed=false;
  state_->held.store(false);
  if(state_->delivery==SdlSoundPlatformConfiguration::Delivery::DeviceCallback)SDL_PauseAudioDevice(replacement,0);
  return {};
}

SdlSoundPlatform::Voice SdlSoundPlatform::create_voice(
    std::uint64_t context, const SdlSoundVoiceParameters &parameters) {
  return create_voice_impl(context,parameters,{});
}

SdlSoundPlatform::Voice SdlSoundPlatform::create_voice_at(
    std::uint64_t context,const SdlSoundVoiceParameters &parameters,std::uint64_t frame) {
  return create_voice_impl(context,parameters,frame);
}

SdlSoundPlatform::Voice SdlSoundPlatform::create_voice_impl(
    std::uint64_t context,const SdlSoundVoiceParameters &parameters,
    std::optional<std::uint64_t> frame,bool raw) {
  state_->require(raw);
  const auto record=std::make_shared<const unsigned char>(0);
  auto native = native_parameters(parameters);
  DeviceLock lock(state_->device);state_->require();
  if (state_->removed) throw std::runtime_error("SDL sound device is unavailable");
  if (state_->next_voice == std::numeric_limits<Voice>::max())
    throw std::overflow_error("Native SDL sound voice identity exhausted");
  const auto voice = state_->next_voice;
  State::Stream stream{};stream.channel_context=context;stream.record=record;stream.parameters=std::move(native);
  stream.start_frame=frame;stream.playback.publication_epoch=state_->epoch.load();
  state_->change();
  if (stream.parameters.effect) {
    state_->effect_bus.parameters=*stream.parameters.effect;
    state_->effect_bus.active=true;
  }
  state_->streams.emplace(voice,std::move(stream));
  ++state_->next_voice;
  return voice;
}

void SdlSoundPlatform::parameters(Voice voice,const SdlSoundVoiceParameters& parameters) {
  state_->require(true);parameters_impl(voice,parameters);
}
void SdlSoundPlatform::parameters_impl(Voice voice, const SdlSoundVoiceParameters &parameters) {
  state_->require();
  auto native = native_parameters(parameters);
  DeviceLock lock(state_->device);state_->require();
  if (state_->removed) throw std::runtime_error("SDL sound device is unavailable");
  auto& stream=state_->stream(voice);state_->change();
  stream.parameters = std::move(native);
}

void SdlSoundPlatform::packet_volume(Voice voice,std::uint64_t context,std::int32_t volume) {
  state_->require(true);packet_volume_impl(voice,context,volume);
}
void SdlSoundPlatform::packet_volume_impl(Voice voice,std::uint64_t context,std::int32_t volume) {
  state_->require();
  if(!context || volume< -10000 || volume>0)throw std::invalid_argument("Invalid native packet context or attenuation");
  DeviceLock lock(state_->device);state_->require();
  if(state_->removed)throw std::runtime_error("SDL sound device is unavailable");
  auto& stream=state_->stream(voice);
  State::Packet* selected{};
  for(std::size_t i=0;i<stream.retained;++i)if(stream.packets[i]->pcm.packet_context==context) {
    if(selected)throw std::logic_error("Native packet context is ambiguous");
    selected=&*stream.packets[i];
  }
  if(!selected)throw std::logic_error("Native packet context is stale or foreign");
  state_->change();selected->pcm.volume_hundredths_db=volume;selected->latch_volume();
}

SdlSoundPlatform::Submission SdlSoundPlatform::submit(Voice voice,SdlSoundPacket packet) {
  state_->require(true);return submit_impl(voice,std::move(packet));
}
SdlSoundPlatform::Submission SdlSoundPlatform::submit_impl(Voice voice, SdlSoundPacket packet) {
  state_->require();
  const auto record=std::make_shared<const unsigned char>(0);
  validate_packet(packet); // Potentially large validation is outside the device lock.
  if(state_->continuable.load() && !packet.stereo.use_count())
    throw std::invalid_argument("SDL continuation PCM requires retained ownership");
  DeviceLock lock(state_->device);state_->require();
  if (state_->removed) throw std::runtime_error("SDL sound device is unavailable");
  auto &stream = state_->stream(voice);
  if (stream.retained == attached_packets) return Submission::Backpressure;
  if (stream.next_submission == std::numeric_limits<std::uint64_t>::max())
    throw std::overflow_error("Native SDL sound packet identity exhausted");
  state_->change();
  stream.packets[stream.retained].emplace(State::Packet{
    std::move(packet), stream.next_submission, std::nullopt,record});
  stream.packets[stream.retained]->latch_volume();
  ++stream.retained;
  ++stream.next_submission;
  return Submission::Accepted;
}

SdlSoundVoiceStatus SdlSoundPlatform::status(Voice voice) const {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  const auto &stream = state_->stream(voice);
  std::size_t completed = 0;
  for (std::size_t i = 0; i < stream.retained; ++i)
    completed += bool(stream.packets[i]->completed);
  return {!state_->removed && stream.retained < attached_packets,
          state_->paused || stream.parameters.paused, state_->removed,
          stream.retained - completed, completed};
}

SdlSoundPlaybackPosition SdlSoundPlatform::playback_position(Voice voice) const {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  return state_->stream(voice).playback;
}

std::optional<SdlSoundCompletion> SdlSoundPlatform::completion(Voice voice) const {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  const auto &stream = state_->stream(voice);
  if (!stream.retained || !stream.packets[0]->completed) return std::nullopt;
  const auto &packet = *stream.packets[0];
  return SdlSoundCompletion{voice, stream.channel_context, packet.pcm.packet_context,
                           packet.submission, *packet.completed, packet.pcm.frame_count};
}

void SdlSoundPlatform::acknowledge(Voice voice,std::uint64_t submission) {
  state_->require(true);acknowledge_impl(voice,submission);
}
void SdlSoundPlatform::acknowledge_impl(Voice voice, std::uint64_t submission) {
  state_->require();
  std::optional<State::Packet> released;
  {
    DeviceLock lock(state_->device);state_->require();
    auto &stream = state_->stream(voice);
    if (!stream.retained || !stream.packets[0]->completed ||
        stream.packets[0]->submission != submission)
      throw std::logic_error("SDL sound completion must be accounted in submission order");
    state_->change();
    released = std::move(stream.packets[0]);
    for (std::size_t i = 1; i < stream.retained; ++i)
      stream.packets[i - 1] = std::move(stream.packets[i]);
    stream.packets[--stream.retained].reset();
  }
  // A potentially final PCM reference is released off the callback and lock.
}

void SdlSoundPlatform::flush(Voice voice) {state_->require(true);flush_impl(voice);}
void SdlSoundPlatform::flush_impl(Voice voice) {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  auto& stream=state_->stream(voice);state_->change();stream.cancel(SdlSoundCompletionStatus::Flushed);
}

void SdlSoundPlatform::destroy_voice(Voice voice) {state_->require(true);destroy_voice_impl(voice);}
void SdlSoundPlatform::destroy_voice_impl(Voice voice) {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  if (state_->stream(voice).retained)
    throw std::logic_error("SDL sound voice still owns unaccounted packets");
  state_->change();
  state_->streams.erase(voice);
}

bool SdlSoundPlatform::device_removed(std::uint32_t which) {
  state_->require();
  if(state_->executor!=std::this_thread::get_id())throw std::logic_error("SDL endpoint removal requires its creator");
  if (which != state_->device) return false;
  {
    DeviceLock lock(state_->device);state_->require();
    state_->change();state_->fail();
  }
  SDL_PauseAudioDevice(state_->device, 1);
  return true;
}

std::vector<float> SdlSoundPlatform::render(std::size_t frames) {
  state_->require();
  if (state_->delivery != SdlSoundPlatformConfiguration::Delivery::Manual)
    throw std::logic_error("Cannot manually render a running SDL sound device");
  if (frames > std::vector<float>().max_size() / state_->channels)
    throw std::length_error("SDL sound render extent exceeds native storage");
  std::vector<float> output(frames * state_->channels);
  DeviceLock lock(state_->device);state_->require();
  if(state_->executor!=std::this_thread::get_id())state_->foreign_producer=true;
  state_->mix(output.data(), frames);
  return output;
}

std::uint64_t SdlSoundPlatform::rendered_frames() const {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  return state_->frames;
}

void SdlSoundPlatform::reverb(const std::array<SdlSoundReverb, 2> &input) {
  state_->require();
  for (const auto &reverb : input) {
    const auto &p = reverb.parameters;
    for (const auto gain : reverb.speaker_gains)
      if (!std::isfinite(gain) || gain < 0 || gain > 1)
        throw std::invalid_argument("SDL reverb speaker gains must lie in [0, 1]");
    for (const auto value : {p.room_rolloff, p.decay_time, p.decay_hf_ratio, p.reflections_delay, p.reverb_delay,
                             p.diffusion, p.density, p.hf_reference})
      if (!std::isfinite(value)) throw std::invalid_argument("SDL reverb parameters must be finite");
  }
  // DirectSound's parameter words, built before taking the device lock.
  std::array<std::optional<SoundI3dl2ReverbWordsTu15>, 2> words;
  for (unsigned n = 0; n < 2; ++n)
    if (state_->reverbs[n].parameters != input[n].parameters) words[n] = sound_i3dl2_reverb_words_tu15(input[n].parameters);
  DeviceLock lock(state_->device);state_->require();
  state_->change();
  for (unsigned n = 0; n < 2; ++n) {
    auto &reverb = state_->reverbs[n];
    if (words[n]) {
      sound_i3dl2_reverb_commit_tu15(reverb.record, *words[n]);
      reverb.parameters = input[n].parameters;
    }
    for (unsigned k = 0; k < 4; ++k) reverb.targets[k] = input[n].speaker_gains[k];
  }
  state_->reverb_active = true;
}

void SdlSoundPlatform::paused(bool paused) {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  state_->change();state_->paused=paused;
}

std::uint64_t SdlSoundPlatform::presentation_frames() const {
  state_->require();
  DeviceLock lock(state_->device);state_->require();
  return state_->presentation_frames;
}


struct SdlSoundPlatform::SnapshotData {
  struct Image {
    std::map<Voice,State::Stream> streams;
    std::uint64_t next_voice{},elapsed{},presentation{};
    bool paused{};
    State::EffectBus effect_bus;
  } image;
  std::weak_ptr<State> owner;
  std::uint64_t device_generation{};
};
struct SdlSoundPlatform::PreparedData {
  SnapshotData::Image image;
  std::weak_ptr<State> owner;
  std::vector<std::pair<Voice,std::shared_ptr<const unsigned char>>> records;
  std::atomic_bool canceled{false};
  std::uint64_t revision{},device_generation{},published_epoch{};
  bool consumed{};
};

void SdlSoundPlatform::enroll_continuation() {
  state_->require();
  DeviceLock lock(state_->device);
  if(state_->removed)throw std::logic_error("Cannot enroll removed SDL endpoint");
  state_->enroll();
}
SdlSoundPlatform::Checkpoint SdlSoundPlatform::checkpoint() const {
  state_->require();
  // Enrollment and every legacy producer's final admission share this lock.
  {DeviceLock lock(state_->device);state_->enroll();}
  auto data=std::make_shared<SnapshotData>();
  // Producer structure is fixed; callback writes only scalar stream members.
  // Allocate every node BEFORE excluding the actual callback.
  for(const auto& [id,stream]:state_->streams){(void)stream;data->image.streams.emplace(id,State::Stream{});}
  {
    DeviceLock lock(state_->device);state_->require();
    if(state_->removed)throw std::logic_error("Cannot checkpoint removed SDL endpoint");
    for(auto& [id,stream]:data->image.streams){
      stream=state_->streams.at(id);
      for(std::size_t i=0;i<stream.retained;++i)
        if(!stream.packets[i]->pcm.stereo || !stream.packets[i]->pcm.stereo.use_count())
          throw std::logic_error("SDL checkpoint PCM has no retained owner");
    }
    data->image.next_voice=state_->next_voice;
    data->image.elapsed=state_->elapsed_frames;
    data->image.presentation=state_->presentation_frames;
    data->image.paused=state_->paused;
    data->image.effect_bus=state_->effect_bus;
    data->device_generation=state_->device_generation;
    data->owner=state_;
  }
  // Private immutable handles are now coherent; validate potentially large PCM
  // extents/finite samples off the callback lock, never re-decode or copy PCM.
  for(const auto& [id,stream]:data->image.streams){(void)id;
    for(std::size_t i=0;i<stream.retained;++i)validate_packet(stream.packets[i]->pcm);}
  Checkpoint out;out.data_=std::move(data);return out;
}
SdlSoundPlatform::PreparedReplacement SdlSoundPlatform::prepare_restore(const Checkpoint& saved) const {
  state_->require();
  if(!saved.data_ || saved.data_->owner.lock()!=state_)
    throw std::logic_error("Foreign or empty SDL sound checkpoint");
  auto data=std::make_shared<PreparedData>();
  data->image=saved.data_->image; // All mutable nodes/strong pins private before hold.
  for(const auto& [id,stream]:data->image.streams){(void)id;
    for(std::size_t i=0;i<stream.retained;++i){
      if(!stream.packets[i]->pcm.stereo.use_count())throw std::logic_error("Unowned SDL checkpoint PCM");
      validate_packet(stream.packets[i]->pcm);
    }
  }
  for(auto& [id,stream]:data->image.streams){
    stream.record=std::make_shared<const unsigned char>(0);
    for(std::size_t i=0;i<stream.retained;++i)
      stream.packets[i]->record=std::make_shared<const unsigned char>(0);
    data->records.emplace_back(id,stream.record);
  }
  {
    DeviceLock lock(state_->device);state_->require();
    if(!state_->continuable.load() || state_->removed ||
       state_->device_generation!=saved.data_->device_generation)
      throw std::logic_error("SDL sound checkpoint endpoint binding changed");
    if(state_->revision==std::numeric_limits<std::uint64_t>::max() ||
       state_->epoch.load()==std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("SDL sound publication identity exhausted");
    data->owner=state_;data->revision=state_->revision;
    data->device_generation=state_->device_generation;
    data->published_epoch=state_->epoch.load()+1;
  }
  for(auto& [id,stream]:data->image.streams){(void)id;
    stream.playback.publication_epoch=data->published_epoch;
    stream.playback.first_resumed_device_frame.reset();}
  PreparedReplacement out;out.data_=std::move(data);return out;
}
SdlSoundPlatform::PreparedReplacement::~PreparedReplacement(){if(data_)data_->canceled.store(true);}
SdlSoundPlatform::PreparedReplacement::PreparedReplacement(PreparedReplacement&& other) noexcept
  :data_(std::move(other.data_)){}
SdlSoundPlatform::PreparedReplacement& SdlSoundPlatform::PreparedReplacement::operator=(PreparedReplacement&& other) noexcept {
  if(this!=&other){if(data_)data_->canceled.store(true);data_=std::move(other.data_);}return *this;
}
std::optional<SdlSoundPlatform::VoiceReceipt> SdlSoundPlatform::PreparedReplacement::voice(Voice id) const noexcept {
  if(!data_)return {};
  const auto state=data_->owner.lock();
  if(!voices())return {};
  for(const auto& [key,record]:data_->records)if(key==id){
    VoiceReceipt out;out.owner_=state;out.id_=key;out.record_=record;out.epoch_=data_->published_epoch;
    if(!data_->consumed || state->accepts(out))return out;
    return {};
  }
  return {};
}
std::optional<std::size_t> SdlSoundPlatform::PreparedReplacement::voices() const noexcept {
  if(!data_)return {};
  const auto state=data_->owner.lock();
  if(!state || state->executor!=std::this_thread::get_id() || !state->alive.load() || data_->canceled.load())return {};
  const auto epoch=state->epoch.load();
  if(data_->consumed ? epoch!=data_->published_epoch :
      (epoch==std::numeric_limits<std::uint64_t>::max() || epoch+1!=data_->published_epoch ||
       state->revision!=data_->revision))return {};
  return data_->records.size();
}
SdlSoundPlatform::HeldPublication::HeldPublication(std::shared_ptr<State> state,std::shared_ptr<PreparedData> data)
  :state_(std::move(state)),data_(std::move(data)) {
  state_->require();
  if(!data_ || data_->owner.lock()!=state_ || data_->canceled.load() || data_->consumed)
    throw std::logic_error("Unavailable SDL sound preparation");
  state_->held.store(true);
  SDL_LockAudioDevice(state_->device);locked_=true;
  if(!state_->alive.load() || state_->removed || state_->revision!=data_->revision ||
     state_->device_generation!=data_->device_generation || data_->canceled.load()){
    SDL_UnlockAudioDevice(state_->device);locked_=false;state_->held.store(false);
    throw std::logic_error("Stale SDL sound prepared publication");
  }
}
SdlSoundPlatform::HeldPublication::~HeldPublication(){
  if(state_ && state_->executor!=std::this_thread::get_id())std::terminate();
  if(locked_){SDL_UnlockAudioDevice(state_->device);locked_=false;state_->held.store(false);}
  // Members release only AFTER unlocking. State may be the last endpoint pin.
}
std::optional<SdlSoundPlatform::Clocks> SdlSoundPlatform::HeldPublication::clocks() const noexcept {
  if(!locked_ || state_->executor!=std::this_thread::get_id() || !state_->alive.load())return {};
  return Clocks{state_->frames,state_->elapsed_frames,state_->presentation_frames,state_->epoch.load()};
}
std::optional<SdlSoundVoiceStatus> SdlSoundPlatform::HeldPublication::status(const VoiceReceipt& voice) const noexcept {
  if(!locked_ || !state_->accepts(voice))return {};
  const auto& stream=state_->streams.find(voice.id())->second;
  std::size_t completed=0;
  for(std::size_t i=0;i<stream.retained;++i)completed+=bool(stream.packets[i]->completed);
  return SdlSoundVoiceStatus{!state_->removed && stream.retained<attached_packets,
    state_->paused || stream.parameters.paused,state_->removed,stream.retained-completed,completed};
}
SdlSoundPlatform::HeldPublication SdlSoundPlatform::hold_publication(PreparedReplacement& prepared){
  return HeldPublication(state_,prepared.data_);
}
bool SdlSoundPlatform::ready(const PreparedReplacement& prepared,const HeldPublication& held) const noexcept {
  if(!state_ || state_->executor!=std::this_thread::get_id() || !state_->alive.load() ||
     held.state_!=state_ || !held.locked_ || !prepared.data_ || held.data_!=prepared.data_)return false;
  const auto& data=*prepared.data_;
  return !data.canceled.load() && !data.consumed && !state_->removed && state_->held.load() &&
    data.owner.lock()==state_ && state_->revision==data.revision &&
    state_->device_generation==data.device_generation &&
    state_->epoch.load()!=std::numeric_limits<std::uint64_t>::max() &&
    state_->revision!=std::numeric_limits<std::uint64_t>::max() &&
    data.published_epoch==state_->epoch.load()+1;
}
bool SdlSoundPlatform::commit(PreparedReplacement& prepared,HeldPublication& held) noexcept {
  if(!ready(prepared,held))return false;
  auto& data=*prepared.data_;
  state_->streams.swap(data.image.streams);
  std::swap(state_->next_voice,data.image.next_voice);
  std::swap(state_->elapsed_frames,data.image.elapsed);
  std::swap(state_->presentation_frames,data.image.presentation);
  std::swap(state_->paused,data.image.paused);
  std::swap(state_->effect_bus,data.image.effect_bus);
  ++state_->epoch;++state_->revision;
  data.consumed=true;
  return true;
}
std::uint64_t SdlSoundPlatform::elapsed_frames() const {
  state_->require();DeviceLock lock(state_->device);state_->require();return state_->elapsed_frames;
}
std::uint64_t SdlSoundPlatform::publication_epoch() const {
  state_->require();return state_->epoch.load();
}
SdlSoundPlatform::VoiceReceipt SdlSoundPlatform::create_voice_receipt(std::uint64_t context,
    const SdlSoundVoiceParameters& parameters,std::optional<std::uint64_t> deadline){
  state_->require();
  {DeviceLock lock(state_->device);state_->enroll();}
  const auto id=create_voice_impl(context,parameters,deadline,false);return state_->receipt(id);
}
void SdlSoundPlatform::parameters(const VoiceReceipt& voice,const SdlSoundVoiceParameters& p){
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");parameters_impl(voice.id_,p);
}
void SdlSoundPlatform::packet_volume(const VoiceReceipt& voice,std::uint64_t context,std::int32_t volume){
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");
  packet_volume_impl(voice.id_,context,volume);
}
SdlSoundPlatform::Submission SdlSoundPlatform::submit(const VoiceReceipt& voice,SdlSoundPacket packet){
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");return submit_impl(voice.id_,std::move(packet));
}
SdlSoundVoiceStatus SdlSoundPlatform::status(const VoiceReceipt& voice)const{
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");return status(voice.id_);
}
SdlSoundPlaybackPosition SdlSoundPlatform::playback_position(const VoiceReceipt& voice)const{
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");return playback_position(voice.id_);
}
std::optional<SdlSoundPlatform::CompletionReceipt> SdlSoundPlatform::completion(const VoiceReceipt& voice)const{
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");
  const auto packet=completion(voice.id_);if(!packet)return {};
  CompletionReceipt out;out.value_=*packet;out.voice_=voice;
  out.record_=state_->stream(voice.id_).packets[0]->record;return out;
}
void SdlSoundPlatform::acknowledge(const CompletionReceipt& packet){
  state_->require();
  if(!state_->accepts(packet.voice_))throw std::logic_error("Stale SDL completion receipt");
  const auto& stream=state_->stream(packet.voice_.id_);
  if(!stream.retained || stream.packets[0]->record!=packet.record_ ||
     stream.packets[0]->record.owner_before(packet.record_) || packet.record_.owner_before(stream.packets[0]->record))
    throw std::logic_error("SDL completion lost its exact packet owner");
  acknowledge_impl(packet.voice_.id_,packet.value_.submission);
}
void SdlSoundPlatform::flush(const VoiceReceipt& voice){
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");flush_impl(voice.id_);
}
void SdlSoundPlatform::destroy_voice(const VoiceReceipt& voice){
  state_->require();if(!state_->accepts(voice))throw std::logic_error("Stale SDL voice receipt");destroy_voice_impl(voice.id_);
}

} // namespace halo2
