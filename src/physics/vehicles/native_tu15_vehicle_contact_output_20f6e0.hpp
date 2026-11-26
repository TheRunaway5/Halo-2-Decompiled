#pragma once

#include "physics/vehicles/native_tu15_vehicle_wheel_output_20e91f.hpp"

namespace halo2::native_vehicle_contact_output_20f6e0_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t advance_contact = 0x20b000u;
inline constexpr std::uint32_t sites[]{
    0x20ade0u, 0x20ade5u, 0x20adedu, 0x20adf2u, 0x20adf7u, 0x20adffu, 0x20ae04u,
    0x20ae09u, 0x20ae11u, 0x20ae16u, 0x20ae1cu, 0x20ae22u, 0x20ae29u, 0x20ae2cu,
    0x20ae31u, 0x20ae36u, 0x20ae3bu, 0x20ae3fu, 0x20ae44u, 0x20ae49u, 0x20ae4du,
    0x20ae52u, 0x20ae57u, 0x20ae5cu, 0x20ae61u, 0x20ae66u, 0x20ae6bu, 0x20ae70u,
    0x20ae75u, 0x20ae7au, 0x20ae7eu, 0x20ae83u, 0x20ae88u, 0x20ae8cu, 0x20ae91u,
    0x20ae96u, 0x20ae9bu, 0x20aea0u, 0x20aea4u, 0x20aea9u, 0x20aeaeu, 0x20aeb2u,
    0x20aeb6u, 0x20aebbu, 0x20aec0u, 0x20aec5u, 0x20aecau, 0x20aecfu, 0x20aed4u,
    0x20aedbu, 0x20aee0u, 0x20aee4u, 0x20aee8u, 0x20aeedu, 0x20aef2u, 0x20aef7u,
    0x20aefcu, 0x20af01u, 0x20af06u, 0x20af0au, 0x20af0cu, 0x20af12u, 0x20af18u,
    0x20af1au, 0x20af20u, 0x20af25u, 0x20af29u, 0x20af2cu, 0x20af31u, 0x20af35u,
    0x20af3au, 0x20af3eu, 0x20af42u, 0x20af45u, 0x20af4au, 0x20af53u, 0x20af5au,
    0x20af61u, 0x20af64u, 0x20af69u, 0x20af6eu, 0x20af77u, 0x20af7eu, 0x20af85u,
    0x20af8eu, 0x20af93u, 0x20af98u, 0x20af9au, 0x20af9fu, 0x20afa6u, 0x20afadu,
    0x20afb4u, 0x20afb8u, 0x20afbcu, 0x20afc0u, 0x20afc2u, 0x20afc6u, 0x20afcdu,
    0x20afd2u, 0x20afd4u, 0x20afd8u, 0x20afdau, 0x20afe0u, 0x20afe5u, 0x20afe7u,
    0x20afedu, 0x20aff2u,
};

