#pragma once

#include "physics/runtime/native_tu15_pose_first_axis_c8704.hpp"

namespace halo2::native_pose_second_axis_c89e3 {

using namespace native_object_tick_detail::boundary;
using native_pose_first_axis_c8704::subtract;
using native_tu15_leaves::ordered_mul_ss;
using physics_step_detail::f;

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_POSE_SECOND_AXIS_C89E3");
        return value && *value;
    }();
    return enabled;
}

template<class State>
bool uses_authored_basis(native_pose_setup_c85a0::Frame<State> &p) {
    p.g[ecx] = p.read(0xc89e3u,p.stack(0x20u));
    p.s.set(edx,p.read(0xc89e7u,p.field(ecx),1),1,0);
    p.native_site(0xc89e9u);
    p.s.set(edx,p.flags.right(p.g[edx] & 0xffu,6u,1,false),1,0);
    p.native_site(0xc89ecu);
    p.flags.logic(p.g[edx] & 1u,1);
    p.native_site(0xc89efu);
    return !p.flags.zf;
}

template<class State, class Basis>
void load_authored_basis(State &s,native_pose_setup_c85a0::Frame<State> &p,Basis &basis) {
    auto &g = p.g;
    auto &x = p.x;
    x[0] = _mm_set_ss(f(p.read(0xc89f5u,p.constant(0x46a0d0u))));
    g[eax] = p.read(0xc89fdu,p.stack(0x90u));
    p.native_site(0xc8a04u);
    g[edi] = g[esp] + 0x74u;
    p.native_site(0xc8a08u);
    g[esi] = g[esp] + 0x5cu;
    p.store(0xc8a0cu,p.stack(0x58u),p.scalar_bits(0));
    p.push(0xc8a12u,p.constant(0xc8a17u));
    basis(s);
    x[4] = _mm_set_ss(f(p.read(0xc8a17u,p.stack(0x7cu))));
    x[5] = _mm_set_ss(f(p.read(0xc8a1du,p.stack(0x78u))));
    x[7] = _mm_set_ss(f(p.read(0xc8a23u,p.stack(0x74u))));
    p.native_site(0xc8a29u);
    g[eax] = g[ebp] + 0x168u;
    g[ecx] = p.read(0xc8a2fu,p.field(eax));
    g[edx] = p.read(0xc8a31u,p.field(eax,4u));
    g[eax] = p.read(0xc8a34u,p.field(eax,8u));
    p.store(0xc8a37u,p.stack(0x64u),p.field(eax));
    g[eax] = p.read(0xc8a3bu,p.constant(0x474ce8u));
    x[1] = _mm_set_ss(f(p.read(0xc8a40u,p.stack(0x64u))));
    p.store(0xc8a46u,p.stack(0x5cu),p.field(ecx));
    g[ecx] = p.read(0xc8a4au,p.field(eax));
    x[6] = _mm_set_ss(f(p.read(0xc8a4cu,p.stack(0x5cu))));
    p.store(0xc8a52u,p.stack(0x60u),p.field(edx));
    g[edx] = p.read(0xc8a56u,p.field(eax,4u));
    g[eax] = p.read(0xc8a59u,p.field(eax,8u));
    x[2] = _mm_set_ss(f(p.read(0xc8a5cu,p.stack(0x60u))));
}

template<class State>
void prepare_projection(native_pose_setup_c85a0::Frame<State> &p) {
    auto &g = p.g;
    auto &x = p.x;
    p.native_site(0xc8a62u);
    x[0] = x[4];
    p.native_site(0xc8a65u);
    x[0] = ordered_mul_ss(x[0],x[2]);
    p.native_site(0xc8a69u);
    x[3] = x[1];
    p.native_site(0xc8a6cu);
    x[3] = ordered_mul_ss(x[3],x[5]);
    p.native_site(0xc8a70u);
    x[3] = subtract(x[3],x[0]);
    p.store(0xc8a74u,p.stack(0x80u),p.field(ecx));
    g[ecx] = p.read(0xc8a7bu,p.stack(0x24u));
    p.store(0xc8a7fu,p.stack(0x88u),p.field(eax));
    g[eax] = p.read(0xc8a86u,p.stack(0x20u));
    p.push(0xc8a8au,p.field(ecx));
    p.native_site(0xc8a8bu);
    x[0] = x[1];
    p.native_site(0xc8a8eu);
    x[0] = ordered_mul_ss(x[0],x[7]);
    p.store(0xc8a92u,p.stack(0x88u),p.field(edx));
    g[edx] = p.read(0xc8a99u,p.stack(0x20u));
    p.native_site(0xc8a9du);
    x[4] = ordered_mul_ss(x[4],x[6]);
    p.native_site(0xc8aa1u);
    x[4] = subtract(x[4],x[0]);
    p.push(0xc8aa5u,p.field(edx));
    g[edx] = p.read(0xc8aa6u,p.constant(0x51df54u));
    p.native_site(0xc8aacu);
    g[eax] = p.flags.plus(g[eax],0x14u,4);
    p.native_site(0xc8aafu);
    x[5] = ordered_mul_ss(x[5],x[6]);
    p.push(0xc8ab3u,p.field(eax));
    g[eax] = p.read(0xc8ab4u,p.field(edx,4u));
    p.native_site(0xc8ab7u);
    x[0] = x[2];
    p.native_site(0xc8abau);
    x[0] = ordered_mul_ss(x[0],x[7]);
    p.native_site(0xc8abeu);
    x[0] = subtract(x[0],x[5]);
    p.native_site(0xc8ac2u);
    x[7] = x[4];
    p.native_site(0xc8ac5u);
    g[ecx] = g[ebp] + 0x198u;
}

