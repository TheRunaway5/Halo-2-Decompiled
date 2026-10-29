#include "halo2/native_campaign_ai_vision_support_tu15.hpp"
#include "halo2/native_campaign_ai_perception_score_tu15.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8u);return v;
}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v){
    std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(v>>(k*8u));m.write(p,b);
}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::uint32_t unit_vector(const SourcePhysicsContactMemory&m,std::uint32_t object){
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u);return read(m,row(rows,object,12u)+8u)+0x168u;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp&fp_;unsigned ambient_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),ambient_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native vision support requires its original SIMD trap owner");_mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(ambient_);}
};
class ExtendedEnvironment {
    alignas(16)std::array<std::byte,108>ambient_{};std::array<std::byte,28>source_{};NativeTu15PhysicsFrameFp&fp_;
public:
    explicit ExtendedEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp){
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u))throw std::runtime_error("Native vision facing requires its original FP trap owner");
        asm volatile("fnsave %0":"=m"(ambient_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(source_.data(),ambient_.data(),source_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(source_.data(),&fp.control_word,2);std::memcpy(source_.data()+4,&fp.status,2);std::memcpy(source_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(source_):"memory");
    }
    ~ExtendedEnvironment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));
        asm volatile("frstor %0"::"m"(ambient_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result!=0;}
bool below(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; setb %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result!=0;}
std::uint32_t transverse_distance(std::uint32_t cosine_bits,std::uint32_t one_bits,
        std::uint32_t distance_bits,NativeTu15PhysicsFrameFp&fp){
    ExtendedEnvironment environment(fp);const auto cosine=std::bit_cast<float>(cosine_bits),one=std::bit_cast<float>(one_bits),distance=std::bit_cast<float>(distance_bits);float output;
    asm volatile("flds %1; fmuls %1; fsubrs %2; fsqrt; fmuls %3; fstps %0"
        :"=m"(output):"m"(cosine),"m"(one),"m"(distance):"memory","st");return std::bit_cast<std::uint32_t>(output);
}
template<class Facing>std::uint8_t classify(const SourcePhysicsContactMemory&m,std::uint32_t direction,
        const Facing&facing,std::uint32_t distance,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    SimdEnvironment environment(fp);
    auto z=scalar(read(m,direction+8u));z=mul(z,scalar(facing(2)));
    auto y=scalar(read(m,direction+4u));y=mul(y,scalar(facing(1)));auto dot=add(z,y);
    auto x=scalar(read(m,direction));x=mul(x,scalar(facing(0)));dot=add(dot,x);
    const auto cosine=sub(scalar(0),dot);__m128 separation;
    if(!below(scalar(0),cosine))separation=scalar(read(m,0x45695cu));
    else if(!below(cosine,scalar(read(m,0x46a0d0u))))separation=scalar(0);
    else{
        if(occupied_x87>7u)throw std::runtime_error("Native vision facing requires its original x87 stack-fault owner");
        const auto one=read(m,0x46a0d0u);separation=scalar(transverse_distance(bits(cosine),one,distance,fp));
    }
    if(above(cosine,scalar(read(m,0x46a2ccu))))return 0;
    const auto half=scalar(read(m,0x46a0ccu));if(above(half,separation))return 0;
    if(above(cosine,scalar(read(m,0x46a2c8u))))return 1;
    if(above(scalar(read(m,0x449fd4u)),separation))return 1;
    if(above(cosine,half))return 2;
    return above(cosine,scalar(0))?3:4;
}
#endif
}
void native_campaign_ai_vision_tracking_timestamp_tu15(SourcePhysicsContactMemory&m,
        std::uint32_t actor,std::uint32_t perception,bool tracked,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    const auto properties=read(m,0x50eef0u),property_rows=read(m,properties+0x44u),query=read(m,row(property_rows,perception,0x3cu)+0x14u);if(query==none)return;
    const auto queries=read(m,0x50eeecu),query_rows=read(m,queries+0x44u),record=row(query_rows,query,0x124u);if(!record||!(record+0x70u))return;
    const auto extension=record+0x70u;std::uint32_t timestamp=none;
    if(tracked){const auto clock=read(m,0x51df54u);timestamp=read(m,clock+8u);}
    write(m,extension+0x50u,timestamp);
    const auto score=native_campaign_ai_perception_score_tu15(m,actor,perception,fp,occupied_x87);
    write(m,extension+0x3cu,score);
}
std::uint32_t native_campaign_ai_vision_attachment_select_tu15(const SourcePhysicsContactMemory&m,
        std::uint32_t owner,std::uint32_t reference,std::uint32_t preferred,NativeTu15PhysicsFrameFp&fp){
    const auto owners=read(m,0x510fa8u),rows=read(m,owners+0x44u);
    const auto maximum=read(m,0x45695cu),flag_weight=read(m,0x46a19cu),preferred_weight=read(m,0x46a4f8u);
    const auto objects=read(m,0x4ecbd0u);auto object=read(m,row(rows,owner,0x34u)+0x18u);std::uint32_t selected=none;
    if(object==none)return none;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);auto best=scalar(maximum);
    while(object!=none){
        const auto object_rows=read(m,objects+0x44u),body=read(m,row(object_rows,object,12u)+8u),detached=read(m,body+0x134u),current=object;
        if(!detached){const auto offset=signed16(read(m,body+0x13au,2));const auto next=body+std::uint32_t(std::int32_t(offset));object=next?read(m,next+0xcu):none;}else object=none;
        std::uint32_t attachment=0;if(!detached){const auto offset=signed16(read(m,body+0x13au,2));attachment=body+std::uint32_t(std::int32_t(offset));}
        auto dx=scalar(read(m,reference+0xcu));dx=sub(dx,scalar(read(m,attachment+0x14u)));
        auto dy=scalar(read(m,reference+0x10u));dy=sub(dy,scalar(read(m,attachment+0x18u)));
        auto dz=scalar(read(m,reference+0x14u));dz=sub(dz,scalar(read(m,attachment+0x1cu)));
        const auto flags=read(m,attachment,1);const auto xx=mul(dx,dx);auto score=mul(dz,dz);score=add(score,xx);const auto yy=mul(dy,dy);score=add(score,yy);
        if(flags&1u)score=mul(score,scalar(flag_weight));else if(current==preferred)score=mul(score,scalar(preferred_weight));
        if(above(best,score)){best=score;selected=current;}
    }
    return selected;
