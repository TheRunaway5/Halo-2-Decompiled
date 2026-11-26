#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_slab_interval_detail {
enum class Continuation { original, reject, accept };

inline bool reference_slab_interval() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SLAB_INTERVAL");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

template<bool Ascending,class State>
Continuation slab_interval(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_slab_interval() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || s.fp.count<2u || s.fp.count>=8u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;

    // A complete ordinary span establishes both read positions before touching
    // either. There is no guest store or callback between the two comparisons.
    const auto scratch=s.gp[4]+0x18u;
    const auto *page=s.memory.direct_page(scratch,16u);
    if(!page)return Continuation::original;
    const auto *values=page->bytes+(scratch&4095u);
    const auto compare=[&](unsigned offset,unsigned lane,unsigned mask) {
        std::uint32_t value;
        std::memcpy(&value,values+offset,4u);
        // Each original FLD is consumed by its immediately following FCOMP.
        using namespace physics_step_detail;
        s.fp.status=physics_solver_detail::compare(ext(f(value)),s.fp.st(lane));
        s.set(0,s.fp.word(),2,0);
        s.flags.logic(s.reg(0,1,8)&mask,1);
    };
    compare(Ascending?12u:0u,1u,5u);
    if(!s.flags.pf) {
        if(cursor)*cursor->position=Ascending?0x30b248u:0x30b222u;
        return Continuation::reject;
    }
    compare(Ascending?0u:12u,0u,0x41u);
    if(s.flags.zf) {
        if(cursor)*cursor->position=Ascending?0x30b259u:0x30b233u;
        return Continuation::reject;
    }
    if constexpr(Ascending) s.gp[1]=1u;
    else s.gp[1]=s.flags.logic(0u,4);
    if(cursor)*cursor->position=Ascending?0x30b25fu:0x30b23bu;
    return Continuation::accept;
#else
    (void)s;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_slab_interval_detail
