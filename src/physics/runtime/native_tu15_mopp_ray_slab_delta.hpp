#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_slab_delta_detail {
enum class Continuation { original, read_end, ratio, skip_ratio };
enum class RatioExit { original, reload, branch, complete };

inline bool reference_slab_delta() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SLAB_DELTA");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_slab_delta_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SLAB_DELTA_PROVIDER");
        return value && *value;
    }();
    return reference;
}

inline bool reference_slab_ratio() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SLAB_RATIO");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
const std::byte *ordinary(const State &s,std::uint32_t address,std::size_t bytes) noexcept {
    const auto *page=s.memory.direct_page(address,bytes);
    return page?page->bytes+(address&4095u):nullptr;
}
template<class State>
const std::byte *ordinary_or_sparse(const State &s,std::uint32_t address,std::size_t bytes) noexcept {
    if(const auto *direct=ordinary(s,address,bytes))return direct;
    const auto view=s.memory.try_read_view(address,bytes);
    return view?view->data():nullptr;
}
inline long double extended(const std::byte *data) {
    float value;
    std::memcpy(&value,data,4u);
    return physics_step_detail::ext(value);
}
#endif

template<bool Second,class State,class Spill>
Continuation slab_delta(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_slab_delta() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || s.fp.count<(Second?2u:1u) || (!Second&&s.fp.count>=8u))
        return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;

    constexpr auto spill_pc=Second?0x30b2cfu:0x30b271u;
    constexpr auto branch_pc=Second?0x30b2e8u:0x30b28au;
    const auto scratch=s.gp[4]+0x14u;
    const auto *stack=ordinary(s,scratch,20u);
    if(!stack)return Continuation::original;
    const std::byte *rays=nullptr;
    if constexpr(!Second) {
        rays=ordinary(s,s.gp[7],32u);
        if(!rays)return Continuation::original;
    }

    // Admission inspects only topology. Read each operand in source order,
    // including the scratch value after its actual store.
    using namespace physics_step_detail;
    if constexpr(Second) {
        s.fp.st(0)=s.fp.st(0);
        s.fp.status&=~0x200u;
        s.fp.pop();
    }
    s.fp.push(extended(stack+0x10u));
    if constexpr(!Second) {
        std::memcpy(&s.xmm[1],rays,16u);
        std::memcpy(&s.xmm[2],rays+16u,16u);
    }
    s.fp.st(0)=sub(s.fp.st(0),s.fp.st(1));
    if(cursor)*cursor->position=spill_pc;
    const auto delta=spill(s.fp.st(0),s.fp.status);
    const bool direct=s.memory.try_callback_free_write(scratch,std::as_bytes(std::span{&delta,1}));
    if(!direct) {
        // Keep the x87 lane live through a real store callback or fault.
        w(s.memory,s.observe,spill_pc,scratch,4,delta);
    }
    s.fp.pop();

    if(!direct) {
        // Discard the pre-store view. The provider may change the frame,
        // its backing storage or the active plane's instruction services.
        if(reference_slab_delta_provider() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
           s.observe || s.active_mmx || !s.fp.count || s.fp.count>=8u)return Continuation::read_end;
        const auto *current=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
        if(current!=cursor || (s.services.instruction ? !current : s.instruction_cursor!=nullptr))
            return Continuation::read_end;
        std::uint16_t after_control;
        asm volatile("fnstcw %0":"=m"(after_control));
        if(after_control!=control)return Continuation::read_end;
        stack=ordinary_or_sparse(s,s.gp[4]+0x14u,8u);
        if(!stack)return Continuation::read_end;
    }

    // World and AI can publish epsilon in a short immutable region.
    const auto *epsilon=ordinary_or_sparse(s,0x46a0e8u,4u);
    if(!epsilon)return Continuation::read_end;

    s.fp.st(0)=sub(extended(stack+4u),s.fp.st(0));
    const auto test=mul(extended(stack),s.fp.st(0));
    // The product is consumed by FCOMP without an observable access to the
    // temporary lane. The ordinary epsilon load cannot invoke a provider.
    s.fp.status=physics_solver_detail::compare(test,extended(epsilon));
    s.set(0,s.fp.word(),2,0);
    s.flags.logic(s.reg(0,1,8)&5u,1);
    if(cursor)*cursor->position=branch_pc;
    return s.flags.pf?Continuation::skip_ratio:Continuation::ratio;
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}

