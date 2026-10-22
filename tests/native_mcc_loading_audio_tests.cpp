#include "halo2/native_mcc_loading_audio.hpp"
#include <SDL.h>
#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cfenv>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace {
std::atomic<bool> watch_allocations{};
std::atomic<std::size_t> observed_allocations{};
void check(bool condition,const char *message) {if(!condition)throw std::runtime_error(message);}
template<class F>void rejects(F &&call,const char *message) {
    bool rejected{};try{call();}catch(const std::exception&){rejected=true;}check(rejected,message);
}
using namespace halo2;
NativeMccLoadingAudioConfiguration manual() {NativeMccLoadingAudioConfiguration c;c.realtime=false;return c;}
NativeMccLoadingPcm descriptor(std::span<const float> samples) {return {samples,samples.size()/2,44100,2};}
void released(const NativeMccLoadingAudioSnapshot &s) {
    check(!s.active&&!s.device_open&&!s.stream_open&&!s.audio_subsystem_owned&&!s.prepared_frames,
        "MCC loading resources survived end or failed admission");
    check(!s.callback_failed,"MCC loading callback rejected its admitted format");
}
void render_without_cpp_allocation(NativeMccLoadingAudio &audio,std::span<float> out) {
    observed_allocations=0;watch_allocations=true;
    try{audio.render(out);}catch(...){watch_allocations=false;throw;}
    watch_allocations=false;check(observed_allocations==0,"MCC loading manual callback consumer allocated C++ storage");
}
void eof_silence(NativeMccLoadingAudio &audio,std::span<const float> out,std::size_t prepared) {
    check(std::all_of(out.begin()+prepared*2,out.end(),[](float value){return value==0.f;}),
        "MCC loading one-shot repeated or synthesized samples after EOF");
    const auto s=audio.snapshot();check(s.eof&&s.position_frames==prepared,"MCC loading EOF/cursor differs from retained conversion");
    std::vector<float> later(512,9.f);render_without_cpp_allocation(audio,later);
    check(std::all_of(later.begin(),later.end(),[](float value){return value==0.f;}),"MCC loading EOF did not remain silent");
}
void conversion_extent(const NativeMccLoadingAudioSnapshot &s) {
    const auto expected=(s.source_frames*48000+44099)/44100;
    check(s.source_rate==44100&&s.output_rate==48000&&s.channels==2&&s.prepared_frames,
        "MCC loading source rate or channel format was relabeled");
    check(std::abs(std::int64_t(s.prepared_frames)-std::int64_t(expected))<=64,
        "SDL stereo conversion did not preserve source duration within64 output frames");
    check(s.conversion_frame_delta==std::int64_t(s.prepared_frames)-std::int64_t(expected),
        "MCC loading reported a fabricated conversion/flush extent");
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
void gain_context_controls() {
    struct CallerEnvironment {
        std::fenv_t saved;
        unsigned mxcsr=_mm_getcsr();
        CallerEnvironment(){check(std::fegetenv(&saved)==0,"Cannot retain caller FP environment");}
        ~CallerEnvironment(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
    } restore;
    std::array<float,128> pcm{};NativeMccLoadingAudio audio(descriptor(pcm),manual());
    audio.begin(true);
    // This nonexact source multiplication rounds to -1999.9998779296875 in
    // the recovered context before CVTTSS2SI truncates, rather than -2000.
    constexpr std::uint32_t input=0x3f19999a;
    audio.gain(input);const auto baseline=audio.snapshot();
    check(baseline.attenuation_centibels==-1999,"MCC loading nonexact source gain control differs");
    const auto amplitude=std::bit_cast<std::uint32_t>(baseline.amplitude);
    for(const auto rounding:{FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
        check(std::feclearexcept(FE_ALL_EXCEPT)==0&&std::fesetround(rounding)==0,
            "Cannot prepare alternate caller FP rounding");
        std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
        cw=std::uint16_t((cw&~0x0300u)|0x0300u); // extended caller precision
        asm volatile("fldcw %0"::"m"(cw):"memory");
        // Deliberately differ from x87 rounding and preserve FTZ/sticky state.
        _mm_setcsr(0x1f80u|0x8000u|(rounding==FE_UPWARD?0x2000u:0x4000u));
        check(std::feraiseexcept(FE_INVALID|FE_DIVBYZERO)==0,"Cannot prepare caller FP sticky exceptions");
        std::uint16_t before_cw,before_sw;asm volatile("fnstcw %0":"=m"(before_cw));
        asm volatile("fnstsw %0":"=m"(before_sw));const auto before_mxcsr=_mm_getcsr();
        const auto before_exceptions=std::fetestexcept(FE_ALL_EXCEPT);
        audio.gain(input);
        std::uint16_t after_cw,after_sw;asm volatile("fnstcw %0":"=m"(after_cw));
        asm volatile("fnstsw %0":"=m"(after_sw));
        check(after_cw==before_cw&&after_sw==before_sw&&_mm_getcsr()==before_mxcsr&&
            std::fetestexcept(FE_ALL_EXCEPT)==before_exceptions,
            "MCC loading gain leaked caller x87/SSE controls or sticky exceptions");
        const auto actual=audio.snapshot();
        check(actual.attenuation_centibels==baseline.attenuation_centibels&&
            std::bit_cast<std::uint32_t>(actual.amplitude)==amplitude&&actual.gain_bits==input&&
            !actual.position_frames&&!actual.device_open,
            "MCC loading gain depends on ambient rounding or changed muted playback ownership");
    }
    audio.end();released(audio.snapshot());
    std::cout<<"PASS MCC loading recovered gain context: nonexact source truncation and identical amplitude across alternate x87/SSE rounding; caller controls and sticky exceptions restored\n";
}
#endif
void synthetic_controls() {
    std::vector<float> samples(4410*2);
    for(std::size_t n=0;n<samples.size();n+=2){samples[n]=1.5f;samples[n+1]=-.375f;}
    auto pcm=descriptor(samples);
    {
        auto invalid=pcm;invalid.channels=1;rejects([&]{NativeMccLoadingAudio a(invalid,manual());},"Mono PCM accepted as stereo");
        invalid=pcm;invalid.sample_rate=48000;rejects([&]{NativeMccLoadingAudio a(invalid,manual());},"44100Hz PCM rate guard missing");
        invalid=pcm;++invalid.frames;rejects([&]{NativeMccLoadingAudio a(invalid,manual());},"Declared PCM extent was ignored");
        invalid=pcm;invalid.frames=NativeMccLoadingAudio::maximum_source_frames+1;
        rejects([&]{NativeMccLoadingAudio a(invalid,manual());},"Excessive PCM extent accepted");
        invalid=pcm;invalid.samples=invalid.samples.first(invalid.samples.size()-1);
        rejects([&]{NativeMccLoadingAudio a(invalid,manual());},"Incomplete stereo sample accepted");
        invalid={};rejects([&]{NativeMccLoadingAudio a(invalid,manual());},"Empty PCM accepted");
        for(float value:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN()}) {
            auto bad=samples;bad[19]=value;rejects([&]{NativeMccLoadingAudio a(descriptor(bad),manual());},"Nonfinite PCM accepted");
        }
        auto config=manual();config.callback_frames=513;
        rejects([&]{NativeMccLoadingAudio a(pcm,config);},"Invalid callback frame extent accepted");
        config=manual();config.device_name=std::string("device\0tail",11);
        rejects([&]{NativeMccLoadingAudio a(pcm,config);},"Embedded device-name terminator accepted");
    }
    NativeMccLoadingAudio audio(pcm,manual());
    // The owner retains a private exact PCM copy, not this later caller edit.
    std::fill(samples.begin(),samples.end(),0.f);
    audio.begin(false);auto state=audio.snapshot();conversion_extent(state);
    check(state.active&&state.device_open&&state.stream_open&&state.audio_subsystem_owned&&!state.muted,
        "Manual MCC loading did not admit its real paused SDL owner");
    rejects([&]{audio.begin(false);},"Overlapping loading interval accepted");
    const auto prepared=std::size_t(state.prepared_frames);
    std::vector<float> full((prepared+257)*2,9.f);render_without_cpp_allocation(audio,full);
    check(prepared>512,"Synthetic stereo duration became too short");
    for(std::size_t n=256;n+256<prepared;++n)
        check(std::abs(full[n*2]-1.5f)<.001f&&std::abs(full[n*2+1]+.375f)<.001f,
            "MCC loading conversion clipped overshoot, changed channels or borrowed edited source PCM");
    eof_silence(audio,full,prepared);audio.end();released(audio.snapshot());audio.end();released(audio.snapshot());
    rejects([&]{audio.render(full);},"Manual rendering continued after end");

    audio.begin(false);state=audio.snapshot();check(!state.position_frames&&!state.render_calls&&state.amplitude==1.f,
        "Restart did not begin at the original one-shot interval");
    std::vector<float> odd(3);rejects([&]{audio.render(odd);},"Odd manual output sample count accepted");
    check(!audio.snapshot().position_frames,"Invalid manual output consumed samples");
    // Independent exact782052 control points. Its centibel mapping is not a
    // linear music gain, nor the original four-voice sinusoidal worker.
    struct Gain {float input;std::int32_t centibels;};
    for(const auto row:{Gain{0.f,-5000},Gain{.25f,-3750},Gain{.5f,-2500},Gain{.75f,-1250},Gain{1.f,0},Gain{-1.f,-5000},Gain{2.f,0}}) {
        audio.gain(std::bit_cast<std::uint32_t>(row.input));const auto gain=audio.snapshot();
        check(gain.attenuation_centibels==row.centibels&&gain.gain_bits==std::bit_cast<std::uint32_t>(row.input),
            "MCC loading gain differs from recovered source attenuation");
        check(std::abs(gain.amplitude-float(std::pow(10.,double(row.centibels)/2000.)))<1e-7f,
            "MCC loading centibel amplitude differs from native reference mapping");
        check(!gain.position_frames,"MCC loading gain advanced the one-shot cursor");
    }
    audio.gain(std::bit_cast<std::uint32_t>(.5f));const auto amplitude=audio.snapshot().amplitude;
    std::vector<float> attenuated(full.size(),9.f);render_without_cpp_allocation(audio,attenuated);
    for(std::size_t n=0;n<prepared*2;++n)
        check(std::abs(attenuated[n]-full[n]*amplitude)<1e-6f,"MCC loading gain did not apply to the unchanged stereo stream");
    eof_silence(audio,attenuated,prepared);audio.end();released(audio.snapshot());
    audio.begin(true);const auto muted=audio.snapshot();
    check(muted.active&&muted.muted&&!muted.device_open&&!muted.stream_open&&!muted.audio_subsystem_owned,
        "Muted loading opened a device/converter");
    std::vector<float> silence(1024,9.f);render_without_cpp_allocation(audio,silence);
    check(std::all_of(silence.begin(),silence.end(),[](float v){return v==0.f;})&&!audio.snapshot().position_frames,
        "Muted loading consumed PCM or emitted audio");
    audio.gain(std::bit_cast<std::uint32_t>(.25f));audio.end();released(audio.snapshot());

    auto bad_device=manual();bad_device.device_name="Halo2-no-such-MCC-loading-output";
    NativeMccLoadingAudio failed(pcm,bad_device);
    rejects([&]{failed.begin(false);},"Unavailable named SDL output silently fell back");
    const auto failure=failed.snapshot();released(failure);check(!failure.failure.empty(),"Partial admission lost its failure receipt");
    failed.end();failed.end();failed.begin(true);check(failed.snapshot().failure.empty(),"Successful muted retry retained stale failure");failed.end();
    check(!(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO),"Failed/repeated loading leaked its audio-subsystem ownership");

    NativeMccLoadingAudio live(pcm);live.begin(false);
    rejects([&]{live.render(silence);},"Manual rendering accepted an unpaused real device");
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!live.snapshot().render_calls&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const auto playing=live.snapshot();check(playing.render_calls&&playing.position_frames&&!playing.callback_failed,
        "Actual dummy SDL callback did not consume the admitted track");
    live.end();released(live.snapshot());
    std::cout<<"PASS MCC loading playback component: exact stereo rate/extent and source copy; finite overshoot; recovered gain; one-shot EOF/restart/mute; real dummy callback; unavailable-device unwind; zero C++ allocations in manual callback consumer\n";
}
std::uint32_t u32(const std::vector<unsigned char> &bytes,std::size_t at) {
    return std::uint32_t(bytes.at(at))|(std::uint32_t(bytes.at(at+1))<<8)|
        (std::uint32_t(bytes.at(at+2))<<16)|(std::uint32_t(bytes.at(at+3))<<24);
}
std::uint16_t u16(const std::vector<unsigned char> &bytes,std::size_t at) {return std::uint16_t(bytes.at(at))|(std::uint16_t(bytes.at(at+1))<<8);}
std::vector<float> actual_pcm(const std::filesystem::path &path) {
    constexpr std::size_t frames=2744320,data_bytes=frames*2*sizeof(float);
    check(path.is_absolute()&&!std::filesystem::is_symlink(path)&&std::filesystem::is_regular_file(path),
        "Actual MCC PCM must be an explicit absolute regular file supplied by the owned fixture");
    check(std::filesystem::file_size(path)==44+data_bytes,"Actual MCC WAV extent differs from authenticated source profile");
    std::ifstream input(path,std::ios::binary);std::vector<unsigned char> raw(44+data_bytes);
    check(bool(input.read(reinterpret_cast<char*>(raw.data()),std::streamsize(raw.size()))),"Actual MCC WAV read was incomplete");
    check(std::memcmp(raw.data(),"RIFF",4)==0&&u32(raw,4)==data_bytes+36&&std::memcmp(raw.data()+8,"WAVEfmt ",8)==0&&
        u32(raw,16)==16&&u16(raw,20)==3&&u16(raw,22)==2&&u32(raw,24)==44100&&u32(raw,28)==44100*8&&
        u16(raw,32)==8&&u16(raw,34)==32&&std::memcmp(raw.data()+36,"data",4)==0&&u32(raw,40)==data_bytes,
        "Actual MCC loading input is not the canonical stereo44100Hz IEEEF32 conversion");
    unsigned char digest[SHA256_DIGEST_LENGTH];SHA256(raw.data()+44,data_bytes,digest);
    constexpr std::string_view expected="4170c17c50ed19f4fd1fcd292431a78564c139ff6f1324e3ea56fdefcac34d6e";
    constexpr char hex[]="0123456789abcdef";
    for(unsigned n=0;n<SHA256_DIGEST_LENGTH;++n)
        check(expected[n*2]==hex[digest[n]>>4]&&expected[n*2+1]==hex[digest[n]&15],"Actual loading PCM hash differs from authenticated MCC media");
    std::vector<float> pcm(frames*2);
    for(std::size_t n=0;n<pcm.size();++n)pcm[n]=std::bit_cast<float>(u32(raw,44+n*4));
    return pcm;
}
void actual_track(const std::filesystem::path &path) {
    auto samples=actual_pcm(path);
    bool source_left{},source_right{},source_separate{};
    for(std::size_t n=0;n<samples.size();n+=2){
        source_left|=samples[n]!=0;source_right|=samples[n+1]!=0;source_separate|=samples[n]!=samples[n+1];
    }
    check(source_left||source_right,"Authenticated MCC track contains no audible source samples");
    NativeMccLoadingAudio audio(descriptor(samples),manual());audio.begin(false);
    const auto state=audio.snapshot();conversion_extent(state);const auto prepared=std::size_t(state.prepared_frames);
    std::vector<float> full((prepared+257)*2,9.f);render_without_cpp_allocation(audio,full);
    bool left{},right{},separate{};
    for(std::size_t n=0;n<prepared;++n){
        check(std::isfinite(full[n*2])&&std::isfinite(full[n*2+1]),"Actual MCC converted output is not finite");
        left|=full[n*2]!=0;right|=full[n*2+1]!=0;separate|=full[n*2]!=full[n*2+1];
    }
    check(left==source_left&&right==source_right&&separate==source_separate,
        "Actual MCC output lost its authenticated source channel/content properties");eof_silence(audio,full,prepared);
    audio.end();released(audio.snapshot());audio.begin(false);
    std::vector<float> prefix(4096*2);render_without_cpp_allocation(audio,prefix);
    check(std::memcmp(prefix.data(),full.data(),prefix.size()*sizeof(float))==0,"Actual MCC restart changed the prepared source prefix");
    audio.end();audio.begin(true);std::fill(prefix.begin(),prefix.end(),9.f);render_without_cpp_allocation(audio,prefix);
    check(std::all_of(prefix.begin(),prefix.end(),[](float v){return v==0.f;}),"Muted actual MCC track emitted audio");
    audio.end();released(audio.snapshot());
    std::cout<<"PASS genuine MCC loading PCM playback: source_sha256=4170c17c50ed19f4fd1fcd292431a78564c139ff6f1324e3ea56fdefcac34d6e source_frames="
        <<state.source_frames<<" prepared_48000_frames="<<state.prepared_frames<<" conversion_frame_delta="<<state.conversion_frame_delta
        <<" source_left_audible="<<source_left<<" source_right_audible="<<source_right<<" source_nondualmono="<<source_separate
        <<" stereo finite one-shot EOF/restart/mute; importer/package/live frontend remain unqualified\n";
}
}
void *operator new(std::size_t bytes) {
    if(watch_allocations.load(std::memory_order_relaxed))observed_allocations.fetch_add(1,std::memory_order_relaxed);
    if(auto *p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();
}
void *operator new[](std::size_t bytes) {return ::operator new(bytes);}
void operator delete(void *p)noexcept {std::free(p);}
void operator delete[](void *p)noexcept {std::free(p);}
void operator delete(void *p,std::size_t)noexcept {std::free(p);}
void operator delete[](void *p,std::size_t)noexcept {std::free(p);}
int main(int argc,char **argv)try {
#if (!defined(__i386__)&&!defined(__x86_64__))||defined(_MSC_VER)
    std::cerr<<"Existing recovered source gain helper is unqualified on this compiler/architecture\n";return 77;
#else
    check(argc==1||(argc==3&&std::string_view(argv[1])=="--mcc-pcm"),"Supply optional --mcc-pcm absolute-owned-WAV");
    check(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"Cannot select the real SDL dummy audio backend");
    gain_context_controls();synthetic_controls();if(argc==3)actual_track(argv[2]);return 0;
#endif
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
