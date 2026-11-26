#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_response_secondary_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
enum class Entry : std::uint32_t { load = 0xd87ceu, scaled = 0xd87dau };
inline constexpr auto last = 0xd88f4u;

template<class State, class Difficulty, class Prepare, class Direct, class Notify, class Dispatch, class Multiply, class Add>
struct Frame : Port<State> {
    Entry entry;
    Difficulty &difficulty;
    Prepare &prepare;
    Direct &direct;
    Notify &notify;
    Dispatch &dispatch;
    Multiply &multiply;
    Add &add;
    Frame(State &s, std::uint32_t *cursor, Trace<State> *trace, Entry start, Difficulty &scale,
          Prepare &build, Direct &trigger, Notify &publish, Dispatch &route, Multiply &mul, Add &sum)
        : Port<State>(s,cursor,trace,{}), entry(start), difficulty(scale), prepare(build), direct(trigger),
          notify(publish), dispatch(route), multiply(mul), add(sum) {}
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
    std::uint32_t local(std::uint32_t pc, std::uint32_t offset) {
        return this->read(pc,this->gp[esp] + offset,4);
    }
    void save_local(std::uint32_t pc, std::uint32_t offset, std::uint32_t value, unsigned width = 4) {
        this->store(pc,this->gp[esp] + offset,width,value);
    }
    void change_flags(std::uint32_t pc, std::uint32_t offset, std::uint32_t keep, std::uint32_t set = 0) {
        const auto address = this->gp[ebp] + offset;
        const auto value = this->flags.logic((this->read(pc,address,1) & keep) | set,1);
        this->store(pc,address,1,value);
    }
    void scale() {
        auto &gp = this->gp;
        bool scale_by_type{};
        guarded([&] {
            if (entry == Entry::load) {
                this->reached(0);
                gp[eax] = local(0xd87ceu,0x18u);
                x()[0] = scalar(0xd87d2u,gp[eax] + 0xd0u);
            }
            gp[ecx] = this->read(0xd87dau,0x51df54u,4);
            x()[1] = scalar(0xd87e0u,gp[ecx] + 4u);
            set_low_byte(gp[ecx],this->read(0xd87e5u,gp[ebp] + 0xaau,1));
            gp[edx] = this->flags.shift(1u,gp[ecx] & 0xffu,4);
            x()[1] = multiply(x()[1],x()[0]);
            save_local(0xd87f6u,0x20u,bits(x()[1]));
            this->flags.logic(gp[edx] & 3u,1);
            scale_by_type = !this->flags.zf;
            if (scale_by_type) {
                gp[ecx] = this->flags.logic(0u,4);
                gp[ecx] = this->read(0xd8803u,gp[ebp] + 0x138u,2);
                gp[eax] = 3u;
                this->push(0xd880fu,0xd8814u);
            }
        });
        if (!scale_by_type)
            return;
        this->reached(1);
        this->enter(0x1f19d0u,difficulty);
        guarded([&] {
            x()[0] = multiply(x()[0],scalar(0xd8814u,gp[esp] + 0x20u));
            x()[2] = scalar(0xd881au,gp[esp] + 0x1cu);
            save_local(0xd8820u,0x20u,bits(x()[0]));
        });
    }
    void maybe_dispatch_reset() {
        auto &gp = this->gp;
        bool reset{};
        guarded([&] {
            set_low_byte(gp[eax],this->read(0xd8826u,gp[ebp] + 0x10au,1));
            set_low_byte(gp[eax],this->flags.right(gp[eax] & 0xffu,3u,1,false));
            this->flags.logic(gp[eax] & 1u,1);
            reset = !this->flags.zf;
            if (reset) {
                change_flags(0xd8833u,0x10au,0xf7u);
                this->push(0xd883au,0u);
                this->push(0xd883cu,2u);
                this->push(0xd883eu,0xffffffffu);
                this->push(0xd8840u,gp[edi]);
                this->push(0xd8841u,0xd8846u);
            }
        });
        if (!reset)
            return;
        this->reached(2);
        this->enter(0xbc820u,[&](State &v) { dispatch(v,0xbc820u,0xd78f1u,false); });
        guarded([&] { x()[2] = scalar(0xd8846u,gp[esp] + 0x1cu); });
    }
    void maybe_apply_response() {
        auto &gp = this->gp;
        bool apply{};
        guarded([&] {
            gp[ecx] = this->flags.logic(0u,4);
            gp[ecx] = this->read(0xd884eu,gp[ebp] + 0x10au,2);
            gp[ecx] = this->flags.right(gp[ecx],9u,4,false);
            this->flags.logic(gp[ecx] & 1u,1);
            apply = this->flags.zf;
            if (apply) {
                gp[esi] = local(0xd885du,0x84u);
                gp[edx] = gp[esp] + 0x40u;
                this->push(0xd8868u,gp[edx]);
                this->push(0xd8869u,gp[esi]);
                this->push(0xd886au,0xd886fu);
            }
        });
        if (!apply) {
            this->reached(3);
            return;
        }
        this->enter(0xd8c20u,prepare);
        guarded([&] {
            gp[eax] = local(0xd886fu,0x18u);
            gp[ecx] = this->read(0xd8873u,gp[eax] + 0xb8u,4);
            x()[0] = _mm_setzero_ps();
            this->push(0xd887cu,0u);
            this->push(0xd887eu,0u);
            this->push(0xd8880u,0u);
            this->push(0xd8882u,gp[ecx]);
            gp[edx] = gp[esi];
            gp[ebx] = gp[esp] + 0x50u;
            this->push(0xd8889u,0xd888eu);
        });
        // The selected owner originally chooses the required service when
        // installed and the unit's direct child otherwise.
        const bool requires_owner = bool(this->s.services.required_owner);
        this->reached(requires_owner ? 4 : 5);
        this->enter(0x179610u,[&](State &v) {
            if (requires_owner)
                v.required(0xd8889u,0x179610u,false);
            else
                direct(v);
        });
        guarded([&] {
            x()[2] = scalar(0xd888eu,gp[esp] + 0x1cu);
            gp[ebx] = local(0xd8894u,0x14u);
            gp[edi] = gp[esi];
        });
    }
    void merge_and_notify() {
        auto &gp = this->gp;
        bool capped{};
        guarded([&] {
            change_flags(0xd889au,0x10bu,0xffu,2u);
            x()[0] = scalar(0xd88a1u,gp[ebp] + 0xf0u);
            x()[0] = add(x()[0],scalar(0xd88a9u,gp[esp] + 0x20u));
            this->flags.scalar_compare(x()[0],x()[2],false);
            capped = !this->flags.cf && !this->flags.zf;
            this->store(0xd88b2u,gp[ebp] + 0xf0u,4,bits(x()[0]));
            if (capped) {
                this->store(0xd88bcu,gp[ebp] + 0xf0u,4,bits(x()[2]));
                change_flags(0xd88c4u,0x10bu,0xfdu);
                change_flags(0xd88cbu,0x10au,0xfdu);
                this->push(0xd88d2u,0x80u);
                gp[eax] = gp[edi];
                this->push(0xd88d9u,0xd88deu);
            } else {
                this->push(0xd88e3u,0x80u);
                gp[eax] = gp[edi];
                save_local(0xd88eau,0x17u,1u,1);
                this->push(0xd88efu,0xd88f4u);
            }
        });
        this->reached(capped ? 6 : 7);
        this->enter(0xb1c30u,notify);
        this->publish(capped ? 0xd88deu : last);
    }
    void run() {
        scale();
        maybe_dispatch_reset();
        maybe_apply_response();
        merge_and_notify();
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Damage secondary response"};
    return value;
}
template<class State, class Reference, class Difficulty, class Prepare, class Direct, class Notify, class Dispatch,
         class Multiply, class Add>
bool bridge(State &s, Entry entry, Reference reference, Difficulty difficulty, Prepare prepare, Direct direct,
            Notify notify, Dispatch dispatch, Multiply multiply, Add add) {
    using native_small_queries_detail::flag;
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || !s.selected_observation_pcs ||
        (_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_DAMAGE_RESPONSE_SECONDARY");
    static const bool verify = flag("HALO2_VERIFY_DAMAGE_RESPONSE_SECONDARY");
    return native_small_queries_detail::run_owner(s,std::uint32_t(entry),last,force_reference,verify,coverage(),summary(),
        [&](State &v) { v.selected_observation_pcs = true; reference(v,entry); },
        [&](State &v,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Difficulty,Prepare,Direct,Notify,Dispatch,Multiply,Add>{
                v,cursor,trace,entry,difficulty,prepare,direct,notify,dispatch,multiply,add};
        });
}

} // namespace halo2::native_damage_response_secondary_detail
