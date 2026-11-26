#pragma once

#include "physics/vehicles/native_tu15_vehicle_wheel_response_20e6c3.hpp"

namespace halo2::native_vehicle_contact_collection_20ea64_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t prepare_collection = 0x20a164u;
inline constexpr std::uint32_t begin_collection = 0x20a17du;
inline constexpr std::uint32_t next_wheel = 0x20a190u;
inline constexpr std::uint32_t apply_contact = 0x20a2d7u;
inline constexpr std::uint32_t collect_child = 0x20a178u;
inline constexpr std::uint32_t contact_child = 0x20a2d2u;
inline constexpr std::uint32_t finish_collection = 0x20b018u;
inline constexpr std::uint32_t skip_wheel = 0x20aff4u;
inline constexpr std::uint32_t use_alternate = 0x20adc1u;
inline constexpr std::uint32_t project_contact = 0x20a592u;
inline constexpr std::uint32_t inspect_contact = 0x20a3f7u;

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
using native_vehicle_wheel_response_20e6c3_detail::ordered_divide_ss;

inline constexpr std::uint32_t arguments_sites[]{
    0x20a164u, 0x20a167u, 0x20a16du, 0x20a16eu, 0x20a175u, 0x20a176u, 0x20a177u,
};
inline constexpr std::uint32_t wheel_sites[]{
    0x20a17du, 0x20a180u, 0x20a184u, 0x20a18au, 0x20a190u, 0x20a194u, 0x20a19bu, 0x20a19du,
    0x20a1a3u, 0x20a1aau, 0x20a1b0u, 0x20a1b2u, 0x20a1b8u, 0x20a1bcu, 0x20a1bfu,
    0x20a1c4u, 0x20a1c9u, 0x20a1cbu, 0x20a1ceu, 0x20a1d2u, 0x20a1d4u, 0x20a1d7u,
    0x20a1e0u, 0x20a1e9u, 0x20a1f2u, 0x20a1f4u, 0x20a1f6u, 0x20a1fau, 0x20a1ffu,
    0x20a201u, 0x20a205u, 0x20a208u, 0x20a20bu, 0x20a210u, 0x20a215u, 0x20a21au, 0x20a21fu,
    0x20a224u, 0x20a229u, 0x20a22eu, 0x20a233u, 0x20a237u, 0x20a239u, 0x20a23cu, 0x20a23fu,
    0x20a244u, 0x20a249u, 0x20a24au, 0x20a24du, 0x20a252u, 0x20a256u, 0x20a259u, 0x20a25eu,
    0x20a263u, 0x20a268u, 0x20a271u, 0x20a275u, 0x20a278u, 0x20a27cu, 0x20a280u, 0x20a285u,
    0x20a289u, 0x20a28du, 0x20a291u, 0x20a295u, 0x20a29au, 0x20a2a3u, 0x20a2a8u, 0x20a2adu,
    0x20a2b6u, 0x20a2bau, 0x20a2c1u, 0x20a2c3u, 0x20a2c5u, 0x20a2c9u, 0x20a2cdu,
};
inline constexpr std::uint32_t contact_sites[]{
    0x20a2d7u, 0x20a2dau, 0x20a2dcu, 0x20a2deu, 0x20a2e2u, 0x20a2e7u, 0x20a2ebu,
    0x20a2f0u, 0x20a2f5u, 0x20a2fau, 0x20a2ffu, 0x20a304u, 0x20a309u, 0x20a30eu,
    0x20a316u, 0x20a31du, 0x20a323u, 0x20a328u, 0x20a330u, 0x20a335u, 0x20a33du, 0x20a343u,
    0x20a345u, 0x20a348u, 0x20a34bu, 0x20a34fu, 0x20a353u, 0x20a35bu, 0x20a35fu,
    0x20a365u, 0x20a369u, 0x20a36eu, 0x20a372u, 0x20a376u, 0x20a37bu, 0x20a380u, 0x20a388u,
    0x20a38cu, 0x20a395u, 0x20a399u, 0x20a3a0u, 0x20a3a4u, 0x20a3a9u, 0x20a3b1u,
    0x20a3b4u, 0x20a3b9u, 0x20a3c1u, 0x20a3c6u, 0x20a3cau, 0x20a3ceu, 0x20a3d1u,
    0x20a3d4u, 0x20a3d8u, 0x20a3e0u, 0x20a3e5u, 0x20a3e9u, 0x20a3f1u,
};

