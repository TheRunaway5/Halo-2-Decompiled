#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_authored_cubic::systems {
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

// The current polynomial's powers remain available to the next layer.
struct Workspace {
    __m128 &parameter, &square, &cube, &result, &coefficient;
};

template<class Io>
bool evaluate_base(Io &io, Workspace work) {
    auto [parameter, square, cube, result, coefficient] = work;
    result = io.base_quadratic();
    coefficient = io.base_linear();
    const bool blended = io.open_base_layer();
    square = ordered_mul_ss(parameter, parameter);
    result = ordered_mul_ss(result, square);
    coefficient = ordered_mul_ss(coefficient, parameter);
    result = ordered_add_ss(result, coefficient);
    coefficient = io.base_cubic();
    cube = ordered_mul_ss(square, parameter);
    coefficient = ordered_mul_ss(coefficient, cube);
    result = ordered_add_ss(result, coefficient);
    result = ordered_add_ss(result, io.base_offset());
    io.store_base(result);
    return blended;
}

inline void refresh_powers(Workspace work) {
    work.square = ordered_mul_ss(work.parameter, work.parameter);
    work.cube = ordered_mul_ss(work.square, work.parameter);
}

template<class Io>
void evaluate_blend(Io &io, Workspace work) {
    auto [parameter, square, cube, result, coefficient] = work;
    auto &second_linear = square;
    auto &second_cubic = parameter;
    result = io.second_quadratic();
    result = ordered_mul_ss(result, square);
    second_linear = io.second_linear();
    second_linear = ordered_mul_ss(second_linear, parameter);
    second_cubic = io.second_cubic();
    result = ordered_add_ss(result, second_linear);
    auto &base_weight = square;
    base_weight = io.unit();
    second_cubic = ordered_mul_ss(second_cubic, cube);
    result = ordered_add_ss(result, second_cubic);
    auto &blend = parameter;
    blend = io.blend_amount();
    result = ordered_add_ss(result, io.second_offset());
    base_weight = _mm_sub_ss(base_weight, blend);
    // A guest access rereads the committed first result at this point.
    base_weight = ordered_mul_ss(base_weight, io.base_result());
    result = ordered_mul_ss(result, blend);
    result = ordered_add_ss(result, base_weight);
    io.store_blend(result);
}
} // namespace halo2::native_authored_cubic::systems
