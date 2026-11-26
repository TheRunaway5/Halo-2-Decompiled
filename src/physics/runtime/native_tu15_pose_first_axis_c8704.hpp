#pragma once

#include "physics/runtime/native_tu15_pose_setup_c85a0.hpp"

namespace halo2::native_pose_first_axis_c8704 {

using namespace native_object_tick_detail::boundary;
using physics_step_detail::f;
using native_tu15_leaves::ordered_mul_ss;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_POSE_FIRST_AXIS_C8704");
        return value && *value;
    }();
    return enabled;
}

inline __m128 subtract(__m128 left, __m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subss %1,%0" : "+x"(left) : "x"(right));
    return left;
#else
    return _mm_sub_ss(left,right);
#endif
}

template<class State>
bool uses_authored_basis(native_pose_setup_c85a0::Frame<State> &p) {
    auto &s = p.s;
    s.set(edx,p.read(0xc8704u,p.field(edi),1),1,0);
    p.native_site(0xc8706u);
    s.set(edx,p.flags.right(p.g[edx] & 0xffu,5u,1,false),1,0);
    p.native_site(0xc8709u);
    p.flags.logic(p.g[edx] & 1u,1);
    p.native_site(0xc870cu);
    return !p.flags.zf;
}

template<class State, class Basis>
void load_authored_basis(State &s,native_pose_setup_c85a0::Frame<State> &p,Basis &basis) {
    auto &g = p.g;
    auto &x = p.x;
    x[0] = _mm_set_ss(f(p.read(0xc8712u,p.constant(0x46a0d0u))));
    p.native_site(0xc871au);
    g[edi] = g[esp] + 0x74u;
    p.native_site(0xc871eu);
    g[esi] = g[esp] + 0x5cu;
    p.native_site(0xc8722u);
    g[eax] = g[ecx];
    p.store(0xc8724u,p.stack(0x58u),p.scalar_bits(0));
    p.push(0xc872au,p.constant(0xc872fu));
    basis(s);
    g[eax] = p.read(0xc872fu,p.constant(0x474ce8u));
    g[ecx] = p.read(0xc8734u,p.field(eax));
    g[edx] = p.read(0xc8736u,p.field(eax,4u));
    g[eax] = p.read(0xc8739u,p.field(eax,8u));
    x[2] = _mm_set_ss(f(p.read(0xc873cu,p.stack(0x64u))));
    x[0] = _mm_set_ss(f(p.read(0xc8742u,p.stack(0x7cu))));
    x[3] = _mm_set_ss(f(p.read(0xc8748u,p.stack(0x78u))));
    x[1] = _mm_set_ss(f(p.read(0xc874eu,p.stack(0x60u))));
    p.store(0xc8754u,p.stack(0x80u),p.field(ecx));
}

template<class State, class Transform>
void project_authored_axis(State &s,native_pose_setup_c85a0::Frame<State> &p,Transform &transform) {
    auto &g = p.g;
    auto &x = p.x;
    g[ecx] = p.read(0xc875bu,p.stack(0x18u));
    p.store(0xc875fu,p.stack(0x88u),p.field(eax));
    g[eax] = p.read(0xc8766u,p.stack(0x20u));
    p.push(0xc876au,p.field(ecx));
    p.store(0xc876bu,p.stack(0x88u),p.field(edx));
    g[edx] = p.read(0xc8772u,p.stack(0x18u));
    p.push(0xc8776u,p.field(edx));
    g[edx] = p.read(0xc8777u,p.constant(0x51df54u));
    p.native_site(0xc877du);
    g[eax] = p.flags.plus(g[eax],4u,4);
    p.native_site(0xc8780u);
    x[5] = x[0];
    p.native_site(0xc8783u);
    x[5] = ordered_mul_ss(x[5],x[1]);
    p.push(0xc8787u,p.field(eax));
    g[eax] = p.read(0xc8788u,p.field(edx,4u));
    p.native_site(0xc878bu);
    x[4] = x[2];
    p.native_site(0xc878eu);
    x[4] = ordered_mul_ss(x[4],x[3]);
    p.native_site(0xc8792u);
    x[4] = subtract(x[4],x[5]);
    x[5] = _mm_set_ss(f(p.read(0xc8796u,p.stack(0x80u))));
    p.native_site(0xc879fu);
    g[ecx] = g[ebp] + 0x174u;
    p.store(0xc87a5u,p.stack(0x74u),p.scalar_bits(4));
    x[4] = _mm_set_ss(f(p.read(0xc87abu,p.stack(0x68u))));
    p.push(0xc87b1u,p.field(ecx));
    p.native_site(0xc87b2u);
    x[0] = ordered_mul_ss(x[0],x[4]);
    p.native_site(0xc87b6u);
    x[2] = ordered_mul_ss(x[2],x[5]);
    p.native_site(0xc87bau);
    x[1] = ordered_mul_ss(x[1],x[5]);
    p.native_site(0xc87beu);
    x[3] = ordered_mul_ss(x[3],x[4]);
    p.push(0xc87c2u,p.field(eax));
    p.native_site(0xc87c3u);
    x[0] = subtract(x[0],x[2]);
    p.native_site(0xc87c7u);
    x[1] = subtract(x[1],x[3]);
    p.native_site(0xc87cbu);
    g[eax] = g[ebp] + 0x15cu;
    p.native_site(0xc87d1u);
    g[edi] = g[ebx];
    p.native_site(0xc87d3u);
    g[esi] = g[esp] + 0x6cu;
    p.store(0xc87d7u,p.stack(0x80u),p.scalar_bits(0));
    p.store(0xc87e0u,p.stack(0x84u),p.scalar_bits(1));
    p.push(0xc87e9u,p.constant(0xc87eeu));
    transform(s);
    p.native_site(0xc87eeu);
}

template<class State, class Rotate>
void blend_default_axis(State &s,native_pose_setup_c85a0::Frame<State> &p,Rotate &rotate) {
    auto &g = p.g;
    g[ecx] = p.read(0xc87f0u,p.stack(0x18u));
    g[edx] = p.read(0xc87f4u,p.stack(0x14u));
    g[eax] = p.read(0xc87f8u,p.constant(0x51df54u));
    p.push(0xc87fdu,p.field(ecx));
    g[ecx] = p.read(0xc87feu,p.field(eax,4u));
    p.push(0xc8801u,p.field(edx));
    p.native_site(0xc8802u);
    g[ebx] = g[ebp] + 0x174u;
    p.push(0xc8808u,p.field(ecx));
    p.native_site(0xc8809u);
    g[edi] = g[ebp] + 0x15cu;
    p.native_site(0xc880fu);
    g[esi] = g[ebp] + 0x168u;
    p.push(0xc8815u,p.constant(0xc881au));
    rotate(s);
}

template<class State, class Basis, class Transform, class Rotate>
bool run(State &s,Basis basis,Transform transform,Rotate rotate) {
    if(!native_pose_setup_c85a0::selected(s) || reference()) return false;
    native_pose_setup_c85a0::Frame<State> p{s};
    if(uses_authored_basis(p)) {
        load_authored_basis(s,p,basis);
        project_authored_axis(s,p,transform);
    } else {
        blend_default_axis(s,p,rotate);
    }
    p.native_site(0xc881au);
    p.x[2] = _mm_setzero_ps();
    p.g[edi] = p.read(0xc881du,p.constant(0x474d04u));
    p.g[esi] = p.read(0xc8823u,p.stack(0x24u));
    return true;
}

} // namespace halo2::native_pose_first_axis_c8704
