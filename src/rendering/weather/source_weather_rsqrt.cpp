#include "halo2/source_weather_rsqrt.hpp"
#include <bit>
#include <stdexcept>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif

namespace halo2 {
std::uint32_t source_weather_rsqrt_xemu_0_8_136(std::uint32_t speed) {
    if(speed<0x00800000u || speed>=0x7f800000u)
        throw std::invalid_argument("xemu weather RSQRT profile requires a positive normal finite operand");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    const auto csr=_mm_getcsr();
    if((csr&~0x3fu)!=0x1f80)
        throw std::invalid_argument("xemu weather RSQRT profile requires nearest masked SSE/noDAZ/noFTZ");
    // Literal profile semantics from target/i386/ops_sse.h:856..867 at
    // https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/target/i386/ops_sse.h
    const auto root=_mm_sqrt_ss(_mm_set_ss(std::bit_cast<float>(speed)));
    const auto reciprocal=_mm_div_ss(_mm_set_ss(1.f),root);
    const auto answer=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(reciprocal));
    _mm_setcsr(csr);
    return answer;
#else
    throw std::invalid_argument("xemu weather RSQRT profile needs an admitted SSE host");
#endif
}
}
