#pragma once

#include "physics/vehicles/native_tu15_vehicle_wheel_response_20e6c3.hpp"

namespace halo2::native_vehicle_contact_query_20f3a8_detail {
using namespace native_vehicle_wheel_stage_detail;

inline constexpr std::uint32_t loaded_force = 0x20aaa8u;
inline constexpr std::uint32_t projected_normal = 0x20aafcu;
inline constexpr std::uint32_t unprojected_force = 0x20ab16u;
inline constexpr std::uint32_t query_ready = 0x20ad31u;
inline constexpr std::uint32_t skip_query = 0x20ade0u;

template<std::uint32_t Site, std::uint32_t Offset, bool Pop>
void spill_reference(MotionState &state) {
    spill_x87_float<Pop>(state, [&](auto bits) {
        w(state.memory, state.observe, Site, state.gp[4] + Offset, 4, bits);
    });
}

inline void negate_load(MotionState &state) {
    state.fp.st(0) = -state.fp.st(0);
    state.fp.status &= ~0x200u;
}

inline void absolute_length(MotionState &state) {
    state.fp.st(0) = std::fabs(state.fp.st(0));
    state.fp.status &= ~0x200u;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;
using native_vehicle_wheel_response_20e6c3_detail::ordered_divide_ss;

inline constexpr std::uint32_t sites[]{
    0x20aaa8u, 0x20aaacu, 0x20aab2u, 0x20aab8u, 0x20aabau, 0x20aabdu, 0x20aac0u,
    0x20aac7u, 0x20aacdu, 0x20aacfu, 0x20aad1u, 0x20aad3u, 0x20aad7u, 0x20aaddu,
    0x20aadfu, 0x20aae2u, 0x20aae6u, 0x20aae9u, 0x20aaf0u, 0x20aaf1u, 0x20aaf8u,
    0x20aafcu, 0x20ab00u, 0x20ab04u, 0x20ab08u, 0x20ab0cu, 0x20ab10u, 0x20ab14u,
    0x20ab16u, 0x20ab1cu, 0x20ab23u, 0x20ab28u, 0x20ab2fu, 0x20ab35u, 0x20ab39u,
    0x20ab41u, 0x20ab45u, 0x20ab4bu, 0x20ab53u, 0x20ab56u, 0x20ab5bu, 0x20ab5eu,
    0x20ab64u, 0x20ab66u, 0x20ab6cu, 0x20ab70u, 0x20ab74u, 0x20ab78u, 0x20ab7cu,
    0x20ab7eu, 0x20ab82u, 0x20ab86u, 0x20ab88u, 0x20ab8au, 0x20ab8eu, 0x20ab90u,
    0x20ab96u, 0x20ab98u, 0x20ab9au, 0x20ab9cu, 0x20aba2u, 0x20abaau, 0x20abaeu,
    0x20abb1u, 0x20abb7u, 0x20abbau, 0x20abc0u, 0x20abc1u, 0x20abc4u, 0x20abc7u,
    0x20abcdu, 0x20abd3u, 0x20abd9u, 0x20abdfu, 0x20abe1u, 0x20abe6u, 0x20abe8u,
    0x20abebu, 0x20abeeu, 0x20abf2u, 0x20abf6u, 0x20abfau, 0x20abffu, 0x20ac07u,
    0x20ac0fu, 0x20ac16u, 0x20ac1au, 0x20ac20u, 0x20ac24u, 0x20ac27u, 0x20ac2bu,
    0x20ac2eu, 0x20ac33u, 0x20ac38u, 0x20ac41u, 0x20ac46u, 0x20ac4au, 0x20ac4fu,
    0x20ac58u, 0x20ac5du, 0x20ac65u, 0x20ac6cu, 0x20ac73u, 0x20ac77u, 0x20ac7cu,
    0x20ac85u, 0x20ac8cu, 0x20ac93u, 0x20ac96u, 0x20ac9du, 0x20aca4u, 0x20acabu,
    0x20acafu, 0x20acb6u, 0x20acb9u, 0x20acbcu, 0x20acc3u, 0x20acc9u, 0x20accbu,
    0x20acd2u, 0x20acd5u, 0x20acddu, 0x20ace8u, 0x20acf0u, 0x20acf7u, 0x20acfeu,
    0x20ad05u, 0x20ad0du, 0x20ad15u, 0x20ad1du, 0x20ad1fu, 0x20ad25u, 0x20ad27u,
    0x20ad29u, 0x20ad2bu, 0x20ad2du, 0x20ad2fu,
};

template<bool Pop>
void spill_force(MotionState &state, Provider &io, std::uint32_t site, std::uint32_t address) {
    io.at(site);
    spill_x87_float<Pop>(state, [&](auto bits) { io.store_word(site, address, bits); });
}

inline void apply_contact_load(MotionState &state, Provider &io) {
    auto &stack = state.gp[4];
    auto &fp = state.fp;
    auto &x = state.xmm;
    auto &eax = state.gp[0];
    auto &clock = state.gp[1];
    const auto &wheel = state.gp[6], &contact = state.gp[7];

    // The load is evaluated in x87 precision and independently spilled
    // for each axis before the accumulated-force count is incremented.
    fp.push(ext(f(io.word(0x20aaa8u, stack + 0x3cu))));
    clock = io.word(0x20aaacu, 0x51df54u);
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aab2u, 0x46a194u))));
    io.at(0x20aab8u);
    fp.st(0) = root(fp.st(0));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aabau, wheel + 8u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aabdu, clock + 4u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aac0u, stack + 0x98u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aac7u, contact + 0x88u))));
    io.at(0x20aacdu);
    negate_load(state);
    io.at(0x20aacfu);
    fp.push(fp.st(0));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aad1u, eax))));
    spill_force<true>(state, io, 0x20aad3u, stack + 0x54u);
    x[1] = io.scalar(0x20aad7u, stack + 0x54u);
    io.at(0x20aaddu);
    fp.push(fp.st(0));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aadfu, eax + 4u))));
    spill_force<true>(state, io, 0x20aae2u, stack + 0x58u);
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20aae6u, eax + 8u))));
    eax = io.word(0x20aae9u, stack + 0x90u);
    io.at(0x20aaf0u);
    eax = state.flags.increment(eax, 4, false);
    io.store_word(0x20aaf1u, stack + 0x90u, eax);
    spill_force<true>(state, io, 0x20aaf8u, stack + 0x5cu);
}

