#include "halo2/native_tu15_constraint_local_points.hpp"
#include "halo2/native_tu15_constraint_body_slots.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 constraint local point conversion requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}
    m.write(p,b);if(o)o(event);
}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool different_or_unordered(__m128 a,__m128 b){unsigned char different,unordered;
    asm volatile("ucomiss %3,%2; setne %0; setp %1":"=qm"(different),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");
    return different||unordered;
}
void publish_frame(SourcePhysicsContactMemory&m,std::uint32_t destination,const NativeTu15ModelConstraintFrame&frame){
    const auto words=std::bit_cast<std::array<std::uint32_t,13>>(frame);std::array<std::byte,52>bytes{};
    for(unsigned n=0;n<13;++n)for(unsigned k=0;k<4;++k)bytes[n*4u+k]=std::byte(words[n]>>(k*8u));m.write(destination,bytes);
}
}
void native_tu15_update_constraint_local_points(SourcePhysicsContactMemory&m,std::uint32_t instance,
    NativeTu15ConstraintBodySlots slots,NativeTu15ConstraintLocalPointScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintLocalPointStats*stats){
    if(std::uint64_t(scratch.frame)+52u>0x100000000ull||std::uint64_t(scratch.inverse)+52u>0x100000000ull||
       (std::uint64_t(scratch.frame)<std::uint64_t(scratch.inverse)+52u&&std::uint64_t(scratch.inverse)<std::uint64_t(scratch.frame)+52u))
        throw std::runtime_error("Native TU1.5 constraint point conversion requires separate caller-owned frame DATA");
    if(stats)++stats->calls;const auto first=read(m,instance+0x24u);
    const auto components=read(m,0x52be5cu),rows=read(m,components+0x44u),component=rows+(first&0xffffu)*160u;
    const auto frame=native_tu15_model_constraint_body_frame(m,{component},slots.first,fp,stats?&stats->frames:nullptr);
    if(read(m,instance+0x9cu,2)!=0xffffu){
        store(m,observer,0x22ebe5u,instance+0x68u,frame.position[0]);
        store(m,observer,0x22ebe7u,instance+0x6cu,frame.position[1]);
        store(m,observer,0x22ebeau,instance+0x70u,frame.position[2]);
        if(stats)++stats->retained_node_positions;return;
    }
    const auto convert=[&](const NativeTu15ModelConstraintFrame&source,std::uint32_t destination,bool second){
        publish_frame(m,scratch.frame,source);native_tu15_camera_inverse_frame(m,scratch.frame,scratch.inverse,fp);
        if(stats)++stats->inverse_frames;
        Environment environment(fp);const auto scale=scalar(read(m,scratch.inverse));
        const bool scaled=different_or_unordered(scale,scalar(read(m,0x46a0d0u)));
        __m128 point[3]{scalar(read(m,instance+0x68u)),scalar(read(m,instance+0x6cu)),scalar(read(m,instance+0x70u))};
        if(scaled)for(auto&value:point)value=second?mul(value,scale):mul(scale,value);
        constexpr std::array<std::uint32_t,3>first_sites{0x22ec71u,0x22ec9cu,0x22eccdu},
            second_sites{0x22ed6cu,0x22ed9au,0x22edceu};
        for(unsigned n=0;n<3;++n){
            auto value=mul(scalar(read(m,scratch.inverse+0x1cu+n*4u)),point[2]);
            const auto side=mul(scalar(read(m,scratch.inverse+0x10u+n*4u)),point[1]);value=add(value,side);
            const auto forward=mul(scalar(read(m,scratch.inverse+4u+n*4u)),point[0]);value=add(value,forward);
            value=add(value,scalar(read(m,scratch.inverse+0x28u+n*4u)));
            store(m,observer,(second?second_sites:first_sites)[n],instance+destination+n*4u,bits(value));
        }
    };
    convert(frame,0x74u,false);if(stats)++stats->first_points;
    const auto second=read(m,instance+0x28u);
    if(second!=0xffffffffu){
        const auto next_components=read(m,0x52be5cu),next_rows=read(m,next_components+0x44u);
        const auto next=native_tu15_model_constraint_body_frame(m,{next_rows+(second&0xffffu)*160u},slots.second,fp,
            stats?&stats->frames:nullptr);
        convert(next,0x80u,true);if(stats)++stats->second_points;
    }
}
} // namespace halo2
