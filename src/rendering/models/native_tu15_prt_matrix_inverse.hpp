#pragma once
#include "rendering/models/native_tu15_prt_projection.hpp"

namespace halo2::prt_matrix_inverse_detail {
using namespace prt_projection_detail;
inline std::uint32_t signed_low(std::uint32_t value) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
}
struct Caller {
    std::uint32_t input, stack, saved_bx, saved_bp, saved_si, saved_di;
    bool backwards{};
};
struct Inverse {
    std::uint32_t column{}, row_counter{};
    __m128 reciprocal{}, cofactor{}, subtrahend{};
};
// Complete1456F0: transpose the3x3 cofactor matrix and divide by the supplied
// determinant. Exact input/output aliasing uses the original nine-word scratch
// copy; all other overlap follows the original scalar read/store sequence.
inline Inverse invert(SourcePhysicsContactMemory& memory, const Caller& caller,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto frame = caller.stack - 0x48u;
    const auto get = [&](std::uint32_t offset) { return read(memory, frame + offset); };
    const auto put = [&](std::uint32_t offset, std::uint32_t value, std::uint32_t site) {
        store(memory, frame + offset, value, 4, site, observe);
    };
    store(memory, caller.stack - 0x3cu, caller.saved_bx, 4, 0x1456f3u, observe);
    auto output_row = get(0x50);
    store(memory, caller.stack - 0x40u, caller.saved_bp, 4, 0x1456f8u, observe);
    store(memory, caller.stack - 0x44u, caller.saved_si, 4, 0x1456f9u, observe);
    store(memory, caller.stack - 0x48u, caller.saved_di, 4, 0x1456feu, observe);
    auto input = caller.input;
    if (input == output_row) {
        auto source = input, destination = frame + 0x24u;
        const auto step = caller.backwards ? 0xfffffffcu : 4u;
        for (unsigned word = 0; word < 9; ++word) {
            store(memory, destination, read(memory, source), 4, 0x14570au, observe);
            source += step; destination += step;
        }
        input = frame + 0x24u;
    }
    Inverse result;
    result.reciprocal = scalar(read(memory, 0x46a0d0u));
    result.reciprocal = _mm_div_ss(result.reciprocal, scalar(get(0x4c)));
    std::uint32_t row_index{};
    put(0x4c, row_index, 0x145722u);
    put(0x10, result.row_counter, 0x145726u);
    put(0x20, output_row, 0x14572au);
    for (;;) {
        result.column = 0;
        std::uint32_t previous_column = 0xffffffffu;
        put(0x14, result.column, 0x145735u);
        put(0x18, previous_column, 0x145739u);
        put(0x1c, output_row, 0x14573du);
        for (;;) {
            const auto next_row = std::int32_t(signed_low(row_index)) < 2 ? result.row_counter + 1u : 0u;
            result.row_counter = std::int32_t(signed_low(row_index)) > 0 ? result.row_counter - 1u : 2u;
            const auto next_column = std::int32_t(signed_low(result.column)) < 2 ? previous_column + 2u : 0u;
            const auto last_column = std::int32_t(signed_low(result.column)) > 0 ? previous_column : 2u;
            const auto first_row = signed_low(next_row) * 3u;
            const auto last_row = signed_low(result.row_counter) * 3u;
            const auto first_col = signed_low(next_column), last_col = signed_low(last_column);
            result.cofactor = scalar(read(memory, input + (first_row + first_col) * 4u));
            auto cursor = get(0x1c);
            result.subtrahend = scalar(read(memory, input + (first_row + last_col) * 4u));
            result.cofactor = multiply(result.cofactor,
                scalar(read(memory, input + (last_row + last_col) * 4u)), false);
            previous_column = get(0x18);
            result.row_counter = get(0x10);
            result.subtrahend = multiply(result.subtrahend,
                scalar(read(memory, input + (last_row + first_col) * 4u)), false);
            result.column = get(0x14);
            result.cofactor = _mm_sub_ss(result.cofactor, result.subtrahend);
            result.cofactor = multiply(result.cofactor, result.reciprocal, false);
            store(memory, cursor, bits(result.cofactor), 4, 0x1457bcu, observe);
            ++result.column; cursor += 12u; ++previous_column;
            put(0x1c, cursor, 0x1457c9u);
            row_index = get(0x4c);
            put(0x14, result.column, 0x1457d1u);
            put(0x18, previous_column, 0x1457d5u);
            if (std::int32_t(signed_low(result.column)) >= 3) break;
        }
        output_row = get(0x20);
        ++row_index; ++result.row_counter; output_row += 4u;
        put(0x4c, row_index, 0x1457ecu);
        put(0x10, result.row_counter, 0x1457f0u);
        put(0x20, output_row, 0x1457f4u);
        if (std::int32_t(signed_low(row_index)) >= 3) break;
    }
    return result;
}
template<class State> void bridge(State& state) {
    const Caller caller{state.gp[0], state.gp[4], state.gp[3], state.gp[5], state.gp[6], state.gp[7],
        (state.other_flags & 1024u) != 0};
    const auto result = invert(state.memory, caller, state.observe);
    state.gp[0] = read(state.memory, caller.stack + 8u);
    state.gp[1] = result.column;
    state.gp[2] = result.row_counter;
    state.gp[7] = read(state.memory, caller.stack - 0x48u);
    state.gp[6] = read(state.memory, caller.stack - 0x44u);
    state.gp[5] = read(state.memory, caller.stack - 0x40u);
    state.gp[3] = read(state.memory, caller.stack - 0x3cu);
    state.xmm[0] = result.reciprocal;
    state.xmm[1] = result.cofactor;
    state.xmm[2] = result.subtrahend;
    state.flags.plus(caller.stack - 0x38u, 0x38u, 4);
    (void)read(state.memory, caller.stack);
    state.gp[4] = caller.stack + 12u;
}
}
