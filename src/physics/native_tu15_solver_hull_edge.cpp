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
        Memory m(memory,observe);
        {
            Environment phase(fp,depth);const auto sign=m.vector(0x461960u),axis=m.vector(direction);
            m.vector(0x313494u,scratch.negative_direction,xor_bits(axis,sign));
        }
        native_tu15_solver_hull_support_pair(memory,context,direction,scratch.first_pair,
            scratch.transformed_direction,fp,depth,services,observe,&stats.support);
        native_tu15_solver_hull_support_pair(memory,context,scratch.negative_direction,output,
            scratch.transformed_direction,fp,depth,services,observe,&stats.support);
        {
            Environment phase(fp,depth);
            const auto subtract=[](Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;};
            const auto position=m.vector(origin),first=m.vector(scratch.first_pair);
            const auto transformed=m.vector(scratch.first_pair+0x20u),current_axis=m.vector(direction);
            const auto first_product=multiply(subtract(subtract(first,transformed),position),current_axis);
            const auto first_xy=add_scalar(lane<0x55u>(first_product),first_product);
            const auto second_transformed=m.vector(output+0x20u);
            const auto first_sum=add_scalar(lane<0xaau>(first_product),first_xy);
            const auto negative_axis=m.vector(scratch.negative_direction);
            m.word(0x3134f0u,scratch.first_projection,scalar_bits(first_sum));push(m.word(scratch.first_projection));
            const auto second=m.vector(output),second_product=multiply(subtract(subtract(second,second_transformed),position),negative_axis);
            const auto second_sum=add_scalar(lane<0xaau>(second_product),add_scalar(lane<0x55u>(second_product),second_product));
            m.word(0x313521u,scratch.second_projection,scalar_bits(second_sum));
            const auto right=m.word(scratch.second_projection);std::uint16_t relation;
            asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(right):"memory","st");
            if(!(relation&0x100u)){
                const auto local_second=m.vector(scratch.first_pair+0x10u);
                const auto value=m.word(scratch.first_projection);
                m.vector(0x313539u,output,first);m.vector(0x31353cu,output+0x10u,local_second);
                m.vector(0x313540u,output+0x20u,transformed);m.word(0x313544u,output+0x30u,value);++stats.selected_first;
            }else{const auto value=m.word(scratch.second_projection);m.word(0x313555u,output+0x30u,value);++stats.selected_second;}
        }
        ++stats.completed;
#else
        (void)memory;(void)context;(void)direction;(void)origin;(void)output;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native hull edge backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
