#include "halo2/sdl_sound_platform.hpp"

#include <SDL.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace halo2 {
namespace {
constexpr unsigned pcm_rate = 48000;
constexpr unsigned pcm_channels = 2;
constexpr unsigned attached_packets = 2;

struct NativeParameters {
  double relative_rate;
  std::array<double, 4> matrix;
  bool paused;
};

NativeParameters native_parameters(const SdlSoundVoiceParameters &input) {
  const float rate = std::bit_cast<float>(input.relative_rate_bits);
  if (!std::isfinite(rate) || rate <= 0)
    throw std::invalid_argument("SDL sound requires an explicit positive relative rate");
  if (input.volume_hundredths_db < -10000 || input.volume_hundredths_db > 0)
    throw std::invalid_argument("SDL sound volume is outside the driver attenuation range");
  if (input.routes.empty())
    throw std::invalid_argument("SDL sound requires supported explicit output routes");
  NativeParameters output{double(rate), {}, input.paused};
  // Native amplitude conversion for an integer attenuation in 1/100 dB.
  // This is not a replacement for the source scalar/routing CPU owners.
  const double amplitude = std::pow(10.0, double(input.volume_hundredths_db) / 2000.0);
  for (const auto &route : input.routes) {
    if (route.pcm_channel >= pcm_channels || route.output_channel >= pcm_channels ||
        !std::isfinite(route.linear_gain) || route.linear_gain < 0)
      throw std::invalid_argument("Unsupported SDL sound output route");
    output.matrix[route.output_channel * pcm_channels + route.pcm_channel] +=
        amplitude * double(route.linear_gain);
  }
  return output;
}

void validate_packet(const SdlSoundPacket &packet) {
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
  };
  struct Stream {
    std::uint64_t channel_context;
    NativeParameters parameters;
    std::array<std::optional<Packet>, attached_packets> packets;
    std::size_t retained = 0;
    double cursor = 0;
    std::uint64_t next_submission = 1;
    SdlSoundPlaybackPosition playback;
    std::optional<std::uint64_t> start_frame;

