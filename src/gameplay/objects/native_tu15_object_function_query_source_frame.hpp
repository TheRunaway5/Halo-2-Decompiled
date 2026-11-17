#pragma once

#include "native_tu15_object_function_query_kernel.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

namespace halo2::native_object_function_query_detail {
using namespace physics_world_runtime_detail;

template<bool Literal, class State, class Children>
class SourceFrame {
    State& state;
    Children& children;

    void step(std::uint32_t pc) {
        if constexpr (Literal) state.instruction_literal(pc);
        else if (state.services.instruction) state.services.instruction(pc);
    }
    std::uint32_t read(std::uint32_t address, unsigned width = 4) {
        return r(state.memory, address, width);
    }
    std::uint32_t symbol(std::uint32_t pc, std::uint32_t original) {
        return source_object_function_compiled_symbol_tu15(state.memory, pc, original);
    }
    void compare_symbol(std::uint32_t pc, std::uint32_t original, unsigned reg = 1) {
        step(pc);
        state.flags.minus(state.reg(reg, 4, 0), symbol(pc, original), 4);
    }
    bool greater(std::uint32_t pc) {
        step(pc);
        return !state.flags.zf && state.flags.sf == state.flags.of;
    }
    bool equal(std::uint32_t pc) {
        step(pc);
        return state.flags.zf;
    }
    bool below_or_equal(std::uint32_t pc) {
        step(pc);
        return state.flags.cf || state.flags.zf;
    }
    void load_scalar(std::uint32_t pc, unsigned reg, std::uint32_t address) {
        step(pc);
        state.xmm[reg] = _mm_set_ss(f(read(address)));
    }
    void constant_zero() {
        step(0xc91ffu);
        state.xmm[0] = _mm_xor_ps(state.xmm[0], state.xmm[0]);
        step(0xc9202u);
    }
    void constant_one() {
        load_scalar(0xc925fu, 0, 0x46a0d0u);
        step(0xc9267u);
    }
    void byte_scale() {
        step(0xc9140u);
        state.xmm[0] = _mm_mul_ss(state.xmm[0], _mm_set_ss(f(read(0x46a0c4u))));
    }
    void return_query(std::uint32_t pc) {
        step(pc);
        state.pop();
        state.gp[4] += 16u;
    }
    Selection select_low() {
        compare_symbol(0xc90a5u, 0x90006aau);
        if (greater(0xc90abu)) {
            compare_symbol(0xc910fu, 0xb0006abu);
            if (equal(0xc9115u)) return Selection::complement_progress;
            compare_symbol(0xc9117u, 0xd00059du);
            if (equal(0xc911du)) return Selection::byte_243;
            compare_symbol(0xc911fu, 0xe00059fu);
            return equal(0xc9125u) ? Selection::field_208 : Selection::linked;
        }
        if (equal(0xc90adu)) return Selection::progress;
        compare_symbol(0xc90afu, 0x5000581u);
        if (equal(0xc90b5u)) return Selection::effect_ratio;
        step(0xc90b7u);
        state.flags.minus(state.reg(1, 4, 0), 0x6000087u, 4);
        if (equal(0xc90bdu)) return Selection::field_2c4;
        compare_symbol(0xc90bfu, 0x90005a1u);
        return equal(0xc90c5u) ? Selection::bit_gate : Selection::linked;
    }
    Selection select_high() {
        compare_symbol(0xc91a6u, 0x140005a2u);
        if (greater(0xc91acu)) {
            compare_symbol(0xc9207u, 0x140005a3u);
            if (equal(0xc920du)) return Selection::linked_sentinel;
            compare_symbol(0xc920fu, 0x160005a0u);
            if (equal(0xc9215u)) return Selection::field_264;
            compare_symbol(0xc9217u, 0x1800059eu);
            return equal(0xc921du) ? Selection::byte_1f8 : Selection::linked;
        }
        if (equal(0xc91aeu)) return Selection::object_sentinel;
        step(0xc91b0u);
        state.set(0, state.reg(1, 4, 0), 4, 0);
        step(0xc91b2u);
        state.set(0, state.flags.minus(state.reg(0, 4, 0), 0x11000088u, 4), 4, 0);
        if (equal(0xc91b7u)) return Selection::field_2b0;
        step(0xc91b9u);
        state.set(0, state.flags.minus(state.reg(0, 4, 0), symbol(0xc91b9u, 0x513u), 4), 4, 0);
        if (equal(0xc91beu)) return Selection::field_25c;
        step(0xc91c0u);
        state.set(0, state.flags.increment(state.reg(0, 4, 0), 4, true), 4, 0);
        return equal(0xc91c1u) ? Selection::field_260 : Selection::linked;
    }

public:
    SourceFrame(State& state_, Children& children_) : state(state_), children(children_) {}

