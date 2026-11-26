#pragma once

#include "physics/runtime/native_tu15_pose_setup_c85a0.hpp"

namespace halo2::native_pose_normalization_c8827 {

using namespace native_object_tick_detail::boundary;
using physics_step_detail::f;
using native_tu15_leaves::ordered_mul_ss;

inline constexpr std::uint32_t one = 0x46a0d0u;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_POSE_NORMALIZATION_C8827");
        return value && *value;
    }();
    return enabled;
}

inline __m128 divide(__m128 numerator, __m128 denominator) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("divss %1,%0" : "+x"(numerator) : "x"(denominator));
    return numerator;
#else
    return _mm_div_ss(numerator,denominator);
#endif
}

template<class State>
bool writes_zero_angles(native_pose_setup_c85a0::Frame<State> &p) {
    const auto motion = p.read(0xc8827u,p.field(esi,0x144u));
    p.flags.scalar_compare(p.x[2],_mm_set_ss(f(motion)),true);
    p.native_site(0xc882eu);
    p.s.set(eax,p.flags.bits() | 2u,1,8);
    p.native_site(0xc882fu);
    p.flags.logic((p.g[eax] >> 8u) & 0x44u,1);
    p.native_site(0xc8832u);
    if(p.flags.pf) return false;
    p.store(0xc8834u,p.stack(0x14u),p.scalar_bits(2));
    p.store(0xc883au,p.stack(0x18u),p.scalar_bits(2));
    p.native_site(0xc8840u);
    return true;
}

template<class State, class Angle>
void first_angle(State &s,native_pose_setup_c85a0::Frame<State> &p,Angle &angle) {
    constexpr std::uint32_t frame_clock = 0x51df54u;
    auto &g = p.g;
    auto &x = p.x;
    auto &flags = p.flags;
    g[edx] = p.read(0xc8845u,p.constant(frame_clock));
    x[0] = _mm_set_ss(f(p.read(0xc884bu,p.field(edx,4u))));
    const auto motion = p.read(0xc8850u,p.field(esi,0x144u));
    x[0] = ordered_mul_ss(x[0],_mm_set_ss(f(motion)));
    p.native_site(0xc8858u);
    g[ebx] = g[ebp] + 0x168u;
    p.native_site(0xc885eu);
    g[ecx] = g[esp] + 0x28u;
    p.native_site(0xc8862u);
    g[edx] = g[ebx];
    p.store(0xc8864u,p.stack(0x24u),p.scalar_bits(0));
    p.push(0xc886au,p.constant(0xc886fu));
    angle(s);
    const auto divisor = p.read(0xc886fu,p.stack(0x24u));
    x[0] = divide(x[0],_mm_set_ss(f(divisor)));
    p.native_site(0xc8875u);
    x[1] = _mm_setzero_ps();
    p.native_site(0xc8878u);
    flags.scalar_compare(x[1],x[0],false);
    p.store(0xc887bu,p.stack(0x14u),p.scalar_bits(0));
    p.native_site(0xc8881u);
    if(!flags.cf && !flags.zf) {
        p.store(0xc8883u,p.stack(0x14u),p.scalar_bits(1));
        p.native_site(0xc8889u);
        return;
    }
    x[2] = _mm_set_ss(f(p.read(0xc888bu,p.constant(one))));
    p.native_site(0xc8893u);
    flags.scalar_compare(x[0],x[2],false);
    p.native_site(0xc8896u);
    if(!flags.cf && !flags.zf) p.store(0xc8898u,p.stack(0x14u),p.scalar_bits(2));
}

template<class State, class Normalize>
void normalize_projected_vectors(State &s,native_pose_setup_c85a0::Frame<State> &p,Normalize &normalize) {
    constexpr std::uint32_t fallback_vector = 0x474d10u;
    auto &g = p.g;
    g[eax] = p.read(0xc889eu,p.stack(0x28u));
    g[ecx] = p.read(0xc88a2u,p.stack(0x2cu));
    g[edx] = p.read(0xc88a6u,p.stack(0x30u));
    p.store(0xc88aau,p.stack(0x40u),p.field(eax));
    g[eax] = p.read(0xc88aeu,p.field(ebx));
    p.store(0xc88b0u,p.stack(0x44u),p.field(ecx));
    g[ecx] = p.read(0xc88b4u,p.field(ebx,4u));
    p.store(0xc88b7u,p.stack(0x48u),p.field(edx));
    g[edx] = p.read(0xc88bbu,p.field(ebx,8u));
    g[ebx] = p.read(0xc88beu,p.constant(fallback_vector));
    p.store(0xc88c4u,p.stack(0x38u),p.field(ecx));
    p.store(0xc88c8u,p.stack(0x34u),p.field(eax));
    p.store(0xc88ccu,p.stack(0x3cu),p.field(edx));
    p.native_site(0xc88d0u);
    g[ecx] = g[ebx];
    p.native_site(0xc88d2u);
    g[eax] = g[esp] + 0x40u;
    p.store(0xc88d6u,p.stack(0x48u),p.scalar_bits(1));
    p.store(0xc88dcu,p.stack(0x3cu),p.scalar_bits(1));
    p.push(0xc88e2u,p.constant(0xc88e7u));
    normalize(s);
    p.native_site(0xc88e7u);
    g[ecx] = g[ebx];
    p.native_site(0xc88e9u);
    g[eax] = g[esp] + 0x34u;
    p.push(0xc88edu,p.constant(0xc88f2u));
    normalize(s);
}

template<class State, class Angle>
void second_angle(State &s,native_pose_setup_c85a0::Frame<State> &p,Angle &angle) {
    auto &g = p.g;
    auto &x = p.x;
    auto &flags = p.flags;
    p.native_site(0xc88f2u);
    g[ecx] = g[eax];
    p.native_site(0xc88f4u);
    g[edx] = g[esp] + 0x40u;
    p.push(0xc88f8u,p.constant(0xc88fdu));
    angle(s);
    const auto divisor = p.read(0xc88fdu,p.stack(0x24u));
    x[0] = divide(x[0],_mm_set_ss(f(divisor)));
    p.native_site(0xc8903u);
    x[1] = _mm_setzero_ps();
    p.native_site(0xc8906u);
    flags.scalar_compare(x[1],x[0],false);
    p.store(0xc8909u,p.stack(0x18u),p.scalar_bits(0));
    p.native_site(0xc890fu);
    if(!flags.cf && !flags.zf) {
        p.store(0xc8911u,p.stack(0x18u),p.scalar_bits(1));
        p.native_site(0xc8917u);
        return;
    }
    const auto one_bits = p.read(0xc8919u,p.constant(one));
    flags.scalar_compare(x[0],_mm_set_ss(f(one_bits)),false);
    p.native_site(0xc8920u);
    if(flags.cf || flags.zf) return;
    x[0] = _mm_set_ss(f(p.read(0xc8922u,p.constant(one))));
    p.store(0xc892au,p.stack(0x18u),p.scalar_bits(0));
}

template<class State, class Angle, class Normalize>
bool run(State &s,Angle angle,Normalize normalize) {
    if(!native_pose_setup_c85a0::selected(s) || reference()) return false;
    native_pose_setup_c85a0::Frame<State> p{s};
    if(writes_zero_angles(p)) return true;
    first_angle(s,p,angle);
    normalize_projected_vectors(s,p,normalize);
    second_angle(s,p,angle);
    return true;
}

} // namespace halo2::native_pose_normalization_c8827
