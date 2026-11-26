#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"

namespace halo2::native_mopp_ray_terminal_key_detail {

inline bool reference_terminal_key() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_TERMINAL_KEY");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

// Return the first unretired instruction. Encoded bytes are borrowed singly
// so a second-page fault sees the intermediate registers and shift flags.
template<unsigned Count,class State>
std::uint32_t terminal_key(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference_terminal_key() || s.image!=SourcePhysicsContactImage::TitleUpdate15 || s.observe)
        return 0;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return 0;
    auto &g=s.gp;
    const auto load=[&](unsigned index,unsigned reg) {
        const auto address=g[6]+index;
        std::byte byte;
        if(!native_raw_x87_detail::direct_read(s.memory,address,std::span{&byte,1}))return false;
        g[reg]=std::to_integer<unsigned>(byte);
        return true;
    };
    const auto combine=[&](unsigned high,unsigned low,unsigned shift) {
        g[high]=s.flags.shift(g[high],shift,4);
        g[high]=s.flags.plus(g[high],g[low],4);
    };
    if constexpr(Count==0u) {
        g[7]=g[0]-0x30u;
    } else if constexpr(Count==1u) {
        if(!load(1,7))return 0;
    } else if constexpr(Count==2u) {
        if(!load(1,7))return 0;
        if(!load(2,2))return 0x30b5adu;
        combine(7,2,8);
    } else if constexpr(Count==3u) {
        if(!load(1,7))return 0;
        if(!load(2,0))return 0x30b5bcu;
        if(!load(3,1))return 0x30b5c0u;
        combine(7,0,8);
        combine(7,1,8);
    } else {
        static_assert(Count==4u);
        if(!load(1,0))return 0;
        if(!load(2,2))return 0x30b5d4u;
        if(!load(3,1))return 0x30b5d8u;
        combine(0,2,8);
        if(!load(4,2))return 0x30b5e1u;
        g[0]=s.flags.shift(g[0],16,4);
        g[1]=s.flags.shift(g[1],8,4);
        g[2]=s.flags.plus(g[2],g[0],4);
        g[7]=g[2]+g[1];
    }
    if(cursor)*cursor->position=std::array{0x30b5f2u,0x30b5a7u,0x30b5b6u,0x30b5ceu,0x30b5f0u}[Count];
    return 0x30b5f5u;
#else
    (void)s;
    return 0;
#endif
}

} // namespace halo2::native_mopp_ray_terminal_key_detail
