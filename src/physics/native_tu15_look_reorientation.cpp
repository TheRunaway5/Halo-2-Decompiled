#include "halo2/native_tu15_look_reorientation.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
namespace halo2 {
void native_tu15_reorient_look_controller(SourcePhysicsContactMemory&memory,
    std::uint32_t index,std::uint32_t forward,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15LookReorientationStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth>4u)throw std::runtime_error("Native TU1.5 look reorientation requires its x87 capacity owner");
        using namespace joint_constraint_numeric;
        Environment environment(fp,depth);Memory m(memory,observe);
        push(m.word(forward+4u));const auto controls=m.word(0x4f9b54u);
        const auto record=controls+index*0x94u+0x14u;push(m.word(forward));
        asm volatile("fpatan":::"memory","st","st(1)");m.pop_float(0x18a2bau,record+0x10u);
        push(m.word(forward+4u));push(m.word(forward));push(m.word(forward+8u));
        asm volatile("fld %%st(2);fmulp %%st,%%st(3);fld %%st(1);fmul %%st(2),%%st;faddp %%st,%%st(3);"
            "fxch %%st(2);fsqrt;fxch %%st(2);fxch %%st(1);fxch %%st(2);fpatan;fstp %%st(1)"
            :::"memory","st","st(1)","st(2)","st(3)");
        m.pop_float(0x18a2ddu,record+0x14u);
        const auto scalar=[](std::uint32_t bits){Vector value{};std::memcpy(&value,&bits,4);return value;};
        const auto zero=_mm_setzero_ps(),yaw_value=scalar(m.word(record+0x10u));unsigned char negative;
        asm volatile("comiss %2,%1;seta %0":"=qm"(negative):"x"(zero),"x"(yaw_value):"cc","memory");
        if(negative){
            const auto current=scalar(m.word(record+0x10u)),turn=scalar(m.word(0x44923cu));
            m.word(0x18a2f3u,record+0x10u,scalar_bits(add_scalar(current,turn)));
            if(stats)++stats->yaw_wraps;
        }
        if(stats)++stats->completed;
#else
        (void)memory;(void)index;(void)forward;(void)fp;(void)depth;(void)observe;
        throw std::runtime_error("Exact native TU1.5 look reorientation backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
