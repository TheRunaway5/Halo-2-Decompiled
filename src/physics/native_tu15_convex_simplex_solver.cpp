#include "halo2/native_tu15_convex_simplex_solver.hpp"
#include "halo2/native_tu15_convex_simplex_input_observer.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
void narrow_store(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t bits,unsigned n){
    std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(bits>>(i*8u));m.write(p,std::span(b).first(n));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using namespace joint_constraint_numeric;
Vector subtract(Vector a,Vector b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
Vector multiply_low(Vector a,Vector b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Vector scalar(std::uint32_t bits){Vector v{};std::memcpy(&v,&bits,4);return v;}
#endif
}
std::uint32_t native_tu15_solve_convex_contact_simplex(NativeTu15ConvexSimplexMemory&memory,
    NativeTu15ConvexSimplexSolverInput input,NativeTu15ConvexSimplexSolverScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ConvexSimplexSolverServices&services,
    const std::function<std::uint64_t()>&clock,NativeTu15ConvexSimplexSolverStats*activity){
    NativeTu15ConvexSimplexSolverStats local;auto&stats=activity?*activity:local;++stats.attempts;
    if(const auto&observer=native_tu15_convex_simplex_input_observer())observer(memory,input,fp,depth);
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth)throw std::runtime_error("Native simplex solver requires empty x87 DATA");
        if((input.work|input.relative_transform|input.plane|scratch.normalization)&15u)
            throw std::runtime_error("Native simplex solver requires aligned owned DATA");
        memory.withhold(input.work,12u);memory.withhold(input.work+0xdu,0x133u);
        memory.withhold(input.plane,16u);
        const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
        const auto first_count=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(input.support_cache+8u,1)))));
        m.word(0x30e944u,input.work,first_count);
        const auto second_count=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(input.support_cache+9u,1)))));
        m.word(0x30e954u,input.work+4u,second_count);
        const auto first_now=m.word(input.work);
        const auto first_table=m.word(input.first.identity);
        m.word(0x30e95du,scratch.epsilon,0u);
        if(first_count!=1u||second_count!=1u){++stats.initial_count_refusals;throw std::runtime_error("Native simplex solver requires its wider initial simplex");}
        if(!services.features)throw std::runtime_error("Native simplex solver requires native feature retrieval");
        const auto first_owner=m.word(first_table+0x34u);
        ++stats.feature_queries;services.features(memory,input.first,{first_owner},input.support_cache,first_now,input.work+0x20u,fp);
        // The table is loaded before current counts, but the virtual owner
        // operand is read at CALL after those source-current count loads.
        const auto second_table=m.word(input.second.identity);
        const auto current_second=m.word(input.work+4u),current_first=m.word(input.work);
        const auto second_owner=m.word(second_table+0x34u);
        ++stats.feature_queries;services.features(memory,input.second,{second_owner},input.support_cache+current_first*2u,current_second,input.work+0xe0u,fp);
        {
            Environment phase(fp,depth);
            const auto count=m.word(input.work+4u);
            if(count!=1u){++stats.initial_count_refusals;throw std::runtime_error("Native simplex solver requires native transformed-row continuation");}
            const auto point=m.vector(input.work+0xe0u),x=m.vector(input.relative_transform),y=m.vector(input.relative_transform+0x10u);
            const auto x_term=multiply(x,lane<0u>(point)),y_term=multiply(y,lane<0x55u>(point));
            const auto z=m.vector(input.relative_transform+0x20u),z_term=multiply(z,lane<0xaau>(point));
            const auto translation=m.vector(input.relative_transform+0x30u),zt=add(z_term,translation),xy=add(x_term,y_term);
            m.vector(0x30e9e9u,input.work+0xa0u,add(xy,zt));
            // The source's first iteration starts at a source-owned zero scalar.
            const auto a=m.word(input.work);push(m.word(scratch.epsilon));
            const auto b=m.word(input.work+4u);asm volatile("fadd %%st(0),%%st(0)":::"memory","st");
            m.pop_float(0x30ea13u,scratch.epsilon);
            m.word(0x30ea22u,input.work+0x10u,0u);
            if(a!=1u||b!=1u){++stats.initial_count_refusals;throw std::runtime_error("Native simplex solver requires its simplex switch continuation");}
            // Exact vertex/vertex case30F253, not an emulated jump table.
            const auto opposite=m.vector(input.work+0xa0u),first=m.vector(input.work+0x20u);
            m.vector(0x30f261u,input.work+0x120u,subtract(opposite,first));
            m.word(0x30f26au,input.work+8u,m.word(input.work+4u));
            const auto axis=m.vector(input.work+0x120u),squared=multiply(axis,axis);
            const auto squared_length=sum_three(squared);
            const auto first_now=m.vector(input.work+0x20u);
            const auto axis_now=m.vector(input.work+0x120u);
            m.word(0x30ee08u,scratch.squared_length,squared_length);
            const auto second_now=m.vector(input.work+0xa0u),product=multiply(subtract(second_now,first_now),axis_now);
            m.word(0x30ee36u,scratch.projection,sum_three(product));
            push(m.word(scratch.projection));multiply_scalar(m.word(scratch.projection));multiply_scalar(m.word(scratch.projection));
            push(m.word(scratch.epsilon));multiply_scalar(m.word(0x41d658u));multiply_scalar(m.word(scratch.squared_length));
            std::uint16_t relation;asm volatile("fcompp;fnstsw %0":"=a"(relation)::"memory","st","st(1)");
            if(relation&0x100u){++stats.projection_refusals;throw std::runtime_error("Native simplex solver requires its projection iteration");}
        }
        std::uint32_t result;
        if(m.word(input.work+0xcu,1u)) {
            // Actual30F5A5..30F5CF: overlap queries keep the current simplex,
            // cap its two signed counts only when their sum exceeds four,
            // and return3. They do not call the30DE60 distance reduction.
            const auto second_count=m.word(input.work+4u),first_count=m.word(input.work);
            if(std::bit_cast<std::int32_t>(first_count+second_count)>4) {
                if(std::bit_cast<std::int32_t>(first_count)>
                    std::bit_cast<std::int32_t>(second_count)) {
                    m.word(0x30f5b5u,input.work,3u);
                    m.word(0x30f5bbu,input.work+4u,1u);
                } else {
                    m.word(0x30f5c3u,input.work,1u);
                    m.word(0x30f5c9u,input.work+4u,3u);
                }
            }
            result=3u;
        } else {
            result=native_tu15_solve_single_vertex_convex_simplex(memory,
                {input.work,input.first,input.second,input.relative_transform},scratch.single,
                fp,depth,services.single,clock,&stats.single);
        }
        {
            Environment phase(fp,depth);
            const auto first=m.vector(input.work+0x20u),second=m.vector(input.work+0xa0u),axis=m.vector(input.work+0x120u);
            const auto difference=subtract(second,first),axis_now=m.vector(input.work+0x120u),squared=multiply(axis_now,axis_now);
            const auto xy=add_scalar(lane<0x55u>(squared),squared),length=add_scalar(lane<0xaau>(squared),xy);
            m.vector(0x30f60du,scratch.normalization,length);
            auto inverse=xy;asm volatile("rsqrtss %1,%0":"+x"(inverse):"x"(length):"memory");
            m.word(0x30f616u,scratch.normalization,scalar_bits(inverse));inverse=m.vector(scratch.normalization);
            auto correction=multiply_low(multiply_low(length,inverse),inverse);
            m.word(0x30f629u,scratch.constant,0x40400000u);
            auto three=scalar(m.word(scratch.constant));asm volatile("subss %1,%0":"+x"(three):"x"(correction):"memory");
            m.word(0x30f63bu,scratch.constant,0x3f000000u);
            auto reciprocal=multiply_low(scalar(m.word(scratch.constant)),inverse);
            const auto projection=multiply(difference,axis);reciprocal=multiply_low(reciprocal,three);
            m.vector(0x30f65eu,input.plane,multiply(lane<0u>(reciprocal),axis));
            const auto distance=multiply_low(scalar(sum_three(projection)),reciprocal);
            m.word(0x30f67fu,scratch.constant,scalar_bits(distance));
            m.word(0x30f68au,input.plane+0xcu,m.word(scratch.constant));
        }
        narrow_store(memory,input.support_cache+8u,m.word(input.work,1u),1u);
        narrow_store(memory,input.support_cache+9u,m.word(input.work+4u,1u),1u);
        unsigned written{};do{narrow_store(memory,input.support_cache+written*2u,m.word(input.work+0x2cu+written*16u,2u),2u);++written;}
            while(std::int32_t(written)<std::bit_cast<std::int32_t>(m.word(input.work)));
        unsigned second_written{};auto target=input.support_cache+written*2u;
        do{narrow_store(memory,target,m.word(input.work+0xecu+second_written*16u,2u),2u);target+=2u;++second_written;}
            while(std::int32_t(second_written)<std::bit_cast<std::int32_t>(m.word(input.work+4u)));
        ++stats.completed;return result;
#else
        (void)memory;(void)input;(void)scratch;(void)fp;(void)depth;(void)services;(void)clock;
        throw std::runtime_error("Exact native simplex solver backend unavailable");
#endif
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
