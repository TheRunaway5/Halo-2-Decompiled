#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"
#include <bit>
#include <cstring>

namespace halo2::native_mopp_ray_scale_translation_detail {

enum class Continuation {
    original, constant_one, zero_lane, read_y, scale_shift, read_z, first_vector
};

inline bool reference_scale_translation() {
    static const bool retained=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SCALE_TRANSLATION");
        return (value&&*value) || native_raw_x87_detail::scoped_access_reference();
    }();
    return retained;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
template<class State>
class Translation {
    State &s_;
    const typename State::InstructionCursor *cursor_;

    void position(std::uint32_t pc) const noexcept {
        if(cursor_)*cursor_->position=pc;
    }
    bool store(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        position(pc);
        if(s_.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1})))return true;
        physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);
        return false;
    }
    bool scratch(std::int32_t &value) const noexcept {
        const auto address=s_.gp[4]+0x14u;
        return s_.fp.count<8u &&
            native_mopp_ray_scale_tail_detail::read_operand(s_,address,value) &&
            std::uint32_t(value)<=255u;
    }
    bool resume_after(bool direct) const {
        using namespace native_mopp_ray_scale_tail_detail;
        return direct || (!reference_scale_provider() && provider_continuation(s_,cursor_) &&
            native_raw_x87_detail::supported(s_));
    }

public:
    Translation(State &s,const typename State::InstructionCursor *cursor) : s_(s),cursor_(cursor) {}

    template<class Spill> Continuation run(Spill spill) {
        std::int32_t component;
        auto &g=s_.gp;
        const auto spill_component=[&](std::uint32_t pc,std::uint32_t offset) {
            position(pc);
            const auto word=spill(s_.fp.st(0),s_.fp.status);
            const bool direct=store(pc,g[4]+offset,word);
            s_.fp.pop();
            return resume_after(direct);
        };
        // Authored operands are unsigned bytes. An inexact conversion could
        // leave a physical x87 C1 difference after the retained stack moves.
        if(g[1]>255u || g[2]>255u || !scratch(component))return Continuation::original;
        s_.fp.push(static_cast<long double>(component));
        if(!resume_after(store(0x30b381u,g[4]+0x14u,g[2])))return Continuation::constant_one;
        g[2]=1u;
        if(!spill_component(0x30b38au,0x60u))return Continuation::zero_lane;

        if(!resume_after(store(0x30b38eu,g[4]+0x6cu,0u)))return Continuation::read_y;
        if(!scratch(component))return Continuation::read_y;
        s_.fp.push(static_cast<long double>(component));
        if(!resume_after(store(0x30b39au,g[4]+0x14u,g[1])))return Continuation::scale_shift;
        g[1]=g[0];
        g[2]=s_.flags.shift(g[2],g[1]&255u,4);
        if(!spill_component(0x30b3a2u,0x64u))return Continuation::read_z;
        if(!scratch(component))return Continuation::read_z;
        s_.fp.push(static_cast<long double>(component));
        spill_component(0x30b3aau,0x68u);
        return Continuation::first_vector;
    }
};
#endif

template<class State,class Spill>
Continuation prepare(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(std::endian::native!=std::endian::little)return Continuation::original;
    // Record observers and tracing own the literal software-stack publication.
    if(reference_scale_translation() || s.observe ||
       !native_mopp_ray_traversal_detail::initial_state_admitted(s))return Continuation::original;
    return Translation<State>(s,s.live_instruction_cursor()).run(spill);
#else
    (void)s;(void)spill;
    return Continuation::original;
#endif
}

} // namespace halo2::native_mopp_ray_scale_translation_detail
