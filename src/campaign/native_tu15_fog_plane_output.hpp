#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_fog_plane_output_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline constexpr std::uint32_t first = 0x134042u;
inline constexpr std::uint32_t last = 0x1342a1u;
enum class Entry : std::uint32_t { no_planes = first, classified = 0x134045u };
struct RecordSites {
    Register output;
    std::array<std::uint32_t,3> source_color;
    std::array<std::uint32_t,8> read, write;
    std::uint32_t weight_only_read, weight_only_write;
};
inline constexpr std::array records{
    RecordSites{edi, {0x13408cu, 0x13409cu, 0x1340b8u},
        {0x134094u, 0x1340a5u, 0x1340e0u, 0x1340d4u, 0x1340e9u, 0x1340aeu, 0x1340c2u, 0x1340f2u},
        {0x134098u, 0x1340b3u, 0x134102u, 0x1340f7u, 0x134107u, 0x1340bdu, 0x1340cbu, 0x13410cu},
        0x134113u, 0x13411cu},
    RecordSites{ebx, {0x134150u, 0x134160u, 0x13417cu},
        {0x134158u, 0x134169u, 0x13419bu, 0x1341a4u, 0x1341adu, 0x134172u, 0x134186u, 0x1341b6u},
        {0x13415cu, 0x134177u, 0x1341bbu, 0x1341c0u, 0x1341c5u, 0x134181u, 0x13418fu, 0x1341cau},
        0x1341d1u, 0x1341dau},
    RecordSites{ebp, {0x134207u, 0x134219u, 0x134235u},
        {0x13420fu, 0x13422bu, 0x134267u, 0x134222u, 0x134246u, 0x134250u, 0x13425eu, 0x134270u},
        {0x134214u, 0x134230u, 0x134279u, 0x13423au, 0x13424bu, 0x134259u, 0x13427eu, 0x134283u},
        0x13428fu, 0x134298u},
};

