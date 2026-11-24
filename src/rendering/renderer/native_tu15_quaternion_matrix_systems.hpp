#pragma once
// Native TU1.5 quaternion matrix, entered at 144910 (AI frame and scene
// device constructor units): a quaternion's 3x3 rotation matrix.
//
//   quaternion_matrix::build   144910(ECX = quaternion*, EAX = matrix*)
//
// The quaternion (x, y, z, w) scales by s = 2 / (((x*x + y*y) + z*z) + w*w),
// with 2 from [46A11C], or by 0 when that sum is at most [46A0EC] (COMISS).
// The nine elements, column by column, are 1 - (zz + yy), xy + zw, xz - yw;
// xy - zw, 1 - (zz + xx), yz + xw; xz + yw, yz - xw, 1 - (yy + xx), each
// product formed against the scaled component (xx = x * xs and so on), with
// 1 from [46A0D0]. The original keeps xx, zw and zz in a 12-byte frame and
// reads the quaternion's components again as it goes.
//
// The single-precision arithmetic is the original's scalar SSE, in its order
// (the first operand the destination), on the live MXCSR. This is the game
// logic only. The game's memory is the retained TU1.5 bank, reached through
// a Memory object with typed loads and stores; every access names its role
// (Access), including the original's frame; the boundary adapter
// (native_tu15_quaternion_matrix.hpp) maps the role to the original
// instruction and owns the ABI, XMM0 to XMM7 as the original leaves them.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <cstdint>

namespace halo2::native_quaternion_matrix {
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t least=0x46a0ecu,two=0x46a11cu,one=0x46a0d0u;}
namespace quaternion {inline constexpr std::uint32_t x=0u,y=4u,z=8u,w=0xcu;}
// The matrix's nine elements, as their offsets.
namespace matrix {inline constexpr std::uint32_t m00=0u,m01=4u,m02=8u,m10=0xcu,m11=0x10u,m12=0x14u,m20=0x18u,m21=0x1cu,m22=0x20u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t xx=-0xc,zw=-8,zz=-4;}
inline constexpr std::uint32_t zero_float=0u;

namespace quaternion_matrix {
enum class Access {component,least,two,one,local_store,local_load,element};
// The registers the original leaves: XMM0 to XMM7.
struct Registers {std::uint32_t xmm[8];};
// Frame: framing() SUB ESP,C; local(offset); compared(norm, least) COMISS; finished(registers) XMM0 to XMM7.
template<class Memory,class Frame> void build(Memory &memory,Frame &frame,std::uint32_t q,std::uint32_t out) {
    using enum Access;
    using sse::add;using sse::subtract;using sse::multiply;
    const auto y=memory.load32(component,q+quaternion::y);
    const auto z=memory.load32(component,q+quaternion::z);
    const auto w=memory.load32(component,q+quaternion::w);
    const auto x=memory.load32(component,q+quaternion::x);
    const auto norm=add(add(add(multiply(x,x),multiply(y,y)),multiply(z,z)),multiply(w,w));
    frame.framing();
    const auto least_value=memory.load32(least,globals::least);
    frame.compared(norm,least_value);
    const auto s=sse::not_above(norm,least_value)?zero_float:sse::divide(memory.load32(two,globals::two),norm);
    const auto x1=memory.load32(component,q+quaternion::x);
    const auto y1=memory.load32(component,q+quaternion::y);
    const auto y2=memory.load32(component,q+quaternion::y);
    const auto one_value=memory.load32(one,globals::one);
    const auto xs=multiply(x1,s);
    const auto ys=multiply(y1,s);
    const auto zs=multiply(s,memory.load32(component,q+quaternion::z));
    const auto zw=multiply(zs,memory.load32(component,q+quaternion::w));
    const auto xw=multiply(xs,memory.load32(component,q+quaternion::w));
    memory.store32(local_store,frame.local(local::zw),zw);
    const auto xx=multiply(memory.load32(component,q+quaternion::x),xs);
    const auto xy=multiply(memory.load32(component,q+quaternion::x),ys);
    memory.store32(local_store,frame.local(local::xx),xx);
    const auto xz=multiply(memory.load32(component,q+quaternion::x),zs);
    const auto yy=multiply(y2,ys);
    const auto y3=memory.load32(component,q+quaternion::y);
    const auto yw=multiply(ys,memory.load32(component,q+quaternion::w));
    const auto yz=multiply(y3,zs);
    const auto zz=multiply(zs,memory.load32(component,q+quaternion::z));
    memory.store32(local_store,frame.local(local::zz),zz);
    const auto zz_yy=add(zz,yy);
    const auto yy_xx=add(yy,memory.load32(local_load,frame.local(local::xx)));
    const auto zw_again=memory.load32(local_load,frame.local(local::zw));
    memory.store32(element,out+matrix::m00,subtract(one_value,zz_yy));
    const auto one_again=memory.load32(one,globals::one);
    memory.store32(element,out+matrix::m10,subtract(xy,zw_again));
    const auto m01=add(xy,zw_again);
    memory.store32(element,out+matrix::m01,m01);
    const auto zz_again=memory.load32(local_load,frame.local(local::zz));
    const auto zz_xx=add(zz_again,memory.load32(local_load,frame.local(local::xx)));
    memory.store32(element,out+matrix::m20,add(xz,yw));
    const auto m11=subtract(one_again,zz_xx);
    const auto m21=subtract(yz,xw);
    const auto m02=subtract(xz,yw);
    const auto m12=add(yz,xw);
    const auto m22=subtract(one_again,yy_xx);
    memory.store32(element,out+matrix::m11,m11);
    memory.store32(element,out+matrix::m21,m21);
    memory.store32(element,out+matrix::m02,m02);
    memory.store32(element,out+matrix::m12,m12);
    memory.store32(element,out+matrix::m22,m22);
    frame.finished(Registers{{m22,m21,m02,yy_xx,m12,xw,yw,m11}});
}
} // namespace quaternion_matrix
} // namespace halo2::native_quaternion_matrix
