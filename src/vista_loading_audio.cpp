#include "halo2/vista_loading_audio.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FloatingPointScope {
  std::fenv_t saved_;unsigned mxcsr_;
public:
  explicit FloatingPointScope(const VistaLoadingFloatingPoint& c):mxcsr_(_mm_getcsr()) {
    if(c.x87_control_word!=0x027f || (c.mxcsr&~0x3fu)!=0x1f80)
      throw std::runtime_error("Original Vista loading audio FP context unverified");
    if(std::fegetenv(&saved_)!=0)throw std::runtime_error("Loading audio FP context unavailable");
    _mm_setcsr(c.mxcsr);asm volatile("fldcw %0"::"m"(c.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&saved_);_mm_setcsr(mxcsr_);}
};
float sub(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float div(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}
std::int32_t envelope_volume(float phase,float amplitude,std::int32_t adjusted) {
  const double pi=3.1415,exponent=std::bit_cast<double>(std::uint64_t{0x3fd3333340000000});
  double base;
  asm volatile("flds %1; fmull %2; fsin; fstpl %0":"=m"(base):"m"(phase),"m"(pi):"st");
  long double power=0;
  if(base!=0)asm volatile("fldl %2; fldl %1; fyl2x; fld %%st(0); frndint; fsub %%st(0),%%st(1); fxch %%st(1); fchs; f2xm1; fld1; faddp; fscale; fstp %%st(1); fstpt %0"
      :"=m"(power):"m"(base),"m"(exponent):"st","st(1)");
  std::int32_t result;std::uint16_t cw,chop;
  asm volatile("fnstcw %0":"=m"(cw));chop=cw|0xc00;
  asm volatile("fldt %1; fmuls %2; fiaddl %3; fldcw %4; fistpl %0; fldcw %5"
      :"=m"(result):"m"(power),"m"(amplitude),"m"(adjusted),"m"(chop),"m"(cw):"st","memory");
  return result;
}
#endif
}
std::vector<std::uint8_t> vista_loading_audio_decode(std::span<const std::uint8_t> input) {
  // Literal PE7BBEF0 and7BBED0, mono49A722. Header predictor is emitted;
  // only63 payload nibbles are consumed, as in original36→64 samples.
  static constexpr std::array<int,89> steps{7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
  static constexpr std::array<int,16> changes{-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
  if(input.size()%36)throw std::runtime_error("Loading audio mono block domain unsupported");
  std::vector<std::uint8_t> out;out.reserve(input.size()/36*128);
  auto emit=[&](int value){auto word=static_cast<std::uint16_t>(value);out.push_back(word&255);out.push_back(word>>8);};
  for(std::size_t offset=0;offset<input.size();offset+=36) {
    int value=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(input[offset])|std::uint16_t(input[offset+1])<<8));
    int index=input[offset+2];if(index>88)throw std::runtime_error("Loading audio step index unsupported");emit(value);
    for(unsigned j=0;j<63;++j) {
      const int nibble=(input[offset+4+j/2]>>(4*(j%2)))&15,step=steps[index];
      const int delta=(step>>3)+((nibble&4)?step:0)+((nibble&2)?step>>1:0)+((nibble&1)?step>>2:0);
      value=std::clamp(value+((nibble&8)?-delta:delta),-32768,32767);
      index=std::clamp(index+changes[nibble],0,88);emit(value);
    }
  }
  return out;
}
void vista_loading_audio_setup(VistaLoadingAudioState& s,std::uint32_t window,
    std::span<const std::uint8_t> encoded,const VistaLoadingAudioServices& api) {
  if(s.worker)return;
  bool ready=!api.setting2() && window!=0;
  if(ready)ready=api.create_device(s.device)>=0;
  if(ready)ready=api.cooperative_level(s.device,window,2)>=0;
  std::uint32_t allocation=0;
  std::vector<std::uint8_t> pcm;
  if(ready) {
    if(encoded.size()!=18396)throw std::runtime_error("Original loading audio bank size mismatch");
    allocation=api.allocate_pcm(65408);ready=allocation!=0;
    if(ready)pcm=vista_loading_audio_decode(encoded);
  }
  VistaLoadingAudioDescription desc;
  if(ready)for(unsigned i=0;i<4;++i) {
    ready=api.create_buffer(s.device,desc,s.buffers[i])>=0;
    VistaLoadingAudioLock lock;
    if(ready)ready=api.lock(s.buffers[i],0,0,2,lock)>=0;
    if(ready) {
      api.copy_pcm(lock.first_token,pcm);
      ready=api.unlock(s.buffers[i],lock)>=0;
    }
    if(ready)ready=api.frequency(s.buffers[i],vista_loading_audio_frequencies[i])>=0;
    s.cached_volumes[i]=-10000;
    if(!ready)break;
  }
  if(ready && !s.event){s.event=api.create_event();ready=s.event!=0;}
  if(ready){s.attenuation=0;s.worker=api.create_worker();}
  api.free_pcm(allocation);
}
void vista_loading_audio_gain(VistaLoadingAudioState& s,std::uint32_t word,const VistaLoadingFloatingPoint& c) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope fp(c);const auto gain=_mm_set_ss(std::bit_cast<float>(word));
  const auto scaled=_mm_mul_ss(_mm_sub_ss(_mm_set_ss(1.f),gain),_mm_set_ss(-5000.f));
  s.attenuation=std::clamp(_mm_cvttss_si32(scaled),-5000,0);
