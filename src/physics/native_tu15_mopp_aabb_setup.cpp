#include "halo2/native_tu15_mopp_aabb_setup.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
std::uint32_t quantized_bound(joint_constraint_numeric::Memory&m,
    std::uint32_t bounds,std::uint32_t coordinate,std::uint32_t code){
    using namespace joint_constraint_numeric;
    push(m.word(bounds+coordinate));
    const auto translation=m.word(code+(coordinate&15u));
    asm volatile("fsubs %0"::"m"(translation):"memory","st");
    multiply_scalar(m.word(code+12u));
    std::uint32_t rounded,integer;
    asm volatile("fstps %0;flds %0;fistpl %1":"=m"(rounded),"=m"(integer)::"memory","st");
    return integer-1u;
}
std::uint32_t signed_high(joint_constraint_numeric::Memory&m,std::uint32_t p){
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(m.word(p,2)))));
}
#endif
}
void native_tu15_prepare_mopp_aabb(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t code,std::uint32_t bounds,
    std::uint32_t key_array,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MoppAabbSetupServices&services,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(depth)throw std::runtime_error("Native MOPP AABB setup requires its nonempty x87 owner");
    NativeTu15MoppAabbVariables variables;
    {
        using namespace joint_constraint_numeric;
        Environment environment(fp,depth);Memory m(memory,observe);
        m.word(0x30a1efu,cache,key_array);m.word(0x30a1f4u,cache+0x30u,code);
        m.word(0x30a210u,cache+0x20u,quantized_bound(m,bounds,0,code));
        // Source loads each next bound before rereading cache+30. Keep this
        // ordering observable, including a failed bounds/cache operand read.
        const auto next=[&](std::uint32_t coordinate){
            push(m.word(bounds+coordinate));const auto current=m.word(cache+0x30u);
            const auto translation=m.word(current+(coordinate&15u));
            asm volatile("fsubs %0"::"m"(translation):"memory","st");
            multiply_scalar(m.word(current+12u));std::uint32_t rounded,integer;
            asm volatile("fstps %0;flds %0;fistpl %1":"=m"(rounded),"=m"(integer)::"memory","st");
            return integer-1u;
        };
        m.word(0x30a22fu,cache+0x10u,next(0x10u));
        m.word(0x30a24fu,cache+0x24u,next(4u));
        m.word(0x30a26fu,cache+0x14u,next(0x14u));
        m.word(0x30a28fu,cache+0x28u,next(8u));
        const auto upper_z=next(0x18u);
        variables.lower[0]=signed_high(m,cache+0x22u);
        variables.upper[0]=signed_high(m,cache+0x12u)+1u;
        variables.lower[1]=signed_high(m,cache+0x26u);
        variables.upper[1]=signed_high(m,cache+0x16u)+1u;
        m.word(0x30a2cdu,cache+0x18u,upper_z);
        variables.lower[2]=signed_high(m,cache+0x2au);
        variables.upper[2]=std::uint32_t(std::bit_cast<std::int32_t>(upper_z)>>16)+1u;
        //30A2EF..30A303 initialize all six translation/property words.
        variables.translation_and_properties.fill(0);
    }
    if(!services.traverse)throw std::runtime_error("Native MOPP AABB setup requires309940 traversal owner");
    services.traverse(cache,code+0x30u,variables,fp,depth);
#else
    (void)memory;(void)cache;(void)code;(void)bounds;(void)key_array;(void)fp;(void)depth;(void)services;(void)observe;
    throw std::runtime_error("Exact native MOPP AABB setup backend unavailable");
#endif
}
void native_tu15_query_mopp_aabb_shape(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t bounds,std::uint32_t key_array,
    std::uint32_t cache,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MoppAabbSetupServices&services,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Memory m(memory,observe);const auto code=m.word(shape+0x10u);
    native_tu15_prepare_mopp_aabb(memory,cache,code,bounds,key_array,fp,depth,services,observe);
#else
    (void)memory;(void)shape;(void)bounds;(void)key_array;(void)cache;(void)fp;(void)depth;(void)services;(void)observe;
    throw std::runtime_error("Exact native MOPP AABB shape backend unavailable");
#endif
}
} // namespace halo2
