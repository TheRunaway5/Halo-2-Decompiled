#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SolverHullInitialPath : std::uint32_t {
    count_zero=0x313db2u,
    count_one=0x313d69u,
    count_two=0x313d14u,
    count_three=0x313c89u,
    count_four=0x313bfcu,
    build_tetrahedron=0x313e8eu,
};

template<class View> NativeTu15SolverHullInitialPath native_tu15_solver_hull_begin_kernel(View& view) {
    view.open_exception_frame();
    view.reserve_solver_stack();
    view.capture_returned_context();
    return view.read_initial_route();
}
} // namespace halo2
