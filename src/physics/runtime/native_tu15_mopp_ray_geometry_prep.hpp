#pragma once

#include "physics/runtime/native_tu15_mopp_ray_slab_bounds.hpp"
#include <bit>

namespace halo2::native_mopp_ray_geometry_prep_detail {
enum class Continuation {
    original, read_first_vector, read_second_vector, store_second_vector, spill_first,
    subtract_first, load_second, load_third, load_comparison, decreasing, numeric
};

inline bool reference_geometry_prep() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_GEOMETRY_PREP");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_geometry_prep_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_GEOMETRY_PREP_PROVIDER");
        return value && *value;
    }();
    return reference;
}

template<class State>
void reverse_subtract(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto &value=s.fp.st(0);
    const auto &older=s.fp.st(1);
    std::uint16_t status;
    asm volatile("fldt %2; fldt %1; fsubr %%st(1), %%st; fnstsw %0; fstpt %1; fstp %%st(0)"
        :"=a"(status),"+m"(value):"m"(older):"st","st(1)");
    s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);
#else
    s.fp.st(0)=physics_step_detail::sub(s.fp.st(1),s.fp.st(0));
#endif
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline bool masked_x87() {
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    return (control&0x3fu)==0x3fu;
}

template<class State> class GeometryPrep {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }
    template<class Value> bool read(std::uint32_t address,Value &value) const {
        const auto *page=s_.memory.direct_page(address,sizeof(Value));
        if(!page)return false;
        std::memcpy(&value,page->bytes+(address&4095u),sizeof(Value));
        return true;
    }
    bool continue_after_store() const {
        return !reference_geometry_prep_provider() &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_) &&
            s_.fp.count<8u && masked_x87();
    }
    bool vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        publish(pc);
        // Keep the real provider, address order and observer of each half.
        physics_step_detail::vw(s_.memory,s_.observe,pc,address,value);
        return s_.fp.count && masked_x87() &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_);
    }
    template<class Spill>
    bool spill_scalar(std::uint32_t pc,std::uint32_t offset,Spill spill,unsigned minimum_depth) {
        publish(pc);
        const auto address=s_.gp[4]+offset;
        const auto value=spill(s_.fp.st(0),s_.fp.status);
        const bool direct=s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1}));
        if(!direct) {
            // During a provider callback/fault FSTP's original x87 value
            // and C1 are live. Retire the pop only once the write completes.
            physics_step_detail::w(s_.memory,s_.observe,pc,address,4,value);
        }
        s_.fp.pop();
        return s_.fp.count>=minimum_depth && (direct || continue_after_store());
    }
    std::optional<Continuation> store_difference(std::uint32_t site,std::uint32_t operand,
        std::uint32_t output,Continuation retry_load,Continuation after_store) {
        const auto stack=s_.gp[4];
        std::uint32_t left,right;
        if(!read(stack+0x18u,left)||!read(stack+operand,right))return retry_load;

        publish(site);
        std::uint32_t encoded;
        std::uint16_t status;
        asm volatile("flds %2; fsubs %3; fstps %0; fnstsw %1"
            :"=m"(encoded),"=a"(status):"m"(left),"m"(right):"st");
        if(s_.memory.try_callback_free_write(stack+output,std::as_bytes(std::span{&encoded,1}))) {
            s_.fp.status=(s_.fp.status&~0x200u)|(status&0x200u);
            return std::nullopt;
        }

        // Neither attempt can invoke a callback. Materialize the unrounded
        // ST(0) only when the real provider can inspect or fault this FSTP.
        long double intermediate;
        asm volatile("flds %1; fsubs %2; fstpt %0"
            :"=m"(intermediate):"m"(left),"m"(right):"st");
        s_.fp.push(intermediate);
        s_.fp.status=(s_.fp.status&~0x200u)|(status&0x200u);
        physics_step_detail::w(s_.memory,s_.observe,site,stack+output,4,encoded);
        s_.fp.pop();
        // A real write may replace the frame or activate tracing. The next
        // difference must use current operands and the current observer.
        if(continue_after_store())return std::nullopt;
        return after_store;
    }

    bool load(std::uint32_t offset) {
        std::uint32_t word;
        if(!read(s_.gp[4]+offset,word))return false;
        s_.fp.push(physics_step_detail::ext(physics_step_detail::f(word)));
        return true;
    }
    bool subtract(std::uint32_t offset) {
        std::uint32_t word;
        if(!read(s_.gp[4]+offset,word))return false;
        native_mopp_ray_slab_bounds_detail::wide_arithmetic<false>(s_,word);
        return true;
    }

public:
    GeometryPrep(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<class Spill> Continuation run(Spill spill) {
        auto &g=s_.gp;
        auto &x=s_.xmm;
        if(!load(0x20u))return Continuation::original;
        if(!read(g[7],x[1]))return Continuation::read_first_vector;
        reverse_subtract(s_);
        if(!read(g[7]+0x10u,x[2]))return Continuation::read_second_vector;
        if(!vector(0x30ae82u,g[4]+0x80u,x[1]))return Continuation::store_second_vector;
        if(!vector(0x30ae8au,g[4]+0x90u,x[2]))return Continuation::spill_first;

        if(!spill_scalar(0x30ae92u,0x34u,spill,1u))return Continuation::subtract_first;
        if(!subtract(0x1cu))return Continuation::subtract_first;
        if(!spill_scalar(0x30ae9au,0x2cu,spill,0u))return Continuation::load_second;
        if(const auto next=store_difference(0x30aea6u,0x20u,0x30u,
            Continuation::load_second,Continuation::load_third))return *next;
        if(const auto next=store_difference(0x30aeb2u,0x1cu,0x18u,
            Continuation::load_third,Continuation::load_comparison))return *next;

        std::uint32_t left,right;
        if(!read(g[4]+0x34u,left)||!read(g[4]+0x30u,right))return Continuation::load_comparison;
        asm volatile("flds %1; fcomps %2; fnstsw %0"
            :"=a"(s_.fp.status):"m"(left),"m"(right):"st");
        s_.set(0,s_.fp.word(),2,0);
        s_.flags.logic(s_.reg(0,1,8)&5u,1);
        publish(0x30aec3u);
        return s_.flags.pf?Continuation::decreasing:Continuation::numeric;
    }
};
#endif

template<class State,class Spill>
Continuation geometry_prep(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_geometry_prep() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || !s.fp.count || s.fp.count>=8u || !masked_x87())return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    return GeometryPrep<State>{s,cursor}.run(spill);
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_geometry_prep_detail
