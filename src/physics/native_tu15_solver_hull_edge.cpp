#include "halo2/native_tu15_solver_hull_edge_value.hpp"
#include "halo2/native_tu15_solver_hull_edge.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <stdexcept>
namespace halo2 {
void native_tu15_select_solver_hull_edge(SourcePhysicsContactMemory&memory,
    std::uint32_t context,std::uint32_t direction,std::uint32_t origin,
    std::uint32_t output,NativeTu15SolverHullEdgeScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15SolverHullSupportServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullEdgeStats*activity){
    NativeTu15SolverHullEdgeStats local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native hull edge requires qualified empty x87 DATA");
        using View=native_solver_hull_edge_value_detail::View<Memory,Environment>;
        View view{{memory,observe},memory,fp,depth,services,observe,stats,
                  context,direction,origin,output,scratch};
        View::Data data;native_tu15_solver_hull_edge_kernel(view,data);
        ++stats.completed;
#else
        (void)memory;(void)context;(void)direction;(void)origin;(void)output;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native hull edge backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
