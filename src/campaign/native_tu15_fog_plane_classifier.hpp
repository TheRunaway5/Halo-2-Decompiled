#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_fog_plane_classifier_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t first = 0x133b40u;
inline constexpr std::uint32_t last = 0x133d86u;
enum class Entry : std::uint32_t { initialize = first, classify = 0x133bd4u, normal = 0x133d1du, member = 0x133d50u };
enum class Next : std::uint32_t {
    fallback, classify = 0x133bd4u, no_planes = 0x134042u, angular = 0x133c2fu, explicit_normal = 0x133cabu,
    skip_plane = 0x134020u, end_members = 0x13400eu, geometry = 0x133d8au
};
struct Output {
    Register address;
    std::array<std::uint32_t,8> clear;
    std::uint32_t unit;
};
inline constexpr std::array outputs{
    Output{edi, {0x133b66u, 0x133b68u, 0x133b6bu, 0x133b6eu, 0x133b71u, 0x133b74u, 0x133b77u, 0x133b7au}, 0x133b7du},
    Output{ebx, {0x133b84u, 0x133b86u, 0x133b89u, 0x133b8cu, 0x133b8fu, 0x133b92u, 0x133b95u, 0x133b98u}, 0x133b9bu},
    Output{ebp, {0x133ba2u, 0x133ba4u, 0x133ba7u, 0x133baau, 0x133badu, 0x133bb0u, 0x133bb3u, 0x133bb6u}, 0x133bb9u},
};

