#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_slab_bounds_detail {
enum class Continuation {
    original, store_low, wide_cache_low, wide_read_lower, wide_scale_cache_low, wide_read_left_code,
    wide_read_right_code, wide_scale_vars_low,
    store_high, wide_cache_high, wide_translate_low, read_upper, wide_scale_cache_high,
    wide_scale_vars_high, wide_translate_high, short_adjust_axis, short_read_lower, endpoint
};

inline bool reference_slab_bounds() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SLAB_BOUNDS");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_slab_bounds_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SLAB_BOUNDS_PROVIDER");
        return value && *value;
    }();
    return reference;
}

inline bool reference_wide_slab_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_WIDE_SLAB_PROVIDER");
        return value && *value;
    }();
    return reference;
}

// Capture C1 at the real arithmetic instruction before the host FSTP
// resets it. Keeping the m32 operand also preserves original denormal/NaN
// handling and the original operand order.
template<bool Multiply,class State>
void wide_arithmetic(State &s,std::uint32_t bits) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto &value=s.fp.st(0);
    std::uint16_t status;
    if constexpr(Multiply) {
        asm volatile("fldt %1; fmuls %2; fnstsw %0; fstpt %1"
            :"=a"(status),"+m"(value):"m"(bits):"st");
    } else {
        asm volatile("fldt %1; fsubs %2; fnstsw %0; fstpt %1"
            :"=a"(status),"+m"(value):"m"(bits):"st");
    }
    s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);
#else
    using namespace physics_step_detail;
    if constexpr(Multiply)s.fp.st(0)=mul(s.fp.st(0),ext(f(bits)));
    else s.fp.st(0)=sub(s.fp.st(0),ext(f(bits)));
#endif
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class Bounds {
    State &s_;
    const typename State::InstructionCursor *cursor_;
    bool provider_store_{};

    void publish(std::uint32_t pc) const {
        if(cursor_)*cursor_->position=pc;
    }

    bool read(std::uint32_t address,std::uint32_t &value) const {
        const auto *page=s_.memory.direct_page(address,4u);
        if(!page)return false;
        std::memcpy(&value,page->bytes+(address&4095u),4u);
        return true;
    }

    bool store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        publish(pc);
        return s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1}));
    }

    template<bool Wide>
    std::optional<Continuation> guarded_store(std::uint32_t pc,std::uint32_t value,
        Continuation before,Continuation after,unsigned minimum_depth,unsigned maximum_depth) {
        const auto address=s_.gp[4]+0x14u;
        if(store(pc,address,value))return {};
        if(Wide?reference_wide_slab_provider():reference_slab_bounds_provider())return before;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);
        provider_store_=true;
        if(!native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_) ||
           s_.fp.count<minimum_depth || s_.fp.count>maximum_depth ||
           !native_raw_x87_detail::supported(s_))return after;
        return {};
    }

    bool read_code(const std::byte *code,unsigned offset,std::uint32_t &word) const {
        if(!provider_store_) {
            word=std::to_integer<std::uint32_t>(code[offset-1u]);
            return true;
        }
        // A real write can change the code cursor or replace its backing.
        // Reopen the current byte at its original operand position.
        const auto address=s_.gp[6]+offset;
        const auto *page=s_.memory.direct_page(address,1u);
        if(!page)return false;
        word=std::to_integer<std::uint32_t>(page->bytes[address&4095u]);
        return true;
    }

