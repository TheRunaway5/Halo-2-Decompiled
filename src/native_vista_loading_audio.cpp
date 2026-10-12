#include "halo2/native_vista_loading_audio.hpp"
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
constexpr std::uint32_t device_token=0x100,pcm_token=0x200,event_token=0x300,worker_token=0x400;
constexpr std::int32_t failed=std::bit_cast<std::int32_t>(0x80004005u);
constexpr VistaLoadingFloatingPoint fp{0x027f,0x1f80};
constexpr unsigned output_rate=48000,output_channels=2,scratch_frames=512;
std::vector<std::uint8_t> read_bank(const std::filesystem::path& path) {
  std::ifstream input(path,std::ios::binary);
  if(!input)throw std::runtime_error("Cannot open Vista loading ambience bank: "+path.string());
  std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(input)),{});
  if(input.bad())throw std::runtime_error("Cannot read Vista loading ambience bank");
  return data;
}
struct DeviceLock {
  SDL_AudioDeviceID device;
  explicit DeviceLock(SDL_AudioDeviceID value):device(value){if(device)SDL_LockAudioDevice(device);}
  ~DeviceLock(){if(device)SDL_UnlockAudioDevice(device);}
};
} // namespace
struct NativeVistaLoadingAudio::State {
  struct Buffer {
    std::vector<std::uint8_t> pcm;
    SDL_AudioStream* stream{};
    std::uint32_t frequency{};
    std::int32_t volume{};
    double amplitude=1;
    bool playing{};
    std::uint64_t submitted{};
  };
  NativeVistaLoadingAudioConfiguration config;
  std::vector<std::uint8_t> encoded,allocation;
  mutable std::mutex lifecycle_mutex,control_mutex,event_mutex;
  std::condition_variable event_condition;
  VistaLoadingAudioState source;
  VistaLoadingAudioServices services;
  std::array<Buffer,4> buffers;
  SDL_AudioDeviceID device{};
  bool initialized{},native_muted{},event_exists{},event_signaled{};
  unsigned created{};
  std::thread worker;
  std::atomic<bool> worker_running{};
  std::uint64_t mixed_frames{};
  std::array<float,scratch_frames*output_channels> scratch;
  std::array<char,256> callback_failure{},worker_failure{};
  std::string failure;