template<class State> struct Classifier : Port<State> {
    using Base = Port<State>;
    Entry entry;
    Next &next;
    Classifier(State &s, std::uint32_t *cursor, Trace<State> *trace, Entry start, Next &result)
        : Base(s, cursor, trace, {}), entry(start), next(result) {}
    auto &x() { return this->s.xmm; }
    std::uint32_t load_word(std::uint32_t pc, std::uint32_t address) { return this->read(pc, address, 4); }
    __m128 load_scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(load_word(pc, address))));
    }
    void store_word(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        this->store(pc, address, 4, value);
    }
    void resume(Next destination, std::uint32_t pc, unsigned branch) {
        next = destination;
        this->reached(branch);
        this->publish(pc);
    }
    bool positive() const { return !this->flags.zf && this->flags.sf == this->flags.of; }
    void initialize() {
        auto &gp = this->gp;
        gp[esp] = this->flags.minus(gp[esp], 0x30u, 4);
        this->push(0x133b43u, gp[ebp]);
        gp[ebp] = load_word(0x133b44u, gp[esp] + 0x3cu);
        gp[ecx] = load_word(0x133b48u, 0x4f0414u);
        gp[eax] = this->flags.logic(gp[eax] & 0xffffu, 4);
        gp[eax] = this->flags.shift(gp[eax], 4, 4);
        gp[edx] = load_word(0x133b56u, gp[ecx] + gp[eax] + 8u);
        gp[eax] = this->flags.logic(0, 4);
        x()[6] = load_scalar(0x133b5cu, 0x46a0d0u);
        const auto unit = std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x()[6])));
        for (const auto &output : outputs) {
            gp[ecx] = gp[output.address];
            for (std::size_t word = 0; word < output.clear.size(); ++word)
                store_word(output.clear[word], gp[ecx] + std::uint32_t(word * 4), gp[eax]);
            store_word(output.unit, gp[output.address] + 0x1cu, unit);
        }
        this->compare32(load_word(0x133bbeu, gp[edx] + 0x78u), gp[eax]);
        store_word(0x133bc1u, gp[esp] + 8u, gp[edx]);
        store_word(0x133bc5u, gp[esp] + 0x10u, gp[eax]);
        if (!positive()) {
            resume(Next::no_planes, 0x133bc9u, 0);
            return;
        }
        store_word(0x133bcfu, gp[esp] + 0x3cu, gp[eax]);
        this->push(0x133bd3u, gp[esi]);
        resume(Next::classify, 0x133bd3u, 1);
    }
    // UCOMISS, LAHF and TEST AH,44h distinguish ordered signed zero.
    // The compare results are recorded in AH and the TEST sets live EFLAGS.
    bool needs_explicit_normal(unsigned lane) {
        this->flags.scalar_compare(x()[lane], x()[2], true);
        auto &word = this->gp[eax];
        const auto ah = (this->flags.bits() | 2u) & 0xffu;
        word = (word & 0xffff00ffu) | (ah << 8);
        this->flags.logic(ah & 0x44u, 1);
        return this->flags.pf;
    }
    void classify() {
        auto &gp = this->gp;
        const auto sp = gp[esp];
        gp[eax] = load_word(0x133bd4u, sp + 0x40u);
        gp[ecx] = load_word(0x133bd8u, gp[edx] + 0x7cu);
        x()[2] = _mm_setzero_ps();
        gp[ecx] = this->flags.plus(gp[ecx], gp[eax], 4);
        gp[eax] = gp[ecx];
        gp[esi] = load_word(0x133be2u, gp[eax]);
        store_word(0x133be4u, sp + 0x2cu, gp[esi]);
        gp[esi] = load_word(0x133be8u, gp[eax] + 4u);
        gp[eax] = load_word(0x133bebu, gp[eax] + 8u);
        x()[1] = load_scalar(0x133beeu, sp + 0x2cu);
        store_word(0x133bf4u, sp + 0x34u, gp[eax]);
        x()[3] = load_scalar(0x133bf8u, sp + 0x34u);
        const bool first_component = needs_explicit_normal(1);
        store_word(0x133c05u, sp + 0x30u, gp[esi]);
        x()[4] = load_scalar(0x133c09u, sp + 0x30u);
        store_word(0x133c0fu, sp + 0x10u, gp[ecx]);
        if (first_component) {
            resume(Next::explicit_normal, 0x133c13u, 2);
        } else if (needs_explicit_normal(4)) {
            resume(Next::explicit_normal, 0x133c20u, 3);
        } else {
            const bool explicit_normal = needs_explicit_normal(3);
            resume(explicit_normal ? Next::explicit_normal : Next::angular, 0x133c2du, explicit_normal ? 4 : 5);
        }
    }
    bool prepare_normal() {
        using native_tu15_leaves::ordered_mul_ss;
        using native_world_leaves_detail::ordered_add_ss;
        // Original squared norm accumulates z*z, y*y, then x*x.
        x()[0] = ordered_mul_ss(x()[3], x()[3]);
        x()[3] = ordered_mul_ss(x()[4], x()[4]);
        x()[0] = ordered_add_ss(x()[0], x()[3]);
        x()[3] = ordered_mul_ss(x()[1], x()[1]);
        x()[0] = ordered_add_ss(x()[0], x()[3]);
        this->flags.scalar_compare(x()[0], x()[2], false);
        if (this->flags.cf || this->flags.zf) {
            resume(Next::skip_plane, 0x133d3du, 6);
            return false;
        }
        this->gp[eax] = this->flags.logic(0, 4);
        store_word(0x133d45u, this->gp[esp] + 8u, this->gp[eax]);
        this->reached(7);
        return true;
    }
    void member() {
        auto &gp = this->gp;
        gp[eax] = this->flags.minus(gp[eax], 0u, 4);
        const bool front = this->flags.zf;
        if (!front) {
            gp[eax] = this->flags.increment(gp[eax], 4, true);
            if (!this->flags.zf) {
                resume(Next::end_members, 0x133d56u, 8);
                return;
            }
        }
        const auto member_fields = gp[ecx] + (front ? 0x1cu : 0x24u);
        gp[eax] = load_word(front ? 0x133d74u : 0x133d5cu, member_fields);
        this->test32(gp[eax]);
        const bool has_shape = positive();
        if (has_shape) {
            gp[esi] = load_word(front ? 0x133d7bu : 0x133d63u, member_fields + 4u);
        } else {
            gp[esi] = this->flags.logic(0, 4);
        }
        gp[edx] = gp[esp] + (front ? 0x2cu : 0x20u);
        gp[ecx] = load_word(last, gp[esp] + 0x3cu);
        resume(Next::geometry, last, unsigned(front ? 9 : 11) + unsigned(has_shape));
    }
    void run() {
        try {
            switch (entry) {
            case Entry::initialize: initialize(); break;
            case Entry::classify: classify(); break;
            case Entry::normal:
                if (prepare_normal()) member();
                break;
            case Entry::member: member(); break;
            }
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

inline auto &coverage() { static std::atomic<std::uint64_t> value{}; return value; }
inline auto &summary() {
    static native_small_queries_detail::VerifySummary value{"Campaign fog plane classifier"};
    return value;
}

template<class State, class Reference> Next bridge(State &s, Entry start, Reference reference) {
    using native_small_queries_detail::flag;
    static const bool force_reference = flag("HALO2_REFERENCE_FOG_PLANE_CLASSIFIER");
    static const bool verify = flag("HALO2_VERIFY_FOG_PLANE_CLASSIFIER");
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
                throw std::runtime_error("Campaign fog plane classifier continuation differs");
        }, [&](State &v, std::uint32_t *cursor, Trace<State> *trace) {
            return Classifier<State>{v, cursor, trace, start, next};
        });
    return next;
}

} // namespace halo2::native_fog_plane_classifier_detail
