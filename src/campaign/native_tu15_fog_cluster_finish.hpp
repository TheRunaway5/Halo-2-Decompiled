#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_fog_cluster_finish_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0x131fc7u;
inline constexpr std::uint32_t last = 0x1320e0u;
enum class Entry : std::uint32_t { bloom = first, recorded = 0x132084u, uncovered = 0x13208cu };
enum class Next : std::uint32_t { fallback, another_cluster = 0x131c31u, resolve = 0x13262bu };

template<class State> struct Finish : Port<State> {
    using Base = Port<State>;
    Entry entry;
    Next &next;
    Finish(State &s, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &result)
        : Base(s, cursor, trace, {}), entry(start), next(result) {}
    auto &x() { return this->s.xmm; }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        const auto word = std::bit_cast<std::int32_t>(this->read(pc, address, 4));
        return _mm_castsi128_ps(_mm_cvtsi32_si128(word));
    }
    __m128 local(std::uint32_t pc, std::uint32_t offset) {
        return scalar(pc, this->gp[esp] + offset);
    }
    void store_bits(std::uint32_t pc, std::uint32_t address, __m128 value) {
        this->store(pc, address, 4, std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value))));
    }
    void store_local(std::uint32_t pc, std::uint32_t offset, unsigned lane) {
        store_bits(pc, this->gp[esp] + offset, x()[lane]);
    }
    void fold(unsigned lane, std::uint32_t pc, std::uint32_t offset) {
        x()[lane] = native_tu15_leaves::ordered_mul_ss(x()[lane], x()[0]);
        x()[lane] = native_world_leaves_detail::ordered_add_ss(x()[lane], local(pc, offset));
    }
    bool greater(__m128 a, __m128 b) {
        this->flags.scalar_compare(a, b, false);
        return !this->flags.cf && !this->flags.zf;
    }
    void bloom() {
        const auto profile = this->gp[ebx];
        x()[0] = scalar(0x131fc7u, profile + 0xe0u);
        if (greater(x()[0], x()[7])) {
            this->reached(3);
            const bool capped = greater(x()[4], x()[3]);
            x()[1] = scalar(0x131fdbu, profile + 0xecu);
            x()[2] = scalar(0x131fe3u, profile + 0xe8u);
            x()[5] = scalar(0x131febu, profile + 0xe4u);
            x()[6] = scalar(0x131ff3u, profile + 0xe0u);
            x()[0] = capped ? x()[3] : x()[4];
            this->reached(capped ? 5 : 6);
            // UCOMISS followed by LAHF and TEST AH,44h determines the
            // authored fourth bloom component. AH remains caller-visible.
            this->flags.scalar_compare(x()[1], x()[7], true);
            auto &eax_word = this->gp[eax];
            const auto ah = (this->flags.bits() | 2u) & 0xffu;
            eax_word = (eax_word & 0xffff00ffu) | (ah << 8);
            this->flags.logic(ah & 0x44u, 1);
            if (!this->flags.pf) {
                x()[1] = x()[3];
                this->reached(7);
            } else {
                this->reached(8);
            }
            fold(1, 0x132013u, 0x60);
            store_local(0x132019u, 0x60, 1);
            x()[1] = x()[0];
            x()[1] = native_world_leaves_detail::ordered_add_ss(x()[1], local(0x132022u, 0x64));
            store_local(0x132028u, 0x64, 1);
            x()[1] = x()[3];
            fold(6, 0x132035u, 0x54);
            fold(5, 0x13203fu, 0x58);
            fold(2, 0x132049u, 0x5c);
            x()[1] = _mm_sub_ss(x()[1], x()[0]);
            x()[1] = native_tu15_leaves::ordered_mul_ss(x()[1], local(0x132053u, 0x68));
            store_local(0x132059u, 0x54, 6);
            store_local(0x13205fu, 0x58, 5);
            store_local(0x132065u, 0x5c, 2);
            store_local(0x13206bu, 0x68, 1);
        } else {
            this->reached(4);
        }
        this->gp[edx] = native_object_tick_detail::sx8(this->read(0x132071u, this->gp[esi] + 0x6fu, 1));
        this->store(0x132075u, this->gp[ebp], 4, 2);
        this->store(0x13207cu, this->gp[ebp] + 4, 4, this->gp[edx]);
        store_bits(0x13207fu, this->gp[ebp] + 8, x()[4]);
    }
    void uncovered_weight() {
        x()[1] = scalar(0x13208cu, this->gp[ebp] - 4u);
        x()[1] = _mm_div_ss(x()[1], scalar(0x132091u, 0x556e10u));
        x()[0] = _mm_sub_ss(x()[3], x()[1]);
        if (greater(x()[7], x()[0])) {
            x()[0] = x()[7];
            this->reached(11);
        } else {
            this->reached(12);
        }
        x()[0] = native_world_leaves_detail::ordered_add_ss(x()[0], local(0x1320a8u, 0xe8));
        store_local(0x1320b1u, 0xe8, 0);
    }
    void cluster_loop() {
        auto &gp = this->gp;
        gp[eax] = this->read(0x1320bau, gp[esp] + 0xb4u, 4);
        gp[ebp] = this->flags.plus(gp[ebp], 0x18u, 4);
        gp[eax] = this->flags.increment(gp[eax], 4, true);
        this->store(0x1320c5u, gp[esp] + 0xb4u, 4, gp[eax]);
        if (!this->flags.zf) {
            next = Next::another_cluster;
            this->reached(13);
            this->publish(0x1320ccu);
            return;
        }
        gp[ebp] = this->read(0x1320d2u, gp[esp] + 0xb0u, 4);
        gp[esi] = this->read(0x1320d9u, gp[esp] + 0x483cu, 4);
        next = Next::resolve;
        this->reached(14);
        this->publish(last);
    }
    void run() {
        try {
            this->reached(entry == Entry::bloom ? 0 : entry == Entry::recorded ? 1 : 2);
            if (entry == Entry::bloom)
                bloom();
            if (entry == Entry::uncovered) {
                uncovered_weight();
            } else {
                set_low_byte(this->gp[eax], this->read(0x132084u, this->gp[esp] + 0x13u, 1));
                this->test8(this->gp[eax]);
                const bool uncovered = this->flags.zf;
                this->reached(uncovered ? 9 : 10);
                if (uncovered)
                    uncovered_weight();
            }
            cluster_loop();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign fog cluster completion"};
    return value;
}

template<class State, class Reference> Next bridge(State &s, Entry start, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_CLUSTER_FINISH");
    static const bool verify = flag("HALO2_VERIFY_FOG_CLUSTER_FINISH");
    if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return Next::fallback;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return Next::fallback;
    }
    Next next{};
    native_small_queries_detail::run_owner(s, first, last, force_reference, verify, coverage(), summary(),
        [&](State &v) {
            if (reference(v, start) != next)
                throw std::runtime_error("Campaign fog cluster continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Finish<State>{v, cursor, trace, start, next};
        });
    return next;
}

} // namespace halo2::native_fog_cluster_finish_detail
