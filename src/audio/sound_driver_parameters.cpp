#include "halo2/sound_driver_parameters.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
[[noreturn]] void unavailable(const char *why) { throw std::runtime_error(why); }
float real(std::uint32_t bits) { return std::bit_cast<float>(bits); }
std::int32_t pitch_base(std::uint32_t rate) {
  return rate == 0 ? -4597 : rate == 1 ? -501 : -2396;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct FloatingPointScope {
  std::fenv_t environment;
  unsigned mxcsr;
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &context)
      : mxcsr(_mm_getcsr()) {
    if ((context.x87_control_word & 0x3fu) != 0x3fu ||
        (context.mxcsr & 0x1f80u) != 0x1f80u || (context.mxcsr & 0xffff0000u))
      unavailable("Unmasked source audio floating-point exception delivery unavailable");
    if (std::fegetenv(&environment) != 0)
      unavailable("Source audio floating-point environment unavailable");
    _mm_setcsr(context.mxcsr);
    asm volatile("fldcw %0" :: "m"(context.x87_control_word) : "memory");
  }
  ~FloatingPointScope() { std::fesetenv(&environment); _mm_setcsr(mxcsr); }
};
float sub(float a, float b) { return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float mul(float a, float b) { return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b))); }
float normalized(float low,float high,float value,std::uint8_t direction,
                 const SoundDriverParameterConstants &c) {
  unsigned char degenerate;
  const auto epsilon = real(c.normalized_parameter_range_epsilon_bits);
  // The difference stays on the x87 stack through the original comparison.
  asm volatile("flds %1\n\tfsubs %2\n\tfabs\n\tflds %3\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
      : "=qm"(degenerate) : "m"(low),"m"(high),"m"(epsilon)
      : "cc","st","st(1)");
  if (degenerate)
    return std::uint32_t(low > value) == direction ? 0.f : real(c.mono_channel_count_bits);
  const auto numerator = sub(value,low), denominator = sub(high,low);
  auto result = _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(numerator),_mm_set_ss(denominator)));
  if (0.f > result) return 0.f;
  const auto one = real(c.mono_channel_count_bits);
  if (result > one) result = one;
  return result;
}
#endif
} // namespace

