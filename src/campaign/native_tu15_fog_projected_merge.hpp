#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_fog_projected_merge_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

inline constexpr std::uint32_t first = 0x132188u;
inline constexpr std::uint32_t last = 0x132490u;
enum class Next : std::uint32_t { fallback, patchy_fog = 0x132492u, bloom = 0x1324d9u };

struct FieldSites { std::uint32_t load, sum, store; };
struct Group {
    std::uint32_t source, accumulated, weight_pc;
    std::array<FieldSites,6> fields;
};
inline constexpr std::array groups{
    Group{0xf0u, 0x34u, 0x1321a4u, {{{0x1321c0u, 0x1321cdu, 0x1321d3u}, {0x1321d9u, 0x1321e6u, 0x1321ecu},
        {0x1321f2u, 0x1321ffu, 0x132205u}, {0x13220bu, 0x132218u, 0x13221eu},
        {0x132224u, 0x132231u, 0x132237u}, {0x13223du, 0x13224au, 0x132250u}}}},
    Group{0x110u, 0x14u, 0x132283u, {{{0x13229cu, 0x1322a9u, 0x1322afu}, {0x1322b5u, 0x1322c2u, 0x1322c8u},
        {0x1322ceu, 0x1322dbu, 0x1322e1u}, {0x1322e7u, 0x1322f4u, 0x1322fau},
        {0x132300u, 0x13230du, 0x132313u}, {0x132319u, 0x132326u, 0x13232cu}}}},
    Group{0x130u, 0x90u, 0x13235fu, {{{0x132378u, 0x132385u, 0x13238eu}, {0x132397u, 0x1323a4u, 0x1323adu},
        {0x1323b6u, 0x1323c3u, 0x1323ccu}, {0x1323d5u, 0x1323e2u, 0x1323ebu},
        {0x1323f4u, 0x132401u, 0x13240au}, {0x132413u, 0x132420u, 0x132429u}}}},
};

template<class State> struct Merge : Port<State> {
    using Base = Port<State>;
    Next &next;
    Merge(State &s, std::uint32_t *cursor, Trace<State> *trace, Next &result) : Base(s, cursor, trace, {}), next(result) {}
    auto &x() { return this->s.xmm; }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        const auto bits = std::bit_cast<std::int32_t>(this->read(pc, address, 4));
        return _mm_castsi128_ps(_mm_cvtsi32_si128(bits));
    }
    __m128 local(std::uint32_t pc, std::uint32_t offset) {
        return scalar(pc, this->gp[esp] + offset);
    }
    void publish_local(std::uint32_t pc, std::uint32_t offset, unsigned lane) {
        this->store(pc, this->gp[esp] + offset, 4, std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[lane]))));
    }
    bool greater(unsigned a, unsigned b) {
        this->flags.scalar_compare(x()[a], x()[b], false);
        return !this->flags.cf && !this->flags.zf;
    }
    void cap_weight(bool above) {
        x()[1] = above ? x()[5] : x()[6];
    }
    void normalize(unsigned index) {
        const auto &group = groups[index];
        x()[2] = local(group.weight_pc, group.source + 24u);
        if (index == 0)
            x()[7] = _mm_setzero_ps();
        const bool present = greater(2, 7);
        this->reached(2u * index + unsigned(present));
        if (!present)
            return;
        x()[0] = _mm_div_ss(x()[1], x()[2]);
        for (unsigned field = 0; field < group.fields.size(); ++field) {
            const auto &pc = group.fields[field];
            x()[2] = local(pc.load, group.source + 4u * field);
            x()[2] = ordered_mul_ss(x()[2], x()[0]);
            x()[2] = ordered_add_ss(x()[2], local(pc.sum, group.accumulated + 4u * field));
            publish_local(pc.store, group.accumulated + 4u * field, 2);
        }
    }
    void accumulate_weight(std::uint32_t offset, std::uint32_t load, std::uint32_t store) {
        x()[0] = x()[1];
        x()[0] = ordered_add_ss(x()[0], local(load, offset));
        publish_local(store, offset, 0);
    }
    void attenuate(std::uint32_t offset, std::uint32_t load, std::uint32_t store) {
        x()[0] = _mm_sub_ss(x()[5], x()[1]);
        x()[0] = ordered_mul_ss(x()[0], local(load, offset));
        publish_local(store, offset, 0);
    }
    void finish(Next destination, std::uint32_t pc, unsigned branch) {
        next = destination;
        this->reached(branch);
        this->publish(pc);
    }
    void patchy_fog() {
        auto &gp = this->gp;
        gp[eax] = this->read(0x132432u, gp[esi] + 0x60u, 4);
        this->test32(gp[eax]);
        accumulate_weight(0xa8u, 0x13243au, 0x132443u);
        attenuate(0xacu, 0x132453u, 0x13245cu);
        x()[0] = local(0x132465u, 0x88u);
        x()[0] = ordered_add_ss(x()[0], x()[6]);
        publish_local(0x132472u, 0x88u, 0);
        if (this->flags.zf || this->flags.sf != this->flags.of) {
            finish(Next::bloom, 0x13247bu, 8);
            return;
        }
        gp[edx] = this->read(0x13247du, gp[esi] + 0x64u, 4);
        gp[edi] = this->read(0x132480u, gp[edx] + 0x4cu, 4);
        this->compare32(gp[edi], 0xffffffffu);
        if (this->flags.zf) {
            finish(Next::bloom, 0x132486u, 9);
            return;
        }
        x()[0] = scalar(0x132488u, gp[edx] + 0x1cu);
        const bool has_patchy_weight = greater(0, 7);
        finish(has_patchy_weight ? Next::patchy_fog : Next::bloom, last, has_patchy_weight ? 11 : 10);
    }
    void merge() {
        x()[6] = local(0x132188u, 0xb8u);
        x()[5] = scalar(0x132191u, 0x46a0d0u);
        const bool above = greater(6, 5);
        this->reached(above ? 7 : 6);
        cap_weight(above);
        normalize(0);
        // The comparison flags precede the ordered sum and attenuation
        // stores, and determine the next group's live weight afterwards.
        const bool above_first = greater(6, 5);
        accumulate_weight(0x4cu, 0x13225cu, 0x132262u);
        attenuate(0x50u, 0x13226fu, 0x132275u);
        cap_weight(above_first);
        normalize(1);
        const bool above_second = greater(6, 5);
        accumulate_weight(0x2cu, 0x132338u, 0x13233eu);
        attenuate(0x30u, 0x13234bu, 0x132351u);
        cap_weight(above_second);
        normalize(2);
        patchy_fog();
    }
    void run() {
        try {
            merge();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign projected fog merge"};
    return value;
}

template<class State, class Reference> Next bridge(State &s, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_PROJECTED_MERGE");
    static const bool verify = flag("HALO2_VERIFY_FOG_PROJECTED_MERGE");
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
                throw std::runtime_error("Campaign projected fog continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Merge<State>{v, cursor, trace, next};
        });
    return next;
}

} // namespace halo2::native_fog_projected_merge_detail
