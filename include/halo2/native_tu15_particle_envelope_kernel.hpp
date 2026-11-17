#pragma once
#include <xmmintrin.h>

namespace halo2::native_particle_envelope_detail {
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

// TU1.5 252EC0: particle envelope weight at time XMM1 for the record in EAX.
// A set global override returns one. Outside the open (start,end) window the
// weight stays zero; the attack and release spans ramp, otherwise one.
template<class Frame> void envelope(Frame& frame) {
    if(frame.override_set()) {frame.return_one();return;}
    if(!frame.after_start()||!frame.before_end()) {frame.return_zero();return;}
    if(frame.in_attack()) {frame.return_attack();return;}
    if(frame.in_release()) {frame.return_release();return;}
    frame.return_one();
}
} // namespace halo2::native_particle_envelope_detail
