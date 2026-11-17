#include "physics/native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2::solver_hull_support_pair_value_reference {
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
        // Admit the native FP policy before any source DATA is accessed.
        {Environment validate(fp,depth);}
        Memory m(memory,observe);
        const auto support=[&](std::uint32_t shape,std::uint32_t input,std::uint32_t output){
            const auto table=m.word(shape),owner=m.word(table+0x30u);
            if(stats)++stats->shape_support_calls;
            if(!services.support)throw std::runtime_error("Native solver hull requires its actual shape-support service");
            services.support({shape},{owner},input,output,fp);
        };
        support(m.word(context+0x40u),direction,point_pair);
        std::uint32_t second_shape;
        {
            Environment phase(fp,depth);
            const auto z=m.vector(context+0x20u),sign=m.vector(0x461960u),input=m.vector(direction);
            const auto x=m.vector(context),y=m.vector(context+0x10u);second_shape=m.word(context+0x44u);
            const auto negative=xor_bits(input,sign);
            const auto xy0=_mm_shuffle_ps(x,y,0x44),xy1=_mm_shuffle_ps(x,y,0xee);
            const auto zz0=_mm_shuffle_ps(z,z,0x44),zz1=_mm_shuffle_ps(z,z,0xee);
            const auto ty=_mm_shuffle_ps(xy0,zz0,0xdd);const auto yterm=multiply(ty,lane<0x55u>(negative));
            const auto tx=_mm_shuffle_ps(xy0,zz0,0x88);const auto xterm=multiply(tx,lane<0u>(negative));
            const auto tz=_mm_shuffle_ps(xy1,zz1,0x88);const auto zterm=multiply(tz,lane<0xaau>(negative));
            m.vector(0x31341du,transformed_direction,add(add(xterm,yterm),zterm));
        }
        support(second_shape,transformed_direction,point_pair+0x10u);
        {
            Environment phase(fp,depth);
            const auto local=m.vector(point_pair+0x10u),translation=m.vector(context+0x30u),z=m.vector(context+0x20u);
            const auto zterm=multiply(z,lane<0xaau>(local));const auto translated=add(zterm,translation);
            const auto y=m.vector(context+0x10u);const auto yterm=multiply(y,lane<0x55u>(local));
            const auto x=m.vector(context);const auto xterm=multiply(x,lane<0u>(local));
            m.vector(0x313461u,point_pair+0x20u,add(add(xterm,yterm),translated));
        }
        if(stats)++stats->pair_completed;
#else
        (void)memory;(void)context;(void)direction;(void)point_pair;(void)transformed_direction;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native solver hull backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
}