template<class State, class Transform>
void project_authored_axis(State &s,native_pose_setup_c85a0::Frame<State> &p,Transform &transform) {
    auto &g = p.g;
    auto &x = p.x;
    p.native_site(0xc8acbu);
    x[5] = x[0];
    p.native_site(0xc8aceu);
    x[7] = ordered_mul_ss(x[7],x[1]);
    p.store(0xc8ad2u,p.stack(0x78u),p.scalar_bits(4));
    p.native_site(0xc8ad8u);
    x[5] = ordered_mul_ss(x[5],x[2]);
    p.push(0xc8adcu,p.field(ecx));
    p.store(0xc8addu,p.stack(0x80u),p.scalar_bits(0));
    p.native_site(0xc8ae6u);
    x[1] = ordered_mul_ss(x[1],x[3]);
    p.native_site(0xc8aeau);
    x[0] = ordered_mul_ss(x[0],x[6]);
    p.native_site(0xc8aeeu);
    x[4] = ordered_mul_ss(x[4],x[6]);
    p.native_site(0xc8af2u);
    x[2] = ordered_mul_ss(x[2],x[3]);
    p.push(0xc8af6u,p.field(eax));
    p.native_site(0xc8af7u);
    x[5] = subtract(x[5],x[7]);
    p.native_site(0xc8afbu);
    x[1] = subtract(x[1],x[0]);
    p.native_site(0xc8affu);
    x[4] = subtract(x[4],x[2]);
    p.native_site(0xc8b03u);
    g[eax] = g[ebp] + 0x180u;
    p.native_site(0xc8b09u);
    g[edi] = g[ebp] + 0x18cu;
    p.native_site(0xc8b0fu);
    g[esi] = g[esp] + 0x6cu;
    p.store(0xc8b13u,p.stack(0x7cu),p.scalar_bits(3));
    p.store(0xc8b19u,p.stack(0x88u),p.scalar_bits(5));
    p.store(0xc8b22u,p.stack(0x8cu),p.scalar_bits(1));
    p.store(0xc8b2bu,p.stack(0x90u),p.scalar_bits(4));
    p.push(0xc8b34u,p.constant(0xc8b39u));
    transform(s);
    p.native_site(0xc8b39u);
}

template<class State, class Rotate>
void blend_default_axis(State &s,native_pose_setup_c85a0::Frame<State> &p,Rotate &rotate) {
    auto &g = p.g;
    g[ecx] = p.read(0xc8b3bu,p.stack(0x24u));
    g[edx] = p.read(0xc8b3fu,p.stack(0x1cu));
    g[eax] = p.read(0xc8b43u,p.constant(0x51df54u));
    p.push(0xc8b48u,p.field(ecx));
    g[ecx] = p.read(0xc8b49u,p.field(eax,4u));
    p.push(0xc8b4cu,p.field(edx));
    p.native_site(0xc8b4du);
    g[ebx] = g[ebp] + 0x198u;
    p.push(0xc8b53u,p.field(ecx));
    p.native_site(0xc8b54u);
    g[edi] = g[ebp] + 0x180u;
    p.native_site(0xc8b5au);
    g[esi] = g[ebp] + 0x18cu;
    p.push(0xc8b60u,p.constant(0xc8b65u));
    rotate(s);
}

template<class State, class Basis, class Transform, class Rotate>
bool run(State &s,Basis basis,Transform transform,Rotate rotate) {
    if(!native_pose_setup_c85a0::selected(s) || reference()) return false;
    native_pose_setup_c85a0::Frame<State> p{s};
    if(uses_authored_basis(p)) {
        load_authored_basis(s,p,basis);
        prepare_projection(p);
        project_authored_axis(s,p,transform);
    } else {
        blend_default_axis(s,p,rotate);
    }
    return true;
}

} // namespace halo2::native_pose_second_axis_c89e3
