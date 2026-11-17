#include "halo2/native_tu15_solver_hull_initialize_value.hpp"
#include "halo2/native_tu15_solver_hull_support_pair_value.hpp"
#include "halo2/native_tu15_solver_hull_support.hpp"
#include "halo2/native_tu15_solver_hull_append_kernel.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <stdexcept>
#include <bit>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
struct HullAppendValueView {
    using Vector=joint_constraint_numeric::Vector;
    using Data=NativeTu15SolverHullAppend<Vector>;using P=NativeTu15SolverHullAppendPhase;
    joint_constraint_numeric::Memory memory;std::uint32_t context,pair;
    void begin(Data&data){data.context=context;}
    std::uint32_t simplex(P,const Data&data){return memory.word(data.context+0x48u);}
    std::uint32_t count(P,const Data&data){return memory.word(data.simplex+0x80u);}
    std::uint32_t point_pair(const Data&){return pair;}
    Vector vector(P,const Data&,std::uint32_t address){return memory.vector(address);}
    Vector first_point(const Data&data){return memory.vector(data.pair);}
    std::uint32_t differences(const Data&data){return memory.word(data.context+0x50u);}
    void difference_destination(const Data&){}
    Vector subtract(Vector first,Vector second){Vector difference=first;asm volatile("subps %1,%0":"+x"(difference):"x"(second):"memory");return difference;}
    void store_difference(const Data&data){memory.vector(0x3132b8u,data.destination,data.difference);}
    void point_destination(P,const Data&){}
    void store_point(P phase,const Data&data){memory.vector(phase==P::first_point?0x3132cau:0x3132e1u,data.destination,data.point);}
    void increment_count(const Data&data){const auto count=memory.word(data.simplex+0x80u);memory.word(0x3132e8u,data.simplex+0x80u,count+1u);}
    void finish(const Data&){}
};
}
#endif
void native_tu15_append_solver_hull_vertex(SourcePhysicsContactMemory&memory,
    std::uint32_t context,std::uint32_t point_pair,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullSupportStats*stats){
    if(stats)++stats->append_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth)throw std::runtime_error("Native solver hull append requires qualified empty x87 DATA");
        using namespace joint_constraint_numeric;
        Environment environment(fp,depth);HullAppendValueView view{{memory,observe},context,point_pair};
        HullAppendValueView::Data data;native_tu15_append_solver_hull_point(view,data);
        if(stats)++stats->append_completed;
#else
        (void)memory;(void)context;(void)point_pair;(void)fp;(void)depth;(void)observe;
        throw std::runtime_error("Exact native solver hull backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
void native_tu15_solver_hull_support_pair(SourcePhysicsContactMemory&memory,
    std::uint32_t context,std::uint32_t direction,std::uint32_t point_pair,
    std::uint32_t transformed_direction,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15SolverHullSupportServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullSupportStats*stats){
    if(stats)++stats->pair_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native solver hull support requires qualified empty x87 DATA");
        // Preserve the existing value-only FP admission and two phase lifetimes.
        {Environment validate(fp,depth);}
        using View=native_solver_hull_support_pair_value_detail::View<Memory,Environment>;
        View view{{memory,observe},fp,depth,services,stats,context,direction,point_pair,transformed_direction};
        View::Data data;native_tu15_solver_hull_support_pair_kernel(view,data);
        if(stats)++stats->pair_completed;
#else
        (void)memory;(void)context;(void)direction;(void)point_pair;(void)transformed_direction;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native solver hull backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
namespace {
void hull_byte(SourcePhysicsContactMemory&memory,std::uint32_t pc,std::uint32_t p,
    std::uint8_t value,const SourcePhysicsContactRecordObserver&observe){
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(observe){event.before.resize(1);memory.read(p,event.before);}
    const std::array b{std::byte(value)};memory.write(p,b);
    if(observe){event.written.assign(b.begin(),b.end());observe(event);}
}
}
std::uint32_t native_tu15_initialize_solver_hull(SourcePhysicsContactMemory&memory,
    std::uint32_t context,std::uint32_t output_flag,std::uint32_t output_plane,
    NativeTu15SolverHullInitializeScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15SolverHullSupportServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SolverHullSupportStats*stats){
    if(stats)++stats->initialize_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native solver hull initialization requires qualified empty x87 DATA");
        using View=native_solver_hull_initialize_value_detail::View<Memory,Environment>;
        View view{{memory,observe},memory,fp,depth,services,observe,stats,
                  context,output_flag,output_plane,scratch};
        View::Data data;native_tu15_solver_hull_initialize_kernel(view,data);
        if(stats)++stats->initialize_completed;return output_flag;
#else
        (void)memory;(void)context;(void)output_flag;(void)output_plane;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native solver hull backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
