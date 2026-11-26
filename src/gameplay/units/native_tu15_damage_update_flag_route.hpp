#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_update_flag_route_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;

enum class Next { retained, keep_flags, clear_flags };
enum Branch : unsigned { unchanged, mode_clears, dispatch_clears, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Dispatch>
struct Frame : Port<State> {
    Next &next;
    Dispatch &dispatch;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Next &resume, Dispatch &route)
        : Port<State>(state,cursor,trace,{}), next(resume), dispatch(route) {}

    bool has_response_flags() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[eax] = (gp[eax]&0xffff0000u) | this->read(0xd8571u,gp[ebp]+0x10au,2);
        set_low_byte(gp[ecx],flags.right(low_byte(gp[eax]),5u,1,false));
        flags.logic(low_byte(gp[ecx])&1u,1);
        if (!flags.zf) return true;
        set_low_byte(gp[edx],flags.right(low_byte(gp[eax]),6u,1,false));
        flags.logic(low_byte(gp[edx])&1u,1);
        if (!flags.zf) return true;
        gp[ecx] = flags.right(gp[eax],10u,4,false);
        flags.logic(low_byte(gp[ecx])&1u,1);
        return !flags.zf;
    }

    void dispatch_response() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[edi] = this->read(0xd85a2u,gp[esp]+0x84u,4);
        gp[ecx] = gp[eax];
        set_low_byte(gp[eax],flags.right(low_byte(gp[eax]),6u,1,false));
        gp[ecx] = flags.right(gp[ecx],10u,4,false);
        this->push(0xd85b1u,0u);
        gp[ecx] = flags.logic(gp[ecx]&0xffffff01u,4);
        this->push(0xd85b9u,gp[ecx]);
        gp[esi] = flags.logic(0u,4);
        gp[eax] = flags.logic(gp[eax]&0xffffff01u,4);
        this->push(0xd85c1u,gp[eax]);
        this->call(0xd85c2u,0xd85c7u,0xde500u,[&](State &state) {
            try {
                // The unit's dispatcher identifies this genuine source site by its disc address.
                dispatch(state,0xde500u,0xd7672u,false);
            } catch (...) {
                gp = state.gp;
                flags = state.flags;
                throw;
            }
        });
        gp[ebx] = this->read(0xd85c7u,gp[esp]+0x14u,4);
    }

    void finish(Branch branch, Next resumed, std::uint32_t pc) {
        this->reached(branch);
        next = resumed;
        this->publish(pc);
    }

    void run() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        try {
            if (!has_response_flags()) {
                finish(unchanged,Next::keep_flags,0xd8594u);
                return;
            }
            gp[edx] = this->read(0xd8596u,0x4f3218u,4);
            flags.minus(this->read(0xd859cu,gp[edx]+0xcu,1),4u,1);
            if (flags.zf) {
                finish(mode_clears,Next::clear_flags,0xd85a0u);
                return;
            }
            dispatch_response();
            finish(dispatch_clears,Next::clear_flags,0xd85c7u);
        } catch (...) {
            this->s.gp = gp;
            this->s.flags = flags;
            throw;
        }
    }
};

template<class State, class Reference, class Dispatch>
Next bridge(State &state, Reference reference, Dispatch dispatch) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return Next::retained;
    static VerifySummary summary{"Damage response flag route"};
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_UPDATE_FLAG_ROUTE");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_UPDATE_FLAG_ROUTE");
    auto next = Next::retained;
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        if (reference(input) != next) throw std::runtime_error("Damage response route continuation differed");
    };
    run_owner(state,0xd8571u,0xd85c7u,reference_mode,verify_mode,coverage(),summary,verify_reference,
        [&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Dispatch>{value,cursor,trace,next,dispatch};
        });
    return next;
}
} // namespace halo2::native_damage_update_flag_route_detail
