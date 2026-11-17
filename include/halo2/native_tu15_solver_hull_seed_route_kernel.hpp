#pragma once
#include "halo2/native_tu15_solver_hull_begin_kernel.hpp"
#include <stdexcept>

namespace halo2 {
enum class NativeTu15SolverHullSeedExit : std::uint32_t {
    count_zero=0x313db2u,build_tetrahedron=0x313e8eu,reduce_triangle=0x313df1u,
    improve_basis=0x313de0u,append_line=0x313dcfu,write_simplex_result=0x3141c2u,
    write_degenerate_result=0x314478u,demote=0u
};

// Only failed geometry tests demote an initial simplex. The next stage
// receives the exact continuation and the live frame after that demotion.
template<class View> NativeTu15SolverHullSeedExit native_tu15_solver_hull_seed_route_kernel(
    View& view,NativeTu15SolverHullInitialPath path) {
    using Path=NativeTu15SolverHullInitialPath;
    using Exit=NativeTu15SolverHullSeedExit;
    Exit exit;
    switch(path) {
    case Path::count_four:
        exit=view.classify_four();if(exit!=Exit::demote)return exit;
        [[fallthrough]];
    case Path::count_three:
        exit=view.classify_three();if(exit!=Exit::demote)return exit;
        [[fallthrough]];
    case Path::count_two:
        exit=view.classify_two();if(exit!=Exit::demote)return exit;
        [[fallthrough]];
    case Path::count_one:
        exit=view.classify_one();if(exit!=Exit::demote)return exit;
        [[fallthrough]];
    case Path::count_zero:return Exit::count_zero;
    case Path::build_tetrahedron:return Exit::build_tetrahedron;
    }
    throw std::invalid_argument("Unknown native hull seed route");
}
} // namespace halo2
