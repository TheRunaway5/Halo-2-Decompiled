#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_update_amounts_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;

enum class Entry : unsigned { refill, clear, armor, defer };
enum class Next { retained, notify, base_armor, section_armor };
enum Branch : unsigned { saturated, deferred, no_armor, no_description, with_description, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Multiply, class Add>
struct Frame : Port<State> {
    Entry entry;
    Next &next;
    Multiply &multiply;
    Add &add;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &resume,
          Multiply &product, Add &sum)
        : Port<State>(state,cursor,trace,{}), entry(start), next(resume), multiply(product), add(sum) {}

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc,address,4)));
    }

    void clear_pending() {
        const auto address = this->gp[ebp]+0x10bu;
        const auto status = this->read(0xd869au,address,1);
        this->store(0xd869au,address,1,this->flags.logic(status&0xfdu,1));
    }

    void mark_pending() {
        const auto address = this->gp[ebp]+0x10bu;
        const auto status = this->read(0xd86e4u,address,1);
        this->store(0xd86e4u,address,1,this->flags.logic(status|2u,1));
        this->store(0xd86ebu,this->gp[esp]+0x13u,1,1u);
        this->reached(deferred);
    }

    void refill_shield() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        gp[edx] = this->read(0xd8650u,0x51df54u,4);
        const auto ticks = static_cast<std::uint16_t>(this->read(0xd8656u,gp[edx]+2u,2));
        gp[eax] = static_cast<std::uint32_t>(std::bit_cast<std::int16_t>(ticks));
        xmm[0] = scalar(0xd865au,0x46a11cu);
        xmm[1] = _mm_cvtsi32_ss(xmm[1],std::bit_cast<std::int32_t>(gp[eax]));
        xmm[1] = multiply(xmm[1],xmm[0]);
        xmm[0] = _mm_div_ss(xmm[0],xmm[1]);
        xmm[0] = add(xmm[0],scalar(0xd866eu,gp[ebp]+0xf0u));
        xmm[1] = scalar(0xd8676u,0x46a0e4u);
        flags.scalar_compare(xmm[0],xmm[1],false);
        this->store(0xd8681u,gp[ebp]+0xf0u,4,physics_step_detail::u(_mm_cvtss_f32(xmm[0])));
        if (flags.cf) {
            mark_pending();
            return;
        }
        this->reached(saturated);
        this->store(0xd868bu,gp[ebp]+0xf0u,4,physics_step_detail::u(_mm_cvtss_f32(xmm[1])));
        const auto status = this->read(0xd8693u,gp[ebp]+0x10au,1);
        this->store(0xd8693u,gp[ebp]+0x10au,1,flags.logic(status&0xefu,1));
        clear_pending();
    }

    void finish(Branch branch, Next continuation, std::uint32_t pc) {
        this->reached(branch);
        next = continuation;
        this->publish(pc);
    }

    void select_armor() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        xmm[0] = scalar(0xd86a1u,gp[ebp]+0xe4u);
        xmm[1] = _mm_setzero_ps();
        flags.scalar_compare(xmm[0],xmm[1],false);
        if (flags.cf || flags.zf) {
            finish(no_armor,Next::notify,0xd86afu);
            return;
        }
        set_low_byte(gp[edx],this->read(0xd86b5u,gp[ebp]+0x10au,1));
        set_low_byte(gp[edx],flags.right(low_byte(gp[edx]),2u,1,false));
        flags.logic(low_byte(gp[edx])&1u,1);
        if (!flags.zf) {
            finish(no_armor,Next::notify,0xd86c1u);
            return;
        }
        flags.logic(gp[ebx],4);
        if (flags.zf) {
            finish(no_description,Next::base_armor,0xd86c9u);
            return;
        }
        xmm[2] = scalar(0xd86cfu,gp[ebx]+0x14u);
        xmm[0] = scalar(0xd86d4u,gp[ebx]+0x68u);
        this->store(0xd86d9u,gp[esp]+0x1cu,4,physics_step_detail::u(_mm_cvtss_f32(xmm[2])));
        finish(with_description,Next::section_armor,0xd86dfu);
    }

    void run() {
        try {
            switch (entry) {
            case Entry::refill: refill_shield(); break;
            case Entry::clear: clear_pending(); break;
            case Entry::armor: break;
            case Entry::defer: mark_pending(); break;
            }
            select_armor();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Multiply, class Add>
Next bridge(State &state, Entry entry, Reference reference, Multiply multiply, Add add) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return Next::retained;
    static VerifySummary summary{"Damage shield refill and armor input"};
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_UPDATE_AMOUNTS");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_UPDATE_AMOUNTS");
    auto next = Next::retained;
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        if (reference(input,entry) != next) throw std::runtime_error("Damage shield or armor continuation differed");
    };
    run_owner(state,0xd8650u,0xd86f0u,reference_mode,verify_mode,
        coverage(),summary,verify_reference,[&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Multiply,Add>{value,cursor,trace,entry,next,multiply,add};
        });
    return next;
}
} // namespace halo2::native_damage_update_amounts_detail