inline void push_argument(MotionState &state, Provider &io, std::uint32_t pc, std::uint32_t value) {
    io.at(pc);
    auto &stack = state.gp[4];
    stack -= 4u;
    io.store_word(pc, stack, value);
}

inline void prepare_collection_arguments(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    gp[1] = io.word(0x20a164u, gp[5] + 0x18u);
    gp[2] = io.word(0x20a167u, gp[6] + 0xb4u);
    push_argument(state, io, 0x20a16du, gp[0]);
    io.at(0x20a16eu);
    gp[0] = gp[4] + 0x1e0u;
    push_argument(state, io, 0x20a175u, gp[0]);
    push_argument(state, io, 0x20a176u, gp[1]);
    push_argument(state, io, 0x20a177u, gp[2]);
}

inline bool begin_wheels(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    flags.minus(io.word(0x20a17du, gp[5] + 0x44u), gp[3], 4);
    io.store_word(0x20a180u, gp[4] + 0x1cu, gp[3]);
    io.at(0x20a184u);
    if (flags.zf || flags.sf != flags.of) return false;
    io.at(0x20a18au); // Original LEA EBX,[EBX] is an observed instruction.
    return true;
}

inline bool select_wheel(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto &stack = gp[4];
    gp[2] = io.word(0x20a190u, stack + 0x1cu);
    gp[5] = io.word(0x20a194u, stack + 0x5a0u);
    io.at(0x20a19bu);
    gp[0] = gp[2];
    io.at(0x20a19du);
    gp[0] = flags.product(gp[0], 0xd8u);
    io.at(0x20a1a3u);
    gp[7] = gp[5] + gp[0] + 0xb10u;
    state.set(0, io.word(0x20a1aau, gp[7] + 0xb3u, 1), 1, 0);
    io.at(0x20a1b0u);
    flags.logic(state.reg(0, 1, 0), 1);
    io.at(0x20a1b2u);
    return flags.zf;
}

inline void place_wheel_contact(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    auto &x = state.xmm;
    const auto &stack = gp[4], &vehicle = gp[5], &wheel = gp[7];

    gp[1] = io.word(0x20a1b8u, stack + 0x20u);
    gp[0] = io.word(0x20a1bcu, gp[1] + 0x48u);
    x[0] = io.scalar(0x20a1bfu, wheel + 0x34u);
    x[0] = ordered_difference_ss(x[0], io.scalar(0x20a1c4u, vehicle + 0x60u));
    io.at(0x20a1c9u);
    gp[6] = gp[2];
    io.at(0x20a1cbu);
    gp[6] = flags.product(gp[6], 0x4cu);
    gp[3] = io.word(0x20a1ceu, stack + 0x34u);
    io.at(0x20a1d2u);
    gp[6] = flags.plus(gp[6], gp[0], 4);
    gp[0] = io.word(0x20a1d4u, gp[6] + 4u);
    x[4] = io.scalar(0x20a1d7u, stack + 0xc0u);
    x[3] = io.scalar(0x20a1e0u, stack + 0xc4u);
    x[2] = io.scalar(0x20a1e9u, stack + 0xc8u);
    io.at(0x20a1f2u);
    gp[0] = flags.right(gp[0], 1, 4, false);
    io.at(0x20a1f4u);
    state.set(0, flags.logic(gp[0] & 1u, 1), 1, 0);
    io.store_word(0x20a1f6u, stack + 0x13u, state.reg(0, 1, 0), 1);

    gp[0] = io.word(0x20a1fau, 0x474d04u);
    gp[1] = io.word(0x20a1ffu, gp[0]);
    io.store_word(0x20a201u, stack + 0x54u, gp[1]);
    gp[1] = io.word(0x20a205u, gp[0] + 4u);
    gp[0] = io.word(0x20a208u, gp[0] + 8u);
    io.store(0x20a20bu, wheel + 0x40u, x[0]);
    x[0] = io.scalar(0x20a210u, wheel + 0x38u);
    x[0] = ordered_difference_ss(x[0], io.scalar(0x20a215u, vehicle + 0x64u));
    io.store(0x20a21au, wheel + 0x44u, x[0]);
    x[0] = io.scalar(0x20a21fu, wheel + 0x3cu);
    x[0] = ordered_difference_ss(x[0], io.scalar(0x20a224u, vehicle + 0x68u));
    io.store(0x20a229u, wheel + 0x48u, x[0]);
    x[5] = io.scalar(0x20a22eu, wheel + 0x48u);
    io.store_word(0x20a233u, stack + 0x58u, gp[1]);
    io.at(0x20a237u);
    gp[1] = gp[2];
    io.at(0x20a239u);
    gp[1] = flags.product(gp[1], 0x3cu);
}

