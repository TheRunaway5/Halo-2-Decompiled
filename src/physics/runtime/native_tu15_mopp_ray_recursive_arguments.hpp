#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>

namespace halo2::native_mopp_ray_recursive_arguments_detail {
enum class Continuation { original, first, advance_code, second, read_cache, third, return_address, invoke };

inline bool reference_recursive_arguments() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_RECURSIVE_ARGUMENTS");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_recursive_arguments_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_RECURSIVE_ARGUMENTS_PROVIDER");
        return value && *value;
    }();
    return reference;
}

inline bool reference_increasing_arguments_provider() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_INCREASING_ARGUMENTS_PROVIDER");
        return value && *value;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State> class Arguments {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    bool cache(std::uint32_t offset) {
        const auto address=s_.gp[4]+offset;
        if(const auto *page=s_.memory.direct_page(address,4u)) {
            std::memcpy(&s_.gp[1],page->bytes+(address&4095u),4u);
            return true;
        }
        const auto view=s_.memory.try_read_view(address,4u);
        if(!view)return false;
        std::memcpy(&s_.gp[1],view->data(),4u);
        return true;
    }

    bool push(std::uint32_t pc,std::uint32_t value) {
        if(cursor_)*cursor_->position=pc;
        const auto address=s_.gp[4]-4u;
        // The admitted operation cannot run a guard callback or observer.
        // A refusal leaves this ESP update to the original PUSH.
        if(!s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1})))
            return false;
        s_.gp[4]=address;
        return true;
    }

    template<bool Increasing>
    std::optional<Continuation> guarded_push(std::uint32_t pc,std::uint32_t value,
        Continuation before,Continuation after) {
        if(push(pc,value))return {};
        if(Increasing?reference_increasing_arguments_provider():reference_recursive_arguments_provider())
            return before;
        // Own the original ESP change before the guarded write. A fault
        // leaves ESP and all completed arguments visible to the provider.
        s_.push(pc,value);
        if(after!=Continuation::invoke &&
           !native_mopp_ray_scale_tail_detail::provider_continuation(s_,cursor_))return after;
        return {};
    }

public:
    Arguments(State &s,const typename State::InstructionCursor *cursor):s_(s),cursor_(cursor) {}

    template<bool Increasing> Continuation run() {
        auto &g=s_.gp;
        if constexpr(Increasing) {
            g[1]=s_.flags.minus(g[1],g[2],4);
            g[0]=g[4]+0x80u;
            if(const auto next=guarded_push<true>(0x30af1fu,g[0],
                Continuation::first,Continuation::advance_code))return *next;
            g[1]=s_.flags.plus(g[1],g[6],4);
            if(const auto next=guarded_push<true>(0x30af22u,g[1],
                Continuation::second,Continuation::read_cache))return *next;
            if(!cache(0x30u))return Continuation::read_cache;
            if(const auto next=guarded_push<true>(0x30af27u,g[3],
                Continuation::third,Continuation::return_address))return *next;
            if(const auto next=guarded_push<true>(0x30af28u,0x30af2du,
                Continuation::return_address,Continuation::invoke))return *next;
        } else {
            if(!cache(0x28u))return Continuation::original;
            g[2]=g[4]+0x80u;
            if(const auto next=guarded_push<false>(0x30b07au,g[2],
                Continuation::first,Continuation::second))return *next;
            if(const auto next=guarded_push<false>(0x30b07bu,g[6],
                Continuation::second,Continuation::third))return *next;
            if(const auto next=guarded_push<false>(0x30b07cu,g[3],
                Continuation::third,Continuation::return_address))return *next;
            if(const auto next=guarded_push<false>(0x30b07du,0x30b082u,
                Continuation::return_address,Continuation::invoke))return *next;
        }
        return Continuation::invoke;
    }
};
#endif

template<bool Increasing,class State>
Continuation publish_arguments(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    if(reference_recursive_arguments() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe)return Continuation::original;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return Continuation::original;
    return Arguments<State>{s,cursor}.template run<Increasing>();
#else
    (void)s;
    return Continuation::original;
#endif
}
}
