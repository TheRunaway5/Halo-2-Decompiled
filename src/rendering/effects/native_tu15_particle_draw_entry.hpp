#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_particle_draw_entry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;

inline constexpr std::uint32_t draw = 0x177376u, descendants = 0x1777c4u;
inline constexpr std::uint32_t retained_entry = 0xffffffffu;
inline constexpr std::uint32_t element_definition = 0x178070u, reference_accessor = 0x13a6b0u;
inline constexpr std::uint32_t particle_model = 0x5052544du, particle_type = 0x70727433u;
enum Branch : unsigned {missing_accessor, table_definition, virtual_definition, skip_draw, begin_draw, branch_count};
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Dispatch>
struct Frame : Port<State> {
    Dispatch &dispatch;
    std::uint32_t &continuation;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Dispatch &route, std::uint32_t &next)
        : Port<State>(state, cursor, trace, {}), dispatch(route), continuation(next) {}

    void routed(std::uint32_t pc, std::uint32_t returned, std::uint32_t target) {
        this->call(pc, returned, target, [&](State &state) {
            try {
                dispatch(state, target, pc);
            } catch (...) {
                this->gp = state.gp;
                this->flags = state.flags;
                throw;
            }
        });
    }

    void select_element() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[eax] = this->read(0x1772c0u, 0x51df74u, 4);
        gp[edx] = this->read(0x1772c5u, gp[eax] + 0x44u, 4);
        gp[esp] = flags.minus(gp[esp], 0x64u, 4);
        this->push(0x1772cbu, gp[ebp]);
        const auto index = this->read(0x1772ccu, gp[esp] + 0x6cu, 4);
        gp[ebp] = flags.product(flags.logic(index & 0xffffu, 4), 0x54u);
        this->push(0x1772d9u, gp[esi]);
        gp[ebp] = flags.plus(gp[ebp], gp[edx], 4);
        this->push(0x1772dcu, gp[edi]);
    }

    void resolve_element(std::uint32_t pc, std::uint32_t returned) {
        this->gp[ecx] = this->gp[ebp];
        routed(pc, returned, element_definition);
    }

    void resolve_shape_definition() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        const auto tag = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(gp[edx])));
        gp[eax] = flags.shift(tag, 4u, 4);
        gp[eax] = this->read(0x17730au, gp[eax] + gp[ecx], 4);
        this->compare32(gp[eax], particle_model);
        if (!flags.zf) this->compare32(gp[eax], particle_type);
        if (flags.zf) {
            this->reached(virtual_definition);
            routed(0x17732du, 0x177332u, reference_accessor);
            gp[edx] = this->read(0x177332u, gp[eax], 4);
            gp[ecx] = gp[eax];
            const auto target = this->read(0x177336u, gp[edx] + 0x28u, 4);
            routed(0x177336u, 0x177339u, target);
            gp[ecx] = this->read(0x177339u, 0x4f0414u, 4);
            return;
        }

        this->reached(table_definition);
        gp[edx] = flags.shift(flags.logic(gp[edx] & 0xffffu, 4), 4u, 4);
        gp[edx] = this->read(0x177324u, gp[edx] + gp[ecx] + 8u, 4);
        gp[eax] = this->read(0x177328u, gp[edx] + 0x24u, 4);
    }

    void stage_shape() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        const auto handle = this->read(0x17733fu, gp[eax], 4);
        gp[eax] = flags.shift(flags.logic(handle & 0xffffu, 4), 4u, 4);
        gp[ecx] = this->read(0x177349u, gp[eax] + gp[ecx] + 8u, 4);
        gp[edx] = this->read(0x17734du, gp[ecx] + 0x5cu, 4);
        gp[eax] = this->read(0x177350u, gp[edx] + 4u, 4);
        gp[ecx] = this->read(0x177353u, gp[eax] + 2u, 4);
        this->store(0x177356u, gp[esp] + 0x1cu, 4, gp[ecx]);
    }

    void resolve() {
        auto &gp = this->gp;
        select_element();
        resolve_element(0x1772dfu, 0x1772e4u);
        gp[edx] = this->read(0x1772e4u, gp[eax] + 4u, 4);
        routed(0x1772e7u, 0x1772ecu, reference_accessor);
        this->test32(gp[eax]);
        if (this->flags.zf) {
            this->reached(missing_accessor);
            continuation = descendants;
            this->publish(0x1772eeu);
            return;
        }

        resolve_element(0x1772f6u, 0x1772fbu);
        gp[edx] = this->read(0x1772fbu, gp[eax] + 4u, 4);
        gp[ecx] = this->read(0x1772feu, 0x4f0414u, 4);
        resolve_shape_definition();
        stage_shape();
        resolve_element(0x17735cu, 0x177361u);
        gp[edi] = gp[eax];
        set_low_byte(gp[edx], this->read(0x177363u, gp[edi] + 0x16u, 1));
        set_low_byte(gp[edx], this->flags.right(gp[edx] & 0xffu, 5u, 1, false));
        this->flags.logic(gp[edx] & 1u, 1);
        this->store(0x17736cu, gp[esp] + 0x10u, 4, gp[edi]);
        if (this->flags.zf) {
            this->reached(begin_draw);
            continuation = draw;
        } else {
            this->reached(skip_draw);
            continuation = descendants;
        }
        this->publish(0x177370u);
    }

    void run() {
        try {
            resolve();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Dispatch>
std::uint32_t bridge(State &state, Reference reference, Dispatch dispatch) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_ENTRY");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_ENTRY");
    static VerifySummary summary{"Particle draw entry"};
    std::uint32_t continuation = retained_entry;
    const auto verify_reference = [&](State &ref) {
        if (reference(ref) != continuation) throw std::runtime_error("Particle draw entry continuation differed");
    };
    run_owner(state, 0x1772c0u, 0x177370u, reference_mode, verify_mode, coverage(), summary, verify_reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State, Dispatch>{value, cursor, trace, dispatch, continuation};
        });
    return continuation;
}
} // namespace halo2::native_particle_draw_entry_detail
