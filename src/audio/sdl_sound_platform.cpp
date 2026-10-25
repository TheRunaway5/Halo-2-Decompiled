#include "halo2/sdl_sound_platform.hpp"

#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <thread>
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
  std::array<double,4> routes{};
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
    std::optional<std::array<double,4>> matrix;
    void routes(const NativeParameters& parameters) {
      if(!pcm.volume_hundredths_db){matrix.reset();return;}
      const double amplitude=std::pow(10.0,double(*pcm.volume_hundredths_db)/2000.0);
      matrix=parameters.routes;for(auto& gain:*matrix)gain*=amplitude;
    }
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
        if(epoch!=1 && !stream.playback.first_resumed_device_frame)
          stream.playback.first_resumed_device_frame=frames+frame;
        ++stream.playback.output_frames;
        stream.playback.source_frames += std::min(stream.remaining_frames(),
            stream.parameters.relative_rate);
        const auto &packet=stream.pending()->matrix;
        const auto &matrix = packet?*packet:stream.parameters.matrix;
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
  auto& stream=state_->stream(voice);state_->change();stream.parameters = std::move(native);
  for(std::size_t i=0;i<stream.retained;++i)stream.packets[i]->routes(stream.parameters);
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
  state_->change();selected->pcm.volume_hundredths_db=volume;selected->routes(stream.parameters);
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
  stream.packets[stream.retained]->routes(stream.parameters);
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
