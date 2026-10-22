#include "halo2/native_mcc_loading_audio.hpp"
#include "halo2/vista_loading_audio.hpp"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);
constexpr unsigned source_rate=44100,output_rate=48000,channels=2;
constexpr std::size_t frame_bytes=channels*sizeof(float);
constexpr VistaLoadingFloatingPoint loading_fp{0x027f,0x1f80};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class LoadingGainFloatingPointScope {
    std::fenv_t saved_;
    unsigned mxcsr_;
public:
    LoadingGainFloatingPointScope():mxcsr_(_mm_getcsr()) {
        if(std::fegetenv(&saved_)!=0)throw std::runtime_error("MCC loading gain FP context unavailable");
        // NativeVistaLoadingAudio's volume service computes this same native
        // amplitude inside the source envelope's recovered FP scope. Keep the
        // conversion here in that context too, then restore the caller state.
        _mm_setcsr(loading_fp.mxcsr);
        asm volatile("fldcw %0"::"m"(loading_fp.x87_control_word):"memory");
    }
    ~LoadingGainFloatingPointScope(){std::fesetenv(&saved_);_mm_setcsr(mxcsr_);}
};
#endif
void require(bool condition,const char *message) {
    if(!condition)throw std::invalid_argument(message);
}
std::runtime_error sdl_failure(const char *operation) {
    return std::runtime_error(std::string(operation)+": "+SDL_GetError());
}
}
struct NativeMccLoadingAudio::State {
    const NativeMccLoadingAudioConfiguration configuration;
    std::vector<float> source,converted;
    mutable std::mutex control;
    SDL_AudioDeviceID device{};
    SDL_AudioStream *stream{};
    bool initialized{},active{},muted{},callback_failed{};
    std::size_t position{};
    std::uint64_t rendered_frames{},render_calls{};
    std::int64_t conversion_frame_delta{};
    std::uint32_t gain_bits=0x3f800000;
    std::int32_t attenuation{};
    float amplitude=1.f;
    std::array<char,256> failure{};

    State(NativeMccLoadingPcm pcm,NativeMccLoadingAudioConfiguration config):configuration(std::move(config)) {
        require(pcm.sample_rate==source_rate&&pcm.channels==channels,"MCC loading PCM requires stereo float32 at 44100Hz");
        require(pcm.frames&&pcm.frames<=maximum_source_frames,"MCC loading PCM has an invalid declared frame extent");
        require(pcm.samples.size()==pcm.frames*channels,"MCC loading PCM sample extent differs from its declared frames");
        require(configuration.callback_frames&&configuration.callback_frames<=32768&&
            !(configuration.callback_frames&(configuration.callback_frames-1)),"MCC loading callback frames must be a bounded power of two");
        require(configuration.device_name.find('\0')==std::string::npos,"MCC loading output name has an embedded terminator");
        source.assign(pcm.samples.begin(),pcm.samples.end());
        for(const auto sample:source)require(std::isfinite(sample),"MCC loading PCM contains a nonfinite sample");
        // Finite Vorbis overshoot is retained. Do not clip or normalize source
        // samples, rewrite channel order, or relabel their sample rate.
    }
    ~State()noexcept { retire(); }
    void remember_failure(const char *message)noexcept {
        failure.fill(0);if(!message)return;
        for(std::size_t n=0;n+1<failure.size()&&message[n];++n)failure[n]=message[n];
    }
    void mix(std::span<float> output)noexcept {
        std::fill(output.begin(),output.end(),0.f);
        ++render_calls;rendered_frames+=output.size()/channels;
        if(!active||muted)return;
        const auto prepared=converted.size()/channels;
        const auto available=position<prepared?prepared-position:0;
        const auto frames=std::min(available,output.size()/channels);
        for(std::size_t n=0;n<frames*channels;++n)output[n]=converted[position*channels+n]*amplitude;
        position+=frames;
    }
    static void callback(void *user,Uint8 *bytes,int count)noexcept {
        auto &s=*static_cast<State*>(user);
        if(count<=0)return;
        std::memset(bytes,0,std::size_t(count));
        if(std::size_t(count)%frame_bytes){s.callback_failed=true;return;}
        s.mix({reinterpret_cast<float*>(bytes),std::size_t(count)/sizeof(float)});
    }
    // Control mutex is held by callers (or destruction has sole ownership).
    // Pause then lock before retiring callback-borrowed storage. The stream
    // is admission-only; it is never accessed by the callback.
    void retire()noexcept {
        const auto old_device=device;
        if(old_device){
            SDL_PauseAudioDevice(old_device,1);SDL_LockAudioDevice(old_device);
            active=false;SDL_UnlockAudioDevice(old_device);SDL_CloseAudioDevice(old_device);device=0;
        }else active=false;
        if(stream){SDL_FreeAudioStream(stream);stream=nullptr;}
        std::vector<float>{}.swap(converted);
        if(initialized){SDL_QuitSubSystem(SDL_INIT_AUDIO);initialized=false;}
    }
    void prepare() {
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)throw sdl_failure("Initialize MCC loading audio");
        initialized=true;
        SDL_AudioSpec wanted{},obtained{};
        wanted.freq=output_rate;wanted.format=AUDIO_F32SYS;wanted.channels=channels;
        wanted.samples=Uint16(configuration.callback_frames);wanted.callback=callback;wanted.userdata=this;
        device=SDL_OpenAudioDevice(configuration.device_name.empty()?nullptr:configuration.device_name.c_str(),
            0,&wanted,&obtained,0);
        if(!device)throw sdl_failure("Open MCC loading audio output");
        if(obtained.freq!=int(output_rate)||obtained.format!=AUDIO_F32SYS||obtained.channels!=channels)
            throw std::runtime_error("MCC loading output did not retain its admitted stereo float32 rate");
        stream=SDL_NewAudioStream(AUDIO_F32SYS,channels,source_rate,AUDIO_F32SYS,channels,output_rate);
        if(!stream)throw sdl_failure("Create MCC loading conversion stream");
        const auto bytes=source.size()*sizeof(float);
        if(bytes>std::size_t(std::numeric_limits<int>::max()))throw std::runtime_error("MCC loading PCM exceeds its SDL extent");
        if(SDL_AudioStreamPut(stream,source.data(),int(bytes))!=0||SDL_AudioStreamFlush(stream)!=0)
            throw sdl_failure("Prepare MCC loading conversion");
        const auto available=SDL_AudioStreamAvailable(stream);
        if(available<=0||std::size_t(available)%frame_bytes)
            throw std::runtime_error("MCC loading conversion returned an incomplete stereo extent");
        const auto prepared=std::uint64_t(available)/frame_bytes;
        const auto expected=(std::uint64_t(source.size()/channels)*output_rate+source_rate-1)/source_rate;
        // Retain the actual SDL flush tail, never synthesize or trim samples.
        // This native storage bound is not an authored loop/duration rule.
        if(prepared>expected+maximum_conversion_tail_frames||
            (expected>maximum_conversion_tail_frames&&prepared<expected-maximum_conversion_tail_frames))
            throw std::runtime_error("MCC loading conversion exceeded its bounded duration extent");
        converted.resize(std::size_t(available)/sizeof(float));
        if(SDL_AudioStreamGet(stream,converted.data(),available)!=available||SDL_AudioStreamAvailable(stream)!=0)
            throw sdl_failure("Retain complete MCC loading conversion");
        for(const auto sample:converted)if(!std::isfinite(sample))
            throw std::runtime_error("MCC loading conversion produced a nonfinite sample");
        conversion_frame_delta=std::int64_t(prepared)-std::int64_t(expected);
    }
};
NativeMccLoadingAudio::NativeMccLoadingAudio(NativeMccLoadingPcm pcm,NativeMccLoadingAudioConfiguration config)
    :state_(std::make_unique<State>(pcm,std::move(config))){}
