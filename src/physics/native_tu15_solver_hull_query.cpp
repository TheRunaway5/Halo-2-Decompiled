#include "halo2/native_tu15_solver_hull_query.hpp"
#include "halo2/native_tu15_solver_hull_context_kernel.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <stdexcept>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
struct HullQueryValueView {
    using Data=NativeTu15SolverHullContext;using P=NativeTu15SolverHullContextPhase;
    SourcePhysicsContactMemory&backing;
    joint_constraint_numeric::Memory memory;
    NativeTu15SolverHullQueryInput input;
    NativeTu15SolverHullQueryScratch scratch;
    NativeTu15PhysicsFrameFp&fp;unsigned depth;
    const NativeTu15SolverHullSupportServices&services;
    const std::function<std::uint64_t()>&clock;
    const SourcePhysicsContactRecordObserver&observe;
    NativeTu15SolverHullQueryStats&stats;
    std::uint32_t timestamp_low{};
    void begin(Data&data){data.context=scratch.context;}
    std::uint32_t profile_cursor(const Data&,bool){return memory.word(0x48c974u);}
    std::uint32_t profile_limit(const Data&,bool){return memory.word(0x48c978u);}
    void profile_ready(const Data&,bool,std::uint32_t,std::uint32_t){}
    void profile_tag(const Data&,bool ending,std::uint32_t cursor,std::uint32_t tag){memory.word(ending?0x3147abu:0x314728u,cursor,tag);}
    std::uint64_t timestamp(const Data&,bool){if(!clock)throw std::runtime_error("Native hull query requires actual platform profiling clock");return clock();}
    // The scalar facade's established contract owns no private CPU timestamp
    // cell. This value view keeps that transport private while the raw view
    // performs its actual stack store and fresh reload at the same phases.
    void stage_timestamp(const Data&,bool,std::uint32_t value){timestamp_low=value;}
    std::uint32_t reload_timestamp(const Data&,bool){return timestamp_low;}
    void profile_low(const Data&,bool ending,std::uint32_t cursor,std::uint32_t value){memory.word(ending?0x3147bbu:0x314738u,cursor+4u,value);}
    void advance_profile(const Data&,bool ending,std::uint32_t cursor){memory.word(ending?0x3147c1u:0x31473eu,0x48c974u,cursor+12u);++stats.profile_events;}
    std::uint32_t relative_transform(const Data&){return input.relative_transform;}
    std::uint32_t first_shape(const Data&){return input.first.identity;}
    std::uint32_t second_shape(const Data&){return input.second.identity;}
    std::uint32_t differences(const Data&){return input.differences;}
    std::uint32_t simplex(const Data&){return input.simplex;}
    std::uint32_t output(const Data&){return input.output_plane;}
    __m128 vector(P,const Data&,std::uint32_t address){return memory.vector(address);}
    void store_vector(P phase,const Data&data,__m128 value){
        const auto pc=phase==P::basis_first?0x314750u:phase==P::basis_second?0x314759u:phase==P::basis_third?0x314769u:0x314785u;
        const auto offset=phase==P::basis_first?0u:phase==P::basis_second?0x10u:phase==P::basis_third?0x20u:0x30u;
        memory.vector(pc,data.context+offset,value);
    }
    void store_word(P phase,const Data&data,std::uint32_t value){const auto p=data.context;
        switch(phase){case P::first_shape:memory.word(0x314762u,p+0x40u,value);break;
        case P::second_shape:memory.word(0x314775u,p+0x44u,value);break;
        case P::differences:memory.word(0x31477cu,p+0x50u,value);break;
        case P::simplex:memory.word(0x31478au,p+0x48u,value);break;
        case P::initial_state:memory.word(0x31478eu,p+0x4cu,value);break;
        default:throw std::runtime_error("Invalid hull-context value field");}
    }
    void prepare_child(const Data&){}
    std::uint32_t hull_child(const Data&data){return native_tu15_complete_single_vertex_solver_hull(backing,data.context,data.output,scratch.single_vertex,fp,depth,services,observe,&stats.hull);}
    std::uint32_t finish(const Data&data){return data.result;}
};
}
#endif
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
        HullQueryValueView view{memory,{memory,observe},input,scratch,fp,depth,services,timestamp_counter,observe,stats};
        NativeTu15SolverHullContext data;
        const auto result=native_tu15_build_solver_hull_context(view,data);
        ++stats.completed;return result;
#else
        (void)memory;(void)input;(void)scratch;(void)fp;(void)depth;(void)services;(void)timestamp_counter;(void)observe;
        throw std::runtime_error("Exact native hull query backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
