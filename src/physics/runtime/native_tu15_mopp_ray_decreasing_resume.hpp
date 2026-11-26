#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <bit>

namespace halo2::native_mopp_ray_decreasing_resume_detail {
enum class Continuation {
    original, shorten, advance, empty, opcode,
    read_fraction, read_end, read_one, read_code_pointer, read_code,
    save_fraction, refill_fraction, read_start, read_scale, read_short_end,
    save_scale, refill_scale, read_axis, read_translation, read_scaled_end,
    branch_axis, read_endpoint, read_delta
};
enum class Phase { fraction, shorten, advance };
enum class IncreasingPhase { before_child, after_child, fraction };

inline bool reference_decreasing_resume() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_DECREASING_RESUME");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_decreasing_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_DECREASING_PROVIDER");
        return value && *value;
    }();
    return reference;
}

inline bool reference_increasing_resume() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_INCREASING_RESUME");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class DecreasingResume {
    using Cursor = typename State::InstructionCursor;
    State &s_;
    const Cursor *cursor_;

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
        const auto view=s_.memory.try_read_view(address,sizeof(Value));
        if(!view)return false;
        std::memcpy(&value,view->data(),sizeof(Value));
        return true;
    }

    bool continuation_allowed() const {
        return (_mm_getcsr()&0x1f80u)==0x1f80u &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_);
    }

    std::optional<Continuation> scalar(std::uint32_t pc,std::uint32_t address,std::uint32_t value,
        Continuation before,Continuation after) {
        publish(pc);
        if(s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1})))return {};
        if(reference_decreasing_provider())return before;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);
        if(!continuation_allowed())return after;
        return {};
    }

    bool vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        publish(pc);
        // The stock provider owns each original eight-byte write. A callback
        // from either half can change admission for the following instruction.
        physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);
        return continuation_allowed();
    }

    template<bool Literal=false>
    bool compare(std::uint32_t first,std::uint32_t second,std::uint32_t mask) {
        if(s_.fp.count>=8u)return false;
        std::uint16_t cw;
        asm volatile("fnstcw %0":"=m"(cw));
        if((cw&0x3fu)!=0x3fu)return false;
        std::uint32_t a,b;
        if(!read(first,a))return false;
        if constexpr(Literal) {
            if(!read_literal(second,b))return false;
        } else if(!read(second,b))return false;
        using namespace physics_step_detail;
        // Both ordinary loads are side-effect free. FCOMP consumes the
        // temporary FLD, leaving the live x87 lanes and TOP unchanged.
        s_.fp.status=physics_solver_detail::compare(ext(f(a)),ext(f(b)));
        s_.set(0,s_.fp.word(),2,0);
        s_.flags.logic(s_.reg(0,1,8)&mask,1);
        return true;
    }
