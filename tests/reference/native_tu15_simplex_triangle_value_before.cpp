#include "halo2/native_tu15_simplex_triangle.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2::triangle_value_reference {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observe_)observe_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));bytes(pc,p,b);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 simplex triangle requires aligned DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 simplex triangle requires aligned output");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);bytes(pc,p,b);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 simplex triangle requires its unsupported FP owner");asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
};
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 triple_products(__m128 a,__m128 b,__m128 c,__m128 normal){const auto ab0=_mm_shuffle_ps(a,b,0x44),ab1=_mm_shuffle_ps(a,b,0xee),cc0=_mm_shuffle_ps(c,c,0x44),cc1=_mm_shuffle_ps(c,c,0xee);const auto x=_mm_shuffle_ps(ab0,cc0,0x88),y=_mm_shuffle_ps(ab0,cc0,0xdd),z=_mm_shuffle_ps(ab1,cc1,0x88);const auto zterm=multiply(z,_mm_shuffle_ps(normal,normal,0xaa)),yterm=multiply(y,_mm_shuffle_ps(normal,normal,0x55)),xterm=multiply(x,_mm_shuffle_ps(normal,normal,0));return add(add(xterm,yterm),zterm);}
#endif
}
std::uint32_t native_tu15_orient_simplex_triangle(SourcePhysicsContactMemory&memory,
    std::uint32_t simplex,std::uint32_t point,std::uint32_t vertices,
    std::uint32_t orient,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);Environment environment(fp,depth);
    const auto a=m.vector(vertices),b=m.vector(vertices+0x10u),c=m.vector(vertices+0x20u);
    const auto ac=subtract(a,c),ac_yzx=_mm_shuffle_ps(ac,ac,0xc9),ac_zxy=_mm_shuffle_ps(ac,ac,0xd2),cb=subtract(c,b),ba=subtract(b,a),cb_zxy=_mm_shuffle_ps(cb,cb,0xd2),cb_yzx=_mm_shuffle_ps(cb,cb,0xc9);
    const auto negative_normal=multiply(cb_zxy,ac_yzx),positive_normal=multiply(cb_yzx,ac_zxy);auto normal=subtract(positive_normal,negative_normal);
    const auto p=m.vector(point),pb=subtract(p,b),negative_pb=multiply(_mm_shuffle_ps(pb,pb,0xd2),cb_yzx),pb_yzx=_mm_shuffle_ps(pb,pb,0xc9);
    const auto c_now=m.vector(vertices+0x20u),positive_pb=multiply(pb_yzx,cb_zxy),pc=subtract(p,c_now),negative_pc=multiply(_mm_shuffle_ps(pc,pc,0xd2),ac_yzx),positive_pc=multiply(_mm_shuffle_ps(pc,pc,0xc9),ac_zxy);
    const auto a_now=m.vector(vertices),pa=subtract(p,a_now),pc_cross=subtract(positive_pc,negative_pc),negative_pa=multiply(_mm_shuffle_ps(pa,pa,0xd2),_mm_shuffle_ps(ba,ba,0xc9));
    const auto pb_cross=subtract(positive_pb,negative_pb),positive_pa=multiply(_mm_shuffle_ps(pa,pa,0xc9),_mm_shuffle_ps(ba,ba,0xd2)),pa_cross=subtract(positive_pa,negative_pa),weights=triple_products(pb_cross,pc_cross,pa_cross,normal);
    const auto mask=std::uint32_t(_mm_movemask_ps(weights))&7u;m.vector(0x30d51eu,simplex+0x160u,weights);if(!orient)return mask;
    m.vector(0x30d52eu,simplex+0x120u,normal);if(mask!=7u)return mask;
    const auto first=m.vector(simplex+0x20u),second=m.vector(simplex+0xa0u),offset=subtract(second,first),product=multiply(normal,offset),xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product),dot=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);
    if(!(_mm_movemask_ps(dot)&1u))return mask;normal=_mm_xor_ps(normal,m.vector(0x461960u));m.vector(0x30d578u,simplex+0x120u,normal);
    const auto v0=m.vector(vertices),v1=m.vector(vertices+0x10u);m.vector(0x30d586u,vertices,v1);m.vector(0x30d589u,vertices+0x10u,v0);
    const auto w0=m.vector(vertices+0x40u),w1=m.vector(vertices+0x50u);m.vector(0x30d595u,vertices+0x40u,w1);m.vector(0x30d599u,vertices+0x50u,w0);
    const auto weight0=m.word(simplex+0x160u);asm volatile("flds %0"::"m"(weight0):"memory","st");const auto weight1=m.word(simplex+0x164u);m.word(0x30d5a9u,simplex+0x160u,weight1);std::uint32_t rounded;asm volatile("fstps %0":"=m"(rounded)::"memory","st");m.word(0x30d5afu,simplex+0x164u,rounded);return mask;
#else
    (void)memory;(void)simplex;(void)point;(void)vertices;(void)orient;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native TU1.5 simplex triangle backend unavailable");
#endif
}
} // namespace halo2
