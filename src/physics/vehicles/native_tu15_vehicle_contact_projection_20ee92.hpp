#pragma once

#include "physics/vehicles/native_tu15_vehicle_wheel_response_20e6c3.hpp"

namespace halo2::native_vehicle_contact_projection_20ee92_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t skip_projection = 0x20ab16u;
inline constexpr std::uint32_t alternate_response = 0x20a79au;
inline constexpr std::uint32_t continue_response = 0x20a887u;
inline constexpr std::uint32_t sites[]{
    0x20a592u, 0x20a597u, 0x20a59cu, 0x20a5a1u, 0x20a5a6u, 0x20a5abu, 0x20a5b0u,
    0x20a5b5u, 0x20a5bau, 0x20a5beu, 0x20a5c3u, 0x20a5c8u, 0x20a5ccu, 0x20a5cfu,
    0x20a5d4u, 0x20a5d8u, 0x20a5ddu, 0x20a5e1u, 0x20a5e5u, 0x20a5eeu, 0x20a5f5u,
    0x20a5fcu, 0x20a600u, 0x20a604u, 0x20a60au, 0x20a60eu, 0x20a612u, 0x20a618u,
    0x20a61cu, 0x20a61eu, 0x20a622u, 0x20a626u, 0x20a628u, 0x20a62au, 0x20a62eu,
    0x20a630u, 0x20a636u, 0x20a638u, 0x20a63au, 0x20a63cu, 0x20a644u, 0x20a64au,
    0x20a64eu, 0x20a651u, 0x20a655u, 0x20a658u, 0x20a65bu, 0x20a65fu, 0x20a662u,
    0x20a666u, 0x20a66cu, 0x20a675u, 0x20a677u, 0x20a67au, 0x20a681u, 0x20a682u,
    0x20a685u, 0x20a68bu, 0x20a690u, 0x20a695u, 0x20a69au, 0x20a69fu, 0x20a6a3u,
    0x20a6a8u, 0x20a6adu, 0x20a6afu, 0x20a6b1u, 0x20a6b8u, 0x20a6bbu, 0x20a6beu,
    0x20a6c5u, 0x20a6ccu, 0x20a6ceu, 0x20a6d0u, 0x20a6d4u, 0x20a6d9u, 0x20a6ddu,
    0x20a6e4u, 0x20a6e7u, 0x20a6eau, 0x20a6eeu, 0x20a6f7u, 0x20a6feu, 0x20a702u,
    0x20a704u, 0x20a708u, 0x20a70bu, 0x20a714u, 0x20a71bu, 0x20a724u, 0x20a728u,
    0x20a72cu, 0x20a730u, 0x20a739u, 0x20a742u, 0x20a74bu, 0x20a74du, 0x20a750u,
    0x20a758u, 0x20a75bu, 0x20a75eu, 0x20a767u, 0x20a76bu, 0x20a76fu, 0x20a773u,
    0x20a779u, 0x20a781u, 0x20a785u, 0x20a789u, 0x20a78fu, 0x20a795u,
};

inline void store_tangent_length_reference(MotionState &state) {
    spill_x87_float<false>(state, [&](std::uint32_t result) {
        w(state.memory, state.observe, 0x20a62au, state.gp[4] + 0x14u, 4, result);
    });
}