public:
    DecreasingResume(State &s,const Cursor *cursor):s_(s),cursor_(cursor) {}

    template<bool Increasing=false>
    Continuation fraction() {
        auto &g=s_.gp;
        if(!read(g[4]+0x28u,g[1]))return Continuation::original;
        if(!compare<true>(g[1]+0x54u,0x46a0d0u,5u))return Continuation::read_fraction;
        publish(Increasing?0x30af8eu:0x30b0e3u);
        return s_.flags.pf?Continuation::advance:Continuation::shorten;
    }

    template<bool BeforeChild>
    Continuation product() {
        if(s_.fp.count>=8u)return Continuation::original;
        std::uint16_t cw;
        asm volatile("fnstcw %0":"=m"(cw));
        if((cw&0x3fu)!=0x3fu)return Continuation::original;
        const auto sp=s_.gp[4];
        std::uint32_t left,right,epsilon;
        if(!read(sp+(BeforeChild?0x30u:0x18u),left) ||
           !read(sp+(BeforeChild?0x34u:0x2cu),right) ||
           !read_literal(0x46a0e8u,epsilon))return Continuation::original;
        // These operands have no provider effects. Keep the literal x87
        // operand order and stack depth through the multiply and compare.
        using namespace physics_step_detail;
        s_.fp.push(ext(f(left)));
        s_.fp.st(0)=mul(s_.fp.st(0),ext(f(right)));
        s_.fp.comparison(ext(f(epsilon)),1);
        s_.set(0,s_.fp.word(),2,0);
        s_.flags.logic(s_.reg(0,1,8)&5u,1);
        publish(BeforeChild?0x30aedcu:0x30af40u);
        return s_.flags.pf?Continuation::advance:Continuation::shorten;
    }

    Continuation shorten() {
        auto &g=s_.gp;
        auto &x=s_.xmm;
        std::uint32_t word;
        if(!read(g[1]+0x54u,g[0]))return Continuation::original;
        if(!read(g[1]+0x30u,x[2]))return Continuation::read_end;
        if(!read_literal(0x41c500u,x[4]))return Continuation::read_one;
        if(!read(g[1]+0x10u,g[2]))return Continuation::read_code_pointer;
        if(!read(g[2],x[1]))return Continuation::read_code;
        if(const auto next=scalar(0x30b0fdu,g[4]+0x74u,g[0],
            Continuation::save_fraction,Continuation::refill_fraction))return *next;
        if(!read(g[4]+0x74u,word))return Continuation::refill_fraction;

        using namespace native_tu15_leaves;
        x[0]=_mm_set1_ps(std::bit_cast<float>(word));
        x[3]=ordered_mul_ps(x[0],x[2]);
        if(!read(g[1]+0x20u,x[2]))return Continuation::read_start;
        x[4]=ordered_add_ps(ordered_mul_ps(_mm_sub_ps(x[4],x[0]),x[2]),x[3]);
        x[4]=_mm_sub_ps(x[4],x[1]);
        if(!vector(0x30b121u,g[7]+0x10u,x[4]))return Continuation::read_scale;

        if(!read(g[3]+0x14u,g[0]))return Continuation::read_scale;
        if(!read(g[7]+0x10u,x[1]))return Continuation::read_short_end;
        if(const auto next=scalar(0x30b12cu,g[4]+0x78u,g[0],
            Continuation::save_scale,Continuation::refill_scale))return *next;
        if(!read(g[4]+0x78u,word))return Continuation::refill_scale;
        x[0]=_mm_set_ss(std::bit_cast<float>(word));
        if(!read(g[4]+0x3cu,g[0]))return Continuation::read_axis;
        s_.flags.minus(g[0],3u,4);
        x[2]=ordered_mul_ps(_mm_shuffle_ps(x[0],x[0],0),x[1]);
        if(!vector(0x30b147u,g[7]+0x10u,x[2]))return Continuation::read_translation;

        if(!read(g[3],x[0]))return Continuation::read_translation;
        if(!read(g[7]+0x10u,x[1]))return Continuation::read_scaled_end;
        x[1]=_mm_sub_ps(x[1],x[0]);
        if(!vector(0x30b155u,g[7]+0x10u,x[1]))return Continuation::branch_axis;
        if(s_.flags.sf==s_.flags.of) {
            publish(0x30b159u);
            return Continuation::advance;
        }
        if(!compare(g[7]+0x10u+g[0]*4u,g[4]+0x20u,0x41u))return Continuation::read_endpoint;
        publish(0x30b168u);
        return s_.flags.zf?Continuation::empty:Continuation::advance;
    }

    Continuation advance() {
        auto &g=s_.gp;
        std::uint32_t offset;
        if(!read(g[4]+0x24u,g[0]))return Continuation::original;
        if(!read(g[4]+0x14u,offset))return Continuation::read_delta;
        g[0]=s_.flags.minus(g[0],offset,4);
        g[6]=s_.flags.plus(g[6],g[0],4);
        publish(0x30b178u);
        return Continuation::opcode;
    }
};
#endif

template<Phase phase,class State>
Continuation decreasing_resume(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_decreasing_resume() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx)return Continuation::original;
    if constexpr(phase==Phase::shorten)
        if((_mm_getcsr()&0x1f80u)!=0x1f80u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    DecreasingResume<State> frame{s,cursor};
    if constexpr(phase==Phase::fraction)return frame.fraction();
    if constexpr(phase==Phase::shorten)return frame.shorten();
    if constexpr(phase==Phase::advance)return frame.advance();
#else
    (void)s;
    return Continuation::original;
#endif
}

template<IncreasingPhase phase,class State>
Continuation increasing_resume(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_increasing_resume() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    DecreasingResume<State> frame{s,cursor};
    if constexpr(phase==IncreasingPhase::fraction)return frame.template fraction<true>();
    else return frame.template product<phase==IncreasingPhase::before_child>();
#else
    (void)s;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_decreasing_resume_detail
