#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SolverHullSearchResume : std::uint32_t {
    iterate=0x313f63u,refresh_seed=0x3140ddu,finish_inline=0x3141b5u,project_boundary=0x3144a0u
};

template<class View> NativeTu15SolverHullSearchResume native_tu15_solver_hull_search_iteration_kernel(View& view) {
    using Resume=NativeTu15SolverHullSearchResume;
    view.closest_support_pair();
    if(view.axes_cross_threshold())return Resume::project_boundary;
    if(view.find_adjacent_facet())return Resume::refresh_seed;
    if(!view.has_inline_room())return Resume::finish_inline;
    view.insert_edge();
    return view.append_support()?Resume::iterate:Resume::refresh_seed;
}
} // namespace halo2
