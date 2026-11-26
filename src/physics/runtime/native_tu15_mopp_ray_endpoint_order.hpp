#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_endpoint_order_detail {
enum class Continuation { original, read_end, read_scratch, ascending, descending };

inline bool reference_endpoint_order() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_ENDPOINT_ORDER");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_endpoint_compare() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_ENDPOINT_COMPARE");
        return value && *value;
    }();
    return reference;
}

inline bool reference_endpoint_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_ENDPOINT_PROVIDER");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline bool spill_environment_admitted() noexcept {
    std::uint16_t control,status;
    asm volatile("fnstcw %0; fnstsw %1":"=m"(control),"=a"(status));
    // Retain live host C1 until the original load or spill can clear it.
    return (control&0x3fu)==0x3fu && !(status&0x200u);
}

template<class State> class Endpoints {
    State &s_;
    const typename State::InstructionCursor *cursor_;
    bool provider_store_{};

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }

    bool endpoint(std::uint32_t address,std::uint32_t &bits) const {
        const auto *page=s_.memory.direct_page(address,4u);
        if(!page)return false;
        std::memcpy(&bits,page->bytes+(address&4095u),4u);
        // FSTP of a just-loaded normal binary32 or signed zero is exact
        // in every x87 precision and rounding mode. Other encodings keep
        // the original x87 instructions and exception behavior.
        const auto magnitude=bits&0x7fffffffu;
        return !magnitude || (magnitude>=0x00800000u && magnitude<0x7f800000u);
    }

    template<class Spill>
    std::optional<Continuation> spill(std::uint32_t pc,std::uint32_t address,std::uint32_t bits,
        Continuation before,Continuation after,Spill spill_word) {
        publish(pc);
        if(s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&bits,1}))) {
            s_.fp.status&=~0x200u;
            return {};
        }
        if(reference_endpoint_provider())return before;
        using namespace physics_step_detail;
        // The provider sees the original lane and FP state, including when
        // its store throws. Reopen admission after that store and its pop.
        s_.fp.push(ext(f(bits)));
        const auto word=spill_word(s_.fp.st(0),s_.fp.status);
        w(s_.memory,s_.observe,pc,address,4u,word);
        s_.fp.pop();
        provider_store_=true;
        if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_) ||
           s_.fp.count>=8u || !spill_environment_admitted())return after;
        return {};
    }

public:
    Endpoints(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<class Spill> Continuation run(Spill spill_word) {
        auto &g=s_.gp;
        std::uint32_t bits;
        if(!endpoint(g[7]+g[0]*4u,bits))return Continuation::original;
        if(const auto next=spill(0x30b1fcu,g[4]+0x24u,bits,Continuation::original,
            Continuation::read_end,spill_word))return *next;

        if(!endpoint(g[7]+g[0]*4u+0x10u,bits))return Continuation::read_end;
        if(const auto next=spill(0x30b204u,g[4]+0x18u,bits,Continuation::read_end,
            Continuation::read_scratch,spill_word))return *next;

        // The older comparison rollback still owns this handoff when the
        // expanded spill route reaches it through a real provider.
        if(provider_store_ && reference_endpoint_compare())return Continuation::read_scratch;
        return compare_stack(false);
    }

    Continuation compare_stack(bool allow_sparse) {
        const auto scratch=s_.gp[4]+0x18u;
        const auto *page=s_.memory.direct_page(scratch,16u);
        const std::byte *base;
        if(page)base=page->bytes+(scratch&4095u);
        else {
            if(!allow_sparse)return Continuation::read_scratch;
            const auto view=s_.memory.try_read_view(scratch,16u);
            if(!view)return Continuation::read_scratch;
            base=view->data();
        }
        std::uint32_t start,end;
        std::memcpy(&start,base+0x0cu,4u);
        std::memcpy(&end,base,4u);
        using namespace physics_step_detail;
        // The two reads are ordinary and callback-free. The temporary FLD
        // lane is consumed by the following comparison at the same depth.
        s_.fp.status=physics_solver_detail::compare(ext(f(start)),ext(f(end)));
        s_.set(0,s_.fp.word(),2,0);
        s_.flags.logic(s_.reg(0,1,8)&5u,1);
        publish(0x30b215u);
        return s_.flags.pf?Continuation::ascending:Continuation::descending;
    }
};
#endif

template<class State,class Spill>
Continuation endpoint_order(State &s,Spill spill_word) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_endpoint_order() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || s.fp.count>=8u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    if(!spill_environment_admitted())return Continuation::original;
    return Endpoints<State>{s,cursor}.run(spill_word);
#else
    (void)s;(void)spill_word;
    return Continuation::original;
#endif
}

template<class State>
Continuation compare_endpoints(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::read_scratch;
    if(reference_endpoint_order() || reference_endpoint_compare() ||
       s.image!=SourcePhysicsContactImage::TitleUpdate15 || s.observe || s.active_mmx || s.fp.count>=8u)
        return Continuation::read_scratch;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::read_scratch;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::read_scratch;
    // Enter after both source stores retire, including any provider callback.
    return Endpoints<State>{s,cursor}.compare_stack(true);
#else
    (void)s;
    return Continuation::read_scratch;
#endif
}
} // namespace halo2::native_mopp_ray_endpoint_order_detail
