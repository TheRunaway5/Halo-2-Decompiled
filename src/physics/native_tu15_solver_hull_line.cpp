#include "halo2/native_tu15_solver_hull_line_value.hpp"
#include "halo2/native_tu15_solver_hull_line.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <stdexcept>
namespace halo2 {
void native_tu15_extend_solver_hull_line(SourcePhysicsContactMemory&memory,
    std::uint32_t context,NativeTu15SolverHullLineScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15SolverHullSupportServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullLineStats*activity){
    NativeTu15SolverHullLineStats local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native hull line requires qualified empty x87 DATA");
        {Environment validate(fp,depth);}
        using View=native_solver_hull_line_value_detail::View<Memory,Environment>;
        View view{{memory,observe},memory,fp,depth,services,observe,stats,context,scratch};
        View::Data data;native_tu15_solver_hull_line_kernel(view,data);
        ++stats.completed;
#else
        (void)memory;(void)context;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native hull line backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
