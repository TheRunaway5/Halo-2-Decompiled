#include "halo2/native_tu15_mopp_oriented_aabb_setup.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
std::uint32_t convert_loaded(joint_constraint_numeric::Memory&m,
    std::uint32_t code,unsigned coordinate){
    using namespace joint_constraint_numeric;const auto translation=m.word(code+coordinate);
    asm volatile("fsubs %0"::"m"(translation):"memory","st");multiply_scalar(m.word(code+12u));
    std::uint32_t rounded,integer;
    asm volatile("fstps %0;flds %0;fistpl %1":"=m"(rounded),"=m"(integer)::"memory","st");
    return integer-1u;
}
std::uint32_t high(joint_constraint_numeric::Memory&m,std::uint32_t p){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(m.word(p,2)))));}
std::array<std::uint32_t,4>words(joint_constraint_numeric::Vector v){std::array<std::uint32_t,4>out;std::memcpy(out.data(),&v,16);return out;}
#endif
}
void native_tu15_prepare_mopp_oriented_aabb(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t code,std::uint32_t transform,std::uint32_t half_extents,
    std::uint32_t tolerance,std::uint32_t key_array,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MoppAabbSetupServices&services,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(depth)throw std::runtime_error("Native oriented MOPP setup requires its nonempty x87 owner");
    NativeTu15MoppAabbVariables variables;
    {
        using namespace joint_constraint_numeric;Environment environment(fp,depth);Memory m(memory,observe);
        m.word(0x30a030u,cache,key_array);m.word(0x30a035u,cache+0x30u,code);
        const auto extent=m.vector(half_extents),mask=m.vector(0x461950u);
        auto z=multiply(lane<0xaa>(extent),m.vector(transform+0x20u));
        auto y=multiply(lane<0x55>(extent),m.vector(transform+0x10u));
        const auto basis_x=m.vector(transform);y=_mm_and_ps(y,mask);
        auto x=multiply(lane<0>(extent),basis_x);x=_mm_and_ps(x,mask);z=_mm_and_ps(z,mask);
        const auto sign=m.vector(0x461960u);const auto xy=add(x,y);auto expanded=add(xy,z);
        Vector scalar;const std::array<std::uint32_t,4>t{tolerance,tolerance,tolerance,tolerance};std::memcpy(&scalar,t.data(),16);
        expanded=add(expanded,scalar);const auto negative=xor_bits(expanded,sign),translation=m.vector(transform+0x30u);
        const auto minimum=words(add(negative,translation));
        push(minimum[0]);const auto origin=m.word(code);asm volatile("fsubs %0"::"m"(origin):"memory","st");
        const auto maximum=words(add(expanded,translation));multiply_scalar(m.word(code+12u));
        std::uint32_t rounded,integer;asm volatile("fstps %0;flds %0;fistpl %1":"=m"(rounded),"=m"(integer)::"memory","st");
        auto value=integer-1u;
        // Source preloads the following coordinate before each cache store.
        // Observers/faults at those stores must see that live native ST0.
        push(maximum[0]);m.word(0x30a0d3u,cache+0x20u,value);
        value=convert_loaded(m,m.word(cache+0x30u),0);
        push(minimum[1]);m.word(0x30a0f3u,cache+0x10u,value);
        value=convert_loaded(m,m.word(cache+0x30u),4);
        push(maximum[1]);m.word(0x30a114u,cache+0x24u,value);
        value=convert_loaded(m,m.word(cache+0x30u),4);
        push(minimum[2]);m.word(0x30a135u,cache+0x14u,value);
        value=convert_loaded(m,m.word(cache+0x30u),8);
        push(maximum[2]);m.word(0x30a156u,cache+0x28u,value);
        const auto upper_z=convert_loaded(m,m.word(cache+0x30u),8);
        variables.lower[0]=high(m,cache+0x22u);variables.upper[0]=high(m,cache+0x12u)+1u;
        variables.lower[1]=high(m,cache+0x26u);variables.upper[1]=high(m,cache+0x16u)+1u;
        m.word(0x30a190u,cache+0x18u,upper_z);variables.upper[2]=std::uint32_t(std::bit_cast<std::int32_t>(upper_z)>>16)+1u;
        variables.lower[2]=high(m,cache+0x2au);variables.translation_and_properties.fill(0);
    }
    if(!services.traverse)throw std::runtime_error("Native oriented MOPP setup requires309940 traversal owner");
    services.traverse(cache,code+0x30u,variables,fp,depth);
#else
    (void)memory;(void)cache;(void)code;(void)transform;(void)half_extents;(void)tolerance;(void)key_array;(void)fp;(void)depth;(void)services;(void)observe;
    throw std::runtime_error("Exact native oriented MOPP setup backend unavailable");
#endif
}
void native_tu15_query_mopp_oriented_shape(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t transform,std::uint32_t half_extents,
    std::uint32_t tolerance,std::uint32_t array,std::uint32_t cache,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15MoppAabbSetupServices&services,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Memory m(memory,observe);const auto code=m.word(shape+16u);
    native_tu15_prepare_mopp_oriented_aabb(memory,cache,code,transform,half_extents,tolerance,array,fp,depth,services,observe);
#else
    (void)memory;(void)shape;(void)transform;(void)half_extents;(void)tolerance;(void)array;(void)cache;(void)fp;(void)depth;(void)services;(void)observe;
    throw std::runtime_error("Exact native oriented MOPP shape backend unavailable");
#endif
}
} // namespace halo2