NativeMccLoadingAudio::~NativeMccLoadingAudio()noexcept=default;
void NativeMccLoadingAudio::begin(bool muted) {
    auto &s=*state_;std::lock_guard lock(s.control);
    if(s.active)throw std::logic_error("MCC loading audio already owns an active interval");
    s.failure.fill(0);s.muted=muted;s.callback_failed=false;s.position=0;
    s.rendered_frames=s.render_calls=0;s.conversion_frame_delta=0;
    s.gain_bits=0x3f800000;s.attenuation=0;s.amplitude=1.f;s.active=true;
    if(muted)return;
    try {
        s.prepare();SDL_PauseAudioDevice(s.device,s.configuration.realtime?0:1);
    }catch(const std::exception &error){s.remember_failure(error.what());s.retire();throw;}
    catch(...){s.remember_failure("Unknown MCC loading audio admission failure");s.retire();throw;}
}
void NativeMccLoadingAudio::gain(std::uint32_t bits) {
    auto &s=*state_;std::lock_guard lock(s.control);
    VistaLoadingAudioState source;
    // Exact recovered782052 attenuation, without constructing its four-voice
    // owner/worker. The existing native adapter uses the same centibel mapping.
    float amplitude;
    {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        LoadingGainFloatingPointScope fp;
#endif
        vista_loading_audio_gain(source,bits,loading_fp);
        amplitude=float(std::pow(10.0,double(source.attenuation)/2000.0));
    }
    if(s.device)SDL_LockAudioDevice(s.device);
    s.gain_bits=bits;s.attenuation=source.attenuation;s.amplitude=amplitude;
    if(s.device)SDL_UnlockAudioDevice(s.device);
}
void NativeMccLoadingAudio::end()noexcept {
    auto &s=*state_;std::lock_guard lock(s.control);s.retire();
}
NativeMccLoadingAudioSnapshot NativeMccLoadingAudio::snapshot()const {
    const auto &s=*state_;std::lock_guard lock(s.control);
    NativeMccLoadingAudioSnapshot out;out.failure=s.failure.data();
    if(s.device)SDL_LockAudioDevice(s.device);
    out.active=s.active;out.muted=s.muted;out.device_open=bool(s.device);out.stream_open=bool(s.stream);
    out.audio_subsystem_owned=s.initialized;out.callback_failed=s.callback_failed;
    out.source_frames=s.source.size()/channels;out.prepared_frames=s.converted.size()/channels;out.position_frames=s.position;
    out.eof=s.active&&!s.muted&&s.position==s.converted.size()/channels;
    out.rendered_frames=s.rendered_frames;out.render_calls=s.render_calls;out.conversion_frame_delta=s.conversion_frame_delta;
    out.gain_bits=s.gain_bits;out.attenuation_centibels=s.attenuation;out.amplitude=s.amplitude;
    if(s.device)SDL_UnlockAudioDevice(s.device);
    return out;
}
void NativeMccLoadingAudio::render(std::span<float> output) {
    auto &s=*state_;std::lock_guard lock(s.control);
    if(s.configuration.realtime)throw std::logic_error("MCC loading manual output requires a paused manual device");
    if(!s.active)throw std::logic_error("MCC loading manual output has no active interval");
    if(output.size()%channels)throw std::invalid_argument("MCC loading manual output must contain stereo frames");
    if(output.empty())return;
    if(s.device)SDL_LockAudioDevice(s.device);
    s.mix(output);
    if(s.device)SDL_UnlockAudioDevice(s.device);
}
}
