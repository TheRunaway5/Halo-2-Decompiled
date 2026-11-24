#pragma once
// Native TU1.5 perpendicular, entered at 11FA50 (physics live support
// unit): a vector perpendicular to the given one.
//
//   perpendicular::of   11FA50(ECX = vector, EDX = result) -> EAX = result
//
// The vector's horizontal perpendicular (y, -x, 0) is normalized by the
// horizontal length, which the original takes on the x87 (|(x, y)|, the
// components and the length spilled to floats). A length within the
// epsilon ([46A0EC]) of zero, or exactly zero, leaves no horizontal
// direction: the default axis ([474D08]) stands in. The result is that
// direction crossed with the vector (direction x vector), so it is
// perpendicular to both.
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order (operands as the original's destination and source, since the
// first NaN operand's payload wins), on the live MXCSR; the x87 steps are
// the adapter's (frame hooks). This is the game logic only. The game's
// memory is the retained TU1.5 bank, reached through a Memory object with
// typed loads and stores; every access names its role (Access), and the
// boundary adapter (native_tu15_perpendicular.hpp) maps the role to the
// original instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_perpendicular {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t default_axis=0x474d08u;   // Vector*: the fallback direction
inline constexpr std::uint32_t one=0x46a0d0u;            // float 1
inline constexpr std::uint32_t epsilon=0x46a0ecu;        // float: the shortest usable length
}
inline constexpr std::uint32_t zero_bits=0u;
struct Vector {std::uint32_t x,y,z;};

// UCOMISS a,b; LAHF; TEST AH,44h; JP: a and b unequal, or unordered.
inline bool unequal(std::uint32_t a,std::uint32_t b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char zero,parity;
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetp %1":"=qm"(zero),"=qm"(parity):"x"(sse::scalar(a)),"x"(sse::scalar(b)):"cc");
    return !zero || parity;
#else
    return !_mm_ucomieq_ss(sse::scalar(a),sse::scalar(b));
#endif
}

namespace perpendicular {
enum class Local {length,y,negated_x,spare};   // the frame's four floats
enum class Access {
    x_read,y_read,y_store,negated_store,negated_square,y_load,y_square,length_store,epsilon,
    length_read,one,scale_y,scale_x,
    axis_global,axis_x,axis_x_store,axis_y,axis_z,axis_x_load,axis_y_store,axis_y_load,axis_z_store,axis_z_load,
    cross_x,cross_z,cross_y,cross_x_again,cross_z_again,cross_y_again,result_x,result_y,result_z
};
// Frame (the x87 and the registers):
//   negate()                  FCHS
//   spill() -> bits           FST m32 of ST0
//   length()                  FADDP; FSQRT
//   magnitude()               FABS
//   within_epsilon() -> bool  FCOMIP; FSTP ST0; JA: the epsilon above the length
//   save_esi() / restore_esi()
//   crossed(Cross)            the cross product's registers
//   local(Local)
struct Cross {std::uint32_t xmm0,xmm1,xmm2,xmm3,xmm4,xmm5;};
enum class Direction {horizontal,short_length,zero_length};
template<class Memory,class Frame> Direction of(Memory &memory,Frame &frame,std::uint32_t vector,std::uint32_t result) {
    using enum Access;
    // |(x, y)| on the x87, -x and y kept as floats, the length spilled.
    (void)memory.load32(x_read,at(vector,0u));
    const auto y=memory.load32(y_read,at(vector,4u));
    frame.negate();
    memory.store32(y_store,frame.local(Local::y),y);
    memory.store32(negated_store,frame.local(Local::negated_x),frame.spill());
    (void)memory.load32(negated_square,frame.local(Local::negated_x));
    (void)memory.load32(y_load,frame.local(Local::y));
    (void)memory.load32(y_square,frame.local(Local::y));
    frame.length();
    memory.store32(length_store,frame.local(Local::length),frame.spill());
    frame.magnitude();
    (void)memory.load32(epsilon,globals::epsilon);
    auto direction=frame.within_epsilon()?Direction::short_length:Direction::horizontal;
    Vector p{};
    if(direction==Direction::horizontal) {
        // The horizontal perpendicular, scaled by 1 / length.
        const auto length=memory.load32(length_read,frame.local(Local::length));
        const auto one_value=memory.load32(one,globals::one);
        const bool nonzero=unequal(length,zero_bits);
        const auto inverse=sse::divide(one_value,length);
        p.x=sse::multiply(inverse,memory.load32(scale_y,frame.local(Local::y)));
        p.y=sse::multiply(inverse,memory.load32(scale_x,frame.local(Local::negated_x)));
        p.z=sse::multiply(inverse,zero_bits);
        if(!nonzero)direction=Direction::zero_length;
    }
    if(direction!=Direction::horizontal) {
        // The default axis, copied through the frame's floats as the original does.
        const auto axis=memory.load32(axis_global,globals::default_axis);
        frame.save_esi();
        memory.store32(axis_x_store,frame.local(Local::y),memory.load32(axis_x,at(axis,0u)));
        const auto axis_y_value=memory.load32(axis_y,at(axis,4u));
        const auto axis_z_value=memory.load32(axis_z,at(axis,8u));
        p.x=memory.load32(axis_x_load,frame.local(Local::y));
        memory.store32(axis_y_store,frame.local(Local::negated_x),axis_y_value);
        p.y=memory.load32(axis_y_load,frame.local(Local::negated_x));
        memory.store32(axis_z_store,frame.local(Local::spare),axis_z_value);
        p.z=memory.load32(axis_z_load,frame.local(Local::spare));
        frame.restore_esi();
    }
    // direction x vector, in the original's order.
    const auto vector_x=memory.load32(cross_x,at(vector,0u));
    const auto vector_z=memory.load32(cross_z,at(vector,8u));
    const auto x_y=sse::multiply(p.x,memory.load32(cross_y,at(vector,4u)));
    const auto result_z_value=sse::subtract(x_y,sse::multiply(vector_x,p.y));
    const auto vector_x_again=memory.load32(cross_x_again,at(vector,0u));
    const auto z_x=sse::multiply(vector_z,p.x);
    const auto vector_z_again=memory.load32(cross_z_again,at(vector,8u));
    const auto x_z=sse::multiply(vector_x_again,p.z);
    const auto z_y=sse::multiply(p.z,memory.load32(cross_y_again,at(vector,4u)));
    const auto y_z=sse::multiply(vector_z_again,p.y);
    const auto result_y_value=sse::subtract(x_z,z_x);
    const auto result_x_value=sse::subtract(y_z,z_y);
    memory.store32(result_x,at(result,0u),result_x_value);
    memory.store32(result_y,at(result,4u),result_y_value);
    memory.store32(result_z,at(result,8u),result_z_value);
    frame.crossed({z_y,result_z_value,p.y,result_x_value,result_y_value,z_x});
    return direction;
}
} // namespace perpendicular
} // namespace halo2::native_perpendicular