    Packet *pending(std::size_t start = 0) noexcept {
      for (auto i = start; i < retained; ++i)
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
    }
    void advance() noexcept {
      while (auto *packet = pending()) {
        if (cursor < double(packet->pcm.frame_count)) break;
        cursor -= double(packet->pcm.frame_count);
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
      // Linear interpolation is an explicitly native service. Crossing an
      // already supplied packet uses that packet's first decoded PCM frame;
      // an underrun holds the final supplied frame for this sample only.
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
  };

  SDL_AudioDeviceID device = 0;
  SdlSoundPlatformConfiguration::Delivery delivery;
  unsigned channels=2,layout=5;
  std::uint16_t callback_frames{};std::optional<std::string> device_name;
  bool initialized = false;
  bool removed = false;
  std::uint64_t next_voice = 1;
  std::uint64_t frames = 0;
  std::uint64_t presentation_frames = 0;
  bool paused = false;
  std::map<Voice, Stream> streams;

  explicit State(const SdlSoundPlatformConfiguration &configuration)
      : delivery(configuration.delivery),callback_frames(configuration.callback_frames),device_name(configuration.device_name) {
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
    for (auto &[id, stream] : streams) {
      (void)id;
      stream.cancel(SdlSoundCompletionStatus::Failure);
    }
  }
  void mix(float *output, std::size_t count) noexcept {
    // The SDL callback already holds the device lock. No allocation, release
    // of retained PCM or source callbacks are allowed on this thread.
    std::fill(output,output+count*channels,0.f);
    for (std::size_t frame = 0; frame < count; ++frame) {
      std::array<double, pcm_channels> accumulated{};
      if (!removed && !paused) for (auto &[id, stream] : streams) {
        (void)id;
        if (stream.parameters.paused) continue;
        if (stream.start_frame && presentation_frames < *stream.start_frame) continue;
        stream.advance();
        if (!stream.pending()) continue;
        const auto sample = stream.sample();
        if (!stream.playback.first_device_frame) {
          stream.playback.first_device_frame = frames + frame;
          stream.playback.first_presentation_frame = presentation_frames;
        }
        ++stream.playback.output_frames;
        stream.playback.source_frames += std::min(stream.remaining_frames(),
            stream.parameters.relative_rate);
        const auto &matrix = stream.parameters.matrix;
        accumulated[0] += sample[0] * matrix[0] + sample[1] * matrix[1];
        accumulated[1] += sample[0] * matrix[2] + sample[1] * matrix[3];
        stream.cursor += stream.parameters.relative_rate;
        stream.advance();
      }
      for (unsigned channel = 0; channel < pcm_channels; ++channel)
        output[frame * channels + channel] = float(std::clamp(accumulated[channel], -1.0, 1.0));
      if (!paused) ++presentation_frames;
    }
    frames += count;
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
    : state_(std::make_unique<State>(configuration)) {}
SdlSoundPlatform::~SdlSoundPlatform() = default;

std::uint32_t SdlSoundPlatform::device_id() const { return state_->device; }

unsigned SdlSoundPlatform::output_channels() const {return state_->channels;}
std::string SdlSoundPlatform::speaker_layout(unsigned layout) {
  if(layout>4)return "Invalid speaker configuration";
  if(layout==state_->layout)return {};
  unsigned channels=layout==2?4:layout==3?6:2;
#if SDL_VERSION_ATLEAST(2,24,0)
  if(layout==4) {SDL_AudioSpec actual{};if(SDL_GetDefaultAudioInfo(nullptr,&actual,0)==0 && actual.channels>=2 && actual.channels<=8)channels=actual.channels;}
#endif
  if(channels==state_->channels){state_->layout=layout;return {};}
  SDL_AudioSpec desired{};desired.freq=pcm_rate;desired.format=AUDIO_F32SYS;desired.channels=channels;
  desired.samples=state_->callback_frames;desired.callback=State::callback;desired.userdata=state_.get();
  const auto replacement=SDL_OpenAudioDevice(state_->device_name?state_->device_name->c_str():nullptr,0,&desired,nullptr,0);
  if(!replacement)return std::string("Cannot open selected speaker configuration: ")+SDL_GetError();
  SDL_PauseAudioDevice(state_->device,1);SDL_CloseAudioDevice(state_->device);
  state_->device=replacement;state_->channels=channels;state_->layout=layout;state_->removed=false;
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
    std::optional<std::uint64_t> frame) {
  auto native = native_parameters(parameters);
  DeviceLock lock(state_->device);
  if (state_->removed) throw std::runtime_error("SDL sound device is unavailable");
  if (state_->next_voice == std::numeric_limits<Voice>::max())
    throw std::overflow_error("Native SDL sound voice identity exhausted");
  const auto voice = state_->next_voice;
  State::Stream stream{context,std::move(native),{}};
  stream.start_frame=frame;
  state_->streams.emplace(voice,std::move(stream));
  ++state_->next_voice;
  return voice;
}

void SdlSoundPlatform::parameters(Voice voice, const SdlSoundVoiceParameters &parameters) {
  auto native = native_parameters(parameters);
  DeviceLock lock(state_->device);
  if (state_->removed) throw std::runtime_error("SDL sound device is unavailable");
  state_->stream(voice).parameters = std::move(native);
}

SdlSoundPlatform::Submission SdlSoundPlatform::submit(Voice voice, SdlSoundPacket packet) {
  validate_packet(packet); // Potentially large validation is outside the device lock.
  DeviceLock lock(state_->device);
  if (state_->removed) throw std::runtime_error("SDL sound device is unavailable");
  auto &stream = state_->stream(voice);
  if (stream.retained == attached_packets) return Submission::Backpressure;
  if (stream.next_submission == std::numeric_limits<std::uint64_t>::max())
    throw std::overflow_error("Native SDL sound packet identity exhausted");
  stream.packets[stream.retained].emplace(State::Packet{
    std::move(packet), stream.next_submission, std::nullopt});
  ++stream.retained;
  ++stream.next_submission;
  return Submission::Accepted;
}

SdlSoundVoiceStatus SdlSoundPlatform::status(Voice voice) const {
  DeviceLock lock(state_->device);
  const auto &stream = state_->stream(voice);
  std::size_t completed = 0;
  for (std::size_t i = 0; i < stream.retained; ++i)
    completed += bool(stream.packets[i]->completed);
  return {!state_->removed && stream.retained < attached_packets,
          state_->paused || stream.parameters.paused, state_->removed,
          stream.retained - completed, completed};
}

SdlSoundPlaybackPosition SdlSoundPlatform::playback_position(Voice voice) const {
  DeviceLock lock(state_->device);
  return state_->stream(voice).playback;
}

std::optional<SdlSoundCompletion> SdlSoundPlatform::completion(Voice voice) const {
  DeviceLock lock(state_->device);
  const auto &stream = state_->stream(voice);
  if (!stream.retained || !stream.packets[0]->completed) return std::nullopt;
  const auto &packet = *stream.packets[0];
  return SdlSoundCompletion{voice, stream.channel_context, packet.pcm.packet_context,
                           packet.submission, *packet.completed, packet.pcm.frame_count};
}

void SdlSoundPlatform::acknowledge(Voice voice, std::uint64_t submission) {
  std::optional<State::Packet> released;
  {
    DeviceLock lock(state_->device);
    auto &stream = state_->stream(voice);
    if (!stream.retained || !stream.packets[0]->completed ||
        stream.packets[0]->submission != submission)
      throw std::logic_error("SDL sound completion must be accounted in submission order");
    released = std::move(stream.packets[0]);
    for (std::size_t i = 1; i < stream.retained; ++i)
      stream.packets[i - 1] = std::move(stream.packets[i]);
    stream.packets[--stream.retained].reset();
  }
  // A potentially final PCM reference is released off the callback and lock.
}

void SdlSoundPlatform::flush(Voice voice) {
  DeviceLock lock(state_->device);
  state_->stream(voice).cancel(SdlSoundCompletionStatus::Flushed);
}

void SdlSoundPlatform::destroy_voice(Voice voice) {
  DeviceLock lock(state_->device);
  if (state_->stream(voice).retained)
    throw std::logic_error("SDL sound voice still owns unaccounted packets");
  state_->streams.erase(voice);
}

bool SdlSoundPlatform::device_removed(std::uint32_t which) {
  if (which != state_->device) return false;
  SDL_PauseAudioDevice(state_->device, 1);
  DeviceLock lock(state_->device);
  state_->fail();
  return true;
}

std::vector<float> SdlSoundPlatform::render(std::size_t frames) {
  if (state_->delivery != SdlSoundPlatformConfiguration::Delivery::Manual)
    throw std::logic_error("Cannot manually render a running SDL sound device");
  if (frames > std::vector<float>().max_size() / state_->channels)
    throw std::length_error("SDL sound render extent exceeds native storage");
  std::vector<float> output(frames * state_->channels);
  DeviceLock lock(state_->device);
  state_->mix(output.data(), frames);
  return output;
}

std::uint64_t SdlSoundPlatform::rendered_frames() const {
  DeviceLock lock(state_->device);
  return state_->frames;
}

void SdlSoundPlatform::paused(bool paused) {
  DeviceLock lock(state_->device);
  state_->paused=paused;
}

std::uint64_t SdlSoundPlatform::presentation_frames() const {
  DeviceLock lock(state_->device);
  return state_->presentation_frames;
}

} // namespace halo2