inline void absolute_tangent_length(MotionState &state) {
    state.fp.st(0) = std::fabs(state.fp.st(0));
    state.fp.status &= ~0x200u;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
using native_vehicle_wheel_response_20e6c3_detail::ordered_divide_ss;

inline void find_tangent_length(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &fp = state.fp;
    auto &gp = state.gp;
    const auto &normal = gp[5], &contact = gp[7], &stack = gp[4];

    // Remove the normal component and keep the original binary32 spills
    // and reloads before accumulating the extended-precision length.
    x[0] = io.scalar(0x20a592u, contact + 0xcu);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a597u, normal + 8u));
    x[1] = io.scalar(0x20a59cu, contact + 8u);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a5a1u, normal + 4u));
    x[6] = io.scalar(0x20a5a6u, contact + 4u);
    x[3] = io.scalar(0x20a5abu, normal + 8u);
    x[2] = io.scalar(0x20a5b0u, normal + 4u);
    x[7] = io.scalar(0x20a5b5u, contact + 8u);
    io.at(0x20a5bau);
    x[0] = ordered_add_ss(x[0], x[1]);
    x[1] = io.scalar(0x20a5beu, normal);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a5c3u, contact + 4u));
    io.at(0x20a5c8u);
    x[0] = ordered_add_ss(x[0], x[1]);
    io.at(0x20a5ccu);
    x[1] = x[0];
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a5cfu, normal));
    io.at(0x20a5d4u);
    x[6] = ordered_difference_ss(x[6], x[1]);
    x[1] = io.scalar(0x20a5d8u, contact + 0xcu);
    io.at(0x20a5ddu);
    x[3] = ordered_mul_ss(x[3], x[0]);
    io.at(0x20a5e1u);
    x[1] = ordered_difference_ss(x[1], x[3]);
    io.store(0x20a5e5u, stack + 0x80u, x[1]);
    fp.push(ext(f(io.word(0x20a5eeu, stack + 0x80u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20a5f5u, stack + 0x80u))));
    io.at(0x20a5fcu);
    x[2] = ordered_mul_ss(x[2], x[0]);
    io.at(0x20a600u);
    x[7] = ordered_difference_ss(x[7], x[2]);
    io.store(0x20a604u, stack + 0x7cu, x[7]);
    fp.push(ext(f(io.word(0x20a60au, stack + 0x7cu))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20a60eu, stack + 0x7cu))));
    io.store(0x20a612u, stack + 0x78u, x[6]);
    gp[1] = io.word(0x20a618u, stack + 0x20u);
    io.at(0x20a61cu);
    fp.st(1) = add(fp.st(1), fp.st(0));
    fp.pop();
    fp.push(ext(f(io.word(0x20a61eu, stack + 0x78u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20a622u, stack + 0x78u))));
    io.at(0x20a626u);
    fp.st(1) = add(fp.st(1), fp.st(0));
    fp.pop();
    io.at(0x20a628u);
    fp.st(0) = root(fp.st(0));
    io.at(0x20a62au);
    spill_x87_float<false>(state, [&](std::uint32_t result) {
        io.store_word(0x20a62au, stack + 0x14u, result);
    });
}

inline bool normalize_tangent(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &fp = state.fp;
    auto &flags = state.flags;
    const auto &stack = state.gp[4];

    io.at(0x20a62eu);
    absolute_tangent_length(state);
    fp.push(ext(f(io.word(0x20a630u, 0x46a0ecu))));
    io.at(0x20a636u);
    state.compare_and_pop(fp.st(1));
    io.at(0x20a638u);
    fp.status &= ~0x200u;
    fp.pop();
    io.at(0x20a63au);
    if (!flags.cf && !flags.zf) {
        io.at(0x20a677u);
        x[2] = _mm_setzero_ps();
    } else {
        x[0] = io.scalar(0x20a63cu, 0x46a0d0u);
        x[2] = io.scalar(0x20a644u, stack + 0x14u);
        io.at(0x20a64au);
        x[0] = ordered_divide_ss(x[0], x[2]);
        io.at(0x20a64eu);
        x[3] = x[0];
        io.at(0x20a651u);
        x[3] = ordered_mul_ss(x[3], x[6]);
        io.at(0x20a655u);
        x[6] = x[3];
        io.at(0x20a658u);
        x[3] = x[0];
        io.at(0x20a65bu);
        x[3] = ordered_mul_ss(x[3], x[7]);
        io.at(0x20a65fu);
        x[7] = x[3];
        io.at(0x20a662u);
        x[0] = ordered_mul_ss(x[0], x[1]);
        io.store(0x20a666u, stack + 0x7cu, x[7]);
        io.store(0x20a66cu, stack + 0x80u, x[0]);
        io.at(0x20a675u);
    }

    flags.scalar_compare(x[2], io.scalar(0x20a67au, 0x46a0e8u), true);
    io.at(0x20a681u);
    state.set(0, flags.bits() | 2u, 1, 8);
    io.at(0x20a682u);
    flags.logic(state.reg(0, 1, 8) & 0x44u, 1);
    io.at(0x20a685u);
    return flags.pf;
}

