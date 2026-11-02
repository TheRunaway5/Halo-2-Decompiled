#include "halo2/native_tu15_joint_constraint_angular_row.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using namespace joint_constraint_numeric;
struct MemoryRow {
    Memory&m;std::uint32_t identity;
    Vector vector(unsigned offset)const{return m.vector(identity+offset);}
    std::uint32_t word(unsigned offset)const{return m.word(identity+offset);}
};
struct HostRow {
    const NativeTu15JointConstraintAngularRow&value;
    const NativeTu15ConstraintVector4&words(unsigned offset)const{return offset<16u?value.first_axis:offset<32u?value.second_axis:value.axis;}
    Vector vector(unsigned offset)const{Vector result;const auto&v=words(offset);std::memcpy(&result,v.data(),16);return result;}
    std::uint32_t word(unsigned offset)const{return words(offset)[(offset&15u)/4u];}
};
template<class Row>void emit(Memory&m,const Row&row,std::uint32_t input,std::uint32_t builder){
    auto axis=row.vector(0x20u);const auto first=m.word(input);
    const auto first_fixed=m.word(first+0xcu,1);const auto output=m.word(builder);const auto second=m.word(input+4u);
    if(!first_fixed)axis=transform_axis(m,first,axis);
    m.vector(0x328904u,output,axis);
    if(m.word(second+0xcu,1)){
        const auto mask=m.vector(0x461960u);axis=row.vector(0x20u);axis=xor_bits(axis,mask);
        m.vector(0x32891bu,output+0x10u,axis);
    }else{
        const auto mask=m.vector(0x461960u);axis=row.vector(0x20u);
        auto last=m.vector(second+0x70u);auto middle=m.vector(second+0x60u);axis=xor_bits(axis,mask);
        last=multiply(last,lane<0xaau>(axis));middle=multiply(middle,lane<0x55u>(axis));
        auto leading=m.vector(second+0x50u);leading=multiply(leading,lane<0u>(axis));leading=add(leading,middle);axis=add(leading,last);
        m.vector(0x32895fu,output+0x10u,axis);
    }
    inverse_mass(m,first,second,output);m.pop_float(0x3289bfu,output+0xcu);
    push(row.word(0x18u));multiply_scalar(row.word(8u));push(row.word(0x14u));multiply_scalar(row.word(4u));
    asm volatile("faddp":::"memory","st","st(1)");push(row.word(0x10u));multiply_scalar(row.word(0u));
    const auto schema=m.word(builder+4u);asm volatile("faddp":::"memory","st","st(1)");multiply_scalar(m.word(input+0xcu));
    m.word(0x3289e0u,schema,0x4040cu);m.word(0x3289e7u,builder,output+0x20u);m.word(0x3289e9u,builder+4u,schema+4u);
    asm volatile("fchs":::"memory","st");m.pop_float(0x3289efu,output+0x1cu);
}
}
#endif
void native_tu15_emit_joint_constraint_angular_row(SourcePhysicsContactMemory&memory,
    std::uint32_t row,NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket builder,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);joint_constraint_numeric::Memory m(memory,observe);emit(m,MemoryRow{m,row},input.identity,builder.identity);
#else
    (void)memory;(void)row;(void)input;(void)builder;(void)fp;(void)depth;(void)observe;
    throw std::runtime_error("Exact native TU1.5 joint angular row backend unavailable");
#endif
}
void native_tu15_emit_joint_constraint_angular_row_active(SourcePhysicsContactMemory&memory,
    const NativeTu15JointConstraintAngularRow&row,NativeTu15ConstraintProducerInput input,
    NativeTu15ConstraintSchemaPacket builder,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(depth>5u)throw std::runtime_error("Native TU1.5 active joint angular row requires its x87 capacity owner");
    joint_constraint_numeric::Memory m(memory,observe);emit(m,HostRow{row},input.identity,builder.identity);
#else
    (void)memory;(void)row;(void)input;(void)builder;(void)depth;(void)observe;
    throw std::runtime_error("Exact native TU1.5 active joint angular row backend unavailable");
#endif
}
} // namespace halo2