#else
  (void)s;(void)word;(void)c;throw std::runtime_error("Original Vista loading audio x87/SSE unavailable");
#endif
}
void vista_loading_audio_envelope(VistaLoadingAudioState& s,std::uint32_t word,
    const VistaLoadingAudioServices& api,const VistaLoadingFloatingPoint& c) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope fp(c);const float seconds=std::bit_cast<float>(word);
  if(!std::isfinite(seconds) || s.attenuation < -5000 || s.attenuation>0 || s.maximum_volume < -9600 || s.maximum_volume>0)
    throw std::runtime_error("Original Vista loading audio envelope domain unverified");
  constexpr std::array<float,4> start{0,2,3.5,5.5},amplitude{3500,4500,3500,4500};
  for(unsigned i=0;i<4;++i) {
    const float phase=start[i]>seconds?0.f:seconds>20.f?1.f:div(sub(seconds,start[i]),sub(20.f,start[i]));
    const auto buffer=s.buffers[i];if(!buffer)continue;
    const auto volume=std::clamp(envelope_volume(phase,amplitude[i],s.attenuation-5000),-9600,s.maximum_volume);
    if(volume!=s.cached_volumes[i]){api.volume(buffer,volume);s.cached_volumes[i]=volume;}
  }
#else
  (void)s;(void)word;(void)api;(void)c;throw std::runtime_error("Original Vista loading audio x87/SSE unavailable");
#endif
}
std::uint32_t vista_loading_audio_elapsed_bits(std::uint32_t origin,std::uint32_t now,const VistaLoadingFloatingPoint& c) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope fp(c);const auto difference=std::bit_cast<std::int32_t>(now-origin);
  const float wrap=std::bit_cast<float>(0x4f800000u),scale=std::bit_cast<float>(0x3a83126fu);float result;
  if(difference<0)asm volatile("fildl %1; fadds %2; fmuls %3; fstps %0":"=m"(result):"m"(difference),"m"(wrap),"m"(scale):"st");
  else asm volatile("fildl %1; fmuls %2; fstps %0":"=m"(result):"m"(difference),"m"(scale):"st");
  return std::bit_cast<std::uint32_t>(result);
#else
  (void)origin;(void)now;(void)c;throw std::runtime_error("Original Vista loading audio x87 unavailable");
#endif
}
void vista_loading_audio_worker(VistaLoadingAudioState& s,const VistaLoadingAudioServices& api,const VistaLoadingFloatingPoint& c) {
  vista_loading_audio_envelope(s,0,api,c);
  for(auto buffer:s.buffers){if(!buffer)throw std::runtime_error("Original loading audio worker requires four buffers");api.play(buffer,0,0xffffffff,1);}
  const auto origin=api.clock();
  while(api.wait(s.event,100)!=0)vista_loading_audio_envelope(s,vista_loading_audio_elapsed_bits(origin,api.clock(),c),api,c);
}
void vista_loading_audio_shutdown(VistaLoadingAudioState& s,const VistaLoadingAudioServices& api) {
  if(s.worker){api.signal(s.event);api.wait(s.worker,0xffffffff);api.close(s.worker);s.worker=0;}
  for(auto& buffer:s.buffers)if(buffer){api.stop(buffer);if(buffer){api.release(buffer);buffer=0;}}
  if(s.device){api.release(s.device);s.device=0;}
}
} // namespace halo2
