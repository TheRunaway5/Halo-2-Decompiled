#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_response_amount_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
enum class Entry : std::uint32_t { counter = 0xd88f9u, load = 0xd892eu, compare = 0xd8945u };
enum class Next : unsigned { retained, continue_flags, depletion };

template<class State, class Check, class Difficulty, class Notify, class Multiply, class Add>
struct Frame : Port<State> {
    Next &next;
    Entry entry;
    Check &check;
    Difficulty &difficulty;
    Notify &notify;
    Multiply &multiply;
    Add &add;
    Frame(State &s, std::uint32_t *cursor, Trace<State> *trace, Next &result, Entry start,
          Check &limit, Difficulty &scale, Notify &publish, Multiply &mul, Add &sum)
        : Port<State>(s,cursor,trace,{}), next(result), entry(start), check(limit), difficulty(scale),
          notify(publish), multiply(mul), add(sum) {}
    template<class Action> void guarded(Action &&action) {
        try {
            action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
    auto &x() { return this->s.xmm; }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(this->read(pc,address,4))));
    }
    static std::uint32_t bits(__m128 value) {
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));
    }
    bool above(__m128 left, __m128 right) {
        this->flags.scalar_compare(left,right,false);
        return !this->flags.cf && !this->flags.zf;
    }
    void finish(Next destination, std::uint32_t pc) {
        this->publish(pc);
        next = destination;
    }
    void change_flags(std::uint32_t pc, std::uint32_t keep, std::uint32_t set = 0) {
        const auto address = this->gp[ebp] + 0x10bu;
        const auto value = this->flags.logic((this->read(pc,address,1) & keep) | set,1);
        this->store(pc,address,1,value);
    }
    void changed(std::uint32_t pc) {
        this->store(pc,this->gp[esp] + 0x13u,1,1u);
    }
    void call_check(std::uint32_t pc, std::uint32_t resume) {
        guarded([&] { this->push(pc,resume); });
        this->enter(0x13b360u,check);
        this->test8(this->gp[eax]);
    }
    void counter() {
        auto &gp = this->gp;
        call_check(0xd88f9u,0xd88feu);
        guarded([&] {
            if (this->flags.zf) {
                this->flags.minus(gp[esi] & 0xffffu,0x7fffu,2);
                if (!this->flags.zf) {
                    this->reached(0);
                    gp[esi] = this->flags.increment(gp[esi],4,true);
                    this->store(0xd890au,gp[ebp] + 0x104u,2,gp[esi] & 0xffffu);
                    changed(0xd8911u);
                }
            }
            change_flags(0xd8916u,0xfdu);
            this->push(0xd891du,0x80u);
            gp[eax] = gp[edi];
            this->push(0xd8924u,0xd8929u);
        });
        this->enter(0xb1c30u,notify);
        finish(Next::continue_flags,0xd8929u);
    }
    void load_threshold() {
        auto &gp = this->gp;
        gp[eax] = this->read(0xd892eu,gp[esp] + 0x18u,4);
        x()[2] = scalar(0xd8932u,gp[eax] + 0x38u);
        x()[0] = scalar(0xd8937u,gp[eax] + 0xd4u);
        this->store(0xd893fu,gp[esp] + 0x1cu,4,bits(x()[2]));
        this->reached(1);
    }
    void accumulate() {
        auto &gp = this->gp;
        bool scale_by_type{};
        guarded([&] {
            set_low_byte(gp[ecx],this->read(0xd8967u,gp[ebp] + 0xaau,1));
            gp[eax] = this->read(0xd896du,0x51df54u,4);
            x()[1] = scalar(0xd8972u,gp[eax] + 4u);
            gp[edx] = this->flags.shift(1u,gp[ecx] & 0xffu,4);
            x()[1] = multiply(x()[1],x()[0]);
            this->store(0xd8982u,gp[esp] + 0x20u,4,bits(x()[1]));
            this->flags.logic(gp[edx] & 3u,1);
            scale_by_type = !this->flags.zf;
            if (scale_by_type) {
                gp[ecx] = this->flags.logic(0u,4);
                gp[ecx] = this->read(0xd898fu,gp[ebp] + 0x138u,2);
                gp[eax] = 3u;
                this->push(0xd899bu,0xd89a0u);
            }
        });
        if (scale_by_type) {
            this->reached(6);
            this->enter(0x1f19d0u,difficulty);
            guarded([&] {
                x()[0] = multiply(x()[0],scalar(0xd89a0u,gp[esp] + 0x20u));
                x()[2] = scalar(0xd89a6u,gp[esp] + 0x1cu);
                this->store(0xd89acu,gp[esp] + 0x20u,4,bits(x()[0]));
                x()[1] = x()[0];
            });
        }
        guarded([&] {
            change_flags(0xd89b5u,0xffu,1u);
            x()[0] = scalar(0xd89bcu,gp[ebp] + 0xecu);
            x()[0] = add(x()[0],x()[1]);
            const bool exceeds = above(x()[0],x()[2]);
            this->store(0xd89cbu,gp[ebp] + 0xecu,4,bits(x()[0]));
            if (exceeds) {
                this->reached(7);
                this->store(0xd89d5u,gp[ebp] + 0xecu,4,bits(x()[2]));
                change_flags(0xd89ddu,0xfeu);
            } else {
                this->reached(8);
                changed(0xd89f7u);
            }
        });
    }
    void notify_depletion() {
        auto &gp = this->gp;
        guarded([&] {
            this->push(0xd89fcu,0x40u);
            gp[eax] = gp[edi];
            this->push(0xd8a00u,0xd8a05u);
        });
        this->enter(0xb1c30u,notify);
        x()[1] = _mm_setzero_ps();
        finish(Next::depletion,0xd8a05u);
    }
    void gate() {
        auto &gp = this->gp;
        bool passes{}, counting{};
        guarded([&] {
            if (!above(x()[2],scalar(0xd8945u,gp[ebp] + 0xecu))) {
                this->reached(2);
                finish(Next::depletion,0xd894cu);
                return;
            }
            if (!above(x()[0],x()[1])) {
                this->reached(3);
                finish(Next::depletion,0xd8955u);
                return;
            }
            gp[esi] = (gp[esi] & 0xffff0000u) | this->read(0xd895bu,gp[ebp] + 0x106u,2);
            counting = this->flags.logic(gp[esi] & 0xffffu,2) != 0;
            passes = true;
        });
        if (!passes)
            return;
        if (counting) {
            this->reached(4);
            call_check(0xd89e6u,0xd89ebu);
            if (this->flags.zf)
                guarded([&] {
                    this->reached(9);
                    gp[esi] = this->flags.increment(gp[esi],4,true);
                    this->store(0xd89f0u,gp[ebp] + 0x106u,2,gp[esi] & 0xffffu);
                    changed(0xd89f7u);
                });
        } else {
            this->reached(5);
            accumulate();
        }
        notify_depletion();
    }
    void run() {
        if (entry == Entry::counter) {
            counter();
            return;
        }
        if (entry == Entry::load)
            guarded([&] { load_threshold(); });
        gate();
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Damage response amount"};
    return value;
}
template<class State, class Reference, class Check, class Difficulty, class Notify, class Multiply, class Add>
Next bridge(State &s, Entry entry, Reference reference, Check check, Difficulty difficulty,
            Notify notify, Multiply multiply, Add add) {
    using native_small_queries_detail::flag;
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || !s.selected_observation_pcs)
        return Next::retained;
    if (entry != Entry::counter && (_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return Next::retained;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return Next::retained;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_DAMAGE_RESPONSE_AMOUNT");
    static const bool verify = flag("HALO2_VERIFY_DAMAGE_RESPONSE_AMOUNT");
    Next next{};
    const auto last = entry == Entry::counter ? 0xd8929u : 0xd8a05u;
    native_small_queries_detail::run_owner(s,std::uint32_t(entry),last,force_reference,verify,coverage(),summary(),
        [&](State &v) {
            v.selected_observation_pcs = true;
            if (reference(v,entry) != next)
                throw std::runtime_error("Damage response amount continuation differs");
        }, [&](State &v,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Check,Difficulty,Notify,Multiply,Add>{
                v,cursor,trace,next,entry,check,difficulty,notify,multiply,add};
        });
    return next;
}

} // namespace halo2::native_damage_response_amount_detail
