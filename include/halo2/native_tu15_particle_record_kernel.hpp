#pragma once
#include <xmmintrin.h>

namespace halo2::native_particle_record_detail {
// Original SUBSS/ADDSS/MULSS: the destination stays the destination, so a
// paired NaN returns the destination's quieted payload as on the original.
inline __m128 subtract(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subss %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_sub_ss(destination,source);
#endif
}
inline __m128 add(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addss %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_add_ss(destination,source);
#endif
}
inline __m128 multiply(__m128 destination,__m128 source) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0":"+x"(destination):"x"(source):"memory");return destination;
#else
    return _mm_mul_ss(destination,source);
#endif
}

// TU1.5 1E4C0: append one 0x70-byte particle record to the 1024-entry pool.
// A full pool only acknowledges a pending overflow flag. Otherwise the record
// takes the caller's payload bytes, its id and the point's signed distance to
// the fixed plane, plus a copy of the point.
template<class Frame> void append(Frame& frame) {
    if(frame.pool_full()) {
        if(frame.overflow_pending())frame.clear_overflow();
        else frame.return_full();
        return;
    }
    if(frame.reserve())frame.copy_payload();
    frame.publish();
}
} // namespace halo2::native_particle_record_detail
