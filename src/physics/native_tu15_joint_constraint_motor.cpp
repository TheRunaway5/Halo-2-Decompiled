#include "halo2/native_tu15_joint_constraint_motor.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using namespace joint_constraint_numeric;
struct MemoryMotor {
    Memory&m;std::uint32_t identity;
    Vector axis()const{return m.vector(identity);}
    std::uint32_t parameter(unsigned lane)const{return m.word(identity+16u+lane*4u);}
};
struct HostMotor {
    const NativeTu15JointConstraintMotor&value;
    Vector axis()const{Vector out;std::memcpy(&out,value.axis.data(),16);return out;}
    std::uint32_t parameter(unsigned lane)const{return value.parameters[lane];}
};
template<class Motor>void emit(Memory&m,const Motor&motor,std::uint32_t input,std::uint32_t builder){
    const auto second=m.word(input+4u),first=m.word(input);
    const auto first_fixed=m.word(first+12u,1),output=m.word(builder),schema=m.word(builder+4u);
    auto axis=motor.axis();if(!first_fixed)axis=transform_axis(m,first,axis);
    m.vector(0x329157u,output,axis);
    if(m.word(second+12u,1)){
        const auto mask=m.vector(0x461960u);axis=motor.axis();axis=xor_bits(axis,mask);
        m.vector(0x32916du,output+16u,axis);
    }else{
        const auto mask=m.vector(0x461960u);axis=motor.axis();
        auto last=m.vector(second+0x70u),middle=m.vector(second+0x60u);axis=xor_bits(axis,mask);
        last=multiply(last,lane<0xaau>(axis));middle=multiply(middle,lane<0x55u>(axis));
        auto leading=m.vector(second+0x50u);leading=multiply(leading,lane<0u>(axis));
        leading=add(leading,middle);axis=add(leading,last);m.vector(0x3291b0u,output+16u,axis);
    }
    inverse_mass(m,first,second,output);m.pop_float(0x329212u,output+12u);
    push(m.word(input+12u));asm volatile("fchs;fld %%st(0)":::"memory","st","st(1)");
    multiply_scalar(motor.parameter(0));asm volatile("fld %%st(1)":::"memory","st");
    multiply_scalar(motor.parameter(1));asm volatile("fxch %%st(1)":::"memory","st","st(1)");
    m.pop_float(0x329226u,schema+4u);m.word(0x329229u,schema,0x4100au);
    m.word(0x329230u,schema+12u,0x3f000000u);m.pop_float(0x329237u,schema+8u);
    m.word(0x32923au,builder+4u,schema+16u);m.word(0x329240u,builder,output+32u);
    multiply_scalar(motor.parameter(2));m.pop_float(0x329247u,output+28u);
    push(m.word(output+12u));multiply_scalar(m.word(0x46a0ccu));m.pop_float(0x329254u,output+12u);
}
}
#endif
void native_tu15_emit_joint_constraint_motor(SourcePhysicsContactMemory&memory,
    std::uint32_t motor,NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket builder,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);joint_constraint_numeric::Memory m(memory,observe);
    emit(m,MemoryMotor{m,motor},input.identity,builder.identity);
#else
    (void)memory;(void)motor;(void)input;(void)builder;(void)fp;(void)depth;(void)observe;
    throw std::runtime_error("Exact native TU1.5 joint motor backend unavailable");
#endif
}
void native_tu15_emit_joint_constraint_motor_active(SourcePhysicsContactMemory&memory,
    const NativeTu15JointConstraintMotor&motor,NativeTu15ConstraintProducerInput input,
    NativeTu15ConstraintSchemaPacket builder,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(depth>5u)throw std::runtime_error("Native joint motor requires its x87 capacity owner");
    joint_constraint_numeric::Memory m(memory,observe);emit(m,HostMotor{motor},input.identity,builder.identity);
#else
    (void)memory;(void)motor;(void)input;(void)builder;(void)depth;(void)observe;
    throw std::runtime_error("Exact native TU1.5 joint motor backend unavailable");
#endif
}
} // namespace halo2
