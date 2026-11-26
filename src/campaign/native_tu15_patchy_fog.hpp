#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_patchy_fog_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0x1342b0u;
inline constexpr std::uint32_t last = 0x1343a6u;

struct Sample {
    Register source;
    std::uint32_t offset, load_pc, sum_pc, store_pc;
};
// Two triples supplied in registers, then four scalars supplied on the stack.
// The first component places the weight in the multiply destination. The
// remaining nine components multiply with the sample as destination.
inline constexpr std::array samples{
    Sample{edx, 0, 0x1342d0u, 0x1342d4u, 0x1342d8u},
    Sample{edx, 4, 0x1342dcu, 0x1342e5u, 0x1342eau},
    Sample{edx, 8, 0x1342efu, 0x1342f8u, 0x1342fdu},
    Sample{ecx, 0, 0x134302u, 0x13430au, 0x13430fu},
    Sample{ecx, 4, 0x134314u, 0x13431du, 0x134322u},
    Sample{ecx, 8, 0x134327u, 0x134330u, 0x134335u},
    Sample{esp, 4, 0x13433au, 0x134344u, 0x134349u},
    Sample{esp, 8, 0x13434eu, 0x134358u, 0x13435du},
    Sample{esp, 12, 0x134362u, 0x13436cu, 0x134371u},
    Sample{esp, 16, 0x134376u, 0x134380u, 0x134385u},
};

template<class State> struct Accumulator : Port<State> {
    using Base = Port<State>;
    Accumulator(State &s, std::uint32_t *cursor, Trace<State> *trace) : Base(s, cursor, trace, {}) {}
    auto &x() { return this->s.xmm; }
    __m128 load_scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(
            std::bit_cast<std::int32_t>(this->read(pc, address, 4))));
    }
    void output(std::uint32_t pc, std::uint32_t address, __m128 value) {
        this->store(pc, address, 4, std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value))));
    }
    void clamp_weight() {
        x()[2] = _mm_setzero_ps();
        this->flags.scalar_compare(x()[2], x()[0], false);
        x()[1] = load_scalar(0x1342b6u, 0x46a0d0u);
        if (!this->flags.cf && !this->flags.zf) {
            x()[0] = x()[2];
            this->reached(0);
        } else {
            this->flags.scalar_compare(x()[0], x()[1], false);
            if (!this->flags.cf && !this->flags.zf) {
                x()[0] = x()[1];
                this->reached(1);
            } else {
                this->reached(2);
            }
        }
    }
    void add_sample(const Sample &sample, std::uint32_t output_address) {
        x()[2] = native_world_leaves_detail::ordered_add_ss(x()[2], load_scalar(sample.sum_pc, output_address));
        output(sample.store_pc, output_address, x()[2]);
    }
    void blend() {
        const auto base = this->gp[eax];
        clamp_weight();
        const auto &first_sample = samples.front();
        x()[2] = x()[0];
        x()[2] = native_tu15_leaves::ordered_mul_ss(x()[2], load_scalar(first_sample.load_pc, this->gp[edx]));
        add_sample(first_sample, base);
        for (std::size_t n = 1; n < samples.size(); ++n) {
            const auto &sample = samples[n];
            x()[2] = load_scalar(sample.load_pc, this->gp[sample.source] + sample.offset);
            x()[2] = native_tu15_leaves::ordered_mul_ss(x()[2], x()[0]);
            add_sample(sample, base + std::uint32_t(n * 4));
        }
        x()[2] = load_scalar(0x13438au, base + 0x28u);
        x()[1] = _mm_sub_ss(x()[1], x()[0]);
        x()[1] = native_tu15_leaves::ordered_mul_ss(x()[1], load_scalar(0x134393u, base + 0x2cu));
        x()[2] = native_world_leaves_detail::ordered_add_ss(x()[2], x()[0]);
        output(0x13439cu, base + 0x28u, x()[2]);
        output(0x1343a1u, base + 0x2cu, x()[1]);
        this->ret(last, 16);
    }
    void run() {
        try {
            blend();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
        this->publish(last);
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Patchy fog accumulation"};
    return value;
}

template<class State, class Reference> bool bridge(State &s, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_PATCHY_FOG");
    static const bool verify = flag("HALO2_VERIFY_PATCHY_FOG");
    if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    return native_small_queries_detail::run_owner(s, first, last, force_reference, verify, coverage(), summary(),
        reference, [](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Accumulator<State>{v, cursor, trace};
        });
}

} // namespace halo2::native_patchy_fog_detail
