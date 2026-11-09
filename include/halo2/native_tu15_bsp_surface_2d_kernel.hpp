#pragma once
#include <bit>
#include <cstdint>
#include <cstring>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
namespace halo2::native_bsp_surface_2d_detail {
using Scalar=__m128;
inline Scalar scalar(std::uint32_t bits){Scalar value{};std::memcpy(&value,&bits,4);return value;}
inline Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Comparison {bool below{};std::uint32_t flags{};};
inline Comparison compare_zero(Scalar value){
    unsigned char below{},equal{},unordered{};const Scalar zero{};
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(value),"x"(zero):"cc","memory");
    return {below!=0,0x02u|std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u)};
}
enum class Phase {begin,nodes,row,y_loaded,y_product,x_loaded,products,sum,child,next,finish};
struct Progress {
    std::uint32_t descriptor{},node{},nodes{},offset{},row{},side{};
    Scalar y{},x{};
    Comparison comparison;
};
// Actual2A0230: descend a packed 2D BSP by signed distances. Plane and point
// operands retain the original lazy access order. The nodes pointer is cached
// once; sign-extended child halves retain the original leaf/sentinel encoding.
template<class Memory,class Point,class Notify>
std::uint32_t find(Memory& memory,std::uint32_t descriptor,std::uint32_t node,const Point& point,Notify notify){
    Progress data{};data.descriptor=descriptor;data.node=node;
    if(!(node&0x8000u)) {
        notify(Phase::begin,data);notify(Phase::nodes,data);data.nodes=memory.word(descriptor+4u);
        do {
            data.offset=(data.node&0x7fffu)*16u;data.row=data.nodes+data.offset;notify(Phase::row,data);
            data.y=scalar(memory.word(data.row+4u));notify(Phase::y_loaded,data);
            data.y=multiply(data.y,scalar(point.word(1)));notify(Phase::y_product,data);
            data.x=scalar(memory.word(data.row));notify(Phase::x_loaded,data);
            data.x=multiply(data.x,scalar(point.word(0)));notify(Phase::products,data);
            data.y=add(data.y,data.x);notify(Phase::sum,data);
            data.y=subtract(data.y,scalar(memory.word(data.row+8u)));
            data.comparison=compare_zero(data.y);data.side=data.comparison.below?0u:1u;notify(Phase::child,data);
            data.node=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(memory.word(data.row+12u+data.side*2u,2)))));
            notify(Phase::next,data);
        }while(!(data.node&0x8000u));
        notify(Phase::finish,data);
    }
    return data.node==0xffffffffu?data.node:data.node&0x7fffu;
}
} // namespace halo2::native_bsp_surface_2d_detail
#endif
