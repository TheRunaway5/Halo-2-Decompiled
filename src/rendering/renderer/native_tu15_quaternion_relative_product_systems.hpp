#pragma once

#include "physics/runtime/native_tu15_packed_sse.hpp"
#include <bit>
#include <cstdint>

namespace halo2::native_quaternion_relative_product {

enum class Access {
    first_argument, first_vector, second_argument, second_vector, cross_store,
    first_scalar, second_reload, negated_scalar_store, negated_scalar_load, first_sum_store,
    first_reload, second_scalar, second_scalar_store, second_scalar_load, second_sum_store,
    dot_second, dot_first, dot_spill, dot_reload, scalar_result,
};

// Compute conjugate(second) * first. The source publishes the cross product
// and both vector sums. Each later load sees earlier stores when the output
// overlaps either input, including the inputs to the final dot product.
template<class Memory, class Frame>
void compose(Memory &memory, Frame &frame, std::uint32_t arguments, std::uint32_t output) {
    using enum Access;
    using native_packed_sse::add;
    using native_packed_sse::broadcast;
    using native_packed_sse::multiply;
    using native_packed_sse::subtract;

    const auto first = memory.load32(first_argument, arguments);
    const auto a = frame.xmm(0, memory.load128(first_vector, first));
    const auto second = memory.load32(second_argument, arguments + 4u);
    const auto b = frame.xmm(1, memory.load128(second_vector, second));

    const auto a_zxy = frame.xmm(2, _mm_shuffle_ps(a, a, 210));
    const auto cross_subtrahend = frame.xmm(3, multiply(_mm_shuffle_ps(b, b, 210), _mm_shuffle_ps(a, a, 201)));
    const auto cross = frame.xmm(0, subtract(multiply(_mm_shuffle_ps(b, b, 201), a_zxy), cross_subtrahend));
    memory.store128(cross_store, output, cross);

    frame.load_first_scalar(memory.load32(first_scalar, first + 12u));
    const auto live_b = frame.xmm(2, memory.load128(second_reload, second));
    const auto slot = frame.scalar_slot();
    memory.store32(negated_scalar_store, slot, frame.negated_scalar());
    frame.pop_first_scalar();
    const auto negative_aw = frame.xmm(1, _mm_set_ss(std::bit_cast<float>(memory.load32(negated_scalar_load, slot))));
    const auto b_term = frame.xmm(3, multiply(broadcast<0>(negative_aw), live_b));
    const auto partial = frame.xmm(0, add(cross, b_term));
    memory.store128(first_sum_store, output, partial);

    const auto live_a = frame.xmm(2, memory.load128(first_reload, first));
    memory.store32(second_scalar_store, slot, memory.load32(second_scalar, second + 12u));
    const auto bw = frame.xmm(1, _mm_set_ss(std::bit_cast<float>(memory.load32(second_scalar_load, slot))));
    const auto a_term = frame.xmm(3, multiply(broadcast<0>(bw), live_a));
    memory.store128(second_sum_store, output, frame.xmm(0, add(partial, a_term)));

    const auto dot_b = frame.xmm(0, memory.load128(dot_second, second));
    const auto dot_a = memory.load128(dot_first, first);
    const auto products = frame.xmm(1, multiply(dot_a, dot_b));
    const auto pairs = frame.xmm(0, add(_mm_shuffle_ps(products, products, 78), products));
    const auto adjacent = frame.xmm(1, _mm_shuffle_ps(pairs, pairs, 177));
    const auto dot = frame.xmm(0, add(pairs, adjacent));
    frame.publish_scalar_slot();
    memory.store32(dot_spill, slot, std::bit_cast<std::uint32_t>(_mm_cvtss_f32(dot)));
    memory.store32(scalar_result, output + 12u, memory.load32(dot_reload, slot));
}

} // namespace halo2::native_quaternion_relative_product
