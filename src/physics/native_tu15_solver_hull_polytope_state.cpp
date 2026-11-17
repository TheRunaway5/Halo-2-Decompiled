#include "native_tu15_solver_hull_clip_value.hpp"
#include "halo2/native_tu15_solver_hull_clear_value.hpp"
#include "halo2/native_tu15_solver_hull_polytope_state.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
void native_tu15_clear_solver_hull_polytope_flags(SourcePhysicsContactMemory&memory,
    std::uint32_t polytope,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullPolytopeStateStats*stats) {
    if(stats)++stats->clear_calls;
    try {
        native_solver_hull_clear_value_detail::View view(memory,polytope,observe,stats);
        native_tu15_solver_hull_clear_kernel(view);
        if(stats)++stats->clear_completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}

std::uint32_t native_tu15_clip_solver_hull_facet(SourcePhysicsContactMemory&memory,
    std::uint32_t facet,std::uint32_t point,std::uint32_t lower,std::uint32_t upper,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullPolytopeStateStats*stats) {
    if(stats)++stats->clip_calls;
    try {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native facet clipping requires qualified empty x87 DATA");
        native_solver_hull_clip_value_detail::View view(memory,facet,point,lower,upper,fp,depth,observe);
        const auto result=native_tu15_solver_hull_clip_kernel(view);
        if(stats)++stats->clip_completed;return result;
#else
        (void)memory;(void)facet;(void)point;(void)lower;(void)upper;
        (void)fp;(void)depth;(void)observe;
        throw std::runtime_error("Exact native facet clipping backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