inline void retain_normal(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    const auto &stack = gp[4];
    gp[2] = io.word(0x20aafcu, stack + 0x60u);
    gp[0] = io.word(0x20ab00u, stack + 0x64u);
    gp[1] = io.word(0x20ab04u, stack + 0x68u);
    io.store_word(0x20ab08u, stack + 0x44u, gp[2]);
    io.store_word(0x20ab0cu, stack + 0x48u, gp[0]);
    io.store_word(0x20ab10u, stack + 0x4cu, gp[1]);
    io.at(0x20ab14u);
}

inline bool query_is_needed(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto &stack = gp[4], &wheel = gp[6], &contact = gp[7];

    io.store_word(0x20ab1cu, contact + 0x80u, 1u, 1);
    x[0] = io.scalar(0x20ab23u, wheel + 0xcu);
    flags.scalar_compare(x[0], io.scalar(0x20ab28u, 0x46a0ecu), false);
    io.at(0x20ab2fu);
    if (flags.cf || flags.zf) return false;

    gp[2] = io.word(0x20ab35u, stack + 0x20u);
    x[0] = io.scalar(0x20ab39u, contact + 0xa8u);
    flags.scalar_compare(x[0], io.scalar(0x20ab41u, gp[2] + 0x18u), false);
    io.at(0x20ab45u);
    if (flags.cf || flags.zf) return false;

    x[0] = io.scalar(0x20ab4bu, contact + 0x90u);
    io.at(0x20ab53u);
    x[2] = _mm_setzero_ps();
    x[0] = ordered_difference_ss(x[0], io.scalar(0x20ab56u, wheel + 0x18u));
    io.at(0x20ab5bu);
    flags.scalar_compare(x[2], x[0], false);
    io.store(0x20ab5eu, stack + 0x30u, x[2]);
    io.at(0x20ab64u);
    if (flags.cf || flags.zf) io.store(0x20ab66u, stack + 0x30u, x[0]);
    return true;
}

