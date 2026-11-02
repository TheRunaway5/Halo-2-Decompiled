#include "halo2/native_tu15_solver_hull_single_vertex.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
std::uint32_t native_tu15_complete_single_vertex_solver_hull(
    SourcePhysicsContactMemory&memory,std::uint32_t context,std::uint32_t output_plane,
    NativeTu15SolverHullSingleVertexScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const NativeTu15SolverHullSupportServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullSingleVertexStats*activity){
    NativeTu15SolverHullSingleVertexStats local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native single-vertex hull requires qualified empty x87 DATA");
        {Environment validate(fp,depth);}
        Memory m(memory,observe);const auto simplex=m.word(context+0x48u),count=m.word(simplex+0x80u);
        if(count!=1u){++stats.initial_count_refusals;throw std::runtime_error("Native hull requires remaining initial-count algorithm");}
        std::int32_t retries=-1;
        for(;;){
            bool extend;
            {
                Environment phase(fp,depth);const auto differences=m.word(context+0x50u);const auto vertex=m.vector(differences);
                const auto product=multiply(vertex,vertex),sum=add_scalar(lane<0xaau>(product),add_scalar(lane<0x55u>(product),product));
                m.word(0x313d8cu,scratch.scalar,scalar_bits(sum));push(m.word(scratch.scalar));
                const auto threshold=m.word(0x46a454u);std::uint16_t status;
                asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(threshold):"memory","st");
                extend=(std::popcount(unsigned((status>>8u)&0x41u))&1u)==0u;
            }
            if(extend){++stats.refused_owners[0x3136a0u];throw std::runtime_error("Native hull requires remaining line-extension owner3136A0");}
            if(retries>=0){++stats.returned_retry_limit;++stats.completed;return 2u;}
            const auto current_simplex=m.word(context+0x48u),current_count=m.word(current_simplex+0x80u);
            m.word(0x313dacu,current_simplex+0x80u,current_count-1u);++stats.initializations;
            native_tu15_initialize_solver_hull(memory,context,scratch.initialized_flag,output_plane,
                scratch.initialize,fp,depth,services,observe,&stats.support);
            if(m.word(scratch.initialized_flag,1)){++stats.returned_overlap;++stats.completed;return 0u;}
            retries=0;
        }
#else
        (void)memory;(void)context;(void)output_plane;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native solver hull backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
