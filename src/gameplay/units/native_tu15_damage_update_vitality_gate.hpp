#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_update_vitality_gate_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;

enum class Entry { clear, preserve };
enum class Next { retained, recovery, armor, shield };
enum Branch : unsigned {
    flags_cleared, scale_child, scale_default, skip_shield, shield_recovery, shield_scale, branch_count,
};
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Scale>
struct Frame : Port<State> {
    Entry entry;
    Next &next;
    Scale &scale;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &resume, Scale &route)
        : Port<State>(state,cursor,trace,{}), entry(start), next(resume), scale(route) {}

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc,address,4)));
    }

    void clear_flags() {
        const auto object = this->gp[ebp];
        auto &flags = this->flags;
        auto status = this->read(0xd85cbu,object+0x10au,1);
        this->store(0xd85cbu,object+0x10au,1,flags.logic(status&0x9fu,1));
        status = this->read(0xd85d2u,object+0x10bu,1);
        this->store(0xd85d2u,object+0x10bu,1,flags.logic(status&0xfbu,1));
        this->reached(flags_cleared);
    }

    void initialize_scale() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        gp[edx] = this->read(0xd85d9u,0x4f3218u,4);
        gp[esi] = this->read(0xd85dfu,gp[edx]+8u,4);
        flags.minus(gp[esi],2u,4);
        xmm[3] = scalar(0xd85e5u,0x46a0d0u);
        xmm[2] = xmm[3];
        this->store(0xd85f0u,gp[esp]+0x1cu,4,physics_step_detail::u(_mm_cvtss_f32(xmm[2])));
        if (!flags.zf) {
            this->reached(scale_default);
            return;
        }
        set_low_byte(gp[ecx],this->read(0xd85f8u,gp[ebp]+0xaau,1));
        gp[eax] = flags.shift(1u,low_byte(gp[ecx]),4);
        flags.logic(low_byte(gp[eax])&3u,1);
        if (flags.zf) {
            this->reached(scale_default);
            return;
        }
        gp[ecx] = this->read(0xd8609u,gp[ebp]+0x13cu,4);
        flags.minus(gp[ecx],0xffffffffu,4);
        if (flags.zf) {
            this->reached(scale_default);
            return;
        }
        this->push(0xd8614u,gp[esi]);
        this->call(0xd8615u,0xd861au,0x15b2d0u,[&](State &state) {
            try {
                scale(state);
            } catch (...) {
                gp = state.gp;
                flags = state.flags;
                throw;
            }
        });
        this->store(0xd861au,gp[esp]+0x1cu,4,physics_step_detail::u(_mm_cvtss_f32(xmm[0])));
        xmm[2] = xmm[0];
        this->reached(scale_child);
    }

    void finish(Branch branch, Next continuation, std::uint32_t pc) {
        this->reached(branch);
        next = continuation;
        this->publish(pc);
    }

    void select_vitality() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        gp[eax] = (gp[eax]&0xffff0000u) | this->read(0xd8623u,gp[ebp]+0x10au,2);
        set_low_byte(gp[ecx],low_byte(gp[eax]));
        set_low_byte(gp[ecx],flags.right(low_byte(gp[ecx]),2u,1,false));
        flags.logic(low_byte(gp[ecx])&1u,1);
        if (!flags.zf) {
            finish(skip_shield,Next::armor,0xd8632u);
            return;
        }
        xmm[0] = scalar(0xd8634u,gp[ebp]+0xe8u);
        flags.scalar_compare(xmm[0],scalar(0xd863cu,0x46a0e8u),false);
        if (flags.cf || flags.zf) {
            finish(skip_shield,Next::armor,0xd8643u);
            return;
        }
        set_low_byte(gp[eax],flags.right(low_byte(gp[eax]),4u,1,false));
        flags.logic(low_byte(gp[eax])&1u,1);
        if (flags.zf) {
            finish(shield_recovery,Next::recovery,0xd864au);
            return;
        }
        finish(shield_scale,Next::shield,0xd864au);
    }

    void run() {
        try {
            if (entry == Entry::clear) clear_flags();
            initialize_scale();
            select_vitality();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Scale>
Next bridge(State &state, Entry entry, Reference reference, Scale scale) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return Next::retained;
    static VerifySummary summary{"Damage vitality gate"};
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_VITALITY_GATE");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_VITALITY_GATE");
    const auto first = entry == Entry::clear ? 0xd85cbu : 0xd85d9u;
    auto next = Next::retained;
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        if (reference(input,entry) != next) throw std::runtime_error("Damage vitality gate continuation differed");
    };
    run_owner(state,first,0xd864au,reference_mode,verify_mode,coverage(),summary,verify_reference,
        [&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Scale>{value,cursor,trace,entry,next,scale};
        });
    return next;
}
} // namespace halo2::native_damage_update_vitality_gate_detail
