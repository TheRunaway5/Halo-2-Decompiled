#include "halo2/native_tu15_camera_dynamic_query_leaves.hpp"
#include <cstring>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8*k);return n;}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned width=4){std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(8*k));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o){e.before.resize(width);m.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m.write(p,std::span(b).first(width));if(o)o(e);}
struct Material{std::uint16_t value;std::uint32_t store_pc;};
Material material(const SourcePhysicsContactMemory&m,std::uint32_t object,std::uint32_t index){
    const auto fallback=std::uint16_t(word(m,0x48a0b0u,2));if(object==0xffffffffu)return{fallback,0xdb08du};
    const auto objects=word(m,0x4ecbd0u),headers=word(m,objects+0x44u),unit=word(m,headers+(object&0xffffu)*12u+8u),definition=word(m,unit),tags=word(m,0x4f0414u),tag=word(m,tags+(definition&0xffffu)*16u+8u),model=word(m,tag+0x38u);
    if(model==0xffffffffu)return{fallback,0xdb08du};
    const auto model_tag=word(m,tags+(model&0xffffu)*16u+8u),materials=word(m,model_tag+0x5cu);return{std::uint16_t(word(m,materials+index*20u+0x10u,2)),0xdb088u};
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment{NativeTu15PhysicsFrameFp&fp_;unsigned previous_;public:explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native camera collision requires SSE exception owner");_mm_setcsr(fp.mxcsr);}~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}};
using F=__m128;
F scalar(std::uint32_t n){F v{};std::memcpy(&v,&n,4);return v;}
std::uint32_t bits(F v){std::uint32_t n;std::memcpy(&n,&v,4);return n;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
#endif
}
bool native_tu15_admit_camera_query_object(const SourcePhysicsContactMemory&m,
    std::uint32_t datum,std::uint32_t header,std::uint32_t object,std::uint32_t mask,std::uint32_t first,std::uint32_t second){
    if(datum==first||datum==second)return false;
    if(word(m,header+2u,1)&0x10u)return false;if(word(m,object+4u,1)&1u)return false;
    const auto kind=word(m,header+3u,1);if(!(mask&(1u<<((kind+4u)&31u))))return false;
    if((mask&0x80000u)&&(word(m,object+4u)&0x100000u))return false;
    if((mask&0x40000u)&&!(word(m,object+4u)&0x400000u))return false;
    if((mask&0x40000000u)&&!(word(m,object+4u)&0x80000000u))return false;
    if(kind==0u){if((mask&0x100000u)&&(word(m,object+0x10au,1)&4u))return false;if((mask&0x400000u)&&(word(m,object+0x348u,1)&8u))return false;}
    else if(kind==11u&&(mask&0x200000u)&&(word(m,object+0x12cu)&2u))return false;
    return true;
}
std::uint16_t native_tu15_camera_collision_material(const SourcePhysicsContactMemory&m,std::uint32_t object,std::uint32_t index){return material(m,object,index).value;}
std::uint32_t native_tu15_camera_collision_material_to(SourcePhysicsContactMemory&m,std::uint32_t object,std::uint32_t index,std::uint32_t output,const SourcePhysicsContactRecordObserver&o){const auto result=material(m,object,index);store(m,o,result.store_pc,output,result.value,2);return output;}
void native_tu15_publish_camera_collision(SourcePhysicsContactMemory&m,std::uint32_t out,
    std::uint32_t transform,std::uint32_t collision,NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&o){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp);store(m,o,0x16b9a5u,out+4u,word(m,collision));
    if(transform){
        const auto plane=word(m,collision+4u);const auto y=scalar(word(m,plane+4u)),z=scalar(word(m,plane+8u));
        auto nx=scalar(word(m,transform+28u)),middle=scalar(word(m,transform+16u));const auto x=scalar(word(m,plane));nx=mul(nx,z);middle=mul(middle,y);nx=add(nx,middle);middle=scalar(word(m,transform+4u));middle=mul(middle,x);nx=add(nx,middle);store(m,o,0x16b9e2u,out+40u,bits(nx));
        auto ny=scalar(word(m,transform+32u));middle=scalar(word(m,transform+20u));ny=mul(ny,z);middle=mul(middle,y);ny=add(ny,middle);middle=scalar(word(m,transform+8u));middle=mul(middle,x);ny=add(ny,middle);store(m,o,0x16ba0au,out+44u,bits(ny));
        auto nz=scalar(word(m,transform+36u));nz=mul(nz,z);auto term=scalar(word(m,transform+24u));term=mul(term,y);middle=scalar(word(m,transform+12u));nz=add(nz,term);middle=mul(middle,x);nz=add(nz,middle);store(m,o,0x16ba32u,out+48u,bits(nz));
        auto distance=scalar(word(m,transform+48u));distance=mul(distance,nz);auto value=scalar(word(m,plane+12u));value=mul(value,scalar(word(m,transform)));distance=add(distance,value);value=scalar(word(m,transform+44u));value=mul(value,scalar(word(m,out+44u)));distance=add(distance,value);value=scalar(word(m,out+40u));value=mul(value,scalar(word(m,transform+40u)));distance=add(distance,value);store(m,o,0x16ba69u,out+52u,bits(distance));
        const auto index=word(m,collision+16u);if(index&0x8000u){const auto zero=F{};for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16ba84u,0x16ba91u,0x16ba9eu,0x16baa8u};const auto n=sub(zero,scalar(word(m,out+40u+lane*4u)));store(m,o,pc[lane],out+40u+lane*4u,bits(n));}}
    }else{
        const auto index=word(m,collision+16u);
        if(index&0x8000u){const auto plane=word(m,collision+4u);const auto zero=F{};for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16bac3u,0x16bad0u,0x16baddu,0x16bae7u};const auto n=sub(zero,scalar(word(m,plane+lane*4u)));store(m,o,pc[lane],out+40u+lane*4u,bits(n));}}
        else{const auto plane=word(m,collision+4u);for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16baf7u,0x16bafcu,0x16bb02u,0x16bb08u};store(m,o,pc[lane],out+40u+lane*4u,word(m,plane+lane*4u));}}
    }
    for(unsigned lane=0;lane<3;++lane){constexpr std::array pc{0x16bb0fu,0x16bb15u,0x16bb1bu};store(m,o,pc[lane],out+76u+lane*4u,word(m,collision+8u+lane*4u));}
    store(m,o,0x16bb21u,out+88u,word(m,collision+20u,1),1);store(m,o,0x16bb27u,out+89u,word(m,collision+21u,1),1);store(m,o,0x16bb2eu,out+90u,word(m,collision+22u,2),2);
#else
    (void)m;(void)out;(void)transform;(void)collision;(void)fp;(void)o;throw std::runtime_error("Native camera collision backend unavailable");
#endif
}
} // namespace halo2
