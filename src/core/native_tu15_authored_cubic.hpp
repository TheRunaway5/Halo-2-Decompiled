#pragma once

#include "core/native_tu15_authored_cubic_systems.hpp"
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_authored_cubic {
using native_object_tick_detail::Trace;
using native_object_tick_detail::boundary::Port;
using native_object_tick_detail::boundary::eax;
using native_object_tick_detail::boundary::ebp;
using native_object_tick_detail::boundary::ebx;
using native_object_tick_detail::boundary::esp;
using native_object_tick_detail::boundary::set_low_byte;

enum class Continuation : std::uint32_t {
    retained, clamp = 0x13e0d2u, powered_input = 0x13e2b0u, polynomial = 0x13e3e8u,
    powered_layer = 0x13e43eu, blend = 0x13e582u
};
enum class Entry : std::uint32_t { input = 0x13e2a2u, base = 0x13e3e8u, refresh = 0x13e574u, blend = 0x13e582u };
enum Branch : unsigned { ordinary_input, powered_input, single, blended, powered_layer, refresh, reuse, branch_count };

inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

inline void count(Branch route) {
    struct Totals {
        std::array<std::atomic<std::uint64_t>, branch_count> routes{};
        ~Totals() {
            // A route identifies executed work even if the DATA jump table
            // or the kind byte selects a different case from its usual kind.
            constexpr std::array labels{"13E2A2->13E3E8", "13E2A2->13E2B0", "13E3E8->13E0D2",
                "13E3E8->13E582", "13E3E8->13E43E", "13E574->13E0D2", "13E582->13E0D2"};
            for (unsigned n = 0; n < routes.size(); ++n)
                if (auto value = routes[n].load())
                    std::fprintf(stderr, "Authored function cubic route %s: %llu\n", labels[n],
                        static_cast<unsigned long long>(value));
        }
    };
    static Totals totals;
    totals.routes[route].fetch_add(1, std::memory_order_relaxed);
}

template<class State>
struct Adapter : Port<State> {
    Entry entry;
    Continuation *continuation;

    Adapter(State &state, std::uint32_t *cursor, Trace<State> *trace, Entry part,
        Continuation *next = nullptr) : Port<State>{state, cursor, trace, {}}, entry(part), continuation(next) {}

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(this->read(pc, address, 4)));
    }
    __m128 base_quadratic() { return scalar(0x13e3e8u, this->gp[ebx] + 0x38u); }
    __m128 base_linear() { return scalar(0x13e3edu, this->gp[ebx] + 0x3cu); }
    __m128 base_cubic() { return scalar(0x13e40cu, this->gp[ebp]); }
    __m128 base_offset() { return scalar(0x13e420u, this->gp[ebp] + 0xcu); }
    __m128 second_quadratic() { return scalar(0x13e582u, this->gp[ebp] + 4u); }
    __m128 second_linear() { return scalar(0x13e58bu, this->gp[ebp] + 8u); }
    __m128 second_cubic() { return scalar(0x13e594u, this->gp[ebp]); }
    __m128 second_offset() { return scalar(0x13e5b3u, this->gp[ebp] + 0xcu); }
    __m128 unit() { return scalar(0x13e59du, 0x46a0d0u); }
    __m128 blend_amount() { return scalar(0x13e5adu, this->gp[esp] + 0x24u); }
    __m128 base_result() { return scalar(0x13e5bcu, this->gp[esp] + 0xcu); }

    bool open_base_layer() {
        const auto blob = this->gp[ebx];
        this->test8(this->read(0x13e3f2u, blob + 1u, 1) & 1u);
        this->gp[ebp] = blob + 0x34u;
        return !this->flags.zf;
    }
    void store_result(std::uint32_t pc, __m128 result) {
        this->store(pc, this->gp[esp] + 0xcu, 4, physics_step_detail::u(_mm_cvtss_f32(result)));
    }
    void store_base(__m128 base) { store_result(0x13e425u, base); }
    void store_blend(__m128 blend) { store_result(0x13e5cau, blend); }

    void open_blend_layer() {
        auto &gp = this->gp;
        const auto kind = this->read(0x13e431u, gp[ebx], 1);
        set_low_byte(gp[eax], kind);
        gp[ebp] = this->flags.plus(gp[ebp], 0x30u, 4);
        this->flags.minus(kind, 10u, 1);
        const auto powered = this->flags.zf;
        this->reached(powered ? powered_layer : blended);
        *continuation = powered ? Continuation::powered_layer : Continuation::blend;
        this->publish(0x13e438u);
    }

    void open_input() {
        // AL is the kind already loaded by the original entry. The compare
        // precedes the argument read, including on a faulting stack page.
        this->flags.minus(this->gp[eax] & 0xffu, 10u, 1);
        this->s.xmm[0] = scalar(0x13e2a4u, this->gp[esp] + 0x20u);
        const bool powered = this->flags.zf;
        this->reached(powered ? powered_input : ordinary_input);
        *continuation = powered ? Continuation::powered_input : Continuation::polynomial;
        this->publish(0x13e2aau);
    }

    void run() {
        auto &x = this->s.xmm;
        const systems::Workspace work{x[0], x[1], x[2], x[3], x[4]};
        try {
            if (entry == Entry::input) {
                open_input();
                return;
            }
            if (entry == Entry::base) {
                if (systems::evaluate_base(*this, work)) {
                    open_blend_layer();
                } else {
                    this->reached(single);
                    *continuation = Continuation::clamp;
                    this->publish(0x13e42bu);
                }
                return;
            }
            if (entry == Entry::refresh) {
                this->reached(refresh);
                systems::refresh_powers(work);
            } else this->reached(reuse);
            systems::evaluate_blend(*this, work);
            this->publish(0x13e5d0u);
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Authored function cubic"};
    return value;
}
inline bool reference_mode() {
    return native_small_queries_detail::flag("HALO2_REFERENCE_AUTHORED_FUNCTION_BOUNDED") ||
        native_small_queries_detail::flag("HALO2_REFERENCE_AUTHORED_CUBIC");
}
inline bool verify_mode() { return native_small_queries_detail::flag("HALO2_VERIFY_AUTHORED_CUBIC"); }

// After the first polynomial, the original may recompute the blend curve's
// parameter via real control-point and power owners, then rejoin at 13E574.
template<class State, class Reference>
Continuation begin(State &state, Entry entry, Reference reference) {
    static const bool fallback = reference_mode(), verify = verify_mode();
    auto continuation = Continuation::retained;
    auto replay = [&](State &original) {
        if (reference(original) != continuation)
            throw std::runtime_error("Authored function cubic continuation differs from original");
    };
    const auto last = entry == Entry::input ? 0x13e2aau : 0x13e438u;
    native_small_queries_detail::run_owner(state, std::uint32_t(entry), last,
        fallback, verify, coverage(), summary(), replay,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State>{value, cursor, trace, entry, &continuation};
        });
    if (verify || native_census_detail::enabled()) {
        switch (continuation) {
        case Continuation::retained: break;
        case Continuation::clamp: count(single); break;
        case Continuation::polynomial: count(ordinary_input); break;
        case Continuation::powered_input: count(powered_input); break;
        case Continuation::powered_layer: count(powered_layer); break;
        case Continuation::blend: count(blended); break;
        }
    }
    return continuation;
}

template<class State, class Reference>
bool blend(State &state, Entry entry, Reference reference) {
    static const bool fallback = reference_mode(), verify = verify_mode();
    const bool admitted = native_small_queries_detail::run_owner(state, std::uint32_t(entry), 0x13e5d0u,
        fallback, verify, coverage(), summary(), reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Adapter<State>{value, cursor, trace, entry};
        });
    if (admitted && (verify || native_census_detail::enabled())) count(entry == Entry::refresh ? refresh : reuse);
    return admitted;
}
} // namespace halo2::native_authored_cubic
