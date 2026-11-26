#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_fog_plane_projection_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline constexpr std::uint32_t first = 0x133d8fu;
inline constexpr std::uint32_t last = 0x13401au;
enum class Entry : std::uint32_t { project = first, advance = 0x13400eu };
enum class Next : std::uint32_t { fallback, member = 0x133d50u, plane = 0x134020u };

struct RecordSites {
    Register output;
    // Profile scale and the three parameters, then three color loads.
    std::array<std::uint32_t,4> profile;
    std::array<std::uint32_t,3> color;
    // Accumulator reads and ordered write sites by dword offset.
    std::array<std::uint32_t,8> read, write;
};
inline constexpr std::array records{
    RecordSites{edi, {0x133e07u, 0x133e0cu, 0x133e11u, 0x133e16u}, {0x133e31u, 0x133e4du, 0x133e5fu},
        {0x133e39u, 0x133e52u, 0x133e64u, 0x133e80u, 0x133e89u, 0x133e45u, 0x133e6eu, 0x133ea4u},
        {0x133e3du, 0x133e57u, 0x133e8eu, 0x133e93u, 0x133e98u, 0x133e69u, 0x133e77u, 0x133ea9u}},
    RecordSites{ebx, {0x133eaeu, 0x133eb3u, 0x133eb8u, 0x133ebdu}, {0x133ed8u, 0x133ef4u, 0x133f06u},
        {0x133ee0u, 0x133ef9u, 0x133f0bu, 0x133f27u, 0x133f30u, 0x133eecu, 0x133f15u, 0x133f4bu},
        {0x133ee4u, 0x133efeu, 0x133f35u, 0x133f3au, 0x133f3fu, 0x133f10u, 0x133f1eu, 0x133f50u}},
    RecordSites{ebp, {0x133f55u, 0x133f5au, 0x133f5fu, 0x133f64u}, {0x133f7fu, 0x133f94u, 0x133fafu},
        {0x133f87u, 0x133f99u, 0x133fb4u, 0x133fc7u, 0x133fd0u, 0x133fa7u, 0x133fbeu, 0x133fd9u},
        {0x133f8cu, 0x133f9eu, 0x133fe2u, 0x133fe7u, 0x133fecu, 0x133fb9u, 0x133ff1u, 0x133ff6u}},
};

