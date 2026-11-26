#pragma once

#include "physics/runtime/native_tu15_pose_setup_c85a0.hpp"

namespace halo2::native_pose_angle_c8930 {

using namespace native_object_tick_detail::boundary;
using physics_step_detail::f;
using physics_step_detail::r;
using native_tu15_leaves::ordered_mul_ss;

enum class Resume { retained, authored_axis, finish };

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_POSE_ANGLE_C8930");
        return value && *value;
    }();
    return enabled;
}

template<class State> void load_single(State &s, std::uint32_t bits) {
    s.fp.push(native_live_support::x87::load_single(bits));
}

template<class State> void scale_angle(State &s, std::uint32_t bits) {
    auto &angle = s.fp.st(0);
    long double scaled;
    std::uint16_t status;
    asm volatile("fldt %2\n\tfmuls %3\n\tfnstsw %1\n\tfstpt %0"
                 : "=m"(scaled), "=m"(status) : "m"(angle), "m"(bits) : "st");
    angle = scaled;
    s.fp.status = (s.fp.status & ~0x200u) | (status & 0x200u);
}

template<class State, class Convert> void quantize_angles(State &s, native_pose_setup_c85a0::Frame<State> &p, Convert &convert) {
    const auto angle_scale = p.constant(0x46a148u);
    load_single(s,p.read(0xc8930u,p.stack(0x14u)));
    scale_angle(s,p.read(0xc8934u,angle_scale));
    p.push(0xc893au,p.constant(0xc893fu));
    convert(s);
    load_single(s,p.read(0xc893fu,p.stack(0x18u)));
    scale_angle(s,p.read(0xc8943u,angle_scale));
    p.store(0xc8949u,p.field(ebp,0x243u),p.field(eax),1);
    p.push(0xc894fu,p.constant(0xc8954u));
    convert(s);
}

template<class State> bool second_axis_can_reset(native_pose_setup_c85a0::Frame<State> &p) {
    auto &x = p.x;
    auto &flags = p.flags;
    x[2] = _mm_set_ss(f(p.read(0xc8954u,p.stack(0x1cu))));
    p.store(0xc895au,p.field(ebp,0x1f8u),p.field(eax),1);
    x[0] = _mm_set_ss(f(p.read(0xc8960u,p.field(esi,0x150u))));
    x[1] = _mm_set_ss(f(p.read(0xc8968u,p.field(esi,0x154u))));
    p.native_site(0xc8970u);
    x[0] = ordered_mul_ss(x[0],x[2]);
    p.native_site(0xc8974u);
    x[1] = ordered_mul_ss(x[1],x[2]);
    p.native_site(0xc8978u);
    x[2] = _mm_setzero_ps();
    p.native_site(0xc897bu);
    flags.scalar_compare(x[0],x[2],true);
    p.native_site(0xc897eu);
    p.s.set(eax,flags.bits() | 2u,1,8);
    p.native_site(0xc897fu);
    flags.logic((p.g[eax] >> 8u) & 0x44u,1);
    p.store(0xc8982u,p.stack(0x1cu),p.scalar_bits(0));
    p.store(0xc8988u,p.stack(0x24u),p.scalar_bits(1));
    p.native_site(0xc898eu);
    if(flags.pf) return false;
    p.native_site(0xc8990u);
    flags.scalar_compare(x[1],x[2],true);
    p.native_site(0xc8993u);
    p.s.set(eax,flags.bits() | 2u,1,8);
    p.native_site(0xc8994u);
    flags.logic((p.g[eax] >> 8u) & 0x44u,1);
    p.native_site(0xc8997u);
    return !flags.pf;
}

template<class State, class Update> void reset_second_axis(State &s, native_pose_setup_c85a0::Frame<State> &p, Update &update) {
    auto &g = p.g;
    p.native_site(0xc8999u);
    g[eax] = g[ebp] + 0x180u;
    p.native_site(0xc899fu);
    g[ecx] = g[eax];
    g[esi] = p.read(0xc89a1u,p.field(ecx));
    p.native_site(0xc89a3u);
    g[eax] = g[ebp] + 0x18cu;
    p.native_site(0xc89a9u);
    g[edx] = g[eax];
    p.store(0xc89abu,p.field(edx),p.field(esi));
    g[esi] = p.read(0xc89adu,p.field(ecx,4u));
    g[ecx] = p.read(0xc89b0u,p.field(ecx,8u));
    p.push(0xc89b3u,p.constant(1u));
    p.store(0xc89b5u,p.field(edx,4u),p.field(esi));
    p.push(0xc89b8u,p.field(eax));
    g[eax] = p.read(0xc89b9u,p.stack(0x98u));
    p.store(0xc89c0u,p.field(edx,8u),p.field(ecx));
    p.push(0xc89c3u,p.constant(0xc89c8u));
    update(s);
    g[eax] = p.read(0xc89c8u,p.field(edi));
    p.native_site(0xc89cau);
    g[edx] = g[ebp] + 0x198u;
    p.store(0xc89d0u,p.field(edx),p.field(eax));
    g[ecx] = p.read(0xc89d2u,p.field(edi,4u));
    p.store(0xc89d5u,p.field(edx,4u),p.field(ecx));
    g[eax] = p.read(0xc89d8u,p.field(edi,8u));
    p.store(0xc89dbu,p.field(edx,8u),p.field(eax));
    p.native_site(0xc89deu);
}

template<class State, class Convert, class Update> Resume run(State &s, Convert convert, Update update) {
    // FLD and the converter's duplicate require two available x87 entries.
    if(!native_pose_setup_c85a0::selected(s) || reference() || s.active_mmx || s.fp.count > 6u)
        return Resume::retained;
    native_pose_setup_c85a0::Frame<State> p{s};
    quantize_angles(s,p,convert);
    if(!second_axis_can_reset(p)) return Resume::authored_axis;
    reset_second_axis(s,p,update);
    return Resume::finish;
}

} // namespace halo2::native_pose_angle_c8930