inline bool normalize_query_axis(MotionState &state, Provider &io) {
    auto &fp = state.fp;
    auto &x = state.xmm;
    auto &flags = state.flags;
    const auto &stack = state.gp[4];

    fp.push(ext(f(io.word(0x20ab6cu, stack + 0x44u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20ab70u, stack + 0x44u))));
    fp.push(ext(f(io.word(0x20ab74u, stack + 0x4cu))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20ab78u, stack + 0x4cu))));
    io.at(0x20ab7cu);
    fp.st(1) = add(fp.st(1), fp.st(0));
    fp.pop();
    fp.push(ext(f(io.word(0x20ab7eu, stack + 0x48u))));
    fp.st(0) = mul(fp.st(0), ext(f(io.word(0x20ab82u, stack + 0x48u))));
    io.at(0x20ab86u);
    fp.st(1) = add(fp.st(1), fp.st(0));
    fp.pop();
    io.at(0x20ab88u);
    fp.st(0) = root(fp.st(0));
    spill_force<false>(state, io, 0x20ab8au, stack + 0x14u);
    io.at(0x20ab8eu);
    absolute_length(state);
    fp.push(ext(f(io.word(0x20ab90u, 0x46a0ecu))));
    io.at(0x20ab96u);
    state.compare_and_pop(fp.st(1));
    io.at(0x20ab98u);
    fp.status &= ~0x200u;
    fp.pop();
    io.at(0x20ab9au);
    if (!flags.cf && !flags.zf) return false;

    x[1] = io.scalar(0x20ab9cu, stack + 0x14u);
    x[0] = io.scalar(0x20aba2u, 0x46a0d0u);
    io.at(0x20abaau);
    x[0] = ordered_divide_ss(x[0], x[1]);
    io.at(0x20abaeu);
    x[3] = x[0];
    x[3] = ordered_mul_ss(x[3], io.scalar(0x20abb1u, stack + 0x44u));
    io.at(0x20abb7u);
    flags.scalar_compare(x[1], x[2], true);
    io.store(0x20abbau, stack + 0x44u, x[3]);
    io.at(0x20abc0u);
    state.set(0, flags.bits() | 2u, 1, 8);
    io.at(0x20abc1u);
    flags.logic(state.reg(0, 1, 8) & 0x44u, 1);
    io.at(0x20abc4u);
    x[3] = x[0];
    x[3] = ordered_mul_ss(x[3], io.scalar(0x20abc7u, stack + 0x48u));
    x[0] = ordered_mul_ss(x[0], io.scalar(0x20abcdu, stack + 0x4cu));
    io.store(0x20abd3u, stack + 0x48u, x[3]);
    io.store(0x20abd9u, stack + 0x4cu, x[0]);
    io.at(0x20abdfu);
    return flags.pf;
}

inline void restore_default_axis(MotionState &state, Provider &io) {
    auto &gp = state.gp;
    const auto &stack = gp[4];
    gp[0] = io.word(0x20abe1u, 0x474d10u);
    gp[1] = io.word(0x20abe6u, gp[0]);
    gp[2] = io.word(0x20abe8u, gp[0] + 4u);
    gp[0] = io.word(0x20abebu, gp[0] + 8u);
    io.store_word(0x20abeeu, stack + 0x44u, gp[1]);
    io.store_word(0x20abf2u, stack + 0x48u, gp[2]);
    io.store_word(0x20abf6u, stack + 0x4cu, gp[0]);
}

