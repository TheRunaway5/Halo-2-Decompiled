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
        Memory m(memory,observe);const auto initial_direction=m.word(context+0x50u);
        native_tu15_solver_hull_support_pair(memory,context,initial_direction,scratch.first_pair,
            scratch.transformed_direction,fp,depth,services,observe,&stats.support);
        const auto subtract=[](Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;};
        {
            Environment phase(fp,depth);
            const auto differences=m.word(context+0x50u);
            const auto vertex=m.vector(differences);
            const auto transformed=m.vector(scratch.first_pair+0x20u),first=m.vector(scratch.first_pair);
            const auto delta=subtract(subtract(first,transformed),vertex),squared=multiply(delta,delta);
            const auto sum=add_scalar(lane<0xaau>(squared),add_scalar(lane<0x55u>(squared),squared));
            const auto sign=m.vector(0x461960u),current_direction=m.vector(initial_direction);
            m.word(0x31370au,scratch.first_distance,scalar_bits(sum));
            m.vector(0x31370eu,scratch.negative_direction,xor_bits(current_direction,sign));
        }
        native_tu15_solver_hull_support_pair(memory,context,scratch.negative_direction,scratch.second_pair,
            scratch.transformed_direction,fp,depth,services,observe,&stats.support);
        bool second;
        {
            Environment phase(fp,depth);push(m.word(scratch.first_distance));
            const auto transformed=m.vector(scratch.second_pair+0x20u);
            const auto differences=m.word(context+0x50u);
            const auto first=m.vector(scratch.second_pair),vertex=m.vector(differences);
            const auto delta=subtract(subtract(first,transformed),vertex),squared=multiply(delta,delta);
            const auto sum=add_scalar(lane<0xaau>(squared),add_scalar(lane<0x55u>(squared),squared));
            m.word(0x313752u,scratch.second_distance,scalar_bits(sum));
            const auto right=m.word(scratch.second_distance);std::uint16_t relation;
            asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(right):"memory","st");
            second=(relation&0x100u)!=0;
        }
        if(second)++stats.selected_second;else ++stats.selected_first;
        native_tu15_append_solver_hull_vertex(memory,context,
            second?scratch.second_pair:scratch.first_pair,fp,depth,observe,&stats.support);
        ++stats.completed;
#else
        (void)memory;(void)context;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native hull line backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
