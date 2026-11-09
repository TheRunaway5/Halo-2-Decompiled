#pragma once
#include "rendering/models/native_tu15_prt_projection.hpp"
#include "physics/runtime/physics_solver_internal.hpp"

namespace halo2::prt_basis_detail {
using namespace prt_projection_detail;
using physics_step_detail::ext;
using physics_step_detail::mul;
using physics_step_detail::sub;
using physics_solver_detail::divide;
using physics_solver_detail::root;
inline long double single(const SourcePhysicsContactMemory& memory, std::uint32_t at) {
    return ext(std::bit_cast<float>(read(memory, at)));
}
inline long double double_value(const SourcePhysicsContactMemory& memory, std::uint32_t at) {
    std::array<std::byte, 8> bytes{};
    memory.read(at, bytes);
    return static_cast<long double>(std::bit_cast<double>(bytes));
}
struct RoundedSingle { std::uint32_t bits; std::uint16_t status; };
inline RoundedSingle round_single(long double value) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float output; std::uint16_t status;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        : "=m"(output), "=m"(status) : "m"(value) : "st");
    return {std::bit_cast<std::uint32_t>(output), status};
#else
    throw std::runtime_error("Native TU1.5 SH basis requires its x87 rounding owner");
#endif
}
struct Basis {
    __m128 first{}, second{}, third{}, z{}, x{}, y{}, sixth{}, half{};
    std::uint16_t final_status{};
    bool quadratic{};
};
// Complete145E60 real-SH basis (four, nine or sixteen coefficients). Native
// extended operations preserve authored x87 precision and32-bit spill points;
// SSE polynomial grouping and ordered bank writes remain literal-equivalent.
inline Basis evaluate(SourcePhysicsContactMemory& memory, std::uint32_t normal,
    std::uint32_t output, std::uint32_t order, std::uint32_t stack,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto frame = stack - 0x28u;
    const auto get = [&](std::uint32_t offset) { return read(memory, frame + offset); };
    const auto put = [&](std::uint32_t offset, std::uint32_t value, std::uint32_t pc) {
        store(memory, frame + offset, value, 4, pc, observe);
    };
    Basis result;
    const auto sqrt_pi = root(double_value(memory, 0x46a420u));
    result.y = scalar(read(memory, normal + 4u));
    result.z = scalar(read(memory, normal + 8u));
    result.x = scalar(read(memory, normal));
    result.second = scalar(read(memory, 0x46a0d4u));
    result.half = scalar(read(memory, 0x46a0ccu));
    put(8, bits(result.y), 0x145e8cu);
    put(4, bits(result.z), 0x145e92u);
    auto rounded = round_single(divide(single(memory, 0x46a0d0u), sqrt_pi));
    put(0, rounded.bits, 0x145e9eu);
    rounded = round_single(mul(root(double_value(memory, 0x46a418u)), single(memory, frame)));
    put(0x10, rounded.bits, 0x145eacu);
    result.first = scalar(get(0x10));
    const auto constant = mul(single(memory, frame), single(memory, 0x46a0ccu));
    result.third = multiply(multiply(result.first, result.y, false), result.second, false);
    store(memory, output + 4u, bits(result.third), 4, 0x145ecau, observe);
    rounded = round_single(constant);
    store(memory, output, rounded.bits, 4, 0x145ecfu, observe);
    result.final_status = rounded.status;
    result.third = multiply(result.first, result.z, false);
    result.first = multiply(result.first, result.x, false);
    result.third = multiply(result.third, result.half, false);
    result.first = multiply(result.first, result.second, false);
    store(memory, output + 8u, bits(result.third), 4, 0x145ee4u, observe);
    store(memory, output + 12u, bits(result.first), 4, 0x145ee9u, observe);
    if (order <= 2u) return result;

    const auto sqrt_fifteen = root(double_value(memory, 0x46a3f8u));
    result.third = multiply(result.y, result.x, false);
    put(0x1c, bits(result.third), 0x145f06u);
    result.first = multiply(result.x, result.x, false);
    result.y = multiply(result.y, result.y, false);
    result.second = _mm_sub_ss(result.first, result.y);
    put(0x24, bits(result.second), 0x145f1eu);
    put(0x14, bits(result.first), 0x145f24u);
    put(0x18, bits(result.y), 0x145f2au);
    rounded = round_single(sqrt_fifteen);
    put(0x0c, rounded.bits, 0x145f30u);
    auto z_squared = single(memory, frame + 4u);
    result.sixth = scalar(get(0x0c));
    z_squared = mul(z_squared, single(memory, frame + 4u));
    result.sixth = multiply(result.sixth, scalar(get(0)), false);
    result.second = multiply(result.second, scalar(get(0x0c)), false);
    result.third = multiply(result.third, result.sixth, false);
    rounded = round_single(z_squared);
    put(0x10, rounded.bits, 0x145f51u);
    result.third = multiply(result.third, result.half, false);
    const auto sqrt_five = root(double_value(memory, 0x46a400u));
    result.half = multiply(result.sixth, result.z, false);
    result.half = multiply(result.half, scalar(get(8)), false);
    store(memory, output + 0x10u, bits(result.third), 4, 0x145f6eu, observe);
    result.third = scalar(read(memory, 0x46a0d4u));
    result.z = multiply(result.z, result.x, false);
    result.half = multiply(result.half, result.third, false);
    result.z = multiply(result.z, result.sixth, false);
    result.z = multiply(result.z, result.third, false);
    result.third = scalar(get(0));
    result.second = multiply(result.second, result.third, false);
    result.second = multiply(result.second, scalar(read(memory, 0x46a130u)), false);
    store(memory, output + 0x14u, bits(result.half), 4, 0x145f9cu, observe);
    store(memory, output + 0x1cu, bits(result.z), 4, 0x145fa1u, observe);
    store(memory, output + 0x20u, bits(result.second), 4, 0x145fa6u, observe);
    auto zonal = single(memory, frame + 0x10u);
    zonal = mul(zonal, single(memory, 0x46a0e4u));
    zonal = sub(zonal, single(memory, 0x46a0d0u));
    zonal = mul(sqrt_five, zonal);
    zonal = mul(zonal, single(memory, frame));
    zonal = mul(zonal, single(memory, 0x46a130u));
    rounded = round_single(zonal);
    store(memory, output + 0x18u, rounded.bits, 4, 0x145fc6u, observe);
    result.final_status = rounded.status;
    result.quadratic = true;
    if (order <= 3u) return result;

    const auto sqrt_two = root(double_value(memory, 0x46a408u));
    result.second = scalar(get(0x10));
    result.second = multiply(result.second, scalar(read(memory, 0x46a100u)), false);
    put(0x10, bits(result.second), 0x145fe5u);
    result.second = _mm_sub_ss(result.second, scalar(read(memory, 0x46a0d0u)));
    result.z = scalar(read(memory, 0x46a410u));
    result.y = multiply(result.y, scalar(read(memory, 0x46a0e4u)), false);
    result.half = result.second;
    result.first = _mm_sub_ss(result.first, result.y);
    rounded = round_single(mul(root(double_value(memory, 0x46a3e8u)), sqrt_two));
    put(0x0c, rounded.bits, 0x146014u);
    const auto sqrt_one_hundred_five = root(double_value(memory, 0x46a3e0u));
    result.first = multiply(result.first, scalar(get(0x0c)), false);
    result.first = multiply(result.first, result.third, false);
    result.first = multiply(result.first, result.x, false);
    result.first = multiply(result.first, result.z, false);
    store(memory, output + 0x3cu, bits(result.first), 4, 0x146032u, observe);
    rounded = round_single(mul(root(double_value(memory, 0x46a3d8u)), sqrt_two));
    put(0x20, rounded.bits, 0x146041u);
    auto sectoral = single(memory, frame + 0x14u);
    result.sixth = scalar(get(0x20));
    sectoral = mul(sectoral, single(memory, 0x46a0e4u));
    result.half = multiply(result.half, result.sixth, false);
    result.half = multiply(result.half, result.third, false);
    result.half = multiply(result.half, scalar(get(8)), false);
    sectoral = sub(sectoral, single(memory, frame + 0x18u));
    result.half = multiply(result.half, result.z, false);
    store(memory, output + 0x2cu, bits(result.half), 4, 0x14606bu, observe);
    result.half = result.third;
    sectoral = mul(sectoral, single(memory, frame + 0x0cu));
    result.half = multiply(result.half, result.x, false);
    result.half = multiply(result.half, result.second, false);
    sectoral = mul(sectoral, single(memory, frame));
    result.half = multiply(result.half, result.sixth, false);
    result.half = multiply(result.half, result.z, false);
    store(memory, output + 0x34u, bits(result.half), 4, 0x14608au, observe);
    sectoral = mul(sectoral, single(memory, frame + 8u));
    sectoral = mul(sectoral, single(memory, 0x46a410u));
    rounded = round_single(sectoral);
    store(memory, output + 0x24u, rounded.bits, 4, 0x146099u, observe);
    auto mixed = mul(single(memory, frame + 0x1cu), sqrt_one_hundred_five);
    mixed = mul(mixed, single(memory, frame));
    mixed = mul(mixed, single(memory, frame + 4u));
    mixed = mul(mixed, single(memory, 0x46a0ccu));
    rounded = round_single(mixed);
    store(memory, output + 0x28u, rounded.bits, 4, 0x1460afu, observe);
    auto cubic_zonal = root(double_value(memory, 0x46a3d0u));
    auto fifth_z_squared = single(memory, frame + 0x10u);
    fifth_z_squared = sub(fifth_z_squared, single(memory, 0x46a0e4u));
    cubic_zonal = mul(cubic_zonal, fifth_z_squared);
    cubic_zonal = mul(cubic_zonal, single(memory, frame));
    cubic_zonal = mul(cubic_zonal, single(memory, frame + 4u));
    cubic_zonal = mul(cubic_zonal, single(memory, 0x46a130u));
    rounded = round_single(cubic_zonal);
    store(memory, output + 0x30u, rounded.bits, 4, 0x1460d3u, observe);
    auto last = single(memory, frame);
    last = mul(last, single(memory, frame + 4u));
    last = mul(last, sqrt_one_hundred_five);
    last = mul(last, single(memory, frame + 0x24u));
    last = mul(last, single(memory, 0x46a130u));
    rounded = round_single(last);
    store(memory, output + 0x38u, rounded.bits, 4, 0x1460e9u, observe);
    // The two final FSTP ST(0) operations discard the retained roots and clear C1.
    result.final_status = 0;
    return result;
}
template<class State> void bridge(State& state) {
    const auto stack = state.gp[4];
    const auto result = evaluate(state.memory, state.gp[1], state.gp[0], state.gp[2], stack, state.observe);
    state.xmm[0] = result.first;
    state.xmm[1] = result.second;
    state.xmm[2] = result.third;
    state.xmm[3] = result.z;
    state.xmm[4] = result.x;
    state.xmm[5] = result.y;
    if (result.quadratic) state.xmm[6] = result.sixth;
    state.xmm[7] = result.half;
    state.fp.status = (state.fp.status & ~0x200u) | (result.final_status & 0x200u);
    state.flags.plus(stack - 0x28u, 0x28u, 4);
    (void)read(state.memory, stack);
    state.gp[4] = stack + 4u;
}
}
