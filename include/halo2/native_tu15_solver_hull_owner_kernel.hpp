#pragma once
#include "halo2/native_tu15_solver_hull_seed_transition_kernel.hpp"
#include "halo2/native_tu15_solver_hull_search_iteration_kernel.hpp"
#include "halo2/native_tu15_solver_hull_search_refresh_kernel.hpp"

namespace halo2 {
// The private controller owns the actual seed/iteration/restart decisions.
// Children run through the live caller frame and return before the next view
// is entered; a returned GP, FP or backing slot is never replaced with input.
template<class View> void native_tu15_solver_hull_owner_kernel(View& view) {
    using Seed=NativeTu15SolverHullSeedExit;
    using SeedResume=NativeTu15SolverHullSeedResume;
    using Search=NativeTu15SolverHullSearchResume;
    auto route=view.begin();
    for(;;) {
        const auto seed=view.classify(route);
        if(seed==Seed::write_simplex_result){view.write_seed_result();return;}
        if(seed==Seed::write_degenerate_result){view.return_degenerate();return;}
        if(seed!=Seed::build_tetrahedron) {
            const auto next=view.transition(seed);
            if(next==SeedResume::near_origin){view.return_near_origin();return;}
            if(next==SeedResume::write_triangle_projection){view.write_triangle_result();view.return_near_origin();return;}
            route=static_cast<NativeTu15SolverHullInitialPath>(next);
            continue;
        }

        view.setup_search();
        Search result;
        do{result=view.iterate();}while(result==Search::iterate);
        if(result==Search::refresh_seed) {
            if(view.refresh()==NativeTu15SolverHullRefreshResume::retry_seed){route=view.restart();continue;}
            view.mark_iteration_limit();
        }else if(result==Search::finish_inline)view.mark_inline_limit();
        view.project_boundary();
        view.finish();
        return;
    }
}
} // namespace halo2