    Selection select() {
        auto& gp = state.gp;
        step(0xc9070u);
        gp[0] = read(gp[4] + 4u);
        step(0xc9074u);
        gp[2] = read(0x4ecbd0u);
        step(0xc907au);
        gp[2] = read(gp[2] + 0x44u);
        step(0xc907du);
        gp[1] = gp[0];
        step(0xc907fu);
        gp[1] = state.flags.logic(gp[1] & 0xffffu, 4);
        step(0xc9085u);
        gp[1] *= 3u;
        step(0xc9088u);
        gp[2] = read(gp[2] + gp[1] * 4u + 8u);
        step(0xc908cu);
        gp[1] = read(gp[4] + 8u);
        compare_symbol(0xc9090u, 0xf0006bfu);
        step(0xc9096u);
        state.push(0xc9096u, gp[3]);
        step(0xc9097u);
        state.set(3, 1u, 1, 0);
        if (greater(0xc9099u)) return select_high();
        if (equal(0xc909fu)) return Selection::field_1b8;
        return select_low();
    }

    void acquire_scalar(Selection selection) {
        auto& gp = state.gp;
        switch (selection) {
        case Selection::effect_ratio:
            step(0xc9101u);
            state.push(0xc9101u, 0xc9106u);
            children.effect_ratio(state);
            step(0xc9106u);
            return;
        case Selection::progress:
            step(0xc9108u);
            state.push(0xc9108u, 0xc910du);
            children.progress(state);
            step(0xc910du);
            return;
        case Selection::complement_progress:
            step(0xc9186u);
            state.push(0xc9186u, 0xc918bu);
            children.progress(state);
            step(0xc918bu);
            state.xmm[1] = state.xmm[0];
            step(0xc918eu);
            state.xmm[0] = _mm_set_ss(f(read(0x46a0d0u)));
            step(0xc9196u);
            state.xmm[0] = _mm_sub_ss(state.xmm[0], state.xmm[1]);
            step(0xc919au);
            return;
        case Selection::byte_243:
            step(0xc9135u);
            gp[2] = read(gp[2] + 0x243u, 1);
            step(0xc913cu);
            state.xmm[0] = _mm_cvtsi32_ss(state.xmm[0], si(gp[2]));
            return byte_scale();
        case Selection::byte_1f8:
            step(0xc9239u);
            gp[1] = read(gp[2] + 0x1f8u, 1);
            step(0xc9240u);
            state.xmm[0] = _mm_cvtsi32_ss(state.xmm[0], si(gp[1]));
            step(0xc9244u);
            return byte_scale();
        default:
            break;
        }
        std::uint32_t load{}, offset{}, jump{};
        switch (selection) {
        case Selection::field_2c4: load = 0xc90f7u; offset = 0x2c4u; jump = 0xc90ffu; break;
        case Selection::field_208: load = 0xc912bu; offset = 0x208u; jump = 0xc9133u; break;
        case Selection::field_1b8: load = 0xc919cu; offset = 0x1b8u; jump = 0xc91a4u; break;
        case Selection::field_2b0: load = 0xc91ddu; offset = 0x2b0u; jump = 0xc91e5u; break;
        case Selection::field_25c: load = 0xc91d0u; offset = 0x25cu; jump = 0xc91d8u; break;
        case Selection::field_260: load = 0xc91c3u; offset = 0x260u; jump = 0xc91cbu; break;
        case Selection::field_264: load = 0xc9249u; offset = 0x264u; jump = 0xc9251u; break;
        default: throw std::logic_error("Unexpected C9070 scalar selection");
        }
        step(load);
        state.xmm[0] = _mm_set_ss(f(read(gp[2] + offset)));
        step(jump);
    }

    void clamp_scalar() {
        step(0xc9148u);
        state.xmm[1] = _mm_xor_ps(state.xmm[1], state.xmm[1]);
        step(0xc914bu);
        state.flags.scalar_compare(state.xmm[1], state.xmm[0], false);
        if (below_or_equal(0xc914eu)) {
            load_scalar(0xc9150u, 1, 0x46a0d0u);
            step(0xc9158u);
            state.flags.scalar_compare(state.xmm[0], state.xmm[1], false);
            if (below_or_equal(0xc915bu)) return;
        }
        step(0xc915du);
        state.xmm[0] = state.xmm[1];
    }

    void publish_bit_gate() {
        auto& gp = state.gp;
        step(0xc90cbu);
        state.set(0, read(gp[2] + 0x10au, 1), 1, 0);
        step(0xc90d1u);
        state.set(0, state.flags.right(state.reg(0, 1, 0), 2, 1, false), 1, 0);
        step(0xc90d4u);
        state.flags.logic(state.reg(3, 1, 0) & state.reg(0, 1, 0), 1);
        if (!equal(0xc90d6u)) return constant_zero();
        step(0xc90dcu);
        gp[1] = read(gp[2] + 0x134u);
        step(0xc90e2u);
        gp[1] = state.flags.right(gp[1], 18, 4, false);
        step(0xc90e5u);
        state.flags.logic(state.reg(3, 1, 0) & state.reg(1, 1, 0), 1);
        if (!equal(0xc90e7u)) return constant_zero();
        load_scalar(0xc90edu, 0, 0x46a0d0u);
        step(0xc90f5u);
    }

