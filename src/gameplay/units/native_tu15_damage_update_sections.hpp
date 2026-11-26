#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2::native_damage_update_sections_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;

enum class Entry { pending, depletion };
enum class Next { retained, countdown, validation, notify };
enum Branch : unsigned {
    pending_more, pending_complete, depletion_empty, depletion_countdown,
    depletion_below_threshold, depletion_partial, depletion_complete, depletion_repeat, branch_count,
};

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Multiply>
struct Frame : Port<State> {
    Entry entry;
    Next &next;
    Multiply &multiply;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &resume, Multiply &mul)
        : Port<State>(state,cursor,trace,{}), entry(start), next(resume), multiply(mul) {}

    void advance_pending() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[ecx] = this->read(0xd84dcu,gp[esp]+0x1cu,4);
        gp[eax] = this->read(0xd84e0u,gp[esp]+0x24u,4);
        gp[ebx] = flags.increment(gp[ebx],4,false);
        gp[ecx] = flags.plus(gp[ecx],8u,4);
        flags.minus(gp[ebx],gp[eax],4);
        this->store(0xd84eau,gp[esp]+0x1cu,4,gp[ecx]);
        if (flags.sf != flags.of) {
            this->reached(pending_more);
            next = Next::countdown;
        } else {
            this->reached(pending_complete);
            next = Next::validation;
        }
        this->publish(0xd84eeu);
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc,address,4)));
    }

    void apply_damage() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        gp[edx] = this->read(0xd8a87u,0x51df54u,4);
        xmm[2] = scalar(0xd8a8du,gp[edx]+4u);
        const auto scale = scalar(0xd8a92u,gp[ecx]+0x2cu);
        xmm[2] = multiply(xmm[2],scale);
        xmm[2] = multiply(xmm[2],xmm[0]);
        this->store(0xd8a9bu,gp[esp]+0x24u,4,physics_step_detail::u(_mm_cvtss_f32(xmm[2])));

        this->s.require_x87();
        const auto product = this->read(0xd8aa1u,gp[esp]+0x24u,4);
        auto &stack = this->s.fp;
        stack.push(physics_step_detail::ext(physics_step_detail::f(product)));
        this->s.require_x87();
        this->at(0xd8aa5u);
        const auto points = physics_contact_runtime_detail::integer_spill(stack.st(0),stack.status);
        this->store(0xd8aa5u,gp[esp]+0x1cu,4,points);
        stack.pop();

        set_low_byte(gp[ecx],this->read(0xd8aa9u,gp[eax],1));
        gp[edx] = this->read(0xd8aabu,gp[esp]+0x1cu,4);
        gp[ebx] = low_byte(gp[ecx]);
        flags.minus(gp[ebx],gp[edx],4);
        if (flags.zf || flags.sf != flags.of) {
            this->reached(depletion_complete);
            this->store(0xd8abcu,gp[eax],1,0u);
            return;
        }
        this->reached(depletion_partial);
        set_low_byte(gp[ecx],flags.minus(low_byte(gp[ecx]),low_byte(gp[edx]),1));
        this->store(0xd8ab8u,gp[eax],1,low_byte(gp[ecx]));
        this->store(0xd8ac6u,gp[esp]+0x13u,1,1u);
    }

    void update_depletion() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        flags.minus(this->read(0xd8a56u,gp[eax],1),0u,1);
        if (flags.cf || flags.zf) {
            this->reached(depletion_empty);
            return;
        }
        gp[ecx] = (gp[ecx]&0xffff0000u) | this->read(0xd8a5bu,gp[eax]+4u,2);
        flags.logic(gp[ecx]&0xffffu,2);
        bool countdown = !flags.zf;
        if (!countdown) {
            gp[edx] = this->read(0xd8a64u,gp[esp]+0x18u,4);
            flags.minus(this->read(0xd8a68u,gp[edx]+0xbcu,4),gp[esi],4);
            countdown = flags.zf || flags.sf != flags.of;
        }
        if (countdown) {
            this->reached(depletion_countdown);
            gp[ecx] = flags.increment(gp[ecx],4,true);
            this->store(0xd8ac2u,gp[eax]+4u,2,gp[ecx]);
            this->store(0xd8ac6u,gp[esp]+0x13u,1,1u);
            return;
        }

        gp[ecx] = this->read(0xd8a70u,gp[edx]+0xc0u,4);
        gp[edx] = this->read(0xd8a76u,gp[esp]+0x20u,4);
        auto &xmm = this->s.xmm;
        xmm[2] = scalar(0xd8a7au,gp[ecx]+gp[edx]+0x2cu);
        gp[ecx] = flags.plus(gp[ecx],gp[edx],4);
        flags.scalar_compare(xmm[2],xmm[1],false);
        if (flags.cf || flags.zf) {
            this->reached(depletion_below_threshold);
            return;
        }
        apply_damage();
    }

    void deplete_sections() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        for (;;) {
            update_depletion();
            gp[ebx] = this->read(0xd8acbu,gp[esp]+0x20u,4);
            gp[esi] = flags.increment(gp[esi],4,false);
            gp[ebx] = flags.plus(gp[ebx],0x38u,4);
            gp[eax] = flags.plus(gp[eax],8u,4);
            flags.minus(gp[esi],gp[edi],4);
            this->store(0xd8ad8u,gp[esp]+0x20u,4,gp[ebx]);
            if (flags.sf == flags.of) break;
            this->reached(depletion_repeat);
        }
        this->publish(0xd8adcu);
        next = Next::notify;
    }

    void run() {
        try {
            if (entry == Entry::pending) advance_pending();
            else deplete_sections();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Multiply>
Next bridge(State &state, Entry entry, Reference reference, Multiply multiply) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return Next::retained;
    static VerifySummary summary{"Damage section update"};
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_UPDATE_SECTIONS");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_UPDATE_SECTIONS");
    const auto first = entry == Entry::pending ? 0xd84dcu : 0xd8a56u;
    const auto last = entry == Entry::pending ? 0xd84eeu : 0xd8adcu;
    auto next = Next::retained;
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        if (reference(input,entry) != next) throw std::runtime_error("Damage section continuation differed");
    };
    run_owner(state,first,last,reference_mode,verify_mode,coverage(),summary,verify_reference,
        [&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Multiply>{value,cursor,trace,entry,next,multiply};
        });
    return next;
}
} // namespace halo2::native_damage_update_sections_detail
