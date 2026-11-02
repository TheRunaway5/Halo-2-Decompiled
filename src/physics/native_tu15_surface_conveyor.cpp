#include "halo2/native_tu15_surface_conveyor.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t read(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t v{};
        for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;
    }
    void write(std::uint32_t pc,std::uint32_t p,std::uint32_t v){
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(o_){event.before.resize(4);m_.read(p,event.before);event.written.assign(b.begin(),b.end());}
        m_.write(p,b);if(o_)o_(event);
    }
};
std::uint32_t signed_word(std::uint32_t v){return std::bit_cast<std::uint32_t>(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Comparison {bool below{},equal{},unordered{};bool above()const{return!below&&!equal;}};
Comparison compare(Scalar a,Scalar b,bool unordered_instruction=false){
    unsigned char below,equal,unordered;
    if(unordered_instruction)asm volatile("ucomiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");
    else asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");
    return{below!=0,equal!=0,unordered!=0};
}
class SimdScope {
    NativeTu15PhysicsFrameFp&fp_;unsigned host_;bool active_{};
public:
    explicit SimdScope(NativeTu15PhysicsFrameFp&fp):fp_(fp),host_(_mm_getcsr()){resume();}
    void resume(){if((fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 surface conveyor requires its unsupported SIMD trap owner");_mm_setcsr(fp_.mxcsr);active_=true;}
    void suspend(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);active_=false;}
    ~SimdScope(){if(active_)fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
Scalar range_sample(Memory&m,std::uint32_t function,Scalar value){
    const auto data=m.read(function+4u);
    if(m.read(data+1u,1)&0xf0u)return value;
    const auto zero=_mm_setzero_ps();const auto first=compare(zero,value);
    const auto low=scalar(m.read(data+4u)),high=scalar(m.read(data+8u));
    if(first.above())value=zero;
    else{const auto one=scalar(m.read(0x46a0d0u));const auto second=compare(value,one);if(!second.below&&!second.equal)value=one;}
    return add(mul(sub(high,low),value),low);
}
#endif
} // namespace
bool native_tu15_shader_conveyor_derivative(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,std::uint32_t map,std::uint32_t first,std::uint32_t second,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ShaderFunctionEvaluationServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SurfaceConveyorStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(stats)++stats->shader_queries;Memory m(memory,observe);SimdScope scope(fp);
    const auto tags=m.read(0x4f0414u),tag=m.read(tags+(datum&0xffffu)*16u+8u),definition=m.read(tag+0x24u);
    m.write(0x355f5u,first,0);m.write(0x355fdu,second,0);
    const auto map_table=m.read(definition+0x60u),entry=signed_word(m.read(map_table+map*4u+2u,2));
    if(entry==0xffffffffu)return false;
    const auto packed=m.read(definition+0x50u)+entry*2u,word=m.read(packed,2);
    auto binding=m.read(definition+0x48u)+(word&0x1ffu)*4u;std::uint32_t successes{},index{};
    if(std::int32_t(word>>9u)<=0)return false;
    do{
        const auto kind=m.read(binding+2u,2);
        if(kind==4u||kind==5u){
            if(stats)++stats->bindings;
            const auto function_index=signed_word(m.read(binding,2)),functions=m.read(definition+0x40u);
            const auto function=functions+function_index*20u;
            auto period=scalar(m.read(0x46a0d0u));const auto candidate=scalar(m.read(function+8u));
            const auto nonzero=compare(candidate,_mm_setzero_ps(),true);
            if(!nonzero.equal||nonzero.unordered)period=candidate;
            const auto sample=[&](std::uint32_t input){
                if(!services.evaluate){if(stats)++stats->unbound_function_calls;throw std::runtime_error("Native TU1.5 shader conveyor requires native13DE70 function evaluation");}
                scope.suspend();if(stats)++stats->function_calls;
                const auto reference=function+0xcu;
                const auto value=services.evaluate(reference,input,0u,fp,depth);scope.resume();return range_sample(m,reference,scalar(value));
            };
            const auto start=sample(0u),finish=sample(0x3f800000u);
            const auto current=signed_word(m.read(binding+2u,2));
            const auto derivative=divide(sub(finish,start),period);
            if(current==4u){m.write(0x35742u,first,bits(derivative));if(stats)++stats->published_axes;}
            else if(current==5u){m.write(0x35738u,second,bits(derivative));if(stats)++stats->published_axes;}
            ++successes;
        }
        const auto current_count=m.read(packed,2)>>9u;++index;binding+=4u;
        if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(current_count))break;
    }while(true);
    return std::bit_cast<std::int32_t>(successes)>0;
#else
    (void)memory;(void)datum;(void)map;(void)first;(void)second;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;
    throw std::runtime_error("Exact native TU1.5 conveyor arithmetic backend unavailable");
#endif
}
bool native_tu15_material_surface_velocity(SourcePhysicsContactMemory&memory,
    std::uint32_t material,std::uint32_t destination,NativeTu15SurfaceConveyorScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ShaderFunctionEvaluationServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15SurfaceConveyorStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(stats)++stats->material_queries;Memory m(memory,observe);SimdScope scope(fp);
    const auto globals=m.read(0x4ecc18u),materials=m.read(globals+0x10u),row=materials+material*20u;
    const auto encoded=m.read(row+0xau,2);if(encoded==0xffffu)return false;
    const auto index=signed_word(encoded),count=m.read(globals+0xb4u);
    if(std::bit_cast<std::int32_t>(count)<=std::bit_cast<std::int32_t>(index))return false;
    const auto basis=m.read(globals+0xb8u)+index*24u;
    const auto query=[&](unsigned map){const auto shader=m.read(row+0x10u);scope.suspend();
        const auto found=native_tu15_shader_conveyor_derivative(memory,shader,map,scratch.first,scratch.second,fp,depth,services,observe,stats);
        scope.resume();return found;};
    if(!query(0)&&!query(1))return false;
    auto x=scalar(m.read(basis)),y=scalar(m.read(basis+4u)),z=scalar(m.read(basis+8u)),vz=scalar(m.read(basis+0x14u));
    const auto u=sub(_mm_setzero_ps(),scalar(m.read(scratch.first))),v=sub(_mm_setzero_ps(),scalar(m.read(scratch.second)));
    auto vy=scalar(m.read(basis+0x10u));y=mul(y,u);z=mul(z,u);x=mul(x,u);
    auto vx=scalar(m.read(basis+0xcu));vx=mul(vx,v);vy=mul(vy,v);vz=mul(vz,v);
    vx=add(vx,x);vy=add(vy,y);vz=add(vz,z);
    m.write(0x184d15u,destination,bits(vx));m.write(0x184d19u,destination+4u,bits(vy));m.write(0x184d1eu,destination+8u,bits(vz));
    if(stats)++stats->material_vectors;return true;
#else
    (void)memory;(void)material;(void)destination;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;
    throw std::runtime_error("Exact native TU1.5 material conveyor arithmetic backend unavailable");
#endif
}
} // namespace halo2