std::uint32_t source_sound_normalized_parameter(std::uint32_t low_bits,
    std::uint32_t high_bits,std::uint32_t value_bits,std::uint8_t direction,
    const SoundDriverParameterConstants &c,const SoundDriverFloatingPointContext &context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  return std::bit_cast<std::uint32_t>(normalized(real(low_bits),real(high_bits),real(value_bits),direction,c));
#else
  (void)low_bits;(void)high_bits;(void)value_bits;(void)direction;(void)c;(void)context;
  unavailable("Original x87/SSE audio parameter backend unavailable");
#endif
}
SoundDriverPitchCalculation calculate_source_sound_driver_pitch(std::uint32_t rate_index,
    std::uint32_t source_pitch_bits,const SoundDriverParameterConstants &c,
    const SoundDriverFloatingPointContext &context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  auto value = _mm_mul_ss(_mm_set_ss(real(source_pitch_bits)),_mm_set_ss(real(c.source_pitch_to_driver_pitch_bits)));
  value = _mm_add_ss(value,_mm_cvtsi32_ss(_mm_setzero_ps(),pitch_base(rate_index)));
  auto result = _mm_cvtss_f32(value);
  const auto minimum = real(c.driver_pitch_min_bits), maximum = real(c.driver_pitch_max_bits);
  if (minimum > result) result = minimum;
  else if (result > maximum) result = maximum;
  return {std::bit_cast<std::uint32_t>(result),_mm_cvttss_si32(_mm_set_ss(result))};
#else
  (void)rate_index;(void)source_pitch_bits;(void)c;(void)context;
  unavailable("Original SSE audio pitch backend unavailable");
#endif
}
std::uint32_t source_sound_distance_gain(std::uint32_t minimum_bits,
    std::uint32_t maximum_bits,std::uint32_t distance_bits,
    const SoundDriverParameterConstants &c,const SoundDriverFloatingPointContext &context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  const auto minimum=real(minimum_bits),maximum=real(maximum_bits),distance=real(distance_bits);
  const auto fraction=normalized(maximum,minimum,distance,0,c);
  //12C450 reloads distance from [ESP+18]. After POP ESI,12C45A reloads
  // minimum from [ESP+0C]; it does not reload maximum. The factor is
  // minimum/max(minimum,distance), followed by the reversed-range envelope.
  const auto denominator=minimum>distance?minimum:distance;
  const auto ratio=_mm_cvtss_f32(_mm_div_ss(_mm_set_ss(minimum),_mm_set_ss(denominator)));
  return std::bit_cast<std::uint32_t>(mul(ratio,fraction));
#else
  (void)minimum_bits;(void)maximum_bits;(void)distance_bits;(void)c;(void)context;
  unavailable("Original SSE sound distance backend unavailable");
#endif
}
SoundDriverVolumeCalculation calculate_source_sound_driver_volume(std::uint32_t source_gain_bits,
    const SoundDriverParameterConstants &c,const SoundDriverFloatingPointContext &context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  // Original pushC2800000/push0/CL=1 at221CF1. This is an explicit numeric
  // slice after its caller's real gain/change/shared-output decisions.
  const auto value = normalized(real(0xc2800000u),0.f,real(source_gain_bits),1,c);
  const auto scale = real(c.gain_normalized_to_driver_volume_bits);
  const auto scaled = sub(mul(value,scale),scale);
  return {std::bit_cast<std::uint32_t>(value),std::bit_cast<std::uint32_t>(scaled),
          _mm_cvttss_si32(_mm_set_ss(scaled))};
#else
  (void)source_gain_bits;(void)c;(void)context;
  unavailable("Original SSE audio volume backend unavailable");
#endif
}
std::int32_t source_sound_driver_pitch(std::uint32_t rate_index,
    std::uint32_t source_pitch_bits,const SoundDriverParameterConstants &c,
    const SoundDriverFloatingPointContext &context) {
  return calculate_source_sound_driver_pitch(rate_index,source_pitch_bits,c,context).driver_pitch;
}
std::int32_t source_sound_driver_volume(std::uint32_t source_gain_bits,
    const SoundDriverParameterConstants &c,const SoundDriverFloatingPointContext &context) {
  return calculate_source_sound_driver_volume(source_gain_bits,c,context).driver_volume;
}
std::uint32_t source_sound_relative_rate(std::int32_t stored_driver_pitch,
    std::uint32_t rate_index,const SoundDriverParameterConstants &c,
    const SoundDriverFloatingPointContext &context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);
  const auto adjusted = std::bit_cast<std::int32_t>(std::uint32_t(stored_driver_pitch) - std::uint32_t(pitch_base(rate_index)));
  const auto scale = real(c.driver_pitch_octave_scale_bits), ln2 = real(c.relative_rate_ln2_bits);
  float output;
  asm volatile("fildl %1\n\tfmuls %2\n\tfmuls %3\n\tfldl2e\n\tfmulp %%st,%%st(1)\n\tfld %%st(0)\n\tfrndint\n\tfxch %%st(1)\n\tfsub %%st(1),%%st\n\tf2xm1\n\tfld1\n\tfaddp %%st,%%st(1)\n\tfscale\n\tfstp %%st(1)\n\tfstps %0"
      : "=m"(output) : "m"(adjusted),"m"(scale),"m"(ln2)
      : "st","st(1)","st(2)");
  return std::bit_cast<std::uint32_t>(output);
#else
  (void)stored_driver_pitch;(void)rate_index;(void)c;(void)context;
  unavailable("Original x87 audio relative-rate backend unavailable");
#endif
}
std::uint32_t source_sound_relative_rate(std::uint32_t voice_index,
    std::uint32_t rate_index,const SoundDriverMemory &memory,
    const SoundDriverParameterConstants &c,const SoundDriverFloatingPointContext &context) {
  if (!memory.state || !memory.read)
    unavailable("Retained source sound driver state is unavailable");
  const auto address = memory.state + 0xcu + voice_index * 0x34u + 8u;
  const auto bytes = memory.read(address,4);
  if (bytes.size()!=4)unavailable("Source sound driver pitch backing is incomplete");
  std::uint32_t bits=0;
  for(unsigned i=0;i<4;++i)bits|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  return source_sound_relative_rate(std::bit_cast<std::int32_t>(bits),rate_index,c,context);
}
} // namespace halo2