  State(std::span<const std::uint8_t> data,NativeVistaLoadingAudioConfiguration c)
      :config(std::move(c)),encoded(data.begin(),data.end()) {
    if(encoded.size()!=18396 || !config.callback_frames || config.callback_frames>32768 ||
        (config.callback_frames&(config.callback_frames-1)))
      throw std::invalid_argument("Unsupported native Vista loading audio bank/device configuration");
    bind_services();
  }
  ~State() {
    // Normally the source shutdown has joined and released everything. This
    // last-resort native cleanup also covers a setup exception: no callback or
    // worker may retain streams while their owning State is being destroyed.
    {
      std::lock_guard lock(event_mutex);event_signaled=true;
    }
    event_condition.notify_one();
    if(worker.joinable())worker.join();
    if(device)SDL_CloseAudioDevice(device);
    for(auto& b:buffers)if(b.stream)SDL_FreeAudioStream(b.stream);
    if(initialized)SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
  unsigned index(std::uint32_t token) const {
    if(token<1 || token>4)throw std::logic_error("Unknown native Vista loading buffer token");
    return token-1;
  }
  std::int32_t open_device() {
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0){failure=SDL_GetError();return failed;}
    initialized=true;
    SDL_AudioSpec spec{};spec.freq=output_rate;spec.format=AUDIO_F32SYS;
    spec.channels=output_channels;spec.samples=Uint16(config.callback_frames);
    spec.callback=callback;spec.userdata=this;
    device=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);
    if(!device){failure=SDL_GetError();return failed;}
    return 0;
  }
  void bind_services() {
    services.setting2=[this]{return native_muted;};
    services.create_device=[this](std::uint32_t& token) {
      const auto result=open_device();if(result>=0)token=device_token;return result;
    };
    services.cooperative_level=[](std::uint32_t token,std::uint32_t window,std::uint32_t level) {
      return token==device_token && window && level==2?0:failed;
    };
    services.allocate_pcm=[this](std::uint32_t bytes) {
      try {allocation.resize(bytes);return pcm_token;}
      catch(const std::bad_alloc&){failure="Native loading PCM allocation failed";return 0u;}
    };
    services.free_pcm=[this](std::uint32_t token) {
      if(token && token!=pcm_token)throw std::logic_error("Unknown native loading PCM allocation");
      std::vector<std::uint8_t>{}.swap(allocation);
    };
    services.create_buffer=[this](std::uint32_t token,const VistaLoadingAudioDescription& desc,std::uint32_t& result) {
      if(token!=device_token || desc!=VistaLoadingAudioDescription{} || created>=4)return failed;
      try {buffers[created].pcm.resize(desc.bytes);result=++created;return 0;}
      catch(const std::bad_alloc&){failure="Native loading buffer allocation failed";return failed;}
    };
    services.lock=[this](std::uint32_t token,std::uint32_t offset,std::uint32_t bytes,std::uint32_t flags,VistaLoadingAudioLock& lock) {
      const auto& b=buffers[index(token)];
      if(offset || bytes || flags!=2 || b.pcm.size()!=65408)return failed;
      lock={token,65408,0,0};return 0;
    };
    services.copy_pcm=[this](std::uint32_t token,std::span<const std::uint8_t> pcm) {
      auto& b=buffers[index(token)];
      if(pcm.size()!=b.pcm.size())throw std::logic_error("Native loading PCM copy extent differs");
      std::copy(pcm.begin(),pcm.end(),b.pcm.begin());
    };
    services.unlock=[](std::uint32_t token,const VistaLoadingAudioLock& lock) {
      return token==lock.first_token && lock.first_bytes==65408 && !lock.second_token && !lock.second_bytes?0:failed;
    };
    services.frequency=[this](std::uint32_t token,std::uint32_t frequency) {
      auto& b=buffers[index(token)];
      // SetFrequency changes playback Hz, not stored PCM. Each stream is
      // continuous across repeated original-bank puts; no rounded loop is made.
      auto* stream=SDL_NewAudioStream(AUDIO_S16LSB,1,int(frequency),AUDIO_F32SYS,2,output_rate);
      if(!stream){failure=SDL_GetError();return failed;}
      if(b.stream)SDL_FreeAudioStream(b.stream);
      b.stream=stream;b.frequency=frequency;return 0;
    };
    services.create_event=[this] {
      std::lock_guard lock(event_mutex);event_exists=true;event_signaled=false;return event_token;
    };
    services.create_worker=[this] {
      try {worker=std::thread([this]{run_worker();});return worker_token;}
      catch(const std::system_error& e){failure=e.what();return 0u;}
    };
    services.volume=[this](std::uint32_t token,std::int32_t value) {
      if(value<-10000 || value>0)return failed;
      DeviceLock lock(device);auto& b=buffers[index(token)];
      b.volume=value;b.amplitude=std::pow(10.0,double(value)/2000.0);return 0;
    };
    services.play=[this](std::uint32_t token,std::uint32_t reserved,std::uint32_t priority,std::uint32_t flags) {
      if(reserved || priority!=0xffffffffu || flags!=1)return failed;
      {
        DeviceLock lock(device);auto& b=buffers[index(token)];
        if(!b.stream)return failed;
        b.playing=true;
      }
      if(config.realtime)SDL_PauseAudioDevice(device,0);
      return 0;
    };
    services.clock=[this]{return config.clock?config.clock():SDL_GetTicks();};
    services.wait=[this](std::uint32_t token,std::uint32_t milliseconds) {
      if(token==worker_token && milliseconds==0xffffffffu) {
        if(worker.joinable())worker.join();
        return 0u;
      }
      if(token!=event_token || milliseconds!=100)throw std::logic_error("Unsupported native loading wait");
      std::unique_lock lock(event_mutex);
      if(!event_exists)throw std::logic_error("Native loading event does not exist");
      if(event_condition.wait_for(lock,std::chrono::milliseconds(milliseconds),[this]{return event_signaled;})) {
        event_signaled=false;return 0u;
      }
      return 0x102u; // actual native timeout, not a source readiness value
    };
    services.signal=[this](std::uint32_t token) {
      if(token!=event_token)throw std::logic_error("Unknown native loading event");
      std::lock_guard lock(event_mutex);event_signaled=true;event_condition.notify_one();
    };
    services.close=[](std::uint32_t token) {
      if(token!=worker_token)throw std::logic_error("Unknown native loading worker");
    };
    services.stop=[this](std::uint32_t token) {
      DeviceLock lock(device);buffers[index(token)].playing=false;
    };
    services.release=[this](std::uint32_t token) {
      if(token==device_token) {
        if(device){SDL_CloseAudioDevice(device);device=0;}
        if(initialized){SDL_QuitSubSystem(SDL_INIT_AUDIO);initialized=false;}
        created=0;return;
      }
      DeviceLock lock(device);auto& b=buffers[index(token)];
      if(b.stream)SDL_FreeAudioStream(b.stream);
      b=Buffer{};
    };
  }
  void run_worker() noexcept {
    worker_running=true;
    try {
      {
        std::lock_guard lock(control_mutex);
        vista_loading_audio_envelope(source,0,services,fp);
      }
      for(auto buffer:source.buffers)services.play(buffer,0,0xffffffffu,1);
      const auto origin=services.clock();
      while(services.wait(source.event,100)!=0) {
        const auto seconds=vista_loading_audio_elapsed_bits(origin,services.clock(),fp);
        std::lock_guard lock(control_mutex);
        vista_loading_audio_envelope(source,seconds,services,fp);
      }
    }catch(const std::exception& e) {
      std::lock_guard lock(control_mutex);
      SDL_strlcpy(worker_failure.data(),e.what(),worker_failure.size());
    }catch(...) {
      std::lock_guard lock(control_mutex);
      SDL_strlcpy(worker_failure.data(),"Native loading audio worker failed",worker_failure.size());
    }
    worker_running=false;
  }
  void stream_failure() noexcept {
    if(!callback_failure[0]) {
      const auto* error=SDL_GetError();
      SDL_strlcpy(callback_failure.data(),error[0]?error:"Native SDL stream returned incomplete output",callback_failure.size());
    }
  }
  void mix(float* output,std::size_t frames) noexcept {
    std::fill_n(output,frames*output_channels,0.f);
    for(std::size_t at=0;at<frames;) {
      const auto count=std::min<std::size_t>(scratch_frames,frames-at);
      const auto bytes=int(count*output_channels*sizeof(float));
      for(auto& b:buffers)if(b.playing && b.stream) {
        int available=SDL_AudioStreamAvailable(b.stream);
        while(available>=0 && available<bytes) {
          if(SDL_AudioStreamPut(b.stream,b.pcm.data(),int(b.pcm.size()))<0){available=-1;break;}
          b.submitted+=b.pcm.size();available=SDL_AudioStreamAvailable(b.stream);
        }
        if(available<0){stream_failure();b.playing=false;continue;}
        const auto got=SDL_AudioStreamGet(b.stream,scratch.data(),bytes);
        if(got!=bytes){stream_failure();b.playing=false;continue;}
        for(std::size_t i=0;i<count*output_channels;++i)
          output[at*output_channels+i]+=float(double(scratch[i])*b.amplitude);
      }
      at+=count;
    }
    // Native float output boundary, matching existing AudioEngine hard clip.
    for(std::size_t i=0;i<frames*output_channels;++i)output[i]=std::clamp(output[i],-1.f,1.f);
    mixed_frames+=frames;
  }
  static void callback(void* opaque,Uint8* bytes,int count) noexcept {
    if(count<0)return;
    if(count%int(output_channels*sizeof(float))){std::memset(bytes,0,std::size_t(count));return;}
    static_cast<State*>(opaque)->mix(reinterpret_cast<float*>(bytes),std::size_t(count)/(output_channels*sizeof(float)));
  }
};
NativeVistaLoadingAudio::NativeVistaLoadingAudio(std::span<const std::uint8_t> bank,
    NativeVistaLoadingAudioConfiguration config):state_(std::make_unique<State>(bank,std::move(config))) {}
