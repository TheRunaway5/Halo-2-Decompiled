#pragma once
#include "rendering/models/native_tu15_prt_projection.hpp"

namespace halo2::prt_matrix_vector_detail {
using namespace prt_projection_detail;
struct Caller {
    std::uint32_t size, output, stack, saved_bp, saved_si, saved_di;
};
struct Product {
    std::uint32_t output{}, vector{};
    __m128 sum{}, partial{};
    bool used_bulk{};
};
// Complete145D50: square matrix/vector product. The four-term blocks retain
// their original operand ownership and accumulator stores, including natural
// in-place/partial aliases. The scalar tail rereads the published accumulator.
inline Product multiply_square(SourcePhysicsContactMemory& memory, const Caller& caller,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto frame = caller.stack - 0x14u;
    const auto get = [&](std::uint32_t offset) { return read(memory, frame + offset); };
    const auto put = [&](std::uint32_t offset, std::uint32_t value, std::uint32_t site) {
        store(memory, frame + offset, value, 4, site, observe);
    };
    store(memory, caller.stack - 0xcu, caller.saved_bp, 4, 0x145d5bu, observe);
    auto matrix_row = get(0x18);
    store(memory, caller.stack - 0x10u, caller.saved_si, 4, 0x145d60u, observe);
    store(memory, caller.stack - 0x14u, caller.saved_di, 4, 0x145d61u, observe);
    put(0x0c, 0, 0x145d62u);
    put(0x10, caller.size, 0x145d6au);
    Product result{caller.output};
    for (;;) {
        result.vector = get(0x1c);
        const auto first_vector = scalar(read(memory, result.vector));
        result.sum = multiply(first_vector, scalar(read(memory, matrix_row)), false);
        store(memory, result.output, bits(result.sum), 4, 0x145d83u, observe);
        std::uint32_t tail = 1;
        if (std::int32_t(caller.size - 1u) >= 4) {
            const auto blocks = ((caller.size - 5u) >> 2u) + 1u;
            auto matrix = matrix_row + 4u, vector = result.vector + 12u;
            tail = 1u + blocks * 4u;
            for (std::uint32_t block = 0; block < blocks; ++block) {
                const auto second_vector = scalar(read(memory, vector - 8u));
                result.sum = multiply(second_vector, scalar(read(memory, matrix)), false);
                result.sum = add(result.sum, scalar(read(memory, result.output)), false);
                store(memory, result.output, bits(result.sum), 4, 0x145dafu, observe);
                const auto third_matrix = scalar(read(memory, matrix + 4u));
                result.partial = multiply(third_matrix, scalar(read(memory, vector - 4u)), false);
                result.partial = add(result.partial, result.sum, false);
                store(memory, result.output, bits(result.partial), 4, 0x145dc1u, observe);
                const auto fourth_matrix = scalar(read(memory, matrix + 8u));
                result.sum = multiply(fourth_matrix, scalar(read(memory, vector)), false);
                result.sum = add(result.sum, result.partial, false);
                store(memory, result.output, bits(result.sum), 4, 0x145dd2u, observe);
                const auto fifth_matrix = scalar(read(memory, matrix + 12u));
                result.partial = multiply(fifth_matrix, scalar(read(memory, vector + 4u)), false);
                matrix += 16u; vector += 16u;
                result.partial = add(result.partial, result.sum, false);
                store(memory, result.output, bits(result.partial), 4, 0x145debu, observe);
            }
            result.vector = get(0x1c);
            result.used_bulk = true;
        }
        if (tail < caller.size) {
            const auto row_offset = get(0x0c);
            auto matrix = get(0x18) + (row_offset + tail) * 4u;
            do {
                const auto source = scalar(read(memory, result.vector + tail * 4u));
                result.sum = multiply(source, scalar(read(memory, matrix)), false);
                result.sum = add(result.sum, scalar(read(memory, result.output)), false);
                ++tail; matrix += 4u;
                store(memory, result.output, bits(result.sum), 4, 0x145e23u, observe);
            } while (tail < caller.size);
        }
        const auto row_offset = get(0x0c);
        matrix_row += caller.size * 4u;
        const auto remaining = get(0x10) - 1u;
        result.output += 4u;
        put(0x0c, row_offset + caller.size, 0x145e40u);
        put(0x10, remaining, 0x145e44u);
        if (!remaining) break;
    }
    return result;
}
template<class State> void bridge(State& state) {
    const auto stack = state.gp[4];
    if (state.gp[3]) {
        const Caller caller{state.gp[3], state.gp[0], stack, state.gp[5], state.gp[6], state.gp[7]};
        const auto result = multiply_square(state.memory, caller, state.observe);
        state.gp[0] = result.output;
        state.gp[1] = 0;
        state.gp[2] = result.vector;
        state.gp[7] = read(state.memory, stack - 0x14u);
        state.gp[6] = read(state.memory, stack - 0x10u);
        state.gp[5] = read(state.memory, stack - 0xcu);
        state.xmm[0] = result.sum;
        if (result.used_bulk) state.xmm[1] = result.partial;
    }
    state.flags.plus(stack - 8u, 8u, 4);
    (void)read(state.memory, stack);
    state.gp[4] = stack + 12u;
}
}
