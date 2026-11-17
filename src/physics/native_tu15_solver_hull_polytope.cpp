#include "halo2/native_tu15_solver_hull_minimum_value.hpp"
#include "halo2/native_tu15_solver_hull_sample_value.hpp"
#include "halo2/native_tu15_solver_hull_polytope.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
std::uint32_t native_tu15_minimum_solver_hull_facet(SourcePhysicsContactMemory&memory,
    std::uint32_t polytope,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    NativeTu15SolverHullPolytopeStats*stats){
    if(stats)++stats->minimum_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native facet minimum requires qualified empty x87 DATA");
        Environment phase(fp,depth);const SourcePhysicsContactRecordObserver no_observer;
        native_solver_hull_minimum_value_detail::View view{Memory{memory,no_observer},polytope,stats};
        native_tu15_solver_hull_minimum_kernel(view);
        if(stats)++stats->minimum_completed;return view.minimum;
#else
        (void)memory;(void)polytope;(void)fp;(void)depth;
        throw std::runtime_error("Exact native facet minimum backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
std::uint32_t native_tu15_sample_solver_hull_direction(SourcePhysicsContactMemory&memory,
    std::uint32_t seed,std::uint32_t lower,std::uint32_t upper,std::uint32_t output,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullPolytopeStats*stats){
    if(stats)++stats->sample_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native hull sampling requires qualified empty x87 DATA");
        Environment phase(fp,depth);
        native_solver_hull_sample_value_detail::View view{Memory{memory,observe},seed,lower,upper,output};
        native_tu15_solver_hull_sample_kernel(view);
        if(stats)++stats->sample_completed;return view.current;
#else
        (void)memory;(void)seed;(void)lower;(void)upper;(void)output;(void)fp;(void)depth;(void)observe;
        throw std::runtime_error("Exact native hull sampling backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
