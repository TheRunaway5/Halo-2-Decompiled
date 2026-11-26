#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_due_response_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0xd842cu;
inline constexpr std::uint32_t last = 0xd84d7u;
inline constexpr std::uint32_t continuation = 0xd84dcu;
inline constexpr std::uint32_t check_target = 0xdc4c0u;
inline constexpr std::uint32_t apply_target = 0xdc660u;
inline constexpr std::uint32_t regions_target = 0xde1d0u;
inline constexpr std::uint32_t notify_target = 0xd9110u;
inline constexpr std::uint32_t notify_site = 0xd7587u;

template<class State, class Check, class Apply, class Regions, class Dispatch>
struct Frame : Port<State> {
    Check &check;
    Apply &apply;
    Regions &regions;
    Dispatch &dispatch;
    Frame(State &s, std::uint32_t *cursor, Trace<State> *trace, Check &eligible, Apply &update,
          Regions &mark_regions, Dispatch &route)
        : Port<State>(s,cursor,trace,{}), check(eligible), apply(update), regions(mark_regions), dispatch(route) {}
    template<class Action> void guarded(Action &&action) {
        try {
            action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
    std::uint32_t local(std::uint32_t pc, std::uint32_t offset, unsigned width = 4) {
        return this->read(pc, this->gp[esp] + offset, width);
    }
    void save_local(std::uint32_t pc, std::uint32_t offset, std::uint32_t value, unsigned width = 4) {
        this->store(pc, this->gp[esp] + offset, width, value);
    }
    void eligibility_arguments() {
        auto &gp = this->gp;
        gp[edx] = this->read(first, 0x4ecbd0u, 4);
        gp[ecx] = this->read(0xd8432u, gp[edx] + 0x44u, 4);
        gp[edx] = local(0xd8435u, 0x20u);
        gp[ecx] = this->read(0xd8439u, gp[ecx] + gp[edx] + 8u, 4);
        gp[edx] = this->read(0xd843du, gp[ecx] + 0xc8u, 4);
        save_local(0xd8443u, 0x2cu, gp[edx]);
        gp[edx] = this->read(0xd8447u, gp[ecx] + 0xc4u, 4);
        save_local(0xd844du, 0x28u, gp[edx]);
        gp[ecx] = (gp[ecx] & 0xffff0000u) | this->read(0xd8451u, gp[ecx] + 0xc2u, 2);
        gp[edx] = local(0xd8458u, 0x84u);
        gp[esi] = gp[eax];
        save_local(0xd8461u, 0x30u, gp[ecx] & 0xffffu, 2);
        gp[esi] = this->flags.logic(gp[esi] & 7u, 4);
        this->push(0xd8469u, gp[edx]);
        gp[edx] = local(0xd846au, 0x1cu);
        gp[ecx] = gp[esi];
        gp[eax] = gp[ebx];
        this->push(0xd8472u, 0xd8477u);
    }
    void update_arguments() {
        auto &gp = this->gp;
        gp[edx] = local(0xd847bu, 0x84u);
        gp[eax] = this->flags.logic(0u, 4);
        gp[ecx] = 13u;
        gp[edi] = gp[esp] + 0x4cu;
        const auto reverse = (this->s.other_flags & 0x400u) != 0;
        const auto stride = reverse ? 0xfffffffcu : 4u;
        this->reached(reverse ? 1 : 2);
        while (gp[ecx]) {
            this->store(0xd848du, gp[edi], 4, gp[eax]);
            gp[edi] += stride;
            --gp[ecx];
        }
        gp[eax] = gp[esp] + 0x4cu;
        this->push(0xd8493u, gp[eax]);
        gp[eax] = local(0xd8494u, 0x1cu);
        this->push(0xd8498u, gp[ebx]);
        gp[ecx] = gp[esp] + 0x30u;
        this->push(0xd849du, gp[ecx]);
        this->push(0xd849eu, gp[edx]);
        this->push(0xd849fu, gp[eax]);
        gp[eax] = gp[esi];
        this->push(0xd84a2u, 0xd84a7u);
    }
    bool region_arguments() {
        auto &gp = this->gp;
        gp[eax] = local(0xd84a7u, 0x78u);
        this->flags.logic(gp[eax] & 0xffffu, 2);
        gp[ecx] = local(0xd84aeu, 0x7au);
        if (this->flags.zf) {
            this->flags.logic(gp[ecx] & 0xffffu, 2);
            if (this->flags.zf) {
                this->reached(3);
                return false;
            }
            this->reached(4);
        } else {
            this->reached(5);
        }
        this->push(0xd84b9u, gp[ecx]);
        gp[ecx] = local(0xd84bau, 0x88u);
        this->push(0xd84c1u, gp[eax]);
        this->push(0xd84c2u, gp[ecx]);
        this->push(0xd84c3u, 0xd84c8u);
        return true;
    }
    void run() {
        auto &gp = this->gp;
        guarded([&] { eligibility_arguments(); });
        this->enter(check_target, check);
        this->test8(gp[eax]);
        if (this->flags.zf) {
            this->reached(0);
            this->publish(0xd8479u);
            return;
        }
        guarded([&] { update_arguments(); });
        this->enter(apply_target, apply);
        bool mark_regions{};
        guarded([&] { mark_regions = region_arguments(); });
        if (mark_regions)
            this->enter(regions_target, regions);
        bool notify{};
        guarded([&] {
            this->test8(local(0xd84c8u, 0x7cu, 1) & 4u);
            notify = !this->flags.zf;
            if (notify) {
                gp[edx] = local(0xd84cfu, 0x84u);
                this->push(0xd84d6u, gp[edx]);
                this->push(last, continuation);
            }
        });
        if (!notify) {
            this->reached(6);
            this->publish(0xd84cdu);
            return;
        }
        this->reached(7);
        // Dispatch owns the live registers and cursor until the next parent instruction.
        this->enter(notify_target, [&](State &v) { dispatch(v, notify_target, notify_site, false); });
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Damage due response"};
    return value;
}
template<class State, class Reference, class Check, class Apply, class Regions, class Dispatch>
bool bridge(State &s, Reference reference, Check check, Apply apply, Regions regions, Dispatch dispatch) {
    using native_small_queries_detail::flag;
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || !s.selected_observation_pcs)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_DAMAGE_DUE_RESPONSE");
    static const bool verify = flag("HALO2_VERIFY_DAMAGE_DUE_RESPONSE");
    return native_small_queries_detail::run_owner(s, first, last, force_reference, verify, coverage(), summary(),
        [&](State &v) {
            v.selected_observation_pcs = true;
            reference(v);
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State,Check,Apply,Regions,Dispatch>{v,cursor,trace,check,apply,regions,dispatch};
        });
}

} // namespace halo2::native_damage_due_response_detail
