#include "halo2/native_tu15_solver_hull_support.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <stdexcept>
#include <bit>
namespace halo2 {
void native_tu15_append_solver_hull_vertex(SourcePhysicsContactMemory&memory,
    std::uint32_t context,std::uint32_t point_pair,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullSupportStats*stats){
    if(stats)++stats->append_calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth)throw std::runtime_error("Native solver hull append requires qualified empty x87 DATA");
        using namespace joint_constraint_numeric;
        Environment environment(fp,depth);Memory m(memory,observe);
        const auto simplex=m.word(context+0x48u),index=m.word(simplex+0x80u);
        const auto second=m.vector(point_pair+0x20u),first=m.vector(point_pair);
        const auto differences=m.word(context+0x50u);Vector difference=first;
        asm volatile("subps %1,%0":"+x"(difference):"x"(second):"memory");
        m.vector(0x3132b8u,differences+(index<<4u),difference);
        const auto current_simplex=m.word(context+0x48u),current_index=m.word(current_simplex+0x80u);
        const auto current_first=m.vector(point_pair);
        m.vector(0x3132cau,current_simplex+(current_index<<4u),current_first);
        const auto next_simplex=m.word(context+0x48u),next_index=m.word(next_simplex+0x80u);
        const auto current_second=m.vector(point_pair+0x10u);
        m.vector(0x3132e1u,next_simplex+((next_index+4u)<<4u),current_second);
        const auto final_simplex=m.word(context+0x48u),count=m.word(final_simplex+0x80u);
        m.word(0x3132e8u,final_simplex+0x80u,count+1u);
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
        Memory m(memory,observe);
        const auto above_or_unordered=[&](std::uint32_t measured,std::uint32_t threshold){
            push(measured);const auto bound=m.word(threshold);std::uint16_t status;
            asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bound):"memory","st");
            return (std::popcount(unsigned((status>>8u)&0x41u))&1u)==0u;
        };
        {
            Environment phase(fp,depth);
            const auto sign=m.vector(0x461960u),translation=m.vector(context+0x30u);
            const auto direction=xor_bits(translation,sign);m.vector(0x31357au,scratch.direction,direction);
            const auto product=multiply(direction,direction),sum=add_scalar(lane<0xaau>(product),add_scalar(lane<0x55u>(product),product));
            m.word(0x31359cu,scratch.scalar,scalar_bits(sum));
            if(!above_or_unordered(m.word(scratch.scalar),0x46a180u)){
                m.word(0x3135b1u,scratch.direction,0);m.word(0x3135b9u,scratch.direction+4u,0x3f800000u);
                m.word(0x3135c1u,scratch.direction+8u,0);m.word(0x3135c9u,scratch.direction+12u,0);
            }
        }
        native_tu15_solver_hull_support_pair(memory,context,scratch.direction,scratch.point_pair,
            scratch.transformed_direction,fp,depth,services,observe,stats);
        native_tu15_append_solver_hull_vertex(memory,context,scratch.point_pair,fp,depth,observe,stats);
        const auto differences=m.word(context+0x50u);
        {
            Environment phase(fp,depth);
            const auto first=m.vector(differences),product=multiply(first,first);
            const auto sum=add_scalar(lane<0xaau>(product),add_scalar(lane<0x55u>(product),product));
            m.word(0x313611u,scratch.scalar,scalar_bits(sum));
            if(!above_or_unordered(m.word(scratch.scalar),0x46a454u)){
                const auto current=m.vector(differences),squared=multiply(current,current);
                auto square=add_scalar(lane<0xaau>(squared),add_scalar(lane<0x55u>(squared),squared));
                m.vector(0x313648u,scratch.squared_vector,square);
                asm volatile("sqrtss %0,%0":"+x"(square)::"memory");
                m.word(0x313651u,scratch.squared_vector,scalar_bits(square));
                const auto rounded=m.vector(scratch.squared_vector);m.word(0x313660u,scratch.scalar,scalar_bits(rounded));
                push(m.word(scratch.scalar));const auto direction=m.vector(scratch.direction);
                m.pop_float(0x31366du,output_plane+0x20u);m.vector(0x313670u,output_plane,direction);
                const auto first_point=m.vector(scratch.point_pair);m.vector(0x313678u,output_plane+0x10u,first_point);
                m.word(0x31367cu,output_plane+0x24u,0x3f000000u);hull_byte(memory,0x313686u,output_flag,1,observe);
            }else hull_byte(memory,0x313693u,output_flag,0,observe);
        }
        if(stats)++stats->initialize_completed;return output_flag;
#else
        (void)memory;(void)context;(void)output_flag;(void)output_plane;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native solver hull backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
