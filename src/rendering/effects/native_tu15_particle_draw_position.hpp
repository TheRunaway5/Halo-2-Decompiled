#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_particle_draw_position_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_world_leaves_detail::ordered_add_ss;
using native_tu15_leaves::ordered_mul_ss;
namespace source = physics_step_detail;

enum class Stage { copy, adjust };
inline constexpr std::uint32_t retained = 0xffffffffu, adjust_position = 0x17770bu;
inline constexpr std::uint32_t draw_result = 0x1777b7u, apply_transform = 0x177759u, submit = 0x177775u;
enum Branch : unsigned {copy_only, copy_with_adjustment, transform, no_transform, branch_count};
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

struct Axis {
    std::uint32_t start;
    std::uint32_t axis_value;
    std::uint32_t store;
    std::uint32_t stack_offset;
    unsigned source_lane;
};
inline constexpr Axis adjust_x{0x17770bu, 0x491e44u, 0x177725u, 0x2cu, 4u};
inline constexpr Axis adjust_y{0x17772bu, 0x491e48u, 0x17773bu, 0x30u, 3u};
inline constexpr Axis adjust_z{0x177741u, 0x491e4cu, 0x177751u, 0x34u, 0u};

template<class State> struct Frame : Port<State> {
    Stage stage;
    std::uint32_t &continuation;
    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Stage operation, std::uint32_t &next)
        : Port<State>(state, cursor, trace, {}), stage(operation), continuation(next) {}

    __m128 load_position(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(source::f(this->read(pc, address, 4)));
    }

    void copy_position() {
        auto &gp = this->gp;
        const auto stack = gp[esp];
        this->test8(gp[ebx]);
        gp[eax] = gp[esi] + 0x10u;
        gp[ecx] = this->read(0x1776dfu, gp[eax], 4);
        gp[edx] = this->read(0x1776e1u, gp[eax] + 4u, 4);
        gp[eax] = this->read(0x1776e4u, gp[eax] + 8u, 4);
        this->store(0x1776e7u, stack + 0x2cu, 4, gp[ecx]);
        this->store(0x1776ebu, stack + 0x30u, 4, gp[edx]);
        this->store(0x1776efu, stack + 0x34u, 4, gp[eax]);
        if (this->flags.zf) {
            this->reached(copy_only);
            continuation = draw_result;
            this->publish(0x1776f3u);
            return;
        }
        auto &xmm = this->s.xmm;
        xmm[0] = load_position(0x1776f9u, stack + 0x34u);
        xmm[3] = load_position(0x1776ffu, stack + 0x30u);
        xmm[4] = load_position(0x177705u, stack + 0x2cu);
        this->reached(copy_with_adjustment);
        continuation = adjust_position;
        this->publish(0x177705u);
    }

    void publish_axis(const Axis &axis) {
        auto &xmm = this->s.xmm;
        xmm[1] = ordered_add_ss(ordered_mul_ss(xmm[1], xmm[7]), xmm[axis.source_lane]);
        this->store(axis.store, this->gp[esp] + axis.stack_offset, 4, source::u(_mm_cvtss_f32(xmm[1])));
    }

    void offset_position() {
        auto &gp = this->gp;
        auto &xmm = this->s.xmm;
        xmm[1] = load_position(adjust_x.start, adjust_x.axis_value);
        gp[edi] = this->read(0x177713u, gp[esp] + 0x20u, 4);
        this->flags.logic(gp[edi] & 0x8000u, 4);
        publish_axis(adjust_x);
        xmm[1] = load_position(adjust_y.start, adjust_y.axis_value);
        publish_axis(adjust_y);
        xmm[1] = load_position(adjust_z.start, adjust_z.axis_value);
        publish_axis(adjust_z);
        const bool needs_transform = !this->flags.zf;
        this->reached(needs_transform ? transform : no_transform);
        continuation = needs_transform ? apply_transform : submit;
        this->publish(0x177757u);
    }

    void run() {
        try {
            if (stage == Stage::copy) copy_position();
            else offset_position();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference>
std::uint32_t bridge(State &state, Stage stage, Reference reference) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_POSITION");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_POSITION");
    static VerifySummary summary{"Particle draw position"};
    std::uint32_t continuation = retained;
    const auto verify_reference = [&](State &ref) {
        if (reference(ref) != continuation) throw std::runtime_error("Particle draw position continuation differed");
    };
    const auto first = stage == Stage::copy ? 0x1776dau : adjust_position;
    const auto last = stage == Stage::copy ? 0x177705u : 0x177757u;
    run_owner(state, first, last, reference_mode, verify_mode, coverage(), summary, verify_reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State>{value, cursor, trace, stage, continuation};
        });
    return continuation;
}
} // namespace halo2::native_particle_draw_position_detail
