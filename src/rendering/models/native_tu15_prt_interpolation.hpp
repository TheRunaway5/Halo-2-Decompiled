#pragma once
#include "rendering/models/native_tu15_prt_basis.hpp"

namespace halo2::prt_interpolation_detail {
using namespace prt_projection_detail;
using prt_basis_detail::single;
using prt_basis_detail::round_single;
using physics_step_detail::sub;
using physics_step_detail::mul;
using physics_solver_detail::root;
inline bool greater_extended(long double value, long double epsilon) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char greater;
    asm volatile("fldt %2\n\tfldt %1\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)"
        : "=qm"(greater) : "m"(value), "m"(epsilon) : "cc", "st");
    return greater;
#else
    throw std::runtime_error("Native PRT interpolation requires extended comparison");
#endif
}
struct ProjectionWeight {
    __m128 value{}, dx{}, dy{}, dz{}, nx{}, ny{}, nz{};
    bool normalized{};
};
// Complete22C10: the projected point weight retains extended source-reference
// differences and the original rounded-length epsilon gate.
inline ProjectionWeight projection_weight(SourcePhysicsContactMemory& memory,
    std::uint32_t point, std::uint32_t first_reference, std::uint32_t second_reference,
    std::uint32_t stack, const SourcePhysicsContactRecordObserver& observe) {
    const auto frame = stack - 16u;
    const auto put = [&](std::uint32_t offset, std::uint32_t value, std::uint32_t pc) {
        store(memory, frame + offset, value, 4, pc, observe);
    };
    auto extended_x = single(memory, point);
    ProjectionWeight result;
    result.nx = scalar(read(memory, point));
    extended_x = sub(extended_x, single(memory, first_reference));
    result.ny = scalar(read(memory, point + 4u));
    result.nz = scalar(read(memory, point + 8u));
    result.nx = _mm_sub_ss(result.nx, scalar(read(memory, second_reference)));
    put(4, round_single(extended_x).bits, 0x22c29u);
    result.ny = _mm_sub_ss(result.ny, scalar(read(memory, second_reference + 4u)));
    auto extended_y = single(memory, point + 4u);
    result.nz = _mm_sub_ss(result.nz, scalar(read(memory, second_reference + 8u)));
    extended_y = sub(extended_y, single(memory, first_reference + 4u));
    put(8, round_single(extended_y).bits, 0x22c40u);
    auto extended_z = single(memory, point + 8u);
    extended_z = sub(extended_z, single(memory, first_reference + 8u));
    put(12, round_single(extended_z).bits, 0x22c4au);
    auto norm = mul(extended_z, single(memory, frame + 12u));
    const auto y = single(memory, frame + 8u);
    norm = physics_step_detail::add(norm, mul(y, single(memory, frame + 8u)));
    const auto x = single(memory, frame + 4u);
    norm = physics_step_detail::add(norm, mul(x, single(memory, frame + 4u)));
    put(0, round_single(root(norm)).bits, 0x22c68u);
    const auto epsilon = single(memory, 0x46a0ecu);
    const auto length = single(memory, frame);
    if (!greater_extended(length, epsilon)) return result;
    result.dz = scalar(read(memory, 0x46a0d0u));
    result.dz = _mm_div_ss(result.dz, scalar(read(memory, frame)));
    result.dy = scalar(read(memory, frame + 8u));
    result.value = scalar(read(memory, frame + 12u));
    result.dx = scalar(read(memory, frame + 4u));
    result.value = multiply(result.value, result.dz, false);
    result.value = multiply(result.value, result.nz, false);
    result.dy = multiply(result.dy, result.dz, false);
    result.dx = multiply(result.dx, result.dz, false);
    result.dy = multiply(result.dy, result.ny, false);
    result.value = add(result.value, result.dy, false);
    result.dx = multiply(result.dx, result.nx, false);
    result.value = add(result.value, result.dx, false);
    result.value = multiply(result.value, result.dz, false);
    result.normalized = true;
    return result;
}

