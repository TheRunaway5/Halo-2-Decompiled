#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_fog_cluster_sky_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t cluster_entry = 0x131c31u;
inline constexpr std::uint32_t cluster_exit = 0x131c83u;
inline constexpr std::uint32_t sky_entry = 0x1320e5u;
inline constexpr std::uint32_t sky_exit = 0x132181u;
inline constexpr std::uint32_t cluster_sky = 0x133a70u;

enum class Next : std::uint32_t {
    fallback,
    environment = 0x131c89u,
    sky = sky_entry,
    uncovered = 0x13208cu,
    project = 0x132183u,
    missing_sky = 0x132597u,
};

template<class State> struct Stage : Port<State> {
    using Base = Port<State>;
    Next &next;
    Stage(State &s, std::uint32_t *cursor, Trace<State> *trace, Next &result)
        : Base(s, cursor, trace, {}), next(result) {}
    void finish(Next destination, std::uint32_t pc, unsigned branch) {
        next = destination;
        this->reached(branch);
        this->publish(pc);
    }
    template<class Action> void with_fault_state(Action &&action) {
        try {
            action();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Child> struct Cluster : Stage<State> {
    Child &child;
    Cluster(State &s, std::uint32_t *cursor, Trace<State> *trace, Next &result, Child &route)
        : Stage<State>(s, cursor, trace, result), child(route) {}
    void run() {
        auto &gp = this->gp;
        this->with_fault_state([&] {
            gp[edi] = this->read(cluster_entry, gp[ebp] - 8u, 4);
            gp[eax] = this->read(0x131c34u, 0x4ecc18u, 4);
            gp[ecx] = this->read(0x131c39u, gp[eax] + 0xa0u, 4);
            gp[esi] = this->flags.product(gp[edi], 0xb0u);
            gp[esi] = this->flags.plus(gp[esi], gp[ecx], 4);
            gp[ebx] = gp[esp] + 0xbcu;
            gp[eax] = gp[esi];
            this->store(0x131c52u, gp[ebx], 4, 0xffffffffu);
            this->push(0x131c5du, 0x131c62u);
        });
        // A child fault leaves its own live register packet and cursor.
        this->enter(cluster_sky, child);
        this->with_fault_state([&] {
            set_low_byte(gp[ecx], this->read(0x131c62u, gp[esi] + 0x6fu, 1));
            this->s.xmm[7] = _mm_setzero_ps();
            gp[edx] = gp[eax];
            gp[eax] = this->flags.logic(0xffffffffu, 4);
            this->flags.minus(gp[ecx] & 0xffu, gp[eax] & 0xffu, 1);
            this->store(0x131c6fu, gp[esp] + 0x13u, 1, 0);
            this->store(0x131c74u, gp[ebp], 4, 0);
            this->store(0x131c7bu, gp[ebp] + 4u, 4, gp[eax]);
            this->store(0x131c7eu, gp[ebp] + 8u, 4, 0);
            const auto sky = this->flags.zf;
            this->finish(sky ? Next::sky : Next::environment, cluster_exit, unsigned(sky));
        });
    }
};

template<class State> struct Sky : Stage<State> {
    using Stage<State>::Stage;
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(this->read(pc, address, 4))));
    }
    bool greater(__m128 left, __m128 right) {
        this->flags.scalar_compare(left, right, false);
        return !this->flags.cf && !this->flags.zf;
    }
    void store_weight(std::uint32_t pc, __m128 value) {
        const auto bits = std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));
        this->store(pc, this->gp[esp] + 0xb8u, 4, bits);
    }
    void select_distance() {
        auto &gp = this->gp;
        auto &x = this->s.xmm;
        const auto distance = gp[esi] + 0x44u;
        x[0] = scalar(0x132101u, distance);
        if (!greater(x[0], x[7])) {
            this->reached(1);
            x[0] = scalar(0x132120u, 0x556e10u);
            return;
        }
        x[0] = scalar(0x13210bu, 0x46a110u);
        if (greater(x[0], scalar(0x132113u, distance))) {
            this->reached(2);
            x[0] = scalar(0x132119u, distance);
        } else {
            this->reached(3);
        }
    }
    void setup_geometry() {
        auto &gp = this->gp;
        gp[ecx] = this->read(0x132163u, gp[esp] + 0x4838u, 4);
        gp[eax] = gp[esp] + 0x130u;
        this->push(0x132171u, gp[eax]);
        this->push(0x132172u, gp[ecx]);
        gp[ebx] = gp[esp] + 0x118u;
        gp[edi] = gp[esp] + 0xf8u;
        gp[eax] = gp[edx];
        this->finish(Next::project, sky_exit, 7);
    }
    void classify() {
        auto &gp = this->gp;
        auto &x = this->s.xmm;
        this->compare32(gp[edx], gp[eax]);
        if (this->flags.zf) {
            this->finish(Next::missing_sky, 0x1320e7u, 0);
            return;
        }
        gp[ecx] = this->read(0x1320edu, 0x4f0414u, 4);
        gp[eax] = this->flags.logic(gp[edx] & 0xffffu, 4);
        gp[eax] = this->flags.shift(gp[eax], 4u, 4);
        gp[esi] = this->read(0x1320fdu, gp[eax] + gp[ecx] + 8u, 4);
        select_distance();
        x[1] = scalar(0x132128u, gp[ebp] - 4u);
        x[1] = _mm_div_ss(x[1], x[0]);
        x[0] = _mm_sub_ss(x[3], x[1]);
        if (greater(x[7], x[0])) {
            this->finish(Next::uncovered, 0x13213bu, 4);
            return;
        }
        if (greater(x[0], x[3])) {
            this->reached(5);
            store_weight(0x132146u, x[3]);
        } else {
            const auto covered = greater(x[0], x[7]);
            store_weight(0x132154u, x[0]);
            if (!covered) {
                this->finish(Next::uncovered, 0x13215du, 6);
                return;
            }
        }
        setup_geometry();
    }
    void run() { this->with_fault_state([&] { classify(); }); }
};

template<std::uint32_t Entry> inline auto &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}
template<std::uint32_t Entry> inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{
        Entry == cluster_entry ? "Campaign fog cluster and sky" : "Campaign fog sky projection gate"};
    return value;
}
template<std::uint32_t Entry, std::uint32_t Exit, class State, class Reference, class Make>
Next bridge(State &s, Reference reference, Make make) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_CLUSTER_SKY");
    static const bool verify = flag("HALO2_VERIFY_FOG_CLUSTER_SKY");
    if constexpr (Entry == sky_entry)
        if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
            return Next::fallback;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return Next::fallback;
    }
    Next next{};
    native_small_queries_detail::run_owner(s, Entry, Exit, force_reference, verify, coverage<Entry>(), summary<Entry>(),
        [&](State &v) {
            if (reference(v) != next)
                throw std::runtime_error("Campaign fog cluster sky continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) { return make(v, cursor, trace, next); });
    return next;
}
template<class State, class Reference, class Child> Next cluster(State &s, Reference reference, Child child) {
    return bridge<cluster_entry,cluster_exit>(s, reference, [&](auto &v, auto *cursor, auto *trace, Next &next) {
        return Cluster<State,Child>{v, cursor, trace, next, child};
    });
}
template<class State, class Reference> Next sky(State &s, Reference reference) {
    return bridge<sky_entry,sky_exit>(s, reference, [](auto &v, auto *cursor, auto *trace, Next &next) {
        return Sky<State>{v, cursor, trace, next};
    });
}

} // namespace halo2::native_fog_cluster_sky_detail