template<class State> struct Output : Port<State> {
    using Base = Port<State>;
    Entry entry;
    Output(State &s, std::uint32_t *cursor, Trace<State> *trace, Entry start) : Base(s, cursor, trace, {}), entry(start) {}
    auto &x() { return this->s.xmm; }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        const auto bits = std::bit_cast<std::int32_t>(this->read(pc, address, 4));
        return _mm_castsi128_ps(_mm_cvtsi32_si128(bits));
    }
    void store_bits(std::uint32_t pc, std::uint32_t address, unsigned lane) {
        const auto word = std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[lane])));
        this->store(pc, address, 4, word);
    }
    void write(const RecordSites &record, unsigned index, unsigned lane) {
        store_bits(record.write[index], this->gp[record.output] + 4u * index, lane);
    }
    void load_color(const RecordSites &record, Register source, unsigned index, unsigned lane) {
        x()[lane] = scalar(record.source_color[index], this->gp[source] + 4u * index);
    }
    __m128 accumulated(const RecordSites &record, unsigned index) {
        return scalar(record.read[index], this->gp[record.output] + 4u * index);
    }
    void fold(const RecordSites &record, unsigned index, unsigned lane, unsigned weight) {
        x()[lane] = ordered_mul_ss(x()[lane], x()[weight]);
        x()[lane] = ordered_add_ss(x()[lane], accumulated(record, index));
    }
    void add_weight(const RecordSites &record, unsigned index, unsigned lane, unsigned weight) {
        x()[lane] = accumulated(record, index);
        x()[lane] = ordered_add_ss(x()[lane], x()[weight]);
        write(record, index, lane);
    }
    void weight_only(const RecordSites &record, unsigned destination, unsigned weight) {
        x()[destination] = scalar(record.weight_only_read, this->gp[record.output] + 0x18u);
        x()[destination] = ordered_add_ss(x()[destination], x()[weight]);
        store_bits(record.weight_only_write, this->gp[record.output] + 0x18u, destination);
    }
    bool positive() const { return !this->flags.zf && this->flags.sf == this->flags.of; }
    bool compare_greater(unsigned left, unsigned right) {
        this->flags.scalar_compare(x()[left], x()[right], false);
        return !this->flags.cf && !this->flags.zf;
    }
    void clamp(bool below, unsigned input, unsigned destination, unsigned outcome) {
        x()[destination] = x()[input];
        if (below) {
            x()[destination] = x()[2];
            this->reached(outcome);
        } else if (compare_greater(input, 6)) {
            x()[destination] = x()[6];
            this->reached(outcome + 1);
        } else {
            this->reached(outcome + 2);
        }
    }
    void first_record() {
        auto &gp = this->gp;
        const auto &record = records.front();
        if (!positive()) {
            weight_only(record, 1, 0);
            this->reached(2);
            return;
        }
        const bool below = compare_greater(2, 0);
        gp[eax] = this->read(0x134068u, gp[edx] + 0x4cu, 4);
        x()[3] = scalar(0x13406bu, gp[eax] + 0x14u);
        x()[4] = scalar(0x134070u, gp[eax] + 0x10u);
        x()[5] = scalar(0x134075u, gp[eax] + 0xcu);
        clamp(below, 0, 1, 3);
        load_color(record, eax, 0, 7);
        fold(record, 0, 7, 1);
        write(record, 0, 7);
        load_color(record, eax, 1, 7);
        fold(record, 1, 7, 1);
        fold(record, 5, 3, 1);
        write(record, 1, 7);
        load_color(record, eax, 2, 7);
        write(record, 5, 3);
        add_weight(record, 6, 3, 1);
        fold(record, 3, 5, 1);
        x()[3] = x()[6];
        fold(record, 2, 7, 1);
        fold(record, 4, 4, 1);
        x()[3] = _mm_sub_ss(x()[3], x()[1]);
        x()[3] = ordered_mul_ss(x()[3], accumulated(record, 7));
        write(record, 3, 5);
        x()[5] = scalar(0x1340fcu, gp[esp] + 0x38u);
        write(record, 2, 7);
        write(record, 4, 4);
        write(record, 7, 3);
    }
    void second_record() {
        auto &gp = this->gp;
        const auto &record = records[1];
        gp[eax] = this->read(0x134121u, gp[edx] + 0x50u, 4);
        this->test32(gp[eax]);
        if (!positive()) {
            weight_only(record, 1, 0);
            this->reached(6);
            return;
        }
        const bool below = compare_greater(2, 0);
        gp[eax] = this->read(0x13412fu, gp[edx] + 0x54u, 4);
        x()[1] = scalar(0x134132u, gp[eax] + 0x14u);
        x()[3] = scalar(0x134137u, gp[eax] + 0x10u);
        x()[4] = scalar(0x13413cu, gp[eax] + 0xcu);
        clamp(below, 0, 0, 7);
        load_color(record, eax, 0, 7);
        fold(record, 0, 7, 0);
        write(record, 0, 7);
        load_color(record, eax, 1, 7);
        fold(record, 1, 7, 0);
        fold(record, 5, 1, 0);
        write(record, 1, 7);
        load_color(record, eax, 2, 7);
        write(record, 5, 1);
        add_weight(record, 6, 1, 0);
        x()[1] = x()[6];
        fold(record, 2, 7, 0);
        fold(record, 3, 4, 0);
        fold(record, 4, 3, 0);
        x()[1] = _mm_sub_ss(x()[1], x()[0]);
        x()[1] = ordered_mul_ss(x()[1], accumulated(record, 7));
        write(record, 2, 7);
        write(record, 3, 4);
        write(record, 4, 3);
        write(record, 7, 1);
    }
    // Both exits restore EBP, release the same scratch frame and return with
    // the original ADD flags, while their pop and return access PCs differ.
    void leave(std::uint32_t restore_pc, std::uint32_t return_pc) {
        auto &gp = this->gp;
        gp[ebp] = this->pop(restore_pc);
        gp[esp] = this->flags.plus(gp[esp], 0x30u, 4);
        this->ret(return_pc, 8);
        this->publish(return_pc);
    }
    void third_record() {
        auto &gp = this->gp;
        const auto &record = records.back();
        gp[eax] = this->read(0x1341dfu, gp[edx] + 0x58u, 4);
        this->test32(gp[eax]);
        if (!positive()) {
            weight_only(record, 0, 5);
            this->reached(10);
            leave(0x13429du, last);
            return;
        }
        const bool below = compare_greater(2, 5);
        gp[edx] = this->read(0x1341edu, gp[edx] + 0x5cu, 4);
        x()[1] = scalar(0x1341f0u, gp[edx] + 0xcu);
        clamp(below, 5, 0, 11);
        load_color(record, edx, 0, 3);
        fold(record, 0, 3, 0);
        write(record, 0, 3);
        load_color(record, edx, 1, 3);
        fold(record, 3, 1, 0);
        fold(record, 1, 3, 0);
        write(record, 1, 3);
        load_color(record, edx, 2, 3);
        write(record, 3, 1);
        x()[1] = x()[0];
        x()[1] = ordered_mul_ss(x()[1], x()[2]);
        x()[1] = ordered_add_ss(x()[1], accumulated(record, 4));
        write(record, 4, 1);
        add_weight(record, 5, 1, 0);
        x()[1] = accumulated(record, 6);
        fold(record, 2, 3, 0);
        x()[6] = _mm_sub_ss(x()[6], x()[0]);
        x()[6] = ordered_mul_ss(x()[6], accumulated(record, 7));
        x()[1] = ordered_add_ss(x()[1], x()[0]);
        write(record, 2, 3);
        write(record, 6, 1);
        write(record, 7, 6);
        leave(0x134288u, 0x13428cu);
    }
    void finish() {
        auto &gp = this->gp;
        if (entry == Entry::no_planes) {
            x()[2] = _mm_setzero_ps();
            this->reached(0);
        } else {
            this->reached(1);
        }
        gp[eax] = this->read(0x134045u, gp[edx] + 0x48u, 4);
        this->test32(gp[eax]);
        x()[0] = scalar(0x13404au, gp[edi] + 0x1cu);
        x()[5] = scalar(0x13404fu, gp[ebp] + 0x1cu);
        x()[0] = ordered_mul_ss(x()[0], scalar(0x134054u, gp[ebx] + 0x1cu));
        store_bits(0x134059u, gp[esp] + 0x38u, 5);
        first_record();
        second_record();
        third_record();
    }
    void run() {
        try {
            finish();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign fog plane output"};
    return value;
}

template<class State, class Reference> bool bridge(State &s, Entry start, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_PLANE_OUTPUT");
    static const bool verify = flag("HALO2_VERIFY_FOG_PLANE_OUTPUT");
    if ((_mm_getcsr() & 0x1f80u) != 0x1f80u)
        return false;
    if (s.instruction_cursor) {
        const auto *cursor = s.live_instruction_cursor();
        if (!cursor || !State::has_literal_service_sites(*cursor))
            return false;
    }
    return native_small_queries_detail::run_owner(s, first, last, force_reference, verify, coverage(), summary(),
        [&](State &v) { reference(v, start); }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Output<State>{v, cursor, trace, start};
        });
}

} // namespace halo2::native_fog_plane_output_detail
