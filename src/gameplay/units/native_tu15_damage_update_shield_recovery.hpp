#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_update_shield_recovery_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;

enum class Entry { recover, evaluate };
enum class Next { retained, armor, clear_pending, countdown, default_scale, apply_scale };
enum Branch : unsigned { clamped, subtracted, without_recipient, with_recipient, clear, wait, default_scale,
    description_scale, branch_count };

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Route, class Multiply>
struct Frame : Port<State> {
    Entry entry;
    Next &next;
    Route &route;
    Multiply &multiply;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &resume,
          Route &child, Multiply &mul)
        : Port<State>(state,cursor,trace,{}), entry(start), next(resume), route(child), multiply(mul) {}

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc,address,4)));
    }

    void store_scalar(std::uint32_t pc, std::uint32_t address, __m128 value) {
        this->store(pc,address,4,physics_step_detail::u(_mm_cvtss_f32(value)));
    }

    template<std::uint32_t target>
    void invoke(std::uint32_t pc, std::uint32_t returned) {
        this->call(pc,returned,target,[&](State &state) {
            try {
                route(state,std::integral_constant<std::uint32_t,target>{});
            } catch (...) {
                this->gp = state.gp;
                this->flags = state.flags;
                throw;
            }
        });
    }

    void calculate_recovery() {
        auto &gp = this->gp;
        auto &xmm = this->s.xmm;
        gp[eax] = gp[edi];
        invoke<0x1509b0u>(0xd870eu,0xd8713u);
        gp[ecx] = this->read(0xd8713u,0x51df54u,4);
        const auto ticks = static_cast<std::uint16_t>(this->read(0xd8719u,gp[ecx]+2u,2));
        gp[edx] = static_cast<std::uint32_t>(std::bit_cast<std::int16_t>(ticks));
        xmm[0] = scalar(0xd871du,0x46a11cu);
        xmm[1] = _mm_cvtsi32_ss(xmm[1],std::bit_cast<std::int32_t>(gp[edx]));
        xmm[1] = multiply(xmm[1],scalar(0xd8729u,0x46a370u));
        xmm[0] = _mm_div_ss(xmm[0],xmm[1]);
        xmm[1] = scalar(0xd8735u,gp[ebp]+0xf0u);
        const auto status = this->read(0xd873du,gp[ebp]+0x10bu,1);
        this->store(0xd873du,gp[ebp]+0x10bu,1,this->flags.logic(status&0xfdu,1));
        xmm[1] = _mm_sub_ss(xmm[1],xmm[3]);
        this->flags.scalar_compare(xmm[0],xmm[1],false);
        store_scalar(0xd874bu,gp[esp]+0x20u,xmm[0]);
        store_scalar(0xd8751u,gp[esp]+0x24u,xmm[1]);
    }

    void commit_recovery() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        const bool subtract = flags.cf || flags.zf;
        flags.minus(gp[eax],0xffffffffu,4);
        if (subtract) {
            this->reached(subtracted);
            xmm[1] = _mm_sub_ss(scalar(0xd8770u,gp[ebp]+0xf0u),xmm[0]);
            store_scalar(0xd877cu,gp[ebp]+0xf0u,xmm[1]);
            if (!flags.zf) {
                gp[edx] = this->read(0xd8786u,gp[esp]+0x20u,4);
                this->push(0xd878au,gp[edx]);
            }
        } else {
            this->reached(clamped);
            store_scalar(0xd875cu,gp[ebp]+0xf0u,xmm[2]);
            if (!flags.zf) {
                gp[ecx] = this->read(0xd8766u,gp[esp]+0x24u,4);
                this->push(0xd876au,gp[ecx]);
            }
        }
        if (flags.zf) this->reached(without_recipient);
        else {
            this->reached(with_recipient);
            invoke<0x13d1c8u>(0xd878bu,0xd8790u);
        }
        this->push(0xd8790u,0x80u);
        gp[eax] = gp[edi];
        invoke<0xb1c30u>(0xd8797u,0xd879cu);
        this->store(0xd879cu,gp[esp]+0x13u,1,1u);
        this->publish(0xd87a1u);
        next = Next::armor;
    }

    void finish(Branch branch, Next continuation, std::uint32_t pc) {
        this->reached(branch);
        next = continuation;
        this->publish(pc);
    }

    void evaluate_remaining() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        flags.scalar_compare(xmm[2],scalar(0xd87a6u,gp[ebp]+0xf0u),false);
        if (flags.cf || flags.zf) {
            finish(clear,Next::clear_pending,0xd87adu);
            return;
        }
        gp[esi] = (gp[esi]&0xffff0000u) | this->read(0xd87b3u,gp[ebp]+0x104u,2);
        flags.logic(gp[esi]&0xffffu,2);
        if (!flags.zf) {
            finish(wait,Next::countdown,0xd87bdu);
            return;
        }
        flags.logic(gp[ebx],4);
        if (flags.zf) {
            finish(default_scale,Next::default_scale,0xd87c5u);
            return;
        }
        xmm[0] = scalar(0xd87c7u,gp[ebx]+0x6cu);
        finish(description_scale,Next::apply_scale,0xd87ccu);
    }

    bool can_recover() {
        auto &xmm = this->s.xmm;
        auto &flags = this->flags;
        xmm[0] = scalar(0xd86f2u,this->gp[ebp]+0xf0u);
        flags.scalar_compare(xmm[0],xmm[2],false);
        if (flags.cf || flags.zf) return false;
        flags.minus(this->gp[esi],2u,4);
        return flags.zf;
    }

    void run() {
        try {
            if (entry == Entry::recover && can_recover()) {
                calculate_recovery();
                commit_recovery();
            } else evaluate_remaining();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Route, class Multiply>
Next bridge(State &state, Entry entry, Reference reference, Route route, Multiply multiply) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return Next::retained;
    static VerifySummary summary{"Damage shield recovery"};
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_UPDATE_SHIELD_RECOVERY");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_UPDATE_SHIELD_RECOVERY");
    const auto first = entry == Entry::recover ? 0xd86f2u : 0xd87a6u;
    auto next = Next::retained;
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        if (reference(input,entry) != next) throw std::runtime_error("Damage shield recovery continuation differed");
    };
    run_owner(state,first,0xd87ccu,reference_mode,verify_mode,coverage(),summary,verify_reference,
        [&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Route,Multiply>{value,cursor,trace,entry,next,route,multiply};
        });
    return next;
}
} // namespace halo2::native_damage_update_shield_recovery_detail