struct InterpolationCaller {
    std::uint32_t order, source, stack, saved_bx, saved_si, saved_di, saved_bp;
    __m128 factor;
};
struct Interpolation {
    std::uint32_t reference_cursor{}, output_cursor{}, processed{}, restored_bp{};
    __m128 value{};
    bool bulk{}, written{};
};
// Complete22730 coefficient interpolation. The second unrolled coefficient
// intentionally reads the saved source/output offsets before the preceding
// store, retaining aliases with those original scratch words.
inline Interpolation interpolate_coefficients(SourcePhysicsContactMemory& memory,
    const InterpolationCaller& caller, const SourcePhysicsContactRecordObserver& observe) {
    store(memory, caller.stack - 12u, caller.saved_bx, 4, 0x22733u, observe);
    auto reference = read(memory, caller.stack + 4u);
    store(memory, caller.stack - 16u, caller.saved_si, 4, 0x22738u, observe);
    store(memory, caller.stack - 20u, caller.saved_di, 4, 0x22739u, observe);
    const auto count = caller.order * caller.order;
    Interpolation result{caller.order, caller.source};
    if (std::int32_t(count) >= 4) {
        auto output_cursor = read(memory, caller.stack + 8u) + 8u;
        store(memory, caller.stack - 24u, caller.saved_bp, 4, 0x22750u, observe);
        const auto frame = caller.stack - 24u;
        store(memory, frame + 0x10u, caller.source - reference, 4, 0x22755u, observe);
        const auto output = read(memory, caller.stack + 8u);
        store(memory, frame + 0x14u, output - reference, 4, 0x2275fu, observe);
        auto reference_cursor = reference + 4u;
        const auto source_output_offset = caller.source - read(memory, caller.stack + 8u);
        do {
            result.value = scalar(read(memory, caller.source + result.processed * 4u));
            result.value = _mm_sub_ss(result.value, scalar(read(memory, reference_cursor - 4u)));
            const auto source_offset = read(memory, frame + 0x10u);
            result.value = multiply(result.value, caller.factor, false);
            result.value = add(result.value, scalar(read(memory, reference_cursor - 4u)), false);
            store(memory, output_cursor - 8u, bits(result.value), 4, 0x22788u, observe);
            result.value = scalar(read(memory, source_offset + reference_cursor));
            result.value = _mm_sub_ss(result.value, scalar(read(memory, reference_cursor)));
            const auto output_offset = read(memory, frame + 0x14u);
            result.value = multiply(result.value, caller.factor, false);
            result.value = add(result.value, scalar(read(memory, reference_cursor)), false);
            store(memory, output_offset + reference_cursor, bits(result.value), 4, 0x227a2u, observe);
            result.value = scalar(read(memory, source_output_offset + output_cursor));
            result.value = _mm_sub_ss(result.value, scalar(read(memory, reference_cursor + 4u)));
            result.value = multiply(result.value, caller.factor, false);
            result.value = add(result.value, scalar(read(memory, reference_cursor + 4u)), false);
            store(memory, output_cursor, bits(result.value), 4, 0x227bau, observe);
            result.value = scalar(read(memory, caller.source + result.processed * 4u + 12u));
            result.value = _mm_sub_ss(result.value, scalar(read(memory, reference_cursor + 8u)));
            result.value = multiply(result.value, caller.factor, false);
            result.value = add(result.value, scalar(read(memory, reference_cursor + 8u)), false);
            store(memory, output_cursor + 4u, bits(result.value), 4, 0x227d2u, observe);
            result.processed += 4u; reference_cursor += 16u; output_cursor += 16u;
        } while (std::int32_t(result.processed) < std::int32_t(count - 3u));
        reference = read(memory, caller.stack + 4u);
        result.restored_bp = read(memory, caller.stack - 24u);
        result.reference_cursor = reference_cursor;
        result.output_cursor = output_cursor;
        result.bulk = result.written = true;
    }
    if (std::int32_t(result.processed) < std::int32_t(count)) {
        const auto output = read(memory, caller.stack + 8u);
        const auto source_offset = caller.source - reference;
        result.output_cursor = output - reference;
        result.reference_cursor = reference + result.processed * 4u;
        auto tail = count - result.processed;
        do {
            result.value = scalar(read(memory, source_offset + result.reference_cursor));
            result.value = _mm_sub_ss(result.value, scalar(read(memory, result.reference_cursor)));
            result.value = multiply(result.value, caller.factor, false);
            result.value = add(result.value, scalar(read(memory, result.reference_cursor)), false);
            store(memory, result.output_cursor + result.reference_cursor, bits(result.value), 4, 0x22811u, observe);
            result.reference_cursor += 4u;
        } while (--tail);
        result.written = true;
    }
    return result;
}
template<class State> void bridge(State& state, std::uint32_t entry) {
    const auto stack = state.gp[4];
    if (entry == 0x22c10u) {
        const auto result = projection_weight(state.memory, state.gp[0], state.gp[1], state.gp[2], stack, state.observe);
        state.xmm[0] = result.value;
        if (result.normalized) {
            state.xmm[1] = result.dx; state.xmm[2] = result.dy; state.xmm[3] = result.dz;
        }
        state.xmm[4] = result.nx; state.xmm[5] = result.ny; state.xmm[6] = result.nz;
        state.fp.status &= ~0x200u;
        state.flags.plus(stack - 16u, 16u, 4);
        (void)read(state.memory, stack); state.gp[4] = stack + 4u;
    } else {
        const InterpolationCaller caller{state.gp[0], state.gp[1], stack, state.gp[3], state.gp[6],
            state.gp[7], state.gp[5], state.xmm[0]};
        const auto result = interpolate_coefficients(state.memory, caller, state.observe);
        state.gp[0] = result.reference_cursor; state.gp[1] = result.output_cursor; state.gp[2] = result.processed;
        if (result.bulk) state.gp[5] = result.restored_bp;
        state.gp[7] = read(state.memory, stack - 20u);
        state.gp[6] = read(state.memory, stack - 16u);
        state.gp[3] = read(state.memory, stack - 12u);
        if (result.written) state.xmm[1] = result.value;
        state.flags.plus(stack - 8u, 8u, 4);
        (void)read(state.memory, stack); state.gp[4] = stack + 12u;
    }
}
}
