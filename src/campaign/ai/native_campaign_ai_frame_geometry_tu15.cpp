#include "halo2/native_campaign_ai_frame_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include <bit>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 { namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
template<class Input,class Sink> void copy(const Input &input,const Sink &sink,
    std::array<std::uint32_t,3> sites) {for(unsigned n=0;n<3;++n)sink(n,input(n),sites[n]);}
template<class Input,class Sink> void permutation(const SourcePhysicsContactMemory &memory,
    const Input &input,std::int16_t axis,std::uint8_t variant,bool inverse,const Sink &sink) {
    const auto base=(inverse?0x44ac8cu:0x44acb0u)+(std::uint32_t(std::int32_t(axis))*2u+variant)*6u;
    const auto iz=std::int16_t(read(memory,base+4,2));const auto z=input(std::uint32_t(std::int32_t(iz)));
    const auto iy=std::int16_t(read(memory,base+2,2));const auto ix=std::int16_t(read(memory,base,2));
    const auto y=input(std::uint32_t(std::int32_t(iy)));const auto x=input(std::uint32_t(std::int32_t(ix)));
    const std::array<std::uint32_t,3> sites=inverse?std::array<std::uint32_t,3>{0x219013,0x219015,0x21901a}:std::array<std::uint32_t,3>{0x219063,0x219065,0x21906a};
    sink(0,x,sites[0]);sink(1,y,sites[1]);sink(2,z,sites[2]);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::runtime_error("Native frame geometry requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t word(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 divide(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool equal(__m128 a,__m128 b) {
    unsigned char same{},ordered{};asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetnp %1":"=qm"(same),"=qm"(ordered):"x"(a),"x"(b):"cc","memory");return same&&ordered;
}
template<class Input,class Sink> void direction(const SourcePhysicsContactMemory &m,
    std::uint32_t matrix,const Input &input,const Sink &sink) {
    auto scale=scalar(read(m,matrix));const bool unit=equal(scale,scalar(read(m,0x46a0d0)));
    auto x=scalar(input(0)),y=scalar(input(1)),z=scalar(input(2));
    if(!unit){x=mul(scale,x);scale=scalar(read(m,matrix));y=mul(scale,y);scale=scalar(read(m,matrix));z=mul(scale,z);}
    auto zterm=scalar(read(m,matrix+0x1c)),yterm=scalar(read(m,matrix+0x10));
    zterm=mul(zterm,z);yterm=mul(yterm,y);auto sum=add(zterm,yterm);sum=add(sum,mul(scalar(read(m,matrix+4)),x));sink(0,word(sum),0x14519f);
    zterm=scalar(read(m,matrix+0x20));yterm=scalar(read(m,matrix+0x14));
    zterm=mul(zterm,z);yterm=mul(yterm,y);sum=add(zterm,yterm);sum=add(sum,mul(scalar(read(m,matrix+8)),x));sink(1,word(sum),0x1451c6);
    zterm=mul(scalar(read(m,matrix+0x24)),z);yterm=mul(scalar(read(m,matrix+0x18)),y);const auto xbasis=scalar(read(m,matrix+0xc));
    sum=add(zterm,yterm);sum=add(sum,mul(xbasis,x));sink(2,word(sum),0x1451ee);
}
template<class Input,class Sink> void inverse(const SourcePhysicsContactMemory &m,
    std::uint32_t matrix,const Input &input,const Sink &sink) {
    if(equal(scalar(read(m,matrix)),scalar(0))) {sink(0,0,0x1452df);sink(1,0,0x1452e3);sink(2,0,0x1452e8);return;}
    const auto scale=scalar(read(m,matrix)),one=scalar(read(m,0x46a0d0));
    auto x=scalar(input(0)),y=scalar(input(1)),z=scalar(input(2));
    x=sub(x,scalar(read(m,matrix+0x28)));y=sub(y,scalar(read(m,matrix+0x2c)));z=sub(z,scalar(read(m,matrix+0x30)));
    if(!equal(scale,one)){const auto reciprocal=divide(one,scale);x=mul(reciprocal,x);y=mul(reciprocal,y);z=mul(reciprocal,z);}
    auto zterm=scalar(read(m,matrix+0xc)),yterm=scalar(read(m,matrix+8));zterm=mul(zterm,z);yterm=mul(yterm,y);
    auto sum=add(zterm,yterm);sum=add(sum,mul(scalar(read(m,matrix+4)),x));sink(0,word(sum),0x145288);
    zterm=scalar(read(m,matrix+0x18));yterm=scalar(read(m,matrix+0x14));zterm=mul(zterm,z);yterm=mul(yterm,y);
    sum=add(zterm,yterm);sum=add(sum,mul(scalar(read(m,matrix+0x10)),x));sink(1,word(sum),0x1452af);
    zterm=mul(scalar(read(m,matrix+0x24)),z);yterm=mul(scalar(read(m,matrix+0x20)),y);const auto xbasis=scalar(read(m,matrix+0x1c));
    sum=add(zterm,yterm);sum=add(sum,mul(xbasis,x));sink(2,word(sum),0x1452d7);
}
template<class Input,class Sink> bool resolve(const SourcePhysicsContactMemory &memory,
    std::uint16_t frame,const Input &input,bool invert,NativeTu15PhysicsFrameFp &fp,const Sink &sink) {
    if(frame==0xffffu){copy(input,sink,invert?std::array<std::uint32_t,3>{0x218a2a,0x218a2f,0x218a35}:std::array<std::uint32_t,3>{0x21894b,0x218950,0x218956});return true;}
    const auto frames=read(memory,0x505e78),row=frames+std::uint32_t(std::int32_t(std::int16_t(frame)))*16u;
    const auto object=read(memory,row);
    if(object==0xffffffffu){copy(input,sink,invert?std::array<std::uint32_t,3>{0x218a11,0x218a16,0x218a1e}:std::array<std::uint32_t,3>{0x218934,0x218939,0x21893f});return false;}
    const auto pool=read(memory,0x4ecbd0),rows=read(memory,pool+0x44),body=read(memory,rows+(object&0xffffu)*12u+8);
    const auto node=std::int16_t(read(memory,row+6,2));
    if(node>=0){const auto bytes=std::uint32_t(std::int32_t(std::int16_t(read(memory,body+0x114,2))));
        const auto count=std::uint32_t((std::uint64_t(bytes)*0x4ec4ec4full)>>36u);
        if(std::uint32_t(node)<count){std::array<std::uint32_t,3> local{};
            const auto local_sink=[&](unsigned n,std::uint32_t value,std::uint32_t){local[n]=value;};
            if(invert){const auto offset=std::int16_t(read(memory,body+0x116,2));const auto matrix=body+std::uint32_t(std::int32_t(offset))+std::uint32_t(node)*52u;
                {SimdEnvironment environment(fp);inverse(memory,matrix,input,local_sink);}
                const auto variant=std::uint8_t(read(memory,row+0xb,1));const auto axis=std::int16_t(std::int8_t(read(memory,row+0xa,1)));
                permutation(memory,[&](std::uint32_t n){if(n>=3)throw std::runtime_error("Native private inverse point selector outside XYZ");return local[n];},axis,variant,true,sink);
            }else{const auto variant=std::uint8_t(read(memory,row+0xb,1));const auto axis=std::int16_t(std::int8_t(read(memory,row+0xa,1)));
                permutation(memory,input,axis,variant,false,local_sink);
                const auto offset=std::int16_t(read(memory,body+0x116,2));const auto matrix=body+std::uint32_t(std::int32_t(offset))+std::uint32_t(node)*52u;
                SimdEnvironment environment(fp);direction(memory,matrix,[&](unsigned n){return local[n];},sink);}
            return true;}}
    copy(input,sink,invert?std::array<std::uint32_t,3>{0x2189f8,0x2189fd,0x218a05}:std::array<std::uint32_t,3>{0x21891c,0x218921,0x218928});return false;
}
#endif
}
void native_campaign_ai_inverse_permute_point_tu15(SourcePhysicsContactMemory &memory,std::uint32_t source,std::uint32_t destination,std::int16_t axis,std::uint8_t variant,const SourcePhysicsContactRecordObserver &observe) {
    permutation(memory,[&](std::uint32_t n){return read(memory,source+n*4u);},axis,variant,true,[&](unsigned n,std::uint32_t value,std::uint32_t site){publish(memory,observe,site,destination+n*4u,value);});
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
void native_campaign_ai_transform_direction_tu15(SourcePhysicsContactMemory &m,std::uint32_t matrix,std::uint32_t source,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    SimdEnvironment environment(fp);direction(m,matrix,[&](unsigned n){return read(m,source+n*4u);},[&](unsigned n,std::uint32_t value,std::uint32_t site){publish(m,observe,site,destination+n*4u,value);});
}
void native_campaign_ai_inverse_point_tu15(SourcePhysicsContactMemory &m,std::uint32_t matrix,std::uint32_t source,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    SimdEnvironment environment(fp);inverse(m,matrix,[&](unsigned n){return read(m,source+n*4u);},[&](unsigned n,std::uint32_t value,std::uint32_t site){publish(m,observe,site,destination+n*4u,value);});
}
bool native_campaign_ai_resolve_direction_tu15(SourcePhysicsContactMemory &m,std::uint16_t frame,std::uint32_t source,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    return resolve(m,frame,[&](std::uint32_t n){return read(m,source+n*4u);},false,fp,[&](unsigned n,std::uint32_t value,std::uint32_t site){publish(m,observe,site,destination+n*4u,value);});
}
bool native_campaign_ai_inverse_frame_point_tu15(SourcePhysicsContactMemory &m,std::uint16_t frame,std::uint32_t source,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    return resolve(m,frame,[&](std::uint32_t n){return read(m,source+n*4u);},true,fp,[&](unsigned n,std::uint32_t value,std::uint32_t site){publish(m,observe,site,destination+n*4u,value);});
}
NativeCampaignAiFramePointValueTu15 native_campaign_ai_inverse_frame_point_value_tu15(const SourcePhysicsContactMemory &m,std::uint16_t frame,std::uint32_t source,NativeTu15PhysicsFrameFp &fp) {
    NativeCampaignAiFramePointValueTu15 result{};result.resolved=resolve(m,frame,[&](std::uint32_t n){return read(m,source+n*4u);},true,fp,[&](unsigned n,std::uint32_t value,std::uint32_t){result.point[n]=value;});return result;
}
void native_campaign_ai_framed_difference_tu15(SourcePhysicsContactMemory &m,std::uint32_t first,std::uint32_t second,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    const auto first_frame=read(m,first+0xc,2),second_frame=read(m,second+0xc,2);
    if(first_frame==second_frame){
        {SimdEnvironment environment(fp);for(unsigned n=0;n<3;++n){auto value=scalar(read(m,second+n*4u));value=sub(value,scalar(read(m,first+n*4u)));publish(m,observe,std::array<std::uint32_t,3>{0x218ecb,0x218ed9,0x218eea}[n],destination+n*4u,word(value));}}
        const auto frame=std::uint16_t(read(m,first+0xc,2));if(frame!=0xffffu)(void)native_campaign_ai_resolve_direction_tu15(m,frame,destination,destination,fp,observe);
    }else{
        const auto a=native_campaign_ai_project_point_value_tu15(m,first,fp),b=native_campaign_ai_project_point_value_tu15(m,second,fp);
        SimdEnvironment environment(fp);for(unsigned n=0;n<3;++n)publish(m,observe,std::array<std::uint32_t,3>{0x218f26,0x218f36,0x218f47}[n],destination+n*4u,word(sub(scalar(b[n]),scalar(a[n]))));
    }
}
#else
#error Native campaign frame geometry requires its exact hardware SIMD backend
#endif
}
