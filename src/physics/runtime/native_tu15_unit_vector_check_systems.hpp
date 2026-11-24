#pragma once
// Native TU1.5 unit vector check, entered at B1720 (physics live support and
// AI frame units): whether a vector is unit length and agrees with another.
//
//   unit_vector_check::check   B1720(ESI = vector, EDI = other) -> EAX
//
// The vector's squared length minus 1.0 (46A0D0) must be finite (its
// exponent not all ones) and, as x87 |d|, below the tolerance at 46A180
// (FCOMIP; equal or unordered fails). Then AA200 must accept the other
// vector and AA270 the dot product (z, then y, then x terms, in the
// original's operand order) with the argument 0. The result is 1, or 0
// when any test fails.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_unit_vector_check.hpp) maps the role to the original
// instruction and owns the ABI of the children (AA200, AA270).
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>
#include <stdexcept>

namespace halo2::native_unit_vector_check {
namespace sse=native_aim_clusters::sse;

namespace globals {
inline constexpr std::uint32_t one=0x46a0d0u;         // float 1.0
inline constexpr std::uint32_t tolerance=0x46a180u;   // float
}
inline constexpr std::uint32_t exponent_mask=0x7f800000u;

namespace x87 {
// FLD m32; FABS; FLD m32 (the tolerance); FCOMIP ST1; FSTP ST0: whether the
// tolerance is above |d| (CF and ZF clear).
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
inline bool tolerance_above(std::uint32_t d,std::uint32_t tolerance) {
    unsigned char carry,zero;
    asm volatile("flds %2\n\tfabs\n\tflds %3\n\tfcomip %%st(1),%%st\n\tsetb %0\n\tsete %1\n\tfstp %%st(0)"
        :"=qm"(carry),"=qm"(zero):"m"(d),"m"(tolerance):"cc","st","st(1)");
    return !carry && !zero;
}
#else
inline constexpr bool available=false;
inline bool tolerance_above(std::uint32_t,std::uint32_t){throw std::runtime_error("Required TU1.5 FCOMIP backend unavailable");}
#endif
} // namespace x87

namespace unit_vector_check {
enum class Local {error,error_copy};
enum class Access {
    vy,vz,vx,one,error_store,error_bits,error_copy_store,error_for_x87,tolerance,
    other_z,vz_again,other_y,vy_again,vx_again,other_x
};
// The squared length's registers (XMM0-XMM3) and the dot product's.
struct Length {std::uint32_t error,z,zz,yy;};
struct Dot {std::uint32_t dot,x_term;};
// The vectors the original keeps in callee-saved registers across AA200.
struct Kept {std::uint32_t vector,other;};   // ESI, EDI
// Frame:
//   local(Local)            a local's address
//   measured(Length)        the squared length ran
//   compared(d, tolerance)  the x87 comparison ran
//   other_valid(kept)       AA200 for the other vector; returns AL; kept as
//                           AA200 returned it
//   dotted(Dot)             the dot product ran
//   agrees()                AA270 with the dot product; returns AL
template<class Memory,class Frame> bool check(Memory &memory,Frame &frame,std::uint32_t vector,std::uint32_t other) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto y=memory.load32(vy,vector+4u);
    const auto z=memory.load32(vz,vector+8u);
    const auto x=memory.load32(vx,vector);
    Length l{};
    l.z=z;l.yy=sse::multiply(y,y);
    auto sum=sse::add(sse::multiply(x,x),l.yy);
    l.zz=sse::multiply(z,z);
    sum=sse::add(sum,l.zz);
    l.error=sse::subtract(sum,memory.load32(one,globals::one));
    frame.measured(l);
    memory.store32(error_store,local(Local::error),l.error);
    const auto bits=memory.load32(error_bits,local(Local::error));
    memory.store32(error_copy_store,local(Local::error_copy),l.error);
    if((bits&exponent_mask)==exponent_mask)return false;
    const auto d=memory.load32(error_for_x87,local(Local::error_copy));
    const auto limit=memory.load32(tolerance,globals::tolerance);
    frame.compared(d,limit);
    if(!x87::tolerance_above(d,limit))return false;
    Kept kept{vector,other};
    if(!(frame.other_valid(kept)&0xffu))return false;
    vector=kept.vector;other=kept.other;
    Dot product{};
    const auto ez=memory.load32(other_z,other+8u);
    const auto z_term=sse::multiply(ez,memory.load32(vz_again,vector+8u));
    const auto ey=memory.load32(other_y,other+4u);
    const auto y_term=sse::multiply(ey,memory.load32(vy_again,vector+4u));
    const auto partial=sse::add(z_term,y_term);
    const auto vx_value=memory.load32(vx_again,vector);
    product.x_term=sse::multiply(vx_value,memory.load32(other_x,other));
    product.dot=sse::add(partial,product.x_term);
    frame.dotted(product);
    return (frame.agrees()&0xffu)!=0;
}
} // namespace unit_vector_check
} // namespace halo2::native_unit_vector_check
