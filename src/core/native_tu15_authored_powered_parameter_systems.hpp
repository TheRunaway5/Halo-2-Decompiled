#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_authored_powered_parameter::systems {
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

enum class Pair : unsigned { starting, ending };
enum class Outcome : unsigned { collapsed_end, collapsed_start, matching_lengths, power_zero, power_unit, power_value };

struct Workspace {
    __m128 &parameter, &ending_distance, &delta_y, &starting_distance, &starting_y_square, &floor;
};

template<class Io>
void squared_distance(Io &io, Pair pair, __m128 &distance, __m128 &delta_y, __m128 &squared_y) {
    delta_y = io.point_y(pair);
    delta_y = _mm_sub_ss(delta_y, io.next_point_y(pair));
    distance = io.point_x(pair);
    distance = _mm_sub_ss(distance, io.next_point_x(pair));
    distance = ordered_mul_ss(distance, distance);
    squared_y = ordered_mul_ss(delta_y, delta_y);
    distance = ordered_add_ss(distance, squared_y);
}

// Reparameterize the cubic by the ratio of its end-point gradient lengths.
// The boundary keeps the authored scratch spills and the required numerical
// services at the exact points where the legacy callers can observe them.
template<class Io>
Outcome evaluate(Io &io, Workspace work) {
    auto [parameter, ending_distance, delta_y, starting_distance, starting_y_square, floor] = work;
    io.load_control_points();
    squared_distance(io, Pair::ending, ending_distance, delta_y, starting_distance);
    if (!io.store_ending_distance(ending_distance)) return Outcome::collapsed_end;

    io.select_starting_pair();
    squared_distance(io, Pair::starting, starting_distance, delta_y, starting_y_square);
    io.store_starting_distance(starting_distance);
    if (io.start_is_near_zero()) {
        parameter = io.unit_for_shortcut();
        return Outcome::collapsed_start;
    }
    if (io.distances_are_near_equal()) return Outcome::matching_lengths;

    io.estimate_exponent(parameter, starting_distance, ending_distance);
    io.raise_parameter();
    Outcome result;
    if (io.power_is_negative()) {
        parameter = _mm_setzero_ps();
        result = Outcome::power_zero;
    } else {
        ending_distance = io.stored_power();
        parameter = io.unit_for_power();
        result = Outcome::power_unit;
        if (!io.power_exceeds_unit(ending_distance, parameter)) {
            parameter = ending_distance;
            result = Outcome::power_value;
        }
    }
    floor = _mm_setzero_ps();
    return result;
}
} // namespace halo2::native_authored_powered_parameter::systems
