#pragma once
// Packed SSE arithmetic for the native TU1.5 logic, in the original's operand
// order. SSE's first operand owns the NaN payload when both operands are
// NaNs, so the destination is explicit (the compiler may not swap a
// commutative intrinsic's operands); MXCSR rounding, DAZ/FTZ and sticky flags
// are the host's, which the boundary keeps equal to the original's.
#include <xmmintrin.h>

namespace halo2::native_packed_sse {
inline __m128 add(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addps %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_add_ps(a,b);
#endif
}
inline __m128 subtract(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subps %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_sub_ps(a,b);
#endif
}
inline __m128 multiply(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulps %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_mul_ps(a,b);
#endif
}
// SHUFPS x,x,lane*0x55: the lane in all four.
template<unsigned lane> inline __m128 broadcast(__m128 x){static_assert(lane<4);return _mm_shuffle_ps(x,x,lane*0x55u);}
} // namespace halo2::native_packed_sse
