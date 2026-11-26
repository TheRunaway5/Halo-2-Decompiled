#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_plane_compare_detail {
enum class Continuation { original, read_end, split, resume };

inline bool reference_plane_compare() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_PLANE_COMPARE");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_provider_handoff() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_PLANE_COMPARE_PROVIDER");
        return value && *value;
    }();
    return reference;
}

template<class State,class Spill>
Continuation plane_compare(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_plane_compare() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || s.fp.count<2u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;
    const auto publish=[&](std::uint32_t pc) {
        if(cursor)*cursor->position=pc;
    };
    auto &g=s.gp;

    using namespace physics_step_detail;
    publish(0x30ade0u);
    const auto end=spill(s.fp.st(0),s.fp.status);
    const auto scratch=g[4]+0x18u;
    const bool direct=s.memory.try_callback_free_write(scratch,std::as_bytes(std::span{&end,1}));
    if(!direct)
        w(s.memory,s.observe,0x30ade0u,scratch,4,end);
    s.fp.pop();

    if(!direct) {
        // The real store, including all guards and observers, has retired.
        // They can reenter the runtime, so reopen admission before eliding
        // the original comparison reads or their instruction callbacks.
        if(reference_provider_handoff() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
           s.observe || s.active_mmx || !s.fp.count || s.fp.count==8u)
            return Continuation::read_end;
        const auto *current=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
        if(s.services.instruction ? (!current || current!=cursor) : s.instruction_cursor!=nullptr)
            return Continuation::read_end;
        std::uint16_t after_control;
        asm volatile("fnstcw %0":"=m"(after_control));
        if(after_control!=control)
            return Continuation::read_end;
    }

    // A store callback may have changed the active frame or either operand.
    // Borrow the ordinary range again, after the write has retired.
    const auto operands_address=g[4]+0x18u;
    const auto *page=s.memory.direct_page(operands_address,8u);
    const std::byte *operands=nullptr;
    if(page)
        operands=page->bytes+(operands_address&4095u);
    else {
        if(reference_provider_handoff())
            return Continuation::read_end;
        const auto view=s.memory.try_read_view(operands_address,8u);
        if(!view)
            return Continuation::read_end;
        operands=view->data();
    }
    std::uint32_t end_word,limit;
    std::memcpy(&end_word,operands,4u);
    std::memcpy(&limit,operands+4u,4u);
    // The unconditional comparison retires its scratch FLD immediately.
    s.fp.status=physics_solver_detail::compare(ext(f(end_word)),ext(f(limit)));
    s.set(0,s.fp.word(),2,0);
    s.flags.logic((s.reg(0,1,8))&5u,1);
    if(s.flags.pf) {
        publish(0x30adf1u);
        return Continuation::split;
    }

    std::memcpy(&limit,operands+4u,4u);
    s.fp.comparison(ext(f(limit)),0);
    s.set(0,s.fp.word(),2,0);
    s.flags.logic((s.reg(0,1,8))&5u,1);
    if(s.flags.pf) {
        publish(0x30adfcu);
        return Continuation::split;
    }
    g[6]=s.flags.plus(g[6],g[1],4);
    s.fp.st(0)=s.fp.st(0);
    s.fp.status&=~0x200u;
    s.fp.pop();
    publish(0x30ae00u);
    return Continuation::resume;
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_plane_compare_detail
