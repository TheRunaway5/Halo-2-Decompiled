#pragma once
#include <xmmintrin.h>

namespace halo2::native_codec_format_four_math_detail {
// Keep the authored destination operand of each SSE operation. Commuting
// an intrinsic can change the result when both inputs carry NaN payloads.
inline __m128 multiply_packed(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulps %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_mul_ps(destination,source);
#endif
}
inline __m128 add_packed(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addps %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_add_ps(destination,source);
#endif
}
inline __m128 subtract_packed(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subps %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_sub_ps(destination,source);
#endif
}
inline __m128 add_scalar(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addss %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_add_ss(destination,source);
#endif
}
inline __m128 multiply_scalar(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_mul_ss(destination,source);
#endif
}
} // namespace halo2::native_codec_format_four_math_detail
