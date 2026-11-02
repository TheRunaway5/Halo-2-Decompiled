#include "halo2/native_tu15_constraint_angle.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(SourcePhysicsContactMemory&memory,std::uint32_t p){
    std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
}
void native_tu15_constraint_angle_active(SourcePhysicsContactMemory&memory,
    std::uint32_t first,std::uint32_t second,unsigned depth,NativeTu15ConstraintAngleStats*stats){
    if(stats)++stats->calls;
    if(depth>5u){if(stats)++stats->unsupported_depth;
        throw std::runtime_error("Native TU1.5 constraint angle requires its x87 capacity owner");}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t comparison;
    asm volatile("flds %1;fabs;flds %2;fabs;fld %%st(1);fcomp %%st(1);fnstsw %%ax"
        :"=a"(comparison):"m"(first),"m"(second):"memory","st","st(1)","st(2)");
    // Source TEST AH,41/JP selects the complementary polynomial for greater
    // or unordered absolute operands. Equality and less use the first form.
    const auto order=comparison&0x4100u;
    const bool complement=order==0u||order==0x4100u;
    if(complement){asm volatile("fxch %%st(1)":::"memory","st","st(1)");if(stats)++stats->complemented;}
    const auto epsilon=word(memory,0x4485ccu);
    asm volatile("fadds %0;fdivrp %%st,%%st(1);fld %%st(0);fmul %%st(1)"
        ::"m"(epsilon):"memory","st","st(1)");
    const auto linear=word(memory,0x4485d8u);
    asm volatile("flds %0;fmul %%st(1);fsubr %%st(2);fxch %%st(1);fmul %%st(2)"
        ::"m"(linear):"memory","st","st(1)","st(2)");
    const auto cubic=word(memory,0x4485d4u);
    asm volatile("fmuls %0;fsubrp %%st,%%st(1)"::"m"(cubic):"memory","st","st(1)");
    if(complement){const auto half_turn=word(memory,0x41c4ccu);asm volatile("fsubrs %0"::"m"(half_turn):"memory","st");}
    asm volatile("fstp %%st(1);flds %0"::"m"(second):"memory","st","st(1)");
    const auto zero_second=word(memory,0x46a0e8u);
    asm volatile("fcomps %1;fnstsw %%ax":"=a"(comparison):"m"(zero_second):"memory","st");
    // TEST AH,5/JP skips when C0 and C2 have even parity (ordered nonnegative
    // or unordered); exactly one set bit is the source negative branch.
    const auto second_order=comparison&0x0500u;
    if(second_order==0x0100u||second_order==0x0400u){
        const auto turn=word(memory,0x4485d0u);asm volatile("fsubrs %0"::"m"(turn):"memory","st");
        if(stats)++stats->negative_second;
    }
    asm volatile("flds %0"::"m"(first):"memory","st");
    const auto zero_first=word(memory,0x46a0e8u);
    asm volatile("fcomps %1;fnstsw %%ax":"=a"(comparison):"m"(zero_first):"memory","st");
    const auto first_order=comparison&0x0500u;
    if(first_order==0x0100u||first_order==0x0400u){asm volatile("fchs":::"memory","st");if(stats)++stats->negative_first;}
    if(stats)++stats->completed;
#else
    (void)memory;(void)first;(void)second;
    throw std::runtime_error("Exact native TU1.5 constraint angle backend unavailable");
#endif
}
} // namespace halo2