inline void build_query_record(MotionState &state, Provider &io) {
    auto &x = state.xmm;
    auto &gp = state.gp;
    auto &flags = state.flags;
    const auto &stack = gp[4], &normal = gp[5], &wheel = gp[6], &contact = gp[7];

    // The query caller consumes the original record's individual field
    // writes. In particular the 16-bit IDs do not overwrite their neighbors.
    x[1] = io.scalar(0x20abfau, wheel + 0xcu);
    x[1] = ordered_difference_ss(x[1], io.scalar(0x20abffu, contact + 0x84u));
    x[1] = ordered_difference_ss(x[1], io.scalar(0x20ac07u, 0x46a0ecu));
    state.set(0, io.word(0x20ac0fu, contact + 0xb0u, 2), 2, 0);
    gp[1] = io.word(0x20ac16u, stack + 0x50u);
    gp[2] = io.word(0x20ac1au, gp[1] + 0xb4u);
    state.set(6, io.word(0x20ac20u, wheel + 0x40u, 2), 2, 0);
    io.at(0x20ac24u);
    x[0] = x[2];
    io.at(0x20ac27u);
    x[0] = ordered_difference_ss(x[0], x[1]);
    io.at(0x20ac2bu);
    x[1] = x[0];
    x[1] = ordered_mul_ss(x[1], io.scalar(0x20ac2eu, normal));
    x[1] = ordered_add_ss(x[1], io.scalar(0x20ac33u, contact + 0x34u));
    io.store(0x20ac38u, stack + 0xdcu, x[1]);
    x[1] = io.scalar(0x20ac41u, normal + 4u);
    io.at(0x20ac46u);
    x[1] = ordered_mul_ss(x[1], x[0]);
    x[1] = ordered_add_ss(x[1], io.scalar(0x20ac4au, contact + 0x38u));
    io.store(0x20ac4fu, stack + 0xe0u, x[1]);
    x[1] = io.scalar(0x20ac58u, normal + 8u);
    io.store_word(0x20ac5du, stack + 0x134u, state.reg(0, 2, 0), 2);
    gp[0] = io.word(0x20ac65u, stack + 0xdcu);
    io.store_word(0x20ac6cu, stack + 0x138u, gp[0]);
    io.at(0x20ac73u);
    x[1] = ordered_mul_ss(x[1], x[0]);
    x[1] = ordered_add_ss(x[1], io.scalar(0x20ac77u, contact + 0x3cu));
    io.store(0x20ac7cu, stack + 0xe4u, x[1]);
    gp[0] = io.word(0x20ac85u, stack + 0xe4u);
    io.store_word(0x20ac8cu, stack + 0x140u, gp[0]);
    gp[0] = io.word(0x20ac93u, normal + 4u);
    io.store_word(0x20ac96u, stack + 0x120u, gp[2]);
    gp[2] = io.word(0x20ac9du, stack + 0xe0u);
    io.store_word(0x20aca4u, stack + 0x148u, gp[0]);
    gp[0] = io.word(0x20acabu, stack + 0x1cu);
    io.store_word(0x20acafu, stack + 0x13cu, gp[2]);
    gp[2] = io.word(0x20acb6u, normal);
    io.at(0x20acb9u);
    gp[1] = flags.logic(0xffffffffu, 4);
    io.store_word(0x20acbcu, stack + 0x150u, gp[0]);
    state.set(0, io.word(0x20acc3u, contact + 0x82u, 1), 1, 0);
    io.at(0x20acc9u);
    flags.logic(state.reg(0, 1, 0), 1);
    io.store_word(0x20accbu, stack + 0x144u, gp[2]);
    gp[2] = io.word(0x20acd2u, normal + 8u);
    io.store_word(0x20acd5u, stack + 0x11cu, 0, 1);
    io.store_word(0x20acddu, stack + 0x124u, 0);
    io.store_word(0x20ace8u, stack + 0x128u, state.reg(6, 2, 0), 2);
    io.store_word(0x20acf0u, stack + 0x12cu, gp[1]);
    io.store_word(0x20acf7u, stack + 0x130u, gp[1]);
    io.store_word(0x20acfeu, stack + 0x14cu, gp[2]);
    io.store_word(0x20ad05u, stack + 0x154u, 0, 1);
    io.store_word(0x20ad0du, stack + 0x156u, state.reg(1, 2, 0), 2);
    io.store_word(0x20ad15u, stack + 0x158u, state.reg(1, 2, 0), 2);
}

inline void select_query_mode(MotionState &state, Provider &io) {
    auto &flags = state.flags;
    io.at(0x20ad1du);
    if (flags.zf) {
        state.set(0, io.word(0x20ad1fu, state.gp[7] + 0x83u, 1), 1, 0);
        io.at(0x20ad25u);
        flags.logic(state.reg(0, 1, 0), 1);
        io.at(0x20ad27u);
        if (flags.zf) {
            io.at(0x20ad29u);
            state.set(0, flags.logic(0u, 1), 1, 0);
            io.at(0x20ad2bu);
            return;
        }
    }
    io.at(0x20ad2du);
    state.gp[0] = flags.logic(0u, 4);
    io.at(0x20ad2fu);
    state.set(0, 1u, 1, 0);
}
#endif

inline std::uint32_t bridge(MotionState &state, std::uint32_t entry) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return run_stage_with_exit(state, sites, [entry](MotionState &live, Provider &io) {
        if (entry == unprojected_force) {
            live.xmm[1] = io.scalar(unprojected_force, live.gp[4] + 0x54u);
        } else {
            if (entry == loaded_force) apply_contact_load(live, io);
            retain_normal(live, io);
        }
        if (!query_is_needed(live, io)) return skip_query;
        if (!normalize_query_axis(live, io)) restore_default_axis(live, io);
        build_query_record(live, io);
        select_query_mode(live, io);
        return query_ready;
    });
#else
    (void)state;
    (void)entry;
    return 0;
#endif
}
} // namespace halo2::native_vehicle_contact_query_20f3a8_detail