public:
    Bounds(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<bool Wide> Continuation run() {
        using namespace physics_step_detail;
        auto &g=s_.gp;
        const auto first=g[6]+1u;
        const auto *page=s_.memory.direct_page(first,Wide?6u:2u);
        if(!page)return Continuation::original;
        const auto *code=page->bytes+(first&4095u);
        const auto byte=[&](unsigned offset){return std::to_integer<std::uint32_t>(code[offset-1u]);};
        g[1]=byte(1u);
        g[2]=byte(2u);
        std::uint32_t word;
        if constexpr(Wide) {
            g[1]=s_.flags.shift(g[1],8u,4);
            g[1]=s_.flags.plus(g[1],g[2],4);
            g[2]=byte(3u);
            g[1]=s_.flags.shift(g[1],8u,4);
            g[1]=s_.flags.plus(g[1],g[2],4);
            g[2]=byte(5u);
            if(const auto next=guarded_store<true>(0x30b197u,g[1],
                Continuation::store_low,Continuation::wide_cache_low,0u,6u))return *next;
            if(!read(g[4]+0x28u,g[1]))return Continuation::wide_cache_low;
            g[0]=s_.flags.plus(g[0],0xffffffd7u,4);
            if(!read(g[4]+0x14u,word))return Continuation::wide_read_lower;
            s_.fp.push(static_cast<long double>(si(word)));
            if(!read(g[1]+0x14u,word))return Continuation::wide_scale_cache_low;
            wide_arithmetic<true>(s_,word);

            // The first scratch store can alias operand bytes. Load the
            // remaining authored bytes here, after that store has retired.
            if(!read_code(code,4u,g[1]))return Continuation::wide_read_left_code;
            g[1]=s_.flags.shift(g[1],8u,4);
            g[2]=s_.flags.plus(g[2],g[1],4);
            if(!read_code(code,6u,g[1]))return Continuation::wide_read_right_code;
            g[2]=s_.flags.shift(g[2],8u,4);
            g[2]=s_.flags.plus(g[2],g[1],4);
            if(!read(g[3]+0x14u,word))return Continuation::wide_scale_vars_low;
            wide_arithmetic<true>(s_,word);
            if(const auto next=guarded_store<true>(0x30b1beu,g[2],
                Continuation::store_high,Continuation::wide_cache_high,1u,7u))return *next;
            if(!read(g[4]+0x28u,g[2]))return Continuation::wide_cache_high;
            if(!read(g[3]+g[0]*4u,word))return Continuation::wide_translate_low;
            wide_arithmetic<false>(s_,word);
            g[6]=s_.flags.plus(g[6],7u,4);
            if(!read(g[4]+0x14u,word))return Continuation::read_upper;
            s_.fp.push(static_cast<long double>(si(word)));
            if(!read(g[2]+0x14u,word))return Continuation::wide_scale_cache_high;
            wide_arithmetic<true>(s_,word);
            if(!read(g[3]+0x14u,word))return Continuation::wide_scale_vars_high;
            wide_arithmetic<true>(s_,word);
            if(!read(g[3]+g[0]*4u,word))return Continuation::wide_translate_high;
            wide_arithmetic<false>(s_,word);
            publish(0x30b1d9u);
        } else {
            if(const auto next=guarded_store<false>(0x30b1e3u,g[1],Continuation::store_low,
                Continuation::short_adjust_axis,0u,6u))return *next;
            g[0]=s_.flags.plus(g[0],0xffffffdau,4);
            g[6]=s_.flags.plus(g[6],3u,4);
            if(!read(g[4]+0x14u,word))return Continuation::short_read_lower;
            s_.fp.push(static_cast<long double>(si(word)));
            if(const auto next=guarded_store<false>(0x30b1f1u,g[2],Continuation::store_high,
                Continuation::read_upper,0u,7u))return *next;
            if(!read(g[4]+0x14u,word))return Continuation::read_upper;
            s_.fp.push(static_cast<long double>(si(word)));
            publish(0x30b1f5u);
        }
        return Continuation::endpoint;
    }
};
#endif

template<bool Wide,class State>
Continuation slab_bounds(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_slab_bounds() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.active_mmx || s.fp.count>6u)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    std::uint16_t control;
    asm volatile("fnstcw %0":"=m"(control));
    if((control&0x3fu)!=0x3fu)return Continuation::original;
    return Bounds<State>{s,cursor}.template run<Wide>();
#else
    (void)s;
    return Continuation::original;
#endif
}
} // namespace halo2::native_mopp_ray_slab_bounds_detail
