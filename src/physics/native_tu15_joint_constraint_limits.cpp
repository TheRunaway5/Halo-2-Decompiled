#include "halo2/native_tu15_joint_constraint_limits.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using namespace joint_constraint_numeric;
struct MemoryLimits {
    Memory&m;std::uint32_t descriptor,axes;
    Vector axis(unsigned n)const{return m.vector(axes+n*16u);}
    std::uint32_t coefficient()const{return m.word(descriptor+8u);}
};
struct HostLimit {
    const NativeTu15JointConstraintLimit&value;
    Vector axis(unsigned)const{Vector out;std::memcpy(&out,value.axis.data(),16);return out;}
    std::uint32_t coefficient()const{return value.coefficient;}
};
void denominator(Memory&m,std::uint32_t first,std::uint32_t second,std::uint32_t output){
    const auto b=m.vector(output+16u),a=m.vector(output),mass_b=m.vector(second+0x30u);
    auto bb=multiply(b,b);const auto mass_a=m.vector(first+0x30u);bb=multiply(bb,mass_b);
    auto aa=multiply(a,a);aa=multiply(aa,mass_a);const auto total=add(aa,bb);push(sum_three(total));
    add_scalar(m.word(0x41f644u));const auto one=m.word(0x46a0d0u);asm volatile("fdivrs %0"::"m"(one):"memory","st");
}
template<class Limits>void emit(Memory&m,const Limits&limits,std::uint32_t values,std::uint32_t count,
    std::uint32_t input,std::uint32_t builder,std::uint32_t output,std::uint32_t schema){
    if(std::bit_cast<std::int32_t>(count-1u)<0){
        m.word(0x328b84u,builder,output);m.word(0x328b86u,builder+4u,schema);return;
    }
    auto value_cursor=values+4u;unsigned index{};
    do{
        const auto first=m.word(input),first_fixed=m.word(first+12u,1),second=m.word(input+4u);
        auto axis=limits.axis(index);if(!first_fixed)axis=transform_axis(m,first,axis);
        m.vector(0x328a7eu,output,axis);
        if(m.word(second+12u,1)){
            const auto mask=m.vector(0x461960u);axis=limits.axis(index);axis=xor_bits(axis,mask);
            m.vector(0x328a94u,output+16u,axis);
        }else{
            const auto mask=m.vector(0x461960u);axis=limits.axis(index);
            auto last=m.vector(second+0x70u),middle=m.vector(second+0x60u);axis=xor_bits(axis,mask);
            last=multiply(last,lane<0xaau>(axis));middle=multiply(middle,lane<0x55u>(axis));
            auto leading=m.vector(second+0x50u);leading=multiply(leading,lane<0u>(axis));
            leading=add(leading,middle);axis=add(leading,last);m.vector(0x328ad7u,output+16u,axis);
        }
        denominator(m,first,second,output);m.pop_float(0x328b3fu,output+12u);
        push(m.word(input+8u));multiply_scalar(limits.coefficient());
        m.word(0x328b4cu,schema,0x40809u);m.pop_float(0x328b56u,schema+4u);
        push(m.word(value_cursor));multiply_scalar(m.word(input+12u));m.pop_float(0x328b6cu,output+28u);
        output+=32u;schema+=8u;value_cursor+=8u;++index;
    }while(--count);
    m.word(0x328b78u,builder,output);m.word(0x328b7au,builder+4u,schema);
}
}
#endif
void native_tu15_emit_joint_constraint_limits(SourcePhysicsContactMemory&memory,
    std::uint32_t descriptor,NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket builder,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);joint_constraint_numeric::Memory m(memory,observe);
    const auto axes=m.word(descriptor),values=m.word(descriptor+4u),count=m.word(descriptor+12u);
    const auto output=m.word(builder.identity),schema=m.word(builder.identity+4u);
    emit(m,MemoryLimits{m,descriptor,axes},values,count,input.identity,builder.identity,output,schema);
#else
    (void)memory;(void)descriptor;(void)input;(void)builder;(void)fp;(void)depth;(void)observe;
    throw std::runtime_error("Exact native TU1.5 joint limits backend unavailable");
#endif
}
void native_tu15_emit_joint_constraint_limit_active(SourcePhysicsContactMemory&memory,
    const NativeTu15JointConstraintLimit&limit,NativeTu15ConstraintProducerInput input,
    NativeTu15ConstraintSchemaPacket builder,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(depth>5u)throw std::runtime_error("Native joint limit requires its x87 capacity owner");
    joint_constraint_numeric::Memory m(memory,observe);const auto output=m.word(builder.identity),schema=m.word(builder.identity+4u);
    emit(m,HostLimit{limit},limit.values,1u,input.identity,builder.identity,output,schema);
#else
    (void)memory;(void)limit;(void)input;(void)builder;(void)depth;(void)observe;
    throw std::runtime_error("Exact native TU1.5 joint limits backend unavailable");
#endif
}
} // namespace halo2