template<class State> struct Projection : Port<State> {
    using Base = Port<State>;
    Entry entry;
    Next &next;
    Projection(State &s, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &result)
        : Base(s, cursor, trace, {}), entry(start), next(result) {}
    auto &x() { return this->s.xmm; }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        const auto bits = std::bit_cast<std::int32_t>(this->read(pc, address, 4));
        return _mm_castsi128_ps(_mm_cvtsi32_si128(bits));
    }
    __m128 profile(std::uint32_t pc, std::uint32_t offset) {
        return scalar(pc, this->gp[esi] + offset);
    }
    void write(const RecordSites &record, unsigned field, unsigned lane) {
        const auto value = std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[lane])));
        this->store(record.write[field], this->gp[record.output] + 4u * field, 4, value);
    }
    __m128 accumulated(const RecordSites &record, unsigned field) {
        return scalar(record.read[field], this->gp[record.output] + 4u * field);
    }
    void sum(const RecordSites &record, unsigned field, unsigned lane) {
        x()[lane] = ordered_add_ss(x()[lane], accumulated(record, field));
    }
    void fold(const RecordSites &record, unsigned field, unsigned lane) {
        x()[lane] = ordered_mul_ss(x()[lane], x()[0]);
        sum(record, field, lane);
    }
    bool greater(__m128 a, __m128 b) {
        this->flags.scalar_compare(a, b, false);
        return !this->flags.cf && !this->flags.zf;
    }
    void interpolate(std::uint32_t low_pc, std::uint32_t high_pc, std::uint32_t angle_pc) {
        x()[2] = profile(low_pc, 0x18u);
        x()[2] = _mm_sub_ss(x()[2], profile(high_pc, 0x1cu));
        x()[1] = x()[0];
        x()[1] = _mm_sub_ss(x()[1], profile(angle_pc, 0x1cu));
        x()[1] = _mm_div_ss(x()[1], x()[2]);
    }
    bool member_weight() {
        this->test32(this->gp[esi]);
        if (this->flags.zf) {
            this->reached(0);
            return false;
        }
        x()[1] = profile(0x133d97u, 0x1cu);
        if (!greater(x()[1], profile(0x133d9cu, 0x18u))) {
            this->reached(1);
            return false;
        }
        x()[2] = profile(0x133da6u, 0x18u);
        x()[2] = _mm_sub_ss(x()[2], profile(0x133dabu, 0x1cu));
        x()[5] = _mm_setzero_ps();
        x()[4] = scalar(0x133db3u, 0x46a0d0u);
        x()[1] = x()[0];
        x()[1] = _mm_sub_ss(x()[1], profile(0x133dbeu, 0x1cu));
        x()[1] = _mm_div_ss(x()[1], x()[2]);
        if (greater(x()[5], x()[1])) {
            x()[6] = x()[5];
            this->reached(2);
        } else {
            // These reads repeat in the reference. Their fault order and
            // observations remain part of the guest ABI.
            interpolate(0x133dd1u, 0x133dd6u, 0x133ddeu);
            if (greater(x()[1], x()[4])) {
                x()[6] = x()[4];
                this->reached(3);
            } else {
                x()[1] = profile(0x133df1u, 0x18u);
                x()[0] = _mm_sub_ss(x()[0], profile(0x133df6u, 0x1cu));
                x()[1] = _mm_sub_ss(x()[1], profile(0x133dfbu, 0x1cu));
                x()[0] = _mm_div_ss(x()[0], x()[1]);
                x()[6] = x()[0];
                this->reached(4);
            }
        }
        return true;
    }
    void scale_record(unsigned index) {
        const auto &site = records[index].profile;
        x()[0] = profile(site[0], 0x20u + 4u * index);
        x()[1] = profile(site[1], 0x14u);
        x()[2] = profile(site[2], 0x10u);
        x()[3] = profile(site[3], 0xcu);
        x()[0] = ordered_mul_ss(x()[0], x()[6]);
        const auto branch = 6u + 3u * index;
        if (greater(x()[5], x()[0])) {
            x()[0] = x()[5];
            this->reached(branch);
        } else if (greater(x()[0], x()[4])) {
            x()[0] = x()[4];
            this->reached(branch + 1);
        } else {
            this->reached(branch + 2);
        }
    }
    void color(const RecordSites &record, unsigned component, unsigned lane) {
        x()[lane] = x()[0];
        x()[lane] = ordered_mul_ss(x()[lane], profile(record.color[component], 4u * component));
        sum(record, component, lane);
    }
    void accumulate_first_two(unsigned index) {
        const auto &record = records[index];
        scale_record(index);
        x()[7] = profile(record.color[0], 0);
        fold(record, 0, 7);
        write(record, 0, 7);
        fold(record, 5, 1);
        color(record, 1, 7);
        write(record, 1, 7);
        color(record, 2, 7);
        write(record, 5, 1);
        x()[1] = accumulated(record, 6);
        x()[1] = ordered_add_ss(x()[1], x()[0]);
        write(record, 6, 1);
        fold(record, 3, 3);
        fold(record, 4, 2);
        write(record, 2, 7);
        write(record, 3, 3);
        write(record, 4, 2);
        x()[1] = _mm_sub_ss(x()[4], x()[0]);
        x()[1] = ordered_mul_ss(x()[1], accumulated(record, 7));
        write(record, 7, 1);
    }
    void accumulate_last() {
        const auto &record = records.back();
        scale_record(2);
        x()[5] = profile(record.color[0], 0);
        fold(record, 0, 5);
        write(record, 0, 5);
        color(record, 1, 5);
        write(record, 1, 5);
        fold(record, 5, 1);
        color(record, 2, 5);
        write(record, 5, 1);
        x()[1] = accumulated(record, 6);
        fold(record, 3, 3);
        fold(record, 4, 2);
        x()[4] = _mm_sub_ss(x()[4], x()[0]);
        x()[4] = ordered_mul_ss(x()[4], accumulated(record, 7));
        x()[1] = ordered_add_ss(x()[1], x()[0]);
        write(record, 2, 5);
        write(record, 3, 3);
        write(record, 4, 2);
        write(record, 6, 1);
        write(record, 7, 4);
    }
    void advance() {
        auto &gp = this->gp;
        gp[eax] = this->read(0x13400eu, gp[esp] + 8u, 4);
        gp[eax] = this->flags.increment(gp[eax], 4, false);
        this->compare32(gp[eax], 2u);
        this->store(0x134016u, gp[esp] + 8u, 4, gp[eax]);
        const bool another = this->flags.sf != this->flags.of;
        next = another ? Next::member : Next::plane;
        this->reached(another ? 15 : 16);
        this->publish(last);
    }
    void run() {
        try {
            if (entry == Entry::project) {
                if (member_weight()) {
                    accumulate_first_two(0);
                    accumulate_first_two(1);
                    accumulate_last();
                }
                x()[2] = _mm_setzero_ps();
                x()[6] = scalar(0x133ffeu, 0x46a0d0u);
                this->gp[edx] = this->read(0x134006u, this->gp[esp] + 0xcu, 4);
                this->gp[ecx] = this->read(0x13400au, this->gp[esp] + 0x10u, 4);
            } else {
                this->reached(5);
            }
            advance();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign fog plane projection"};
    return value;
}

template<class State, class Reference> Next bridge(State &s, Entry start, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_PLANE_PROJECTION");
    static const bool verify = flag("HALO2_VERIFY_FOG_PLANE_PROJECTION");
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
                throw std::runtime_error("Campaign fog plane projection continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Projection<State>{v, cursor, trace, start, next};
        });
    return next;
}

} // namespace halo2::native_fog_plane_projection_detail