NativeVistaLoadingAudio::NativeVistaLoadingAudio(const std::filesystem::path& path,
    NativeVistaLoadingAudioConfiguration config):NativeVistaLoadingAudio(read_bank(path),std::move(config)) {}
NativeVistaLoadingAudio::~NativeVistaLoadingAudio() noexcept {
  try{end();}catch(...){/* State's fallback still closes the native owners. */}
}
void NativeVistaLoadingAudio::begin(bool muted,bool window_present) {
  auto& s=*state_;std::lock_guard life(s.lifecycle_mutex);std::lock_guard control(s.control_mutex);
  if(!s.source.worker && (s.source.device || s.initialized || s.created || s.worker.joinable()))
    throw std::logic_error("Partial native loading audio admission requires end() before retry");
  s.native_muted=muted;
  if(!s.source.worker){s.failure.clear();s.callback_failure.fill(0);s.worker_failure.fill(0);s.mixed_frames=0;}
  vista_loading_audio_setup(s.source,window_present?1u:0u,s.encoded,s.services);
}
void NativeVistaLoadingAudio::gain(std::uint32_t bits) {
  auto& s=*state_;std::lock_guard lock(s.control_mutex);vista_loading_audio_gain(s.source,bits,fp);
}
void NativeVistaLoadingAudio::end() {
  auto& s=*state_;std::lock_guard life(s.lifecycle_mutex);
  // No control mutex is held across join: the worker may be applying an
  // envelope under that mutex. Gain only touches the separate attenuation word.
  vista_loading_audio_shutdown(s.source,s.services);
  // A rejected/corrupt native input can throw before original setup reaches
  // free_pcm. Its outstanding host allocation is still owned by this adapter.
  std::vector<std::uint8_t>{}.swap(s.allocation);
  if(s.initialized && !s.device){SDL_QuitSubSystem(SDL_INIT_AUDIO);s.initialized=false;}
}
NativeVistaLoadingAudioSnapshot NativeVistaLoadingAudio::snapshot() const {
  auto& s=*state_;std::lock_guard life(s.lifecycle_mutex);std::lock_guard control(s.control_mutex);DeviceLock device(s.device);
  NativeVistaLoadingAudioSnapshot out;out.source=s.source;out.device_open=s.device!=0;
  out.worker_running=s.worker_running;out.mixed_output_frames=s.mixed_frames;
  out.failure=!s.failure.empty()?s.failure:s.worker_failure[0]?s.worker_failure.data():s.callback_failure.data();
  for(unsigned i=0;i<4;++i) {
    const auto& b=s.buffers[i];out.frequencies[i]=b.frequency;out.volumes[i]=b.volume;
    out.playing[i]=b.playing;out.submitted_source_bytes[i]=b.submitted;
  }
  return out;
}
std::vector<float> NativeVistaLoadingAudio::render(std::size_t frames) {
  auto& s=*state_;std::lock_guard life(s.lifecycle_mutex);
  if(s.config.realtime)throw std::logic_error("Manual rendering requires a paused loading device");
  if(frames>std::vector<float>{}.max_size()/output_channels)throw std::length_error("Loading render extent exceeds PCM storage");
  std::vector<float> output(frames*output_channels);DeviceLock lock(s.device);s.mix(output.data(),frames);return output;
}
} // namespace halo2
