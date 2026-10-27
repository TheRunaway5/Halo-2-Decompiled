#pragma once
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <optional>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
// Xbox TU1.5 23A0F3 (retail23370E), SHA21b207e7... . The screen185
// flag bypasses its window draw. Screen16 uses the signed wrapped elapsed
// clock from796B0: 2000ms opaque, then2000ms of SSE binary32 alpha followed
// by x87 FISTP. Source233423 stamps75870 only after all six tabs initialize.
// The two FP words are caller context, not a replacement timing policy.
inline std::optional<std::uint32_t> original_pcr_intro_color(
    bool prior_stats,std::uint32_t opened,std::uint32_t now,
    std::uint16_t x87_control_word,std::uint32_t mxcsr) {
  if(prior_stats)return {};
  const auto elapsed=std::bit_cast<std::int32_t>(now-opened);
  if(elapsed<=2000)return 0xff000000u;
  const auto fade=std::clamp(elapsed-2000,0,2000);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if((x87_control_word&0x3f)!=0x3f || (x87_control_word&0x300)==0x100 ||
     (mxcsr&0x1f80)!=0x1f80 || (mxcsr&0xffff0000))
    throw std::invalid_argument("Unsupported original PCR floating point context");
  struct FloatingPointScope {
    std::fenv_t saved;unsigned mxcsr;
    FloatingPointScope(std::uint16_t word,std::uint32_t sse):mxcsr(_mm_getcsr()) {
      if(std::fegetenv(&saved))throw std::runtime_error("Original PCR floating point environment unavailable");
      _mm_setcsr(sse);asm volatile("fldcw %0"::"m"(word):"memory");
    }
    ~FloatingPointScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
  } scope(x87_control_word,mxcsr);
  std::int32_t alpha;float spill;
  const float one=1.f,scale=std::bit_cast<float>(0x3a03126fu),maximum=255.f;
  asm volatile("cvtsi2ssl %2,%%xmm0; mulss %4,%%xmm0; movss %3,%%xmm1; "
      "subss %%xmm0,%%xmm1; mulss %5,%%xmm1; movss %%xmm1,%1; flds %1; fistpl %0"
      :"=m"(alpha),"=m"(spill):"rm"(fade),"m"(one),"m"(scale),"m"(maximum)
      :"xmm0","xmm1","st","memory");
  return std::bit_cast<std::uint32_t>(alpha)<<24;
#else
  throw std::runtime_error("Original PCR x87/SSE kernel unavailable");
#endif
}
} // namespace halo2
