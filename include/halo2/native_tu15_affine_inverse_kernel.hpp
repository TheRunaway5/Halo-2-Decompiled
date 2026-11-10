#pragma once
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"
#include <array>
#include <bit>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_affine_inverse_detail {
namespace N=native_bsp_surface_2d_detail;
inline std::uint32_t bits(N::Scalar value){return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];}
inline std::uint32_t mul(std::uint32_t a,std::uint32_t b){return bits(N::multiply(N::scalar(a),N::scalar(b)));}
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){return bits(N::add(N::scalar(a),N::scalar(b)));}
inline std::uint32_t sub(std::uint32_t a,std::uint32_t b){return bits(N::subtract(N::scalar(a),N::scalar(b)));}
inline std::uint32_t div(std::uint32_t a,std::uint32_t b){auto x=N::scalar(a);const auto y=N::scalar(b);asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
struct Comparison {bool below{},equal{},unordered{};std::uint32_t flags{};bool same()const{return equal&&!unordered;}};
inline Comparison compare(std::uint32_t a,std::uint32_t b){
    const auto x=N::scalar(a),y=N::scalar(b);unsigned char below,equal,unordered;
    asm volatile("ucomiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(x),"x"(y):"cc","memory");
    return{below!=0,equal!=0,unordered!=0,0x02u|std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u)};
}
enum class Read {scale,retained_scale,one,position_x,position_y,position_z,forward_x,left_y,output_forward_x,up_z,left_x,forward_y,up_x,forward_z,output_up_x,output_left_x,up_y,left_z,output_forward_y,output_left_z,output_left_y,output_forward_z,output_up_z};
enum class Write {scale,forward_x,left_y,up_z,forward_y,left_x,up_x,forward_z,position_x,left_z,position_y,up_y,position_z};
struct Progress {
    std::uint32_t source{},destination{},initial_scale{},retained_scale{},one{},reciprocal{},px{},py{},pz{};
    std::uint32_t fx{},ly{},output_fx{},uz{},lx{},fy{},ux{},fz{},forward_product{},tx{},output_lx{},left_product{},uy{},lz{},output_fy{},ty{},temporary{},tz{},up_component{},up_product{},final_y_term{};
    bool unit_scale{};
};
// Actual144090 affine inverse, shared from the existing typed public policy.
// Reads of already-published output axes remain lazy, so exact/partial matrix
// overlaps retain their source semantics. Access owns saved-word transport,
// source observation PCs and the zero-clear direction; this body has no GP,
// SIMD register bank, flags bookkeeping or instruction callbacks.
template<class Access>void invert(Access&memory,Progress&d){
    d.initial_scale=memory.word(Read::scale,d);memory.begin(d);
    if(memory.zero_scale(d)){
        memory.clear_begin(d);for(unsigned n=0;n<13;++n)memory.clear_word(d,n);memory.finish(d,true);return;
    }
    d.retained_scale=memory.word(Read::retained_scale,d);d.one=memory.word(Read::one,d);d.unit_scale=memory.unit_scale(d);
    d.px=sub(0,memory.word(Read::position_x,d));d.py=sub(0,memory.word(Read::position_y,d));d.pz=sub(0,memory.word(Read::position_z,d));
    if(!d.unit_scale){
        d.reciprocal=div(d.one,d.retained_scale);d.px=mul(d.reciprocal,d.px);memory.store(Write::scale,d,d.reciprocal);
        d.py=mul(d.reciprocal,d.py);d.pz=mul(d.reciprocal,d.pz);
    }else memory.store(Write::scale,d,d.one);
    d.fx=memory.word(Read::forward_x,d);memory.store(Write::forward_x,d,d.fx);
    d.ly=memory.word(Read::left_y,d);d.output_fx=memory.word(Read::output_forward_x,d);memory.store(Write::left_y,d,d.ly);
    d.uz=memory.word(Read::up_z,d);memory.store(Write::up_z,d,d.uz);
    d.lx=memory.word(Read::left_x,d);d.fy=memory.word(Read::forward_y,d);memory.store(Write::forward_y,d,d.lx);memory.store(Write::left_x,d,d.fy);
    d.ux=memory.word(Read::up_x,d);d.fz=memory.word(Read::forward_z,d);d.forward_product=mul(d.output_fx,d.px);memory.store(Write::up_x,d,d.fz);
    d.tx=mul(memory.word(Read::output_up_x,d),d.pz);d.tx=add(d.tx,d.forward_product);d.output_lx=memory.word(Read::output_left_x,d);
    memory.store(Write::forward_z,d,d.ux);d.uy=memory.word(Read::up_y,d);d.lz=memory.word(Read::left_z,d);
    d.left_product=mul(d.output_lx,d.py);d.tx=add(d.tx,d.left_product);d.output_fy=memory.word(Read::output_forward_y,d);memory.store(Write::position_x,d,d.tx);
    d.ty=mul(d.output_fy,d.px);memory.store(Write::left_z,d,d.lz);d.temporary=mul(memory.word(Read::output_left_z,d),d.pz);d.ty=add(d.temporary,d.ty);
    d.final_y_term=mul(d.py,memory.word(Read::output_left_y,d));d.ty=add(d.ty,d.final_y_term);memory.store(Write::position_y,d,d.ty);
    d.tz=mul(memory.word(Read::output_forward_z,d),d.px);d.up_component=memory.word(Read::output_up_z,d);memory.store(Write::up_y,d,d.uy);
    d.up_product=mul(d.up_component,d.pz);d.tz=add(d.tz,d.up_product);d.uy=mul(d.uy,d.py);d.tz=add(d.tz,d.uy);memory.store(Write::position_z,d,d.tz);
    memory.finish(d,false);
}
} // namespace halo2::native_affine_inverse_detail
#endif
