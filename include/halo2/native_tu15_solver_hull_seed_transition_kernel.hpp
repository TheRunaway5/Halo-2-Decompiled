#pragma once
#include "halo2/native_tu15_solver_hull_seed_route_kernel.hpp"

namespace halo2 {
enum class NativeTu15SolverHullSeedResume : std::uint32_t {
    classify_one=0x313d69u,classify_two=0x313d14u,classify_three=0x313c89u,
    classify_four=0x313bfcu,near_origin=0x31445fu,write_triangle_projection=0x314353u
};

template<class View> NativeTu15SolverHullSeedResume native_tu15_solver_hull_seed_transition_kernel(
    View& view,NativeTu15SolverHullSeedExit entry) {
    using Exit=NativeTu15SolverHullSeedExit;
    using Resume=NativeTu15SolverHullSeedResume;
    switch(entry) {
    case Exit::count_zero:return view.initialize()?Resume::classify_one:Resume::near_origin;
    case Exit::append_line:view.append_line();return Resume::classify_two;
    case Exit::improve_basis:view.improve_basis();return Resume::classify_three;
    case Exit::reduce_triangle:
        if(view.reduce_triangle())return Resume::write_triangle_projection;
        view.append_triangle_vertex();return Resume::classify_four;
    default:throw std::invalid_argument("Unknown native hull seed transition");
    }
}
} // namespace halo2
