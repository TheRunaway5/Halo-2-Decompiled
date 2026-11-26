#pragma once

#include "physics/runtime/native_tu15_pose_angle_c8930.hpp"

namespace halo2::native_pose_completion_c8b65 {

using namespace native_object_tick_detail::boundary;

struct Component {
    std::uint32_t stack_offset, pose_offset;
    std::uint32_t load, subtract, absolute, threshold, compare, release, branch;
};

inline constexpr std::array components{
    Component{0x28u,0x168u,0xc8b65u,0xc8b6au,0xc8b70u,0xc8b72u,0xc8b78u,0xc8b7au,0xc8b7cu},
    Component{0x28u,0x16cu,0xc8b7eu,0xc8b82u,0xc8b88u,0xc8b8au,0xc8b90u,0xc8b92u,0xc8b94u},
    Component{0x2cu,0x170u,0xc8b96u,0xc8b9au,0xc8ba0u,0xc8ba2u,0xc8ba8u,0xc8baau,0xc8bacu},
    Component{0x48u,0x18cu,0xc8baeu,0xc8bb2u,0xc8bb8u,0xc8bbau,0xc8bc0u,0xc8bc2u,0xc8bc4u},
    Component{0x4cu,0x190u,0xc8bc6u,0xc8bcau,0xc8bd0u,0xc8bd2u,0xc8bd8u,0xc8bdau,0xc8bdcu},
    Component{0x50u,0x194u,0xc8bdeu,0xc8be2u,0xc8be8u,0xc8beau,0xc8bf0u,0xc8bf2u,0xc8bf4u}
};

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_POSE_COMPLETION_C8B65");
        return value && *value;
    }();
    return enabled;
}

template<class State> void subtract_single(State &s, std::uint32_t bits) {
    auto &value = s.fp.st(0);
    long double difference;
    std::uint16_t status;
    asm volatile("fldt %2\n\tfsubs %3\n\tfnstsw %1\n\tfstpt %0"
                 : "=m"(difference), "=m"(status) : "m"(value), "m"(bits) : "st");
    value = difference;
    s.fp.status = (s.fp.status & ~0x200u) | (status & 0x200u);
}

template<class State> void take_absolute(State &s) {
    s.fp.st(0) = std::fabs(s.fp.st(0));
    s.fp.status &= ~0x200u;
}

template<class State> void compare_threshold(State &s) {
    s.compare_and_pop(s.fp.st(1));
    s.fp.status &= ~0x200u;
}

template<class State> void release_difference(State &s) {
    s.fp.status &= ~0x200u;
    s.fp.pop();
}

template<class State> bool compare_component(native_pose_setup_c85a0::Frame<State> &p, const Component &item) {
    auto &s = p.s;
    subtract_single(s,p.read(item.subtract,p.field(ebp,item.pose_offset)));
    p.native_site(item.absolute);
    take_absolute(s);
    native_pose_angle_c8930::load_single(s,p.read(item.threshold,p.constant(0x46a0ecu)));
    p.native_site(item.compare);
    compare_threshold(s);
    p.native_site(item.release);
    release_difference(s);
    p.native_site(item.branch);
    return p.flags.cf || p.flags.zf;
}

template<class State> bool compare_pose(native_pose_setup_c85a0::Frame<State> &p) {
    const auto &first = components.front();
    native_pose_angle_c8930::load_single(p.s,p.read(first.load,p.stack(first.stack_offset)));
    p.access(0xc8b69u);
    p.g[edi] = p.s.pop();
    if(compare_component(p,first)) return true;
    for(std::size_t i=1;i<components.size();++i) {
        const auto &item = components[i];
        native_pose_angle_c8930::load_single(p.s,p.read(item.load,p.stack(item.stack_offset)));
        if(compare_component(p,item)) return true;
    }
    return false;
}

template<class State, class Notify>
bool run(State &s, Notify notify) {
    if(!native_pose_setup_c85a0::selected(s) || reference() || s.active_mmx || s.fp.count > 6u)
        return false;
    native_pose_setup_c85a0::Frame<State> p{s};
    const bool changed = compare_pose(p);
    if(changed) {
        p.g[eax] = p.read(0xc8bf6u,p.stack(0x8cu));
        p.native_site(0xc8bfdu);
        s.set(ebx,1u,1,0);
        p.push(0xc8bffu,p.constant(0xc8c04u));
        notify(s);
        p.native_site(0xc8c04u);
        s.set(eax,p.g[ebx],1,0);
    } else {
        s.set(eax,p.read(0xc8c0fu,p.stack(0x0fu),1),1,0);
    }
    p.access(changed ? 0xc8c06u : 0xc8c13u);
    p.g[ebx] = s.pop();
    p.access(changed ? 0xc8c07u : 0xc8c14u);
    p.g[esi] = s.pop();
    p.access(changed ? 0xc8c08u : 0xc8c15u);
    p.g[ebp] = s.pop();
    p.native_site(changed ? 0xc8c09u : 0xc8c16u);
    p.g[esp] = p.flags.plus(p.g[esp],0x7cu,4);
    p.access(changed ? 0xc8c0cu : 0xc8c19u);
    s.pop();
    p.g[esp] += 4u;
    return true;
}

} // namespace halo2::native_pose_completion_c8b65