template<bool Second,class State,class Spill>
RatioExit slab_ratio(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return RatioExit::original;
    if(reference_slab_ratio() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || !s.fp.count || s.fp.count>=8u ||
       (_mm_getcsr()&0x1f80u)!=0x1f80u)return RatioExit::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return RatioExit::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return RatioExit::original;
    const auto publish=[&](std::uint32_t pc) {
        if(cursor)*cursor->position=pc;
    };
    auto &g=s.gp;
    auto &x=s.xmm;
    const auto address=g[4]+0x14u;
    const auto *delta=ordinary_or_sparse(s,address,4u);
    if(!delta)return RatioExit::original;
    const std::byte *one=nullptr;
    if constexpr(!Second) {
        one=ordinary_or_sparse(s,0x41c500u,16u);
        if(!one)return RatioExit::original;
    }

    using namespace physics_step_detail;
    // No provider can run between these admitted reads and the ratio spill.
    const auto numerator=extended(delta);
    s.fp.push(numerator);
    if constexpr(Second)
        g[1]=s.flags.shift(g[1],4u,4);
    else
        std::memcpy(&x[4],one,16u);
    s.fp.st(0)=sub(s.fp.st(0),s.fp.st(1));
    if constexpr(!Second) {
        g[0]=s.flags.shift(g[1],4u,4);
        g[2]=g[7];
    }
    s.fp.st(0)=physics_solver_detail::divide(numerator,s.fp.st(0));
    if constexpr(!Second)
        g[2]=s.flags.minus(g[2],g[0],4);
    constexpr auto spill_pc=Second?0x30b2fbu:0x30b2a6u;
    publish(spill_pc);
    const auto word=spill(s.fp.st(0),s.fp.status);
    const bool direct=s.memory.try_callback_free_write(address,std::as_bytes(std::span{&word,1}));
    if(!direct)w(s.memory,s.observe,spill_pc,address,4u,word);
    s.fp.pop();
    if(!direct && (!native_mopp_ray_scale_tail_detail::provider_continuation(s,cursor) ||
        (_mm_getcsr()&0x1f80u)!=0x1f80u))return RatioExit::reload;

    // Reopen the stored scalar at the current ESP after its real provider.
    const auto *stored=ordinary_or_sparse(s,g[4]+0x14u,4u);
    if(!stored)return RatioExit::reload;
    if constexpr(Second) {
        one=ordinary_or_sparse(s,0x41c500u,16u);
        if(!one)return RatioExit::reload;
    }
    float factor;
    std::memcpy(&factor,stored,4u);
    x[0]=_mm_set1_ps(factor);
    using namespace native_tu15_leaves;
    if constexpr(Second) {
        x[3]=ordered_mul_ps(x[0],x[2]);
        std::memcpy(&x[2],one,16u);
        x[2]=_mm_sub_ps(x[2],x[0]);
        x[2]=ordered_add_ps(ordered_mul_ps(x[2],x[1]),x[3]);
        publish(0x30b31fu);
        vw(s.memory,s.observe,0x30b31fu,g[1]+g[7],x[2]);
        if(!native_mopp_ray_scale_tail_detail::provider_continuation(s,cursor))
            return RatioExit::branch;
        publish(0x30b323u);
    } else {
        x[4]=_mm_sub_ps(x[4],x[0]);
        x[3]=ordered_mul_ps(x[0],x[2]);
        x[4]=ordered_add_ps(ordered_mul_ps(x[4],x[1]),x[3]);
        publish(0x30b2c3u);
        vw(s.memory,s.observe,0x30b2c3u,g[2]+0x10u,x[4]);
    }
    return RatioExit::complete;
#else
    (void)s;(void)spill;
    return RatioExit::original;
#endif
}
} // namespace halo2::native_mopp_ray_slab_delta_detail
