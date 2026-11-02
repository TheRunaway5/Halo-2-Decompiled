#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <cstring>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
namespace halo2::joint_constraint_numeric {
using Vector=__m128;
class Environment {
    alignas(16)std::array<std::byte,108>host_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),host_mxcsr_(_mm_getcsr()){
        if(depth>5u||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 joint row requires its FP capacity/exception owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffff;
        std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
    void publish(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>bytes){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(bytes.size());memory_.read(p,event.before);}
        memory_.write(p,bytes);
        if(observe_){event.written.assign(bytes.begin(),bytes.end());observe_(event);}
    }
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
    }
    Vector vector(std::uint32_t p)const{std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector value;std::memcpy(&value,b.data(),16);return value;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));publish(pc,p,b);}
    void vector(std::uint32_t pc,std::uint32_t p,Vector value){std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);publish(pc,p,b);}
    void pop_float(std::uint32_t pc,std::uint32_t p){
        // Preserve the pre-instruction native environment if the DATA write
        // itself faults; a post-store observer sees the completed FSTP phase.
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        alignas(16)std::array<std::byte,108>before{};std::uint32_t bits;
        asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        asm volatile("fstps %0":"=m"(bits)::"memory","st");std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(bits>>(n*8u));
        try{memory_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
};
inline Vector multiply(Vector a,Vector b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline Vector add(Vector a,Vector b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline Vector add_scalar(Vector a,Vector b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline Vector xor_bits(Vector a,Vector b){asm volatile("xorps %1,%0":"+x"(a):"x"(b):"memory");return a;}
template<unsigned mask>inline Vector lane(Vector a){return _mm_shuffle_ps(a,a,mask);}
inline std::uint32_t scalar_bits(Vector a){std::uint32_t bits;std::memcpy(&bits,&a,4);return bits;}
inline std::uint32_t sum_three(Vector value){auto y=lane<0x55>(value);y=add_scalar(y,value);auto z=lane<0xaa>(value);z=add_scalar(z,y);return scalar_bits(z);}
inline Vector transform_axis(Memory&m,std::uint32_t body,Vector axis){
    auto last=m.vector(body+0x70u);auto middle=m.vector(body+0x60u);
    last=multiply(last,lane<0xaau>(axis));middle=multiply(middle,lane<0x55u>(axis));
    auto first=m.vector(body+0x50u);first=multiply(first,lane<0u>(axis));first=add(first,middle);return add(first,last);
}
inline void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
inline void multiply_scalar(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
inline void add_scalar(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
inline void inverse_mass(Memory&m,std::uint32_t first,std::uint32_t second,std::uint32_t output){
    const auto b=m.vector(output+0x10u),mass_b=m.vector(second+0x30u),a=m.vector(output);
    auto bb=multiply(b,b);const auto mass_a=m.vector(first+0x30u);bb=multiply(bb,mass_b);
    auto aa=multiply(a,a);aa=multiply(aa,mass_a);const auto total=add(aa,bb);push(sum_three(total));
    add_scalar(m.word(0x41f644u));const auto one=m.word(0x46a0d0u);asm volatile("fdivrs %0"::"m"(one):"memory","st");
}
} // namespace halo2::joint_constraint_numeric
#endif
