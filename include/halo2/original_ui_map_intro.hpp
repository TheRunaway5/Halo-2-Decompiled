#pragma once
#include <bit>
#include <cfenv>
#include <cstdint>
#include <optional>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
// Xbox TU1.5 14A70D..14A7A3. New-map UI callback14A38B stamps
//55C450 from accumulated/truncated UI time55C448. This channel4 overlay
// follows all UI children. It is separate from PCR16's native open clock.
inline std::optional<std::uint32_t> original_ui_map_intro_color(
    std::uint32_t initialized,std::uint32_t ui_clock,int output_user,
    bool game_in_progress,std::uint32_t game_type,
    std::uint16_t x87_control_word,std::uint32_t mxcsr) {
  if(output_user!=4)return {};
  const auto elapsed=ui_clock-initialized;
  if(game_in_progress&&(game_type==1||game_type==2))return {};
  if(elapsed<=3000)return 0xff000000u;
  if(elapsed>4000)return {};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if((x87_control_word&0x3f)!=0x3f || (x87_control_word&0x300)==0x100 ||
     (mxcsr&0x1f80)!=0x1f80 || (mxcsr&0xffff0000))
    throw std::invalid_argument("Unsupported original UI intro floating point context");
  struct FloatingPointScope {
    std::fenv_t saved;unsigned mxcsr;
    FloatingPointScope(std::uint16_t word,std::uint32_t sse):mxcsr(_mm_getcsr()) {
      if(std::fegetenv(&saved))throw std::runtime_error("Original UI intro floating point environment unavailable");
      _mm_setcsr(sse);asm volatile("fldcw %0"::"m"(word):"memory");
    }
    ~FloatingPointScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
  } scope(x87_control_word,mxcsr);
  const auto fade=std::int32_t(elapsed-3000);
  const float scale=std::bit_cast<float>(0x3a83126fu),one=1.f,maximum=255.f;
  float spill;std::int32_t alpha;
  // Preserve the original extended x87 arithmetic, binary32 spill, reload,
  // then FISTP. A host binary32/SSE curve changes boundary alpha values.
  asm volatile("fildl %2; fmuls %3; fsubrs %4; fmuls %5; fstps %1; flds %1; fistpl %0"
      :"=m"(alpha),"=m"(spill):"m"(fade),"m"(scale),"m"(one),"m"(maximum)
      :"st","memory");
  if(!alpha)return {};
  return std::bit_cast<std::uint32_t>(alpha)<<24;
#else
  throw std::runtime_error("Original UI intro x87 kernel unavailable");
#endif
}
} // namespace halo2
