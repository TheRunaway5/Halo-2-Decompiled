#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_descriptor_refresh_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0xd84f4u;
inline constexpr std::uint32_t last = 0xd856au;
inline constexpr std::uint32_t continuation = 0xd8571u;

template<class State, class Lookup, class Prepare, class Apply>
struct Frame : Port<State> {
    using Base = Port<State>;
    Lookup &lookup;
    Prepare &prepare;
    Apply &apply;
    Frame(State &s, std::uint32_t *cursor, Trace<State> *trace, Lookup &find, Prepare &build, Apply &refresh)
        : Base(s,cursor,trace,{}), lookup(find), prepare(build), apply(refresh) {}

    template<class Action> decltype(auto) guarded(Action &&action) {
        try {
            return action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
    bool descriptor_enabled() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        set_low_byte(gp[ecx], this->read(first, gp[ebp] + 0xaau, 1));
        gp[eax] = flags.shift(1u, gp[ecx] & 0xffu, 4);
        flags.logic(gp[eax] & 3u, 1);
        if (flags.zf) {
            this->reached(0);
            return false;
        }
        gp[eax] = this->read(0xd8505u, gp[ebp] + 0x12cu, 4);
        this->compare32(gp[eax], 0xffffffffu);
        if (flags.zf) {
            this->reached(1);
            return false;
        }
        gp[ecx] = this->read(0xd8510u, 0x5020c8u, 4);
        gp[edx] = this->read(0xd8516u, gp[ecx] + 0x44u, 4);
        gp[eax] = flags.logic(gp[eax] & 0xffffu, 4);
        gp[eax] = flags.product(gp[eax], 0x888u);
        gp[ecx] = this->read(0xd8524u, gp[eax] + gp[edx] + 0x54u, 4);
        this->reached(2);
        return true;
    }
    bool pending_refresh() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[edi] = this->read(0xd8539u, gp[esp] + 0x84u, 4);
        gp[eax] = flags.logic(0u, 4);
        gp[eax] = this->read(0xd8542u, gp[ebp] + 0x10au, 2);
        gp[eax] = flags.right(gp[eax], 13u, 4, false);
        flags.logic(gp[eax] & 1u, 1);
        this->reached(flags.zf ? 3 : 4);
        return !flags.zf;
    }
    void run() {
        auto &gp = this->gp;
        if (guarded([&] { return descriptor_enabled(); })) {
            guarded([&] { this->push(0xd8528u, 0xd852du); });
            this->enter(0x1eccc0u, lookup);
            guarded([&] {
                gp[ebx] = gp[eax];
                this->store(0xd852fu, gp[esp] + 0x14u, 4, gp[ebx]);
            });
        } else {
            guarded([&] { gp[ebx] = this->read(0xd8535u, gp[esp] + 0x14u, 4); });
        }
        if (!guarded([&] { return pending_refresh(); })) {
            this->publish(0xd854eu);
            return;
        }
        guarded([&] {
            gp[ecx] = gp[esp] + 0x34u;
            this->push(0xd8554u, gp[ecx]);
            this->push(0xd8555u, gp[edi]);
            this->push(0xd8556u, 0xd855bu);
        });
        // Each original child owns its live state when it returns or faults.
        this->enter(0xd8c20u, prepare);
        guarded([&] {
            this->push(0xd855bu, 0u);
            this->push(0xd855du, 1u);
            gp[edx] = gp[esp] + 0x3cu;
            this->push(0xd8563u, gp[edx]);
            this->push(0xd8564u, gp[edi]);
            this->push(0xd8565u, last);
        });
        this->enter(0xd8d50u, apply);
        guarded([&] {
            const auto address = gp[ebp] + 0x10bu;
            const auto result = this->flags.logic(this->read(last, address, 1) & 0xdfu, 1);
            this->store(last, address, 1, result);
        });
        this->publish(last);
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Damage descriptor refresh"};
    return value;
}
template<class State, class Reference, class Lookup, class Prepare, class Apply>
bool bridge(State &s, Reference reference, Lookup lookup, Prepare prepare, Apply apply) {
    using native_small_queries_detail::flag;
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || !s.selected_observation_pcs)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    static const bool force_reference = flag("HALO2_REFERENCE_DAMAGE_DESCRIPTOR_REFRESH");
    static const bool verify = flag("HALO2_VERIFY_DAMAGE_DESCRIPTOR_REFRESH");
    return native_small_queries_detail::run_owner(s, first, last, force_reference, verify, coverage(), summary(),
        [&](State &v) {
            v.selected_observation_pcs = true;
            reference(v);
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State,Lookup,Prepare,Apply>{v,cursor,trace,lookup,prepare,apply};
        });
}

} // namespace halo2::native_damage_descriptor_refresh_detail