inline void store_projection_magnitude_reference(MotionState &state) {
    spill_x87_float_and_pop(state, [&](std::uint32_t result) {
        w(state.memory, state.observe, 0x20afd4u, state.gp[4] + 0x14u, 4, result);
    });
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
using native_vehicle_wheel_output_20e91f_detail::accumulate_component;

inline void combine_contact_forces(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    const auto &contact = state.gp[7], &stack = state.gp[4];
    accumulate_component(io, x[0], {0x20ade0u, 0x20ade5u, 0x20adedu}, contact + 0x58u, contact + 0x94u, contact + 0x58u);
    accumulate_component(io, x[0], {0x20adf2u, 0x20adf7u, 0x20adffu}, contact + 0x5cu, contact + 0x98u, contact + 0x5cu);
    accumulate_component(io, x[0], {0x20ae04u, 0x20ae09u, 0x20ae11u}, contact + 0x60u, contact + 0x9cu, contact + 0x60u);
    x[4] = io.scalar(0x20ae16u, stack + 0x58u);
    x[3] = io.scalar(0x20ae1cu, stack + 0x5cu);
    state.gp[0] = io.word(0x20ae22u, stack + 0x5a4u);

    io.at(0x20ae29u);
    x[0] = x[1];
    x[0] = ordered_add_ss(x[0], io.scalar(0x20ae2cu, contact + 0x58u));
    io.store(0x20ae31u, contact + 0x58u, x[0]);
    x[0] = io.scalar(0x20ae36u, contact + 0x5cu);
    io.at(0x20ae3bu);
    x[0] = ordered_add_ss(x[0], x[4]);
    io.store(0x20ae3fu, contact + 0x5cu, x[0]);
    x[0] = io.scalar(0x20ae44u, contact + 0x60u);
    io.at(0x20ae49u);
    x[0] = ordered_add_ss(x[0], x[3]);
    io.store(0x20ae4du, contact + 0x60u, x[0]);
}

inline void contact_torque(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    const auto &contact = state.gp[7];
    // The force and lever arm can share provider backing with the output.
    // Preserve the original reloads and the component publication order.
    x[0] = io.scalar(0x20ae52u, contact + 0x5cu);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20ae57u, contact + 0x40u));
    x[2] = io.scalar(0x20ae5cu, contact + 0x44u);
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20ae61u, contact + 0x58u));
    x[5] = io.scalar(0x20ae66u, contact + 0x60u);
    x[5] = ordered_mul_ss(x[5], io.scalar(0x20ae6bu, contact + 0x40u));
    x[6] = io.scalar(0x20ae70u, contact + 0x48u);
    x[6] = ordered_mul_ss(x[6], io.scalar(0x20ae75u, contact + 0x5cu));
    io.at(0x20ae7au);
    x[0] = ordered_difference_ss(x[0], x[2]);
    x[2] = io.scalar(0x20ae7eu, contact + 0x48u);
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20ae83u, contact + 0x58u));
    io.at(0x20ae88u);
    x[2] = ordered_difference_ss(x[2], x[5]);
    x[5] = io.scalar(0x20ae8cu, contact + 0x44u);
    x[5] = ordered_mul_ss(x[5], io.scalar(0x20ae91u, contact + 0x60u));
    io.store(0x20ae96u, contact + 0x68u, x[2]);
    io.store(0x20ae9bu, contact + 0x6cu, x[0]);
    io.at(0x20aea0u);
    x[5] = ordered_difference_ss(x[5], x[6]);
    io.store(0x20aea4u, contact + 0x64u, x[5]);
}

inline void publish_contact_outputs(MotionState &state, Provider &io) {
    auto &value = state.xmm[0];
    auto &output = state.gp[0];
    const auto &contact = state.gp[7];
    accumulate_component(io, value, {0x20aea9u, 0x20aeaeu, 0x20aeb2u}, contact + 0x58u, output, output);
    accumulate_component(io, value, {0x20aeb6u, 0x20aebbu, 0x20aec0u}, contact + 0x5cu, output + 4u, output + 4u);
    accumulate_component(io, value, {0x20aec5u, 0x20aecau, 0x20aecfu}, contact + 0x60u, output + 8u, output + 8u);
    output = io.word(0x20aed4u, state.gp[4] + 0x5a8u);
    accumulate_component(io, value, {0x20aedbu, 0x20aee0u, 0x20aee4u}, contact + 0x64u, output, output);
    accumulate_component(io, value, {0x20aee8u, 0x20aeedu, 0x20aef2u}, contact + 0x68u, output + 4u, output + 4u);
    accumulate_component(io, value, {0x20aef7u, 0x20aefcu, 0x20af01u}, contact + 0x6cu, output + 8u, output + 8u);
}

inline bool uses_projection(MotionState &state, Provider &io) {
    state.set(0, io.word(0x20af06u, state.gp[4] + 0x13u, 1), 1);
    io.at(0x20af0au);
    state.flags.logic(state.reg(0, 1), 1);
    io.at(0x20af0cu);
    if (state.flags.zf) return false;

    state.set(0, io.word(0x20af12u, state.gp[7] + 0x80u, 1), 1);
    io.at(0x20af18u);
    state.flags.logic(state.reg(0, 1), 1);
    io.at(0x20af1au);
    return !state.flags.zf;
}

