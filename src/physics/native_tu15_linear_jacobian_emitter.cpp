#include "halo2/native_tu15_linear_jacobian_host.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
    NativeTu15LinearJacobianEmitterStats*stats_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
        NativeTu15LinearJacobianEmitterStats*s):m_(m),o_(o),stats_(s){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t v{};
        for(unsigned k=0;k<width;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8u);return v;
    }
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(o_){event.before.resize(b.size());m_.read(p,event.before);event.written.assign(b.begin(),b.end());}
        m_.write(p,b);if(o_)o_(event);
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t v){
        std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(v>>(k*8u));bytes(pc,p,b);
    }
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{
        if(p&15u){if(stats_)++stats_->unaligned_vectors;throw std::runtime_error("Native linear Jacobian requires aligned source vector DATA");}
        alignas(16)std::array<std::byte,16>b{};m_.read_vector(p,b);__m128 v;std::memcpy(&v,b.data(),16);return v;
    }
    void vector(std::uint32_t pc,std::uint32_t p,__m128 v){
        if(p&15u){if(stats_)++stats_->unaligned_vectors;throw std::runtime_error("Native linear Jacobian requires aligned output vector DATA");}
        alignas(16)std::array<std::byte,16>b{};std::memcpy(b.data(),&v,16);bytes(pc,p,b);
    }
    void x87(std::uint32_t pc,std::uint32_t p,bool pop){
        // Snapshot only this native kernel's own real hardware state. A
        // failed Memory write occurs before the source FST/FSTP completes;
        // observer failure after a completed write preserves its real result.
        alignas(16)std::array<std::byte,108>pending{};
        asm volatile("fnsave %0;frstor %0":"=m"(pending)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::uint32_t v;
        if(pop)asm volatile("fstps %0":"=m"(v)::"memory","st");
        else asm volatile("fsts %0":"=m"(v)::"memory");
        SourcePhysicsContactRecordStore event{pc,p,{},{}};std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(v>>(k*8u));
        try{if(o_){event.before.resize(4);m_.read(p,event.before);event.written.assign(b.begin(),b.end());}m_.write(p,b);}
        catch(...){asm volatile("frstor %0"::"m"(pending):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
        if(o_)o_(event);
    }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15LinearJacobianEmitterStats*s):fp_(fp),prior_(_mm_getcsr()){
        if(depth>6||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
            if(s)++s->unsupported_fp_profiles;throw std::runtime_error("Native linear Jacobian requires its qualified FP profile");}
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(prior_);}
};
using V=__m128;
V sub(V a,V b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
V mul(V a,V b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
V add(V a,V b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
V scalar_add(V a,V b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t scalar(V a){std::uint32_t n;std::memcpy(&n,&a,4);return n;}
void push(std::uint32_t n){asm volatile("flds %0"::"m"(n):"memory","st");}
void plus(std::uint32_t n){asm volatile("fadds %0"::"m"(n):"memory","st");}
template<class Point>void emit(Memory&m,std::uint32_t input,std::uint32_t packet,
    Point&&point,NativeTu15LinearJacobianEmitterStats*stats){
        const auto first=m.word(input),second=m.word(input+4u);auto output=m.word(packet),schemas=m.word(packet+4u);auto axis=0x46e400u;
        for(unsigned row=0;row<3;++row){
            auto v=m.vector(axis);m.vector(0x329516u,output,v);
            auto arm_a=point(true);const auto center_a=m.vector(first+0x40u);const auto center_b=m.vector(second+0x40u);arm_a=sub(arm_a,center_a);auto arm_b=point(false);
            const auto first_local=m.word(first+0xcu,1);arm_b=sub(arm_b,center_b);const auto direction=m.vector(axis);
            const auto negative_a=mul(_mm_shuffle_ps(arm_a,arm_a,0xd2),_mm_shuffle_ps(direction,direction,0xc9));
            const auto positive_a=mul(_mm_shuffle_ps(arm_a,arm_a,0xc9),_mm_shuffle_ps(direction,direction,0xd2));
            auto angular_a=sub(positive_a,negative_a);
            if(!first_local){
                auto z=m.vector(first+0x70u);auto y=m.vector(first+0x60u);z=mul(z,_mm_shuffle_ps(angular_a,angular_a,0xaa));y=mul(y,_mm_shuffle_ps(angular_a,angular_a,0x55));
                auto x=m.vector(first+0x50u);x=mul(x,_mm_shuffle_ps(angular_a,angular_a,0));angular_a=add(add(x,y),z);if(stats)++stats->rotated_first;
            }
            m.vector(0x329592u,output+0x10u,angular_a);
            auto current_axis=m.vector(axis);const auto second_local=m.word(second+0xcu,1);
            const auto negative_b=mul(_mm_shuffle_ps(current_axis,current_axis,0xd2),_mm_shuffle_ps(arm_b,arm_b,0xc9));
            const auto positive_b=mul(_mm_shuffle_ps(current_axis,current_axis,0xc9),_mm_shuffle_ps(arm_b,arm_b,0xd2));auto angular_b=sub(positive_b,negative_b);
            if(!second_local){
                auto z=m.vector(second+0x70u);auto y=m.vector(second+0x60u);z=mul(z,_mm_shuffle_ps(angular_b,angular_b,0xaa));y=mul(y,_mm_shuffle_ps(angular_b,angular_b,0x55));
                auto x=m.vector(second+0x50u);x=mul(x,_mm_shuffle_ps(angular_b,angular_b,0));angular_b=add(add(x,y),z);if(stats)++stats->rotated_second;
            }
            angular_a=m.vector(output+0x10u);m.vector(0x3295fcu,output+0x20u,angular_b);
            push(m.word(second+0x3cu));angular_b=m.vector(output+0x20u);plus(m.word(first+0x3cu));const auto second_inertia=m.vector(second+0x30u);plus(m.word(0x41f644u));
            auto squared_b=mul(angular_b,angular_b);const auto first_inertia=m.vector(first+0x30u);squared_b=mul(squared_b,second_inertia);auto squared_a=mul(angular_a,angular_a);squared_a=mul(squared_a,first_inertia);const auto terms=add(squared_a,squared_b);
            auto sum=scalar_add(_mm_shuffle_ps(terms,terms,0x55),terms);sum=scalar_add(_mm_shuffle_ps(terms,terms,0xaa),sum);const auto dot=scalar(sum);plus(dot);
            schemas+=4u;m.x87(0x32965bu,output+0x2cu,false);output+=0x30u;push(m.word(0x46a0d0u));axis+=0x10u;asm volatile("fdiv %%st(1)":::"memory","st");m.x87(0x32966cu,output-0x14u,true);
            const auto point_a=point(true),point_b=point(false);asm volatile("fstp %%st(0)":::"memory","st");current_axis=m.vector(axis-0x10u);const auto separation=sub(point_b,point_a),projected=mul(separation,current_axis);
            auto error=scalar_add(_mm_shuffle_ps(projected,projected,0x55),projected);error=scalar_add(_mm_shuffle_ps(projected,projected,0xaa),error);push(scalar(error));const auto scale=m.word(input+0xcu);asm volatile("fmuls %0"::"m"(scale):"memory","st");
            m.x87(0x3296b8u,output-0x24u,true);m.word(0x3296bbu,schemas-4u,0x3040du);if(stats)++stats->rows;
        }
        m.word(0x3296cbu,packet+4u,schemas);m.word(0x3296d0u,packet,output);if(stats)++stats->completed;
}

#endif
}
void native_tu15_emit_world_linear_jacobians(SourcePhysicsContactMemory&memory,
    std::uint32_t first_point,std::uint32_t second_point,std::uint32_t input,
    std::uint32_t packet,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15LinearJacobianEmitterStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        Memory m(memory,observe,stats);Environment environment(fp,depth,stats);
        emit(m,input,packet,[&](bool first){return m.vector(first?first_point:second_point);},stats);
#else
        (void)memory;(void)first_point;(void)second_point;(void)input;(void)packet;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native linear Jacobian backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
void native_tu15_emit_world_linear_jacobians_active(SourcePhysicsContactMemory&memory,
    const NativeTu15ConstraintVector4&first_point,const NativeTu15ConstraintVector4&second_point,
    std::uint32_t input,std::uint32_t packet,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15LinearJacobianEmitterStats*stats){
    if(stats)++stats->calls;
    try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        if(depth>6u){if(stats)++stats->unsupported_fp_profiles;throw std::runtime_error("Native active linear Jacobian requires x87 capacity owner");}
        Memory m(memory,observe,stats);
        emit(m,input,packet,[&](bool first){V v;const auto&point=first?first_point:second_point;std::memcpy(&v,point.data(),16);return v;},stats);
#else
        (void)memory;(void)first_point;(void)second_point;(void)input;(void)packet;(void)depth;(void)observe;throw std::runtime_error("Exact native active linear Jacobian backend unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}

}