inline void project_wheel_contact(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &x = state.xmm;
    const auto &stack = gp[4], &wheel = gp[7];

    // Cross the wheel offset with its axis. The first component is spilled
    // twice at its original sites, before and after adding the frame offset.
    io.at(0x20a23cu);
    x[1] = x[3];
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a23fu, wheel + 0x40u));
    x[3] = ordered_mul_ss(x[3], io.scalar(0x20a244u, wheel + 0x48u));
    io.at(0x20a249u);
    gp[3] = state.flags.increment(gp[3], 4, false);
    io.at(0x20a24au);
    x[0] = x[4];
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a24du, wheel + 0x44u));
    io.at(0x20a252u);
    x[0] = ordered_difference_ss(x[0], x[1]);
    io.at(0x20a256u);
    x[1] = x[2];
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a259u, wheel + 0x40u));
    x[2] = ordered_mul_ss(x[2], io.scalar(0x20a25eu, wheel + 0x44u));
    io.store(0x20a263u, wheel + 0x54u, x[0]);
    x[0] = io.scalar(0x20a268u, stack + 0xb4u);
    io.store_word(0x20a271u, stack + 0x34u, gp[3]);
    io.at(0x20a275u);
    gp[3] = wheel + 0x4cu;
    io.at(0x20a278u);
    x[5] = ordered_mul_ss(x[5], x[4]);
    io.at(0x20a27cu);
    x[1] = ordered_difference_ss(x[1], x[5]);
    io.store(0x20a280u, gp[3] + 4u, x[1]);
    io.at(0x20a285u);
    x[3] = ordered_difference_ss(x[3], x[2]);
    io.store(0x20a289u, gp[3], x[3]);
    io.at(0x20a28du);
    x[0] = ordered_add_ss(x[0], x[3]);
    io.store(0x20a291u, gp[3], x[0]);
    x[0] = io.scalar(0x20a295u, gp[3] + 4u);
    x[0] = ordered_add_ss(x[0], io.scalar(0x20a29au, stack + 0xb8u));
    io.store(0x20a2a3u, gp[3] + 4u, x[0]);
    x[0] = io.scalar(0x20a2a8u, gp[3] + 8u);
    x[0] = ordered_add_ss(x[0], io.scalar(0x20a2adu, stack + 0xbcu));
    io.store_word(0x20a2b6u, stack + 0x5cu, gp[0]);
    io.at(0x20a2bau);
    gp[1] = stack + 0x1dcu + gp[1];
    io.at(0x20a2c1u);
    gp[0] = gp[2];
    io.at(0x20a2c3u);
    gp[2] = gp[5];
    io.store_word(0x20a2c5u, stack + 0x24u, gp[6]);
    io.store_word(0x20a2c9u, stack + 0x14u, gp[1]);
    io.store(0x20a2cdu, gp[3] + 8u, x[0]);
}

inline bool apply_surface_offset(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &x = state.xmm;
    auto &flags = state.flags;
    const auto &stack = gp[4], &offset = gp[3], &wheel = gp[7];
    state.set(0, io.word(0x20a2d7u, wheel + 0x70u, 1), 1, 0);
    io.at(0x20a2dau);
    state.set(0, flags.logic(state.reg(0, 1, 0) & 1u, 1), 1, 0);
    io.at(0x20a2dcu);
    if (!flags.zf) {
        x[0] = io.scalar(0x20a2deu, offset);
        x[0] = ordered_difference_ss(x[0], io.scalar(0x20a2e2u, wheel + 0x74u));
        io.store(0x20a2e7u, offset, x[0]);
        x[0] = io.scalar(0x20a2ebu, offset + 4u);
        x[0] = ordered_difference_ss(x[0], io.scalar(0x20a2f0u, wheel + 0x78u));
        io.store(0x20a2f5u, offset + 4u, x[0]);
        x[0] = io.scalar(0x20a2fau, offset + 8u);
        x[0] = ordered_difference_ss(x[0], io.scalar(0x20a2ffu, wheel + 0x7cu));
        io.store(0x20a304u, offset + 8u, x[0]);
        io.store_word(0x20a309u, stack + 0x1bu, 1u, 1);
    }
    x[0] = io.scalar(0x20a30eu, wheel + 0x84u);
    flags.scalar_compare(x[0], io.scalar(0x20a316u, 0x46a0ecu), false);
    io.at(0x20a31du);
    return !flags.cf && !flags.zf;
}