inline void apply_signed_projection(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    auto &fp = state.fp;
    const auto &contact = gp[7], &stack = gp[4];

    x[2] = io.scalar(0x20af20u, contact + 0xcu);
    io.at(0x20af25u);
    x[2] = ordered_mul_ss(x[2], x[3]);
    io.at(0x20af29u);
    x[0] = x[1];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20af2cu, contact + 4u));
    io.at(0x20af31u);
    x[0] = ordered_add_ss(x[0], x[2]);
    x[2] = io.scalar(0x20af35u, contact + 8u);
    io.at(0x20af3au);
    x[2] = ordered_mul_ss(x[2], x[4]);
    io.at(0x20af3eu);
    x[0] = ordered_add_ss(x[0], x[2]);

    // Use the source spill widths and both reloads of each projected axis.
    // The x87 length retains the original two additions and square root.
    io.at(0x20af42u);
    x[2] = x[0];
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20af45u, contact + 4u));
    io.store(0x20af4au, stack + 0xa8u, x[2]);
    fp.push(ext(f(io.word(0x20af53u, stack + 0xa8u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20af5au, stack + 0xa8u))));
    io.at(0x20af61u);
    x[2] = x[0];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20af64u, contact + 0xcu));
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20af69u, contact + 8u));
    io.store(0x20af6eu, stack + 0xb0u, x[0]);
    fp.push(ext(f(io.word(0x20af77u, stack + 0xb0u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20af7eu, stack + 0xb0u))));
    io.store(0x20af85u, stack + 0xacu, x[2]);
    x[0] = io.scalar(0x20af8eu, contact + 0xcu);
    x[2] = io.scalar(0x20af93u, contact + 8u);
    io.at(0x20af98u);
    fp.st(1) = add(fp.st(1), fp.st(0));
    fp.pop();
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20af9au, contact + 4u));
    fp.push(ext(f(io.word(0x20af9fu, stack + 0xacu))));
    gp[0] = io.word(0x20afa6u, stack + 0x5a0u);
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20afadu, stack + 0xacu))));
    io.at(0x20afb4u);
    x[0] = ordered_mul_ss(x[0], x[3]);
    io.at(0x20afb8u);
    x[2] = ordered_mul_ss(x[2], x[4]);
    io.at(0x20afbcu);
    x[0] = ordered_add_ss(x[0], x[2]);
    io.at(0x20afc0u);
    fp.st(1) = add(fp.st(1), fp.st(0));
    fp.pop();
    io.at(0x20afc2u);
    x[0] = ordered_add_ss(x[0], x[1]);
    state.flags.scalar_compare(x[0], io.scalar(0x20afc6u, 0x46a0e8u), false);
    x[0] = io.scalar(0x20afcdu, gp[0] + 0x70u);
    io.at(0x20afd2u);
    fp.st(0) = root(fp.st(0));
    io.at(0x20afd4u);
    spill_x87_float_and_pop(state, [&](std::uint32_t result) {
        io.store_word(0x20afd4u, stack + 0x14u, result);
    });

    io.at(0x20afd8u);
    if (state.flags.cf || state.flags.zf) {
        x[0] = ordered_add_ss(x[0], io.scalar(0x20afe7u, stack + 0x14u));
        io.store(0x20afedu, gp[0] + 0x70u, x[0]);
        io.at(0x20aff2u);
    } else {
        x[0] = ordered_difference_ss(x[0], io.scalar(0x20afdau, stack + 0x14u));
        io.store(0x20afe0u, gp[0] + 0x70u, x[0]);
        io.at(0x20afe5u);
    }
}

inline void contact_output(MotionState &state, Provider &io) {
    combine_contact_forces(state, io);
    contact_torque(state, io);
    publish_contact_outputs(state, io);
    if (uses_projection(state, io)) apply_signed_projection(state, io);
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_CONTACT_OUTPUT_20F6E0");
        return value && std::string_view{value} != "0";
    }();
    if (!reference) return run_stage(state, sites, advance_contact, contact_output);
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_contact_output_20f6e0_detail
