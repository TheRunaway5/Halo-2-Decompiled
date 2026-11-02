#pragma once
#include "halo2/native_tu15_convex_simplex_solver.hpp"
#include "halo2/native_tu15_convex_contact_query.hpp"
#include "halo2/native_tu15_contact_features.hpp"

namespace halo2 {
struct NativeTu15ConvexSimplexBindingScratch {
    NativeTu15ConvexSimplexSolverScratch solver;
    NativeTu15ContactShapeScratch shape;
};
struct NativeTu15ConvexSimplexBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15ConvexSimplexSolverStats solver;
};
// This is the shared native30E930 service for both2F8630 query and2F8B60 process.
// The returned service retains no stack parameter references or source outcomes.
// Its caller owns memory, activity, the complete nested scratch lifetime, and
// must pass this SAME guarded memory to the complete query/process and all later
// result/publication consumers. It exposes no raw private-store observer.
inline NativeTu15ConvexContactServices native_tu15_make_bound_convex_contact_services(
    NativeTu15ConvexSimplexMemory&memory,NativeTu15ConvexSimplexBindingScratch scratch,
    unsigned actual_x87_depth,std::function<std::uint64_t()>clock={},
    NativeTu15ConvexSimplexBindingActivity*activity=nullptr){
    NativeTu15ConvexContactServices bound;bound.timestamp_counter=clock;
    bound.solve_simplex=[&memory,scratch,actual_x87_depth,clock,activity](auto work,
        auto first,auto second,auto relative,auto cache,auto plane,auto&fp){
        NativeTu15ConvexSimplexBindingActivity local;auto&out=activity?*activity:local;
        ++out.attempts;
        NativeTu15ConvexSimplexSolverServices native;
        native.features=[actual_x87_depth](auto&view,auto shape,auto owner,auto ids,auto count,auto output,auto&current){
            native_tu15_contact_shape_features(view,shape,owner,ids,count,output,current,actual_x87_depth);
        };
        native.single.support=[scratch,actual_x87_depth](auto&view,auto shape,auto owner,auto direction,auto output,auto&current){
            native_tu15_contact_shape_support(view,shape,owner,direction,output,scratch.shape,current,actual_x87_depth);
        };
        try{
            const auto result=native_tu15_solve_convex_contact_simplex(memory,
                {work,first,second,relative,cache,plane},scratch.solver,fp,
                actual_x87_depth,native,clock,&out.solver);
            ++out.completed;return result;
        }catch(...){++out.refusals;throw;}
    };
    return bound;
}
} // namespace halo2