#else
    (void)maximum;(void)flag_weight;(void)preferred_weight;(void)objects;(void)object;(void)selected;(void)fp;throw std::runtime_error("Exact native vision attachment backend unavailable");
#endif
}
void native_campaign_ai_vision_unit_vector_copy_tu15(SourcePhysicsContactMemory&m,std::uint32_t object,std::uint32_t destination){
    const auto source=unit_vector(m,object);for(unsigned n=0;n<3;++n)write(m,destination+n*4u,read(m,source+n*4u));
}
std::array<std::uint32_t,3> native_campaign_ai_vision_unit_vector_value_tu15(const SourcePhysicsContactMemory&m,std::uint32_t object){
    const auto source=unit_vector(m,object);std::array<std::uint32_t,3>value{};for(unsigned n=0;n<3;++n)value[n]=read(m,source+n*4u);return value;
}
std::uint32_t native_campaign_ai_vision_unit_extension_tu15(const SourcePhysicsContactMemory&m,std::uint32_t object){
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),body=read(m,row(rows,object,12u)+8u);
    const auto offset=signed16(read(m,body+0x33eu,2));const auto extension=body+std::uint32_t(std::int32_t(offset))+0x9cu;
    if(read(m,extension)==none||read(m,extension+6u,2)==0xffffu)return none;return read(m,extension+8u);
}
std::uint8_t native_campaign_ai_vision_facing_class_tu15(const SourcePhysicsContactMemory&m,
        std::uint32_t direction,const std::array<std::uint32_t,3>&facing,std::uint32_t distance,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return classify(m,direction,[&](unsigned n){return facing[n];},distance,fp,occupied_x87);
#else
    (void)m;(void)direction;(void)facing;(void)distance;(void)fp;(void)occupied_x87;throw std::runtime_error("Exact native vision facing backend unavailable");
#endif
}
std::uint8_t native_campaign_ai_vision_facing_class_tu15(const SourcePhysicsContactMemory&m,
        std::uint32_t direction,std::uint32_t facing,std::uint32_t distance,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return classify(m,direction,[&](unsigned n){return read(m,facing+n*4u);},distance,fp,occupied_x87);
#else
    (void)m;(void)direction;(void)facing;(void)distance;(void)fp;(void)occupied_x87;throw std::runtime_error("Exact native vision facing backend unavailable");
#endif
}
}
