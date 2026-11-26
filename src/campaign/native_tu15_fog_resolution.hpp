#pragma once

#include "campaign/native_tu15_fog_resolution_systems.hpp"
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_fog_resolution_detail {
using native_fog_resolution::Blend;
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0x13262bu;
inline constexpr std::uint32_t last = 0x1329a6u;

struct Local {
    std::uint32_t pc, offset;
};
struct Field {
    Local local;
    std::uint32_t store_pc, output;
};
struct Weight {
    Local divisor, coverage;
    unsigned divisor_lane, coverage_lane;
};
inline constexpr std::array weights{
    Weight{{0x13262bu, 0x4c}, {0x132636u, 0x40}, 1, 2},
    Weight{{0x13269fu, 0x2c}, {0x1326aau, 0x20}, 1, 2},
    Weight{{0x132713u, 0x88}, {0x132725u, 0x7c}, 1, 2},
    Weight{{0x1327a6u, 0xa8}, {0x1327b4u, 0x9c}, 1, 2},
    Weight{{0x132811u, 0xe8}, {0x132823u, 0xdc}, 2, 1},
};
struct SixFields {
    std::array<Field,3> color;
    std::uint32_t density_pc, density_output;
    std::array<Field,2> distance;
};
inline constexpr std::array six_fields{
    SixFields{{{{{0x132648u, 0x34}, 0x132652u, 0x1c},
                {{0x132657u, 0x38}, 0x132661u, 0x20},
                {{0x132666u, 0x3c}, 0x132670u, 0x24}}},
              0x13267cu, 0x28,
              {{{{0x132681u, 0x44}, 0x13268bu, 0x2c}, {{0x132690u, 0x48}, 0x13269au, 0x30}}}},
    SixFields{{{{{0x1326bcu, 0x14}, 0x1326c6u, 0x34},
                {{0x1326cbu, 0x18}, 0x1326d5u, 0x38},
                {{0x1326dau, 0x1c}, 0x1326e4u, 0x3c}}},
              0x1326f0u, 0x40,
              {{{{0x1326f5u, 0x24}, 0x1326ffu, 0x44}, {{0x132704u, 0x28}, 0x13270eu, 0x48}}}},
    SixFields{{{{{0x132737u, 0x70}, 0x132741u, 0xc8},
                {{0x132749u, 0x74}, 0x132753u, 0xcc},
                {{0x13275bu, 0x78}, 0x132765u, 0xd0}}},
              0x132774u, 0xd4,
              {{{{0x13277cu, 0x80}, 0x132789u, 0xd8}, {{0x132791u, 0x84}, 0x13279eu, 0xdc}}}},
};
inline constexpr std::array patchy_fields{
    Field{{0x13283cu, 0xc0}, 0x132849u, 0x70}, Field{{0x13284eu, 0xc4}, 0x13285bu, 0x74},
    Field{{0x132860u, 0xc8}, 0x13286du, 0x78}, Field{{0x132872u, 0xcc}, 0x13287fu, 0x7c},
    Field{{0x132884u, 0xd0}, 0x132891u, 0x80}, Field{{0x132899u, 0xd4}, 0x1328a6u, 0x84},
    Field{{0x1328aeu, 0xd8}, 0x1328bbu, 0x88},
};

template<class State> struct Resolution : Port<State> {
    using Base = Port<State>;
    Resolution(State &s, std::uint32_t *cursor, Trace<State> *trace) : Base(s, cursor, trace, {}) {}
    auto &x() { return this->s.xmm; }

    __m128 local(Local location) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(
            std::bit_cast<std::int32_t>(this->read(location.pc, this->gp[esp] + location.offset, 4))));
    }
    void output(std::uint32_t pc, std::uint32_t offset, std::uint32_t bits) {
        this->store(pc, this->gp[esi] + offset, 4, bits);
    }
    void publish_field(std::uint32_t pc, std::uint32_t offset, unsigned lane) {
        output(pc, offset, std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[lane]))));
    }
    void multiply(unsigned destination, __m128 source) {
        x()[destination] = native_tu15_leaves::ordered_mul_ss(x()[destination], source);
    }
    bool greater(__m128 left, __m128 right) {
        this->flags.scalar_compare(left, right, false);
        return !this->flags.cf && !this->flags.zf;
    }
    bool load_positive(Local location, unsigned lane) {
        x()[lane] = local(location);
        return greater(x()[lane], x()[7]);
    }
    bool has_weight(Blend blend) {
        const auto index = unsigned(blend);
        const auto &group = weights[index];
        if (!load_positive(group.divisor, group.divisor_lane)) {
            this->reached(index * 3);
            return false;
        }
        const bool positive = load_positive(group.coverage, group.coverage_lane);
        this->reached(index * 3 + (positive ? 2 : 1));
        return positive;
    }
    void load_scaled(Local location, unsigned lane) {
        x()[lane] = local(location);
        multiply(lane, x()[0]);
    }
    void scaled_field(Field field, unsigned lane) {
        load_scaled(field.local, lane);
        publish_field(field.store_pc, field.output, lane);
    }
    void resolve_six(Blend blend) {
        const auto &fields = six_fields[unsigned(blend)];
        x()[0] = _mm_div_ss(x()[3], x()[1]);
        for (const auto &field : fields.color)
            scaled_field(field, 1);
        x()[1] = x()[0];
        multiply(1, x()[2]);
        publish_field(fields.density_pc, fields.density_output, 1);
        for (const auto &field : fields.distance)
            scaled_field(field, 1);
    }
    void resolve_sky() {
        x()[0] = _mm_div_ss(x()[3], x()[1]);
        scaled_field({{0x1327c9u, 0x90}, 0x1327d6u, 0x4c}, 1);
        scaled_field({{0x1327dbu, 0x94}, 0x1327e8u, 0x50}, 1);
        load_scaled({0x1327edu, 0x98}, 1);
        multiply(0, x()[2]);
        publish_field(0x1327feu, 0x54, 1);
        publish_field(0x132803u, 0x58, 0);
    }
    bool has_patchy_datum() {
        this->compare32(this->gp[ebp], 0xffffffffu);
        this->reached(this->flags.zf ? 15 : 16);
        return !this->flags.zf;
    }
    void resolve_patchy() {
        x()[0] = _mm_div_ss(x()[3], x()[2]);
        for (const auto &field : patchy_fields)
            scaled_field(field, 2);
        x()[2] = x()[0];
        multiply(2, x()[1]);
        load_scaled({0x1328cau, 0xe0}, 1);
        publish_field(0x1328d7u, 0x90, 1);
        load_scaled({0x1328dfu, 0xe4}, 1);
        output(0x1328ecu, 0x6c, this->gp[ebp]);
        publish_field(0x1328efu, 0x8c, 2);
        publish_field(0x1328f7u, 0x94, 1);
    }
    bool has_bloom() {
        const bool positive = load_positive({0x1328ffu, 0x64}, 0);
        this->reached(positive ? 18 : 17);
        return positive;
    }
    // UCOMISS against the lower bound first. Unordered comparisons preserve
    // the computed value, including its destination NaN payload.
    void clamp_bloom(unsigned branch) {
        if (greater(x()[7], x()[0])) {
            x()[0] = x()[7];
            this->reached(branch);
        } else if (greater(x()[0], x()[3])) {
            x()[0] = x()[3];
            this->reached(branch + 1);
        } else {
            this->reached(branch + 2);
        }
    }
    void resolve_bloom() {
        x()[1] = _mm_div_ss(x()[3], x()[0]);
        x()[0] = x()[3];
        x()[0] = _mm_sub_ss(x()[0], local({0x132918u, 0x68}));
        multiply(0, local({0x13291eu, 0x54}));
        multiply(0, x()[1]);
        clamp_bloom(19);
        publish_field(0x13293au, 0xe0, 0);
        x()[0] = local({0x132942u, 0x58});
        multiply(0, x()[1]);
        clamp_bloom(22);
        publish_field(0x13295eu, 0xe4, 0);
        x()[0] = local({0x132966u, 0x5c});
        multiply(0, x()[1]);
        clamp_bloom(25);
        publish_field(0x132982u, 0xe8, 0);
        x()[0] = local({0x13298au, 0x60});
        multiply(0, x()[1]);
        publish_field(0x132994u, 0xec, 0);
    }
    void run() {
        try {
            native_fog_resolution::resolve(*this);
            this->gp[edi] = this->pop(0x13299cu);
            this->gp[esi] = this->pop(0x13299du);
            this->gp[ebp] = this->pop(0x13299eu);
            this->gp[ebx] = this->pop(0x13299fu);
            this->gp[esp] = this->flags.plus(this->gp[esp], 0x4820u, 4);
            this->ret(last, 12);
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
        this->publish(last);
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> bits{}; return bits; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary verify{"Campaign fog resolution"};
    return verify;
}

template<class State, class Reference> bool bridge(State &s, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_RESOLUTION");
    static const bool verify = flag("HALO2_VERIFY_FOG_RESOLUTION");
    // With unmasked SIMD exceptions, retain the per-instruction arithmetic
    // cursor. This closure publishes the cursor only at guest accesses.
    if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    // The real callees before 13262B can change the provider that the state
    // encountered at 131AC0. Revalidate that cached cursor at the handoff.
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    return native_small_queries_detail::run_owner(s, first, last, force_reference, verify, coverage(), summary(),
        reference, [](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Resolution<State>{v, cursor, trace};
        });
}

} // namespace halo2::native_fog_resolution_detail
