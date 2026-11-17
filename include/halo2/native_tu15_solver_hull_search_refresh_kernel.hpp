#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SolverHullRefreshResume : std::uint32_t {
    retry_seed=0x313be0u,iteration_limit=0x314494u
};
enum class NativeTu15SolverHullRefreshStep { first_restart, perturb, exhausted };

template<class View> NativeTu15SolverHullRefreshResume native_tu15_solver_hull_search_refresh_kernel(View& view) {
    using Resume=NativeTu15SolverHullRefreshResume;
    switch(view.advance_attempt()) {
    case NativeTu15SolverHullRefreshStep::exhausted:return Resume::iteration_limit;
    case NativeTu15SolverHullRefreshStep::first_restart:view.reset_to_one_seed();break;
    case NativeTu15SolverHullRefreshStep::perturb:view.reseed_translation();break;
    }
    view.release_payload_if_owned();
    view.release_vertices_if_owned();
    view.destroy_tetrahedron();
    return Resume::retry_seed;
}
} // namespace halo2
