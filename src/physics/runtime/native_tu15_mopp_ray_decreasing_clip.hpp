#pragma once

#include "physics/runtime/native_tu15_mopp_ray_slab_bounds.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <bit>

namespace halo2::native_mopp_ray_decreasing_clip_detail {
enum class Continuation { original, ratio, reload_fraction, read_one, read_start, done };

inline bool reference_decreasing_clip() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_DECREASING_CLIP");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_decreasing_clip_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_DECREASING_CLIP_PROVIDER");
        return value && *value;
    }();
    return reference;
}

template<class State>
void reverse_divide(State &s,std::uint32_t bits) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto &value=s.fp.st(0);
    std::uint16_t status;
    asm volatile("fldt %1; fdivrs %2; fnstsw %0; fstpt %1"
        :"=a"(status),"+m"(value):"m"(bits):"st");
    s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);
#else
    using namespace physics_step_detail;
    s.fp.st(0)=physics_solver_detail::divide(ext(f(bits)),s.fp.st(0));
#endif
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<bool After,class State> class DecreasingClip {
    State &s_;
    const typename State::InstructionCursor *cursor_;
    static constexpr auto lower=After?0x30u:0x18u;
    static constexpr auto upper=After?0x34u:0x2cu;

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }
    template<class Value> bool read(std::uint32_t address,Value &value) const {
        const auto *page=s_.memory.direct_page(address,sizeof(Value));
        if(!page)return false;
        std::memcpy(&value,page->bytes+(address&4095u),sizeof(Value));
        return true;
    }

    template<class Value> bool read_literal(std::uint32_t address,Value &value) const {
        if(read(address,value))return true;
        // World publishes these literals as short immutable regions.
        // Borrow only the original operand width from an ordinary owner.
        const auto view=s_.memory.try_read_view(address,sizeof(Value));
        if(!view)return false;
        std::memcpy(&value,view->data(),sizeof(Value));
        return true;
    }

    bool ratio(std::uint32_t numerator,std::uint32_t subtrahend) {
        constexpr auto site=After?0x30b0aeu:0x30b043u;
        const auto address=s_.gp[4]+(After?lower:upper);
        publish(site);
        std::uint32_t encoded;
        std::uint16_t status;
        asm volatile("flds %2; fsubs %3; fdivrs %2; fstps %0; fnstsw %1"
            :"=m"(encoded),"=a"(status):"m"(numerator),"m"(subtrahend):"st");
        if(s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&encoded,1}))) {
            s_.fp.status=(s_.fp.status&~0x200u)|(status&0x200u);
            return true;
        }

        // The paired ordinary reads and refused warm write have no
        // callbacks. Restore the live unrounded x87 lane for the actual
        // scalar store, then retire FSTP only after that store succeeds.
        long double live;
        asm volatile("flds %1; fsubs %2; fdivrs %1; fstpt %0"
            :"=m"(live):"m"(numerator),"m"(subtrahend):"st");
        s_.fp.push(live);
        s_.fp.status=(s_.fp.status&~0x200u)|(status&0x200u);
        physics_step_detail::w(s_.memory,s_.observe,site,address,4,encoded);
        s_.fp.pop();
        return !reference_decreasing_clip_provider() &&
            (_mm_getcsr()&0x1f80u)==0x1f80u &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_);
    }

public:
    DecreasingClip(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    Continuation run() {
        auto &g=s_.gp;
        auto &x=s_.xmm;
        const auto scratch=g[4]+lower;
        const auto *page=s_.memory.direct_page(scratch,upper-lower+4u);
        std::uint32_t a,b,epsilon;
        if(!page)return Continuation::original;
        const auto *slots=page->bytes+(scratch&4095u);
        std::memcpy(&a,slots,4u);
        std::memcpy(&b,slots+upper-lower,4u);
        if(!read_literal(0x46a0e8u,epsilon))return Continuation::original;
        asm volatile("flds %1; fmuls %2; fcomps %3; fnstsw %0"
            :"=a"(s_.fp.status):"m"(a),"m"(b),"m"(epsilon):"st");
        s_.set(0,s_.fp.word(),2,0);
        s_.flags.logic(s_.reg(0,1,8)&5u,1);
        publish(After?0x30b095u:0x30b035u);
        if(s_.flags.pf)return Continuation::done;
        if((_mm_getcsr()&0x1f80u)!=0x1f80u)return Continuation::ratio;

        // There was no callback, store, or ownership change since the
        // product. On the post-child path preflight the ordinary vector
        // operands before any observable interpolation state is committed.
        if constexpr(After) {
            __m128 end,one;
            if(!read(g[7]+0x10u,end)||!read_literal(0x41c500u,one))return Continuation::ratio;
            x[1]=end;
            x[3]=one;
        }
        if(!ratio(b,a))return Continuation::reload_fraction;
        std::uint32_t fraction;
        if(!read(g[4]+(After?lower:upper),fraction))return Continuation::reload_fraction;
        x[0]=_mm_set1_ps(std::bit_cast<float>(fraction));

        using namespace native_tu15_leaves;
        if constexpr(After) {
            x[2]=ordered_mul_ps(x[0],x[1]);
            if(!read(g[7],x[1]))return Continuation::read_start;
            x[3]=ordered_add_ps(ordered_mul_ps(_mm_sub_ps(x[3],x[0]),x[1]),x[2]);
            publish(0x30b0ceu);
            physics_step_detail::vw(s_.memory,s_.observe,0x30b0ceu,g[7],x[3]);
        } else {
            x[3]=ordered_mul_ps(x[0],x[2]);
            if(!read_literal(0x41c500u,x[2]))return Continuation::read_one;
            x[2]=ordered_add_ps(ordered_mul_ps(_mm_sub_ps(x[2],x[0]),x[1]),x[3]);
            publish(0x30b067u);
            physics_step_detail::vw(s_.memory,s_.observe,0x30b067u,g[4]+0x90u,x[2]);
        }
        return Continuation::done;
    }
};
#endif

template<bool After,class State>
Continuation decreasing_clip(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_decreasing_clip() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || s.fp.count>=8u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;
    return DecreasingClip<After,State>{s,cursor}.run();
#else
    (void)s;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_decreasing_clip_detail
