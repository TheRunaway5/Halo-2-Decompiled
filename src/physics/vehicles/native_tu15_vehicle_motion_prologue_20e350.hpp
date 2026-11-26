#pragma once

#include "physics/runtime/native_tu15_two_bone_solve.hpp"
#include "physics/vehicles/native_tu15_vehicle_wheel_stage.hpp"

namespace halo2::native_vehicle_motion_prologue_20e350_detail {
using namespace native_vehicle_wheel_stage_detail;
using namespace native_object_tick_detail::boundary;

inline constexpr std::uint32_t wheel_setup = 0x209b14u;
inline constexpr std::uint32_t skip_wheels = 0x20a132u;
inline constexpr std::uint32_t forward_vector_call = 0x209ab9u;
inline constexpr std::uint32_t forward_vector_return = 0x209abeu;

inline constexpr std::uint32_t sites[]{
    0x209a50u, 0x209a56u, 0x209a5cu, 0x209a5fu, 0x209a60u, 0x209a61u, 0x209a62u,
    0x209a63u, 0x209a6au, 0x209a6cu, 0x209a6fu, 0x209a71u, 0x209a77u, 0x209a7au,
    0x209a7eu, 0x209a84u, 0x209a8au, 0x209a8du, 0x209a93u, 0x209a96u, 0x209a99u,
    0x209a9bu, 0x209aa2u, 0x209aa9u, 0x209aaau, 0x209ab1u, 0x209ab5u, forward_vector_call,
    forward_vector_return, 0x209ac3u, 0x209acau, 0x209accu, 0x209aceu, 0x209ad0u,
    0x209ad3u, 0x209ad6u, 0x209ad9u, 0x209adcu, 0x209adeu, 0x209ae5u, 0x209ae7u,
    0x209aeau, 0x209aedu, 0x209af0u, 0x209af3u, 0x209af6u, 0x209af8u, 0x209afau,
    0x209afeu, 0x209b04u, 0x209b0au, 0x209b0eu,
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline void push(MotionState &state, Provider &io, std::uint32_t pc, std::uint32_t value, std::uint32_t next) {
    io.at(pc);
    native_two_bone_solve::push_word(state, pc, value);
    if (!io.admitted()) throw Resume{next};
}

inline void open_workspace(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    auto &stack = gp[esp];
    io.at(0x209a50u);
    stack = flags.minus(stack, 0x58cu, 4);
    gp[edx] = io.word(0x209a56u, 0x4ecbd0u);
    gp[edx] = io.word(0x209a5cu, gp[edx] + 0x44u);
    push(state, io, 0x209a5fu, gp[ebx], 0x209a60u);
    push(state, io, 0x209a60u, gp[ebp], 0x209a61u);
    push(state, io, 0x209a61u, gp[esi], 0x209a62u);
    push(state, io, 0x209a62u, gp[edi], 0x209a63u);
    gp[edi] = io.word(0x209a63u, stack + 0x5a0u);
    gp[eax] = io.word(0x209a6au, gp[edi]);
    gp[ebp] = io.word(0x209a6cu, gp[edi] + 4u);
    io.at(0x209a6fu);
    gp[ecx] = gp[eax];
    io.at(0x209a71u);
    gp[ecx] = flags.logic(gp[ecx] & 0xffffu, 4);
    io.at(0x209a77u);
    gp[ecx] *= 3u;
    gp[esi] = io.word(0x209a7au, gp[edx] + gp[ecx]*4u + 8u);
    gp[ecx] = io.word(0x209a7eu, gp[esi] + 0xb4u);
    gp[edx] = io.word(0x209a84u, 0x52be5cu);
    gp[ebx] = io.word(0x209a8au, gp[edx] + 0x44u);
    io.at(0x209a8du);
    gp[ecx] = flags.logic(gp[ecx] & 0xffffu, 4);
    io.at(0x209a93u);
    gp[ecx] *= 5u;
    io.at(0x209a96u);
    gp[ecx] = flags.shift(gp[ecx], 5u, 4);
    io.at(0x209a99u);
    gp[ecx] = flags.plus(gp[ecx], gp[ebx], 4);
    io.store_word(0x209a9bu, stack + 0xccu, gp[ecx]);
    io.at(0x209aa2u);
    gp[ecx] = stack + 0xc0u;
    push(state, io, 0x209aa9u, gp[ecx], 0x209aaau);
    io.at(0x209aaau);
    gp[ecx] = stack + 0xb8u;
    io.store_word(0x209ab1u, stack + 0x54u, gp[esi]);
    io.store_word(0x209ab5u, stack + 0x24u, gp[ebp]);
}

inline std::uint32_t initialize_outputs(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto &stack = gp[esp];
    gp[eax] = io.word(forward_vector_return, 0x474d04u);
    gp[ecx] = io.word(0x209ac3u, stack + 0x5a4u);
    io.at(0x209acau);
    gp[edx] = gp[eax];
    gp[ebx] = io.word(0x209accu, gp[edx]);
    io.store_word(0x209aceu, gp[ecx], gp[ebx]);
    gp[ebx] = io.word(0x209ad0u, gp[edx] + 4u);
    io.store_word(0x209ad3u, gp[ecx] + 4u, gp[ebx]);
    gp[edx] = io.word(0x209ad6u, gp[edx] + 8u);
    io.store_word(0x209ad9u, gp[ecx] + 8u, gp[edx]);
    gp[edx] = io.word(0x209adcu, gp[eax]);
    gp[ecx] = io.word(0x209adeu, stack + 0x5a8u);
    io.store_word(0x209ae5u, gp[ecx], gp[edx]);
    gp[edx] = io.word(0x209ae7u, gp[eax] + 4u);
    io.store_word(0x209aeau, gp[ecx] + 4u, gp[edx]);
    gp[eax] = io.word(0x209aedu, gp[eax] + 8u);
    io.store_word(0x209af0u, gp[ecx] + 8u, gp[eax]);
    gp[eax] = io.word(0x209af3u, gp[ebp] + 0x3cu);
    io.at(0x209af6u);
    gp[ebx] = flags.logic(0u, 4);
    io.at(0x209af8u);
    flags.minus(gp[eax], gp[ebx], 4);
    io.store_word(0x209afau, stack + 0x1cu, gp[ebx]);
    io.at(0x209afeu);
    if (flags.zf || flags.sf != flags.of) return skip_wheels;
    gp[ecx] = io.word(0x209b04u, 0x474d1cu);
    io.store_word(0x209b0au, stack + 0x24u, gp[ebx]);
    io.at(0x209b0eu);
    gp[edi] = flags.plus(gp[edi], 0xe0u, 4);
    return wheel_setup;
}
#endif

template<class Dispatch>
std::uint32_t bridge(MotionState &state, Dispatch dispatch) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_VEHICLE_MOTION_PROLOGUE_20E350");
        return value && *value;
    }();
    if (!reference) return run_stage_with_exit(state, sites, [&](MotionState &live, Provider &io) {
        open_workspace(live, io);
        io.at(forward_vector_call);
        native_two_bone_solve::push_word(live, forward_vector_call, 0x20e3beu);
        dispatch(live, 0xbc200u, forward_vector_call);
        if (!io.admitted()) return forward_vector_return;
        return initialize_outputs(live, io);
    });
#else
    (void)state;
    (void)dispatch;
#endif
    return 0;
}

} // namespace halo2::native_vehicle_motion_prologue_20e350_detail
