#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_plane_split_detail {
enum class Continuation { original, read_start, geometry, resume };

inline bool reference_plane_split() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_PLANE_SPLIT");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

template<class State>
Continuation plane_split(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_plane_split() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.active_mmx || s.fp.count==0u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;

    // Topology admission is read-only. All three slots belong to the same
    // ordinary page, so none can invoke a provider or a store observer.
    const auto address=s.gp[4]+0x14u;
    const auto *page=s.memory.direct_page(address,16u);
    if(!page)return Continuation::original;
    const auto *stack=page->bytes+(address&4095u);
    const auto word=[&](unsigned offset) {
        std::uint32_t value;
        std::memcpy(&value,stack+offset,4u);
        return value;
    };
    using namespace physics_step_detail;
    auto &g=s.gp;
    const auto end=word(12u);
    s.fp.comparison(ext(f(end)),0);
    g[2]=word(0u);
    g[6]=s.flags.plus(g[6],g[2],4);
    const auto test_status=[&] {
        s.set(0,s.fp.word(),2,0);
        s.flags.logic(s.reg(0,1,8)&0x41u,1);
    };
    test_status();
    if(cursor)*cursor->position=0x30ae64u;
    if(!s.flags.zf)return Continuation::geometry;

    // Retain the original stack fault if the optional FLD cannot allocate
    // a temporary lane. Its comparison would otherwise pop that lane.
    if(s.fp.count>=8u)return Continuation::read_start;
    s.fp.status=physics_solver_detail::compare(ext(f(word(4u))),ext(f(word(12u))));
    test_status();
    if(cursor)*cursor->position=0x30ae73u;
    return s.flags.zf?Continuation::resume:Continuation::geometry;
#else
    (void)s;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_plane_split_detail