inline bool project_contact_record(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    const auto &normal = gp[5], &record = gp[3], &stack = gp[4];

    x[0] = io.scalar(0x20a68bu, record + 8u);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a690u, normal + 8u));
    x[1] = io.scalar(0x20a695u, record + 4u);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a69au, normal + 4u));
    x[3] = io.scalar(0x20a69fu, record);
    x[4] = io.scalar(0x20a6a3u, record + 4u);
    x[5] = io.scalar(0x20a6a8u, record + 8u);
    io.at(0x20a6adu);
    gp[2] = record;
    gp[0] = io.word(0x20a6afu, gp[2]);
    io.store_word(0x20a6b1u, stack + 0x84u, gp[0]);
    gp[0] = io.word(0x20a6b8u, gp[2] + 4u);
    gp[2] = io.word(0x20a6bbu, gp[2] + 8u);
    io.store_word(0x20a6beu, stack + 0x88u, gp[0]);
    io.store_word(0x20a6c5u, stack + 0x8cu, gp[2]);
    io.at(0x20a6ccu);
    gp[0] = normal;
    gp[2] = io.word(0x20a6ceu, gp[0]);
    io.at(0x20a6d0u);
    x[0] = ordered_add_ss(x[0], x[1]);
    x[1] = io.scalar(0x20a6d4u, normal);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a6d9u, record));
    io.store_word(0x20a6ddu, stack + 0xd0u, gp[2]);
    gp[2] = io.word(0x20a6e4u, gp[0] + 4u);
    gp[0] = io.word(0x20a6e7u, gp[0] + 8u);
    io.at(0x20a6eau);
    x[0] = ordered_add_ss(x[0], x[1]);
    x[1] = io.scalar(0x20a6eeu, stack + 0xd0u);
    io.store_word(0x20a6f7u, stack + 0xd8u, gp[0]);
    state.set(0, io.word(0x20a6feu, stack + 0x13u, 1), 1);
    io.at(0x20a702u);
    state.flags.logic(state.reg(0, 1), 1);
    io.at(0x20a704u);
    x[1] = ordered_mul_ss(x[1], x[0]);
    io.at(0x20a708u);
    x[2] = x[0];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a70bu, stack + 0xd8u));
    io.store_word(0x20a714u, stack + 0xd4u, gp[2]);
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20a71bu, stack + 0xd4u));
    io.at(0x20a724u);
    x[3] = ordered_difference_ss(x[3], x[1]);
    io.at(0x20a728u);
    x[4] = ordered_difference_ss(x[4], x[2]);
    io.at(0x20a72cu);
    x[5] = ordered_difference_ss(x[5], x[0]);
    io.store(0x20a730u, stack + 0x84u, x[3]);
    io.store(0x20a739u, stack + 0x88u, x[4]);
    io.store(0x20a742u, stack + 0x8cu, x[5]);
    io.at(0x20a74bu);
    return !state.flags.zf;
}

inline void correct_projected_record(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    const auto &contact = state.gp[7], &stack = state.gp[4];
    io.at(0x20a74du);
    x[0] = _mm_setzero_ps();
    x[0] = ordered_difference_ss(x[0], io.scalar(0x20a750u, contact + 0x8cu));
    io.at(0x20a758u);
    x[1] = x[0];
    io.at(0x20a75bu);
    x[2] = x[0];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a75eu, stack + 0x80u));
    io.at(0x20a767u);
    x[1] = ordered_mul_ss(x[1], x[6]);
    io.at(0x20a76bu);
    x[1] = ordered_add_ss(x[1], x[3]);
    io.at(0x20a76fu);
    x[2] = ordered_mul_ss(x[2], x[7]);
    io.store(0x20a773u, stack + 0x60u, x[1]);
    x[1] = io.scalar(0x20a779u, 0x46a0d0u);
    io.at(0x20a781u);
    x[2] = ordered_add_ss(x[2], x[4]);
    io.at(0x20a785u);
    x[0] = ordered_add_ss(x[0], x[5]);
    io.store(0x20a789u, stack + 0x64u, x[2]);
    io.store(0x20a78fu, stack + 0x68u, x[0]);
    io.at(0x20a795u);
}

inline std::uint32_t contact_projection(MotionState &state, Provider &io) {
    find_tangent_length(state, io);
    if (!normalize_tangent(state, io)) return skip_projection;
    if (!project_contact_record(state, io)) return alternate_response;
    correct_projected_record(state, io);
    return continue_response;
}
#endif

inline std::uint32_t bridge(MotionState &state) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_CONTACT_PROJECTION_20EE92");
        return value && std::string_view{value} != "0";
    }();
    if (!reference) return run_stage_with_exit(state, sites, contact_projection);
#else
    (void)state;
#endif
    return 0;
}
} // namespace halo2::native_vehicle_contact_projection_20ee92_detail
