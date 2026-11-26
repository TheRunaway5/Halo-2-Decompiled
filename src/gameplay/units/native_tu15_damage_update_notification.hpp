#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_update_notification_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;

enum class Entry { prepare, notify };
enum class Next { retained, deplete, notify, saved_notification, updated };
enum Branch : unsigned { no_depletion, deplete_sections, repeat_notification, no_reserve, consume_reserve, branch_count };

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Notify, class Add>
struct Frame : Port<State> {
    Entry entry;
    Next &next;
    Notify &notify;
    Add &add;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &resume,
          Notify &callback, Add &sum)
        : Port<State>(state,cursor,trace,{}), entry(start), next(resume), notify(callback), add(sum) {}

    void finish(Branch branch, Next continuation, std::uint32_t pc) {
        this->reached(branch);
        next = continuation;
        this->publish(pc);
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc,address,4)));
    }

    std::uint32_t section_word(std::uint32_t pc, std::uint32_t address) {
        const auto word = static_cast<std::uint16_t>(this->read(pc,address,2));
        return static_cast<std::uint32_t>(std::bit_cast<std::int16_t>(word));
    }

    void prepare_depletion() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        auto &xmm = this->s.xmm;
        xmm[0] = scalar(0xd8a08u,gp[ebp]+0xe4u);
        flags.scalar_compare(xmm[0],xmm[1],false);
        if (flags.cf || flags.zf) {
            finish(no_depletion,Next::notify,0xd8a13u);
            return;
        }
        set_low_byte(gp[eax],this->read(0xd8a19u,gp[ebp]+0x10au,1));
        set_low_byte(gp[eax],flags.right(low_byte(gp[eax]),2u,1,false));
        flags.logic(low_byte(gp[eax])&1u,1);
        if (!flags.zf) {
            finish(no_depletion,Next::notify,0xd8a24u);
            return;
        }
        gp[edi] = section_word(0xd8a2au,gp[ebp]+0x120u);
        gp[eax] = section_word(0xd8a31u,gp[ebp]+0x122u);
        gp[edi] = flags.right(gp[edi],3u,4,false);
        gp[esi] = flags.logic(0u,4);
        gp[eax] = flags.plus(gp[eax],gp[ebp],4);
        flags.minus(gp[edi],gp[esi],4);
        if (flags.zf || flags.sf != flags.of) {
            finish(no_depletion,Next::notify,0xd8a41u);
            return;
        }
        xmm[0] = scalar(0xd8a47u,0x46a148u);
        this->store(0xd8a4fu,gp[esp]+0x20u,4,gp[esi]);
        gp[eax] = flags.plus(gp[eax],2u,4);
        finish(deplete_sections,Next::deplete,0xd8a53u);
    }

    void notify_damage() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        set_low_byte(gp[eax],this->read(0xd8ae2u,gp[ebp]+0x10au,1));
        gp[edx] = this->read(0xd8ae8u,gp[esp]+0x14u,4);
        set_low_byte(gp[eax],flags.right(low_byte(gp[eax]),2u,1,false));
        set_low_byte(gp[eax],flags.logic(low_byte(gp[eax])&1u,1));
        this->store(0xd8af1u,gp[esp]+0x24u,1,low_byte(gp[eax]));
        gp[ecx] = this->read(0xd8af5u,gp[esp]+0x24u,4);
        gp[esi] = gp[ebp]+0x109u;
        gp[ebx] = gp[ebp]+0x100u;
        gp[edi] = gp[ebp]+0xf8u;
        this->push(0xd8b0bu,gp[ecx]);
        this->call(0xd8b0cu,0xd8b11u,0xdb0a0u,[&](State &state) {
            try {
                notify(state);
            } catch (...) {
                gp = state.gp;
                flags = state.flags;
                throw;
            }
        });
        set_low_byte(gp[ebx],this->read(0xd8b11u,gp[esp]+0x13u,1));
        set_low_byte(gp[ebx],flags.logic((gp[ebx]|gp[eax])&0xffu,1));
        gp[eax] = flags.logic(0u,4);
        gp[eax] = this->read(0xd8b19u,gp[ebp]+0x10au,2);
        this->store(0xd8b20u,gp[esp]+0x13u,1,low_byte(gp[ebx]));
        gp[edx] = flags.right(gp[eax],11u,4,false);
        flags.logic(low_byte(gp[edx])&1u,1);
        if (flags.zf) {
            finish(repeat_notification,Next::saved_notification,0xd8b2cu);
            return;
        }

        set_low_byte(gp[eax],this->read(0xd8b2eu,gp[ebp]+0x108u,1));
        flags.logic(low_byte(gp[eax]),1);
        if (flags.zf || flags.sf != flags.of) {
            finish(no_reserve,Next::updated,0xd8b36u);
            return;
        }
        auto &xmm = this->s.xmm;
        xmm[0] = scalar(0xd8b38u,gp[ebp]+0xfcu);
        xmm[0] = add(xmm[0],scalar(0xd8b40u,gp[ebp]+0xf4u));
        set_low_byte(gp[eax],flags.increment(low_byte(gp[eax]),1,true));
        this->store(0xd8b4au,gp[ebp]+0x108u,1,low_byte(gp[eax]));
        this->store(0xd8b50u,gp[ebp]+0xf4u,4,physics_step_detail::u(_mm_cvtss_f32(xmm[0])));
        finish(consume_reserve,Next::updated,0xd8b50u);
    }

    void run() {
        try {
            if (entry == Entry::prepare) prepare_depletion();
            else notify_damage();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Notify, class Add>
Next bridge(State &state, Entry entry, Reference reference, Notify notify, Add add) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return Next::retained;
    static VerifySummary summary{"Damage section notification"};
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_UPDATE_NOTIFICATION");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_UPDATE_NOTIFICATION");
    const auto first = entry == Entry::prepare ? 0xd8a08u : 0xd8ae2u;
    const auto last = entry == Entry::prepare ? 0xd8a53u : 0xd8b50u;
    auto next = Next::retained;
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        if (reference(input,entry) != next) throw std::runtime_error("Damage notification continuation differed");
    };
    run_owner(state,first,last,reference_mode,verify_mode,coverage(),summary,verify_reference,
        [&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Notify,Add>{value,cursor,trace,entry,next,notify,add};
        });
    return next;
}
} // namespace halo2::native_damage_update_notification_detail
