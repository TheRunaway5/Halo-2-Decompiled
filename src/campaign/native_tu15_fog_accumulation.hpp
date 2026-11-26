#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_fog_accumulation_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0x131d38u;
inline constexpr std::uint32_t last = 0x131f7eu;
enum class Next : std::uint32_t { fallback, patchy_fog = 0x131f80u, sky_and_bloom = 0x131fc7u, no_weight = 0x13208cu };

struct Field {
    unsigned lane;
    std::uint32_t offset, pc;
};

template<class State> struct Accumulation : Port<State> {
    using Base = Port<State>;
    using Base::load;
    Next &next;
    Accumulation(State &s, std::uint32_t *cursor, Trace<State> *trace, Next &result)
        : Base(s, cursor, trace, {}), next(result) {}
    auto &x() { return this->s.xmm; }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        const auto bits = std::bit_cast<std::int32_t>(this->read(pc, address, 4));
        return _mm_castsi128_ps(_mm_cvtsi32_si128(bits));
    }
    __m128 local(std::uint32_t pc, std::uint32_t offset) {
        return scalar(pc, this->gp[esp] + offset);
    }
    void load(Field field) {
        x()[field.lane] = scalar(field.pc, this->gp[ebx] + field.offset);
    }
    void write(Field field) {
        const auto bits = std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[field.lane])));
        this->store(field.pc, this->gp[esp] + field.offset, 4, bits);
    }
    void accumulate(Field field) {
        auto &value = x()[field.lane];
        value = native_tu15_leaves::ordered_mul_ss(value, x()[0]);
        value = native_world_leaves_detail::ordered_add_ss(value, local(field.pc, field.offset));
    }
    void add_weight(Field field) {
        x()[field.lane] = x()[0];
        x()[field.lane] = native_world_leaves_detail::ordered_add_ss(x()[field.lane], local(field.pc, field.offset));
    }
    void attenuate(Field field) {
        x()[field.lane] = _mm_sub_ss(x()[3], x()[0]);
        x()[field.lane] = native_tu15_leaves::ordered_mul_ss(x()[field.lane], local(field.pc, field.offset));
    }
    bool greater(__m128 a, __m128 b) {
        this->flags.scalar_compare(a, b, false);
        return !this->flags.cf && !this->flags.zf;
    }
    void weight() { x()[0] = (!this->flags.cf && !this->flags.zf) ? x()[3] : x()[4]; }

    void primary() {
        this->reached(greater(x()[4], x()[3]) ? 1 : 2);
        load({1, 0x20, 0x131d44u});
        load({2, 0x1c, 0x131d49u});
        load({5, 0x18, 0x131d4eu});
        weight();
        greater(x()[4], x()[3]);
        load({6, 0x04, 0x131d5eu});
        accumulate({6, 0x34, 0x131d67u});
        accumulate({1, 0x48, 0x131d71u});
        write({6, 0x34, 0x131d77u});
        load({6, 0x08, 0x131d7du});
        write({1, 0x48, 0x131d82u});
        accumulate({6, 0x38, 0x131d8cu});
        add_weight({1, 0x4c, 0x131d95u});
        write({1, 0x4c, 0x131d9bu});
        write({6, 0x38, 0x131da1u});
        load({6, 0x0c, 0x131da7u});
        accumulate({5, 0x40, 0x131db0u});
        accumulate({2, 0x44, 0x131dbau});
        attenuate({1, 0x50, 0x131dc7u});
        accumulate({6, 0x3c, 0x131dd1u});
        write({5, 0x40, 0x131dd7u});
        load({5, 0x34, 0x131dddu});
        write({2, 0x44, 0x131de2u});
        load({2, 0x38, 0x131de8u});
        write({1, 0x50, 0x131dedu});
        load({1, 0x3c, 0x131df3u});
        write({6, 0x3c, 0x131df8u});
    }
    void secondary() {
        weight();
        greater(x()[4], x()[3]);
        accumulate({1, 0x28, 0x131e0du});
        write({1, 0x28, 0x131e13u});
        x()[6] = x()[0];
        x()[6] = native_tu15_leaves::ordered_mul_ss(x()[6], scalar(0x131e1cu, this->gp[ebx] + 0x24u));
        x()[6] = native_world_leaves_detail::ordered_add_ss(x()[6], local(0x131e21u, 0x14u));
        write({6, 0x14, 0x131e27u});
        load({6, 0x28, 0x131e2du});
        accumulate({6, 0x18, 0x131e36u});
        add_weight({1, 0x2c, 0x131e3fu});
        write({1, 0x2c, 0x131e45u});
        write({6, 0x18, 0x131e4bu});
        load({6, 0x2c, 0x131e51u});
        accumulate({5, 0x20, 0x131e5au});
        accumulate({2, 0x24, 0x131e64u});
        attenuate({1, 0x30, 0x131e71u});
        accumulate({6, 0x1c, 0x131e7bu});
        write({5, 0x20, 0x131e81u});
        load({5, 0x50, 0x131e87u});
        write({2, 0x24, 0x131e8cu});
        load({2, 0x58, 0x131e92u});
        write({1, 0x30, 0x131e97u});
        load({1, 0x54, 0x131e9du});
        write({6, 0x1c, 0x131ea2u});
    }
    bool auxiliary() {
        weight();
        load({6, 0x44, 0x131eb0u});
        accumulate({6, 0x70, 0x131eb9u});
        accumulate({1, 0x84, 0x131ec3u});
        write({6, 0x70, 0x131eccu});
        load({6, 0x48, 0x131ed2u});
        write({1, 0x84, 0x131ed7u});
        accumulate({6, 0x74, 0x131ee4u});
        add_weight({1, 0x88, 0x131eedu});
        write({6, 0x74, 0x131ef6u});
        load({6, 0x4c, 0x131efcu});
        write({1, 0x88, 0x131f01u});
        accumulate({6, 0x78, 0x131f0eu});
        accumulate({5, 0x7c, 0x131f18u});
        accumulate({2, 0x80, 0x131f22u});
        x()[1] = _mm_sub_ss(x()[3], x()[0]);
        load({0, 0xa4, 0x131f32u});
        const bool candidate = greater(x()[0], x()[7]);
        x()[1] = native_tu15_leaves::ordered_mul_ss(x()[1], local(0x131f3du, 0x8cu));
        write({6, 0x78, 0x131f46u});
        write({5, 0x7c, 0x131f4cu});
        write({2, 0x80, 0x131f52u});
        write({1, 0x8c, 0x131f5bu});
        return candidate;
    }
    void finish(Next destination, std::uint32_t pc, unsigned branch) {
        next = destination;
        this->reached(branch);
        this->publish(pc);
    }
    void blend() {
        if (!greater(x()[4], x()[7])) {
            finish(Next::no_weight, 0x131d3bu, 0);
            return;
        }
        primary();
        secondary();
        if (!auxiliary()) {
            finish(Next::sky_and_bloom, 0x131f64u, 3);
            return;
        }
        this->gp[edi] = this->read(0x131f66u, this->gp[ebx] + 0xd4u, 4);
        this->compare32(this->gp[edi], this->gp[eax]);
        this->gp[edx] = this->gp[ebx] + 0x88u;
        if (this->flags.zf) {
            finish(Next::sky_and_bloom, 0x131f74u, 4);
            return;
        }
        x()[0] = scalar(0x131f76u, this->gp[edx] + 0x1cu);
        if (!greater(x()[0], x()[7])) {
            finish(Next::sky_and_bloom, last, 5);
            return;
        }
        finish(Next::patchy_fog, last, 6);
    }
    void run() {
        try {
            blend();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign fog accumulation"};
    return value;
}

template<class State, class Reference> Next bridge(State &s, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_ACCUMULATION");
    static const bool verify = flag("HALO2_VERIFY_FOG_ACCUMULATION");
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
            if (reference(v) != next)
                throw std::runtime_error("Campaign fog accumulation continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Accumulation<State>{v, cursor, trace, next};
        });
    return next;
}

} // namespace halo2::native_fog_accumulation_detail
