#include "halo2/native_tu15_solver_hull_query.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <stdexcept>
namespace halo2 {
std::uint32_t native_tu15_query_solver_hull(SourcePhysicsContactMemory&memory,
    NativeTu15SolverHullQueryInput input,NativeTu15SolverHullQueryScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15SolverHullSupportServices&services,
    const std::function<std::uint64_t()>&timestamp_counter,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullQueryStats*activity){
    NativeTu15SolverHullQueryStats local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native hull query requires qualified empty x87 DATA");
        {Environment validate(fp,depth);}
        if(scratch.context&15u)throw std::runtime_error("Native hull query requires aligned owned context DATA");
        Memory m(memory,observe);
        const auto profile=[&](bool ending){
            const auto cursor=m.word(0x48c974u),limit=m.word(0x48c978u);if(cursor>=limit)return;
            m.word(ending?0x3147abu:0x314728u,cursor,ending?0x41c394u:0x41d784u);
            if(!timestamp_counter)throw std::runtime_error("Native hull query requires actual platform profiling clock");
            m.word(ending?0x3147bbu:0x314738u,cursor+4u,std::uint32_t(timestamp_counter()));
            m.word(ending?0x3147c1u:0x31473eu,0x48c974u,cursor+12u);++stats.profile_events;
        };
        profile(false);const auto p=scratch.context;
        const auto first=m.vector(input.relative_transform);m.vector(0x314750u,p,first);
        const auto second=m.vector(input.relative_transform+0x10u);m.vector(0x314759u,p+0x10u,second);
        const auto third=m.vector(input.relative_transform+0x20u);m.word(0x314762u,p+0x40u,input.first.identity);
        m.vector(0x314769u,p+0x20u,third);
        const auto fourth=m.vector(input.relative_transform+0x30u);m.word(0x314775u,p+0x44u,input.second.identity);
        m.word(0x31477cu,p+0x50u,input.differences);m.vector(0x314785u,p+0x30u,fourth);
        m.word(0x31478au,p+0x48u,input.simplex);m.word(0x31478eu,p+0x4cu,0);
        const auto result=native_tu15_complete_single_vertex_solver_hull(memory,p,input.output_plane,
            scratch.single_vertex,fp,depth,services,observe,&stats.hull);
        profile(true);++stats.completed;return result;
#else
        (void)memory;(void)input;(void)scratch;(void)fp;(void)depth;(void)services;(void)timestamp_counter;(void)observe;
        throw std::runtime_error("Exact native hull query backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
