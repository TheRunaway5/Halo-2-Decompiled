#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_authored_linear_ramp::systems {
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

enum class Layer : unsigned { base, blend };
enum class Segment : unsigned { first, middle, last };

struct LinearWorkspace {
    __m128 &base_or_blend;
    __m128 &input_or_base_weight;
    __m128 &second;
};

struct RampWorkspace {
    __m128 &sample, &input, &unit_limit, &middle_weight, &first_weight, &zero_limit;
};

template<class Io>
void evaluate_linear(Io &io, LinearWorkspace work) {
    auto [base, input, second] = work;
    base = io.linear_slope(Layer::base);
    input = io.input();
    const bool blended = io.has_blend();
    base = ordered_mul_ss(base, input);
    base = ordered_add_ss(base, io.linear_offset(Layer::base));
    io.store_result(Layer::base, base);
    if (!blended) return;

    auto &blend = base;
    auto &base_weight = input;
    second = io.linear_slope(Layer::blend);
    blend = io.blend_amount();
    second = ordered_mul_ss(second, input);
    second = ordered_add_ss(second, io.linear_offset(Layer::blend));
    base_weight = io.unit(Layer::blend);
    io.advance_layer();
    base_weight = _mm_sub_ss(base_weight, blend);
    // Reread the committed base result after the second layer's guest reads.
    base_weight = ordered_mul_ss(base_weight, io.base_result());
    second = ordered_mul_ss(second, blend);
    second = ordered_add_ss(second, base_weight);
    io.store_result(Layer::blend, second);
}

template<class Io>
void ramp_weight(Io &io, Layer layer, Segment segment, __m128 &sample, __m128 &weight,
    __m128 zero, __m128 unit) {
    sample = _mm_sub_ss(sample, io.ramp_threshold(layer, segment));
    sample = ordered_mul_ss(sample, io.ramp_slope(layer, segment));
    weight = io.bound_sample(sample, zero, unit);
}

template<class Io>
void evaluate_ramp(Io &io, RampWorkspace work) {
    auto [sample, input, unit, middle, first, zero] = work;
    input = io.input();
    unit = io.unit(Layer::base);
    sample = input;
    ramp_weight(io, Layer::base, Segment::first, sample, first, zero, unit);
    sample = input;
    ramp_weight(io, Layer::base, Segment::middle, sample, middle, zero, unit);
    sample = input;
    ramp_weight(io, Layer::base, Segment::last, sample, unit, zero, unit);

    auto &result = sample;
    auto &weighted_coefficient = unit;
    result = io.ramp_coefficient(Layer::base, Segment::last);
    const bool blended = io.has_blend();
    result = ordered_mul_ss(result, unit);
    weighted_coefficient = io.ramp_coefficient(Layer::base, Segment::middle);
    weighted_coefficient = ordered_mul_ss(weighted_coefficient, middle);
    result = ordered_add_ss(result, weighted_coefficient);
    weighted_coefficient = io.ramp_coefficient(Layer::base, Segment::first);
    weighted_coefficient = ordered_mul_ss(weighted_coefficient, first);
    result = ordered_add_ss(result, weighted_coefficient);
    result = ordered_add_ss(result, io.ramp_offset(Layer::base));
    io.store_result(Layer::base, result);
    if (!blended) return;

    unit = io.unit(Layer::blend);
    io.advance_layer();
    auto &second_first = middle;
    auto &second_middle = sample;
    auto &second_last = input;
    sample = input;
    ramp_weight(io, Layer::blend, Segment::first, sample, second_first, zero, unit);
    sample = input;
    ramp_weight(io, Layer::blend, Segment::middle, sample, second_middle, zero, unit);
    ramp_weight(io, Layer::blend, Segment::last, input, second_last, zero, unit);

    auto &second = first;
    second = io.ramp_coefficient(Layer::blend, Segment::last);
    second = ordered_mul_ss(second, second_last);
    second_last = io.ramp_coefficient(Layer::blend, Segment::middle);
    second_last = ordered_mul_ss(second_last, second_middle);
    second_middle = io.ramp_coefficient(Layer::blend, Segment::first);
    second = ordered_add_ss(second, second_last);
    second_middle = ordered_mul_ss(second_middle, second_first);
    second = ordered_add_ss(second, second_middle);
    auto &blend = sample;
    blend = io.blend_amount();
    second = ordered_add_ss(second, io.ramp_offset(Layer::blend));
    auto &base_weight = unit;
    base_weight = _mm_sub_ss(base_weight, blend);
    base_weight = ordered_mul_ss(base_weight, io.base_result());
    second = ordered_mul_ss(second, blend);
    second = ordered_add_ss(second, base_weight);
    io.store_result(Layer::blend, second);
}
} // namespace halo2::native_authored_linear_ramp::systems