    void publish_object_sentinel() {
        auto& gp = state.gp;
        step(0xc91eau);
        state.flags.minus(read(gp[2] + 0x248u), 0xffffffffu, 4);
        if (!equal(0xc91f1u)) return constant_one();
        step(0xc91f3u);
        gp[0] = read(gp[2] + 0x134u);
        step(0xc91f9u);
        gp[0] = state.flags.right(gp[0], 1, 4, false);
        step(0xc91fbu);
        state.flags.logic(state.reg(0, 1, 0) & 1u, 1);
        if (!equal(0xc91fdu)) return constant_one();
        constant_zero();
    }

    void publish_linked_sentinel() {
        step(0xc9256u);
        state.flags.minus(read(state.gp[2] + 0x24cu), 0xffffffffu, 4);
        if (equal(0xc925du)) return constant_zero();
        constant_one();
    }

    void publish_scalar() {
        auto& gp = state.gp;
        step(0xc9160u);
        state.flags.scalar_compare(state.xmm[0], _mm_set_ss(f(read(0x46a0e8u))), false);
        step(0xc9167u);
        gp[2] = read(gp[4] + 16u);
        step(0xc916bu);
        w(state.memory, state.observe, 0xc916bu, gp[2], 4, u(_mm_cvtss_f32(state.xmm[0])));
        const bool disabled = below_or_equal(0xc916fu);
        step(disabled ? 0xc926cu : 0xc9175u);
        gp[1] = read(gp[4] + 20u);
        step(disabled ? 0xc9270u : 0xc9179u);
        gp[0] = disabled ? state.flags.logic(0u, 4) : 1u;
        const auto byte_store = disabled ? 0xc9272u : 0xc917eu;
        step(byte_store);
        w(state.memory, state.observe, byte_store, gp[1], 1, state.reg(0, 1, 0));
        step(disabled ? 0xc9274u : 0xc9180u);
        state.set(0, state.reg(3, 1, 0), 1, 0);
        step(disabled ? 0xc9276u : 0xc9182u);
        gp[3] = state.pop();
        return_query(disabled ? 0xc9277u : 0xc9183u);
    }

    bool prepare_inherited() {
        auto& gp = state.gp;
        step(0xc921fu);
        state.set(0, state.flags.logic(0u, 1), 1, 0);
        compare_symbol(0xc9221u, 0xb0005a9u);
        step(0xc9227u);
        state.push(0xc9227u, gp[6]);
        if (equal(0xc9228u)) {
            step(0xc927au);
            gp[6] = symbol(0xc927au, 0xb0005a9u);
        } else {
            compare_symbol(0xc922au, 0x130005a8u);
            if (!equal(0xc9230u)) return false;
            step(0xc9232u);
            gp[6] = symbol(0xc9232u, 0xc000569u);
            step(0xc9237u);
        }
        step(0xc927fu);
        const auto slot = std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(read(gp[2] + 0x212u, 1)));
        state.set(1, std::uint32_t(std::int32_t(slot)), 2, 0);
        step(0xc9287u);
        state.flags.minus(state.reg(1, 2, 0), 0xffffffffu, 2);
        if (equal(0xc928bu)) return false;
        step(0xc928du);
        gp[1] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(gp[1]))));
        step(0xc9290u);
        gp[2] = read(gp[2] + gp[1] * 4u + 0x218u);
        step(0xc9297u);
        state.flags.minus(gp[2], 0xffffffffu, 4);
        return !equal(0xc929au);
    }

    void inherited_query() {
        auto& gp = state.gp;
        step(0xc929cu);
        gp[0] = read(gp[4] + 24u);
        step(0xc92a0u);
        gp[1] = read(gp[4] + 20u);
        step(0xc92a4u);
        state.push(0xc92a4u, gp[0]);
        step(0xc92a5u);
        state.push(0xc92a5u, gp[1]);
        step(0xc92a6u);
        state.push(0xc92a6u, gp[6]);
        step(0xc92a7u);
        state.push(0xc92a7u, gp[2]);
        step(0xc92a8u);
        state.push(0xc92a8u, 0xc92adu);
        children.inherited(state);
    }

    void finish_inherited() {
        step(0xc92adu);
        state.gp[6] = state.pop();
        step(0xc92aeu);
        state.gp[3] = state.pop();
        return_query(0xc92afu);
    }
};

template<bool Literal, class State, class Children>
void run_source_frame(State& state, Children children) {
    SourceFrame<Literal, State, Children> view{state, children};
    evaluate(view);
}

} // namespace halo2::native_object_function_query_detail