inline std::uint32_t compute_contact_load(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &x = state.xmm;
    const auto &stack = gp[4], &offset = gp[3], &normal = gp[5], &wheel = gp[7];
    x[1] = io.scalar(0x20a323u, offset + 8u);
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a328u, wheel + 0xa8u));
    x[0] = io.scalar(0x20a330u, offset + 4u);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a335u, wheel + 0xa4u));
    gp[2] = io.word(0x20a33du, 0x474d10u);
    gp[0] = io.word(0x20a343u, gp[2]);
    gp[1] = io.word(0x20a345u, gp[2] + 4u);
    gp[2] = io.word(0x20a348u, gp[2] + 8u);
    io.at(0x20a34bu);
    x[1] = ordered_add_ss(x[1], x[0]);
    x[0] = io.scalar(0x20a34fu, offset);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a353u, wheel + 0xa0u));
    io.at(0x20a35bu);
    x[1] = ordered_add_ss(x[1], x[0]);
    io.at(0x20a35fu);
    gp[5] = wheel + 0xa0u;
    io.store_word(0x20a365u, stack + 0x44u, gp[0]);
    gp[0] = io.word(0x20a369u, 0x52be68u);
    x[0] = io.scalar(0x20a36eu, gp[0]);
    gp[0] = io.word(0x20a372u, stack + 0x20u);
    x[0] = ordered_divide_ss(x[0], io.scalar(0x20a376u, gp[0] + 8u));
    x[2] = io.scalar(0x20a37bu, gp[0] + 0xcu);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a380u, wheel + 0x84u));
    io.at(0x20a388u);
    x[2] = ordered_mul_ss(x[2], x[1]);
    io.store(0x20a38cu, stack + 0x94u, x[1]);
    io.store_word(0x20a395u, stack + 0x48u, gp[1]);
    gp[1] = io.word(0x20a399u, stack + 0x5a0u);
    io.at(0x20a3a0u);
    x[0] = ordered_difference_ss(x[0], x[2]);
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20a3a4u, gp[1] + 0x6cu));
    io.store(0x20a3a9u, wheel + 0x88u, x[0]);

    io.at(0x20a3b1u);
    x[1] = x[0];
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20a3b4u, normal));
    io.store(0x20a3b9u, wheel + 0x94u, x[1]);
    x[1] = io.scalar(0x20a3c1u, normal + 4u);
    io.store_word(0x20a3c6u, stack + 0x4cu, gp[2]);
    gp[2] = io.word(0x20a3cau, stack + 0x14u);
    gp[0] = io.word(0x20a3ceu, gp[2] + 0x24u);
    io.at(0x20a3d1u);
    state.flags.minus(gp[0], 0xffffffffu, 4);
    io.at(0x20a3d4u);
    x[1] = ordered_mul_ss(x[1], x[0]);
    io.store(0x20a3d8u, wheel + 0x98u, x[1]);
    x[1] = io.scalar(0x20a3e0u, normal + 8u);
    io.at(0x20a3e5u);
    x[1] = ordered_mul_ss(x[1], x[0]);
    io.store(0x20a3e9u, wheel + 0x9cu, x[1]);
    io.at(0x20a3f1u);
    return state.flags.zf ? project_contact : inspect_contact;
}
#endif

inline std::uint32_t bridge(MotionState &state, std::uint32_t entry) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (entry == prepare_collection)
        return run_stage(state, arguments_sites, collect_child, prepare_collection_arguments);
    if (entry == apply_contact)
        return run_stage_with_exit(state, contact_sites, [](MotionState &live, Provider &io) {
            if (!apply_surface_offset(live, io)) return use_alternate;
            return compute_contact_load(live, io);
        });
    return run_stage_with_exit(state, wheel_sites, [entry](MotionState &live, Provider &io) {
        if (entry == begin_collection && !begin_wheels(live, io)) return finish_collection;
        if (!select_wheel(live, io)) return skip_wheel;
        place_wheel_contact(live, io);
        project_wheel_contact(live, io);
        return contact_child;
    });
#else
    (void)state;
    (void)entry;
    return 0;
#endif
}
} // namespace halo2::native_vehicle_contact_collection_20ea64_detail
