#include "halo2/native_tu15_joint_constraint_build.hpp"
#include "halo2/native_tu15_constraint_angle.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using namespace joint_constraint_numeric;
NativeTu15ConstraintVector4 words(Vector value){NativeTu15ConstraintVector4 out;std::memcpy(out.data(),&value,16);return out;}
Vector transform(Memory&m,std::uint32_t matrix,Vector value){
    auto last=m.vector(matrix+32u),middle=m.vector(matrix+16u);
    last=multiply(last,lane<0xaau>(value));middle=multiply(middle,lane<0x55u>(value));
    auto first=m.vector(matrix);first=multiply(first,lane<0u>(value));first=add(first,middle);return add(first,last);
}
std::uint16_t compare_pop(std::uint32_t operand){
    std::uint16_t status;asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(operand):"memory","st");return status;
}
}
#endif
void native_tu15_build_joint_constraint(SourcePhysicsContactMemory&memory,
    std::uint32_t constraint,NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket packet,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15JointConstraintBuildServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15JointConstraintBuildStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        using namespace joint_constraint_numeric;
        if(depth)throw std::runtime_error("Native joint parent requires qualified empty-x87 input");
        native_tu15_begin_joint_constraint_schemas(memory,input,packet,constraint+0x90u,8u,observe);if(stats)++stats->headers;
        Environment environment(fp,depth);Memory m(memory,observe);
        const auto first_body=m.word(constraint+12u),first_motion=m.word(first_body+0x3cu);
        std::array<Vector,4>first;
        for(unsigned row=0;row<4;++row){const auto local=m.vector(constraint+0x20u+row*16u);first[row]=transform(m,first_motion+0x80u,local);}
        const auto translated_first_motion=m.word(first_body+0x3cu);const auto first_position=m.vector(translated_first_motion+0xb0u);
        const auto second_body=m.word(constraint+16u);first[0]=add(first[0],first_position);
        const auto second_motion=m.word(second_body+0x3cu);std::array<Vector,3>second;
        for(unsigned row=0;row<3;++row){const auto local=m.vector(constraint+0x60u+row*16u);second[row]=transform(m,second_motion+0x80u,local);}
        const auto translated_second_body=m.word(constraint+16u),translated_second_motion=m.word(translated_second_body+0x3cu);
        const auto second_position=m.vector(translated_second_motion+0xb0u);second[0]=add(second[0],second_position);
        const NativeTu15JointConstraintAngularRow angular0{words(first[3]),words(second[1]),words(first[2])};
        native_tu15_emit_joint_constraint_angular_row_active(memory,angular0,input,packet,depth,observe);if(stats)++stats->angular_rows;
        const auto sign=m.vector(0x461960u);const auto reversed=xor_bits(second[1],sign);
        const NativeTu15JointConstraintAngularRow angular1{words(first[2]),words(reversed),words(first[3])};
        native_tu15_emit_joint_constraint_angular_row_active(memory,angular1,input,packet,depth,observe);if(stats)++stats->angular_rows;
        const auto parameter0=m.word(constraint+0xc8u),parameter1=m.word(constraint+0xccu);
        const auto cosine=sum_three(multiply(second[2],first[2]));
        const auto product0=multiply(lane<0xd2u>(second[2]),lane<0xc9u>(second[1]));
        const auto product1=multiply(lane<0xc9u>(second[2]),lane<0xd2u>(second[1]));
        Vector cross;asm volatile("subps %2,%0":"=x"(cross):"0"(product1),"x"(product0):"memory");
        const auto sine=sum_three(multiply(cross,first[2]));
        native_tu15_constraint_angle_active(memory,cosine,sine,depth);
        std::uint32_t angle;asm volatile("fsts %0":"=m"(angle)::"memory");
        push(m.word(constraint+0xd4u));asm volatile("fsub %%st(1),%%st":::"memory","st");
        auto status=compare_pop(m.word(0x41e90cu));bool changed{};
        if(!(status&0x4100u)){
            add_scalar(m.word(0x41c4c8u));changed=true;if(stats)++stats->positive_unwraps;
        }else{
            asm volatile("fld %%st(0)":::"memory","st");const auto previous=m.word(constraint+0xd4u);
            asm volatile("fsubs %0"::"m"(previous):"memory","st");status=compare_pop(m.word(0x41e90cu));
            if(!(status&0x4100u)){const auto turn=m.word(0x41c4c8u);asm volatile("fsubs %0"::"m"(turn):"memory","st");changed=true;if(stats)++stats->negative_unwraps;}
        }
        if(changed)asm volatile("fsts %0":"=m"(angle)::"memory");
        m.pop_float(0x31dba1u,constraint+0xd4u);
        const NativeTu15JointConstraintMotor motor{words(first[1]),{parameter0,parameter1,angle}};
        native_tu15_emit_joint_constraint_motor_active(memory,motor,input,packet,depth,observe);if(stats)++stats->motors;
        if(stats)++stats->linear_requests;
        if(!services.linear){if(stats)++stats->unbound_linear;throw std::runtime_error("Native joint parent requires3294E0 linear emitter");}
        services.linear(words(first[0]),words(second[0]),input,packet,depth);
        push(m.word(0x46a0e8u));push(m.word(constraint+0xd0u));
        asm volatile("fucompp;fnstsw %0":"=a"(status)::"memory","st","st(1)");
        if((std::popcount(unsigned((status>>8u)&0x44u))&1u)==0u){
            const auto coefficient=m.word(constraint+0xd0u);
            const NativeTu15JointConstraintLimit limit{words(first[1]),constraint+0xc0u,coefficient};
            native_tu15_emit_joint_constraint_limit_active(memory,limit,input,packet,depth,observe);if(stats)++stats->limits;
        }
        if(stats)++stats->completed;
#else
        (void)memory;(void)constraint;(void)input;(void)packet;(void)fp;(void)depth;(void)services;(void)observe;
        throw std::runtime_error("Exact native TU1.5 joint parent backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
