#include "halo2/native_tu15_camera_collision_publication_value.hpp"
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
    Environment environment(fp);
    auto read=[&](std::uint32_t address,unsigned width=4u){return word(m,address,width);};
    auto write=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){store(m,o,pc,address,value,width);};
    native_camera_collision_publication_detail::Values view{read,write};
    native_camera_collision_publication_detail::Progress data;data.output=out;data.transform=transform;data.collision=collision;
    native_camera_collision_publication_detail::publish(view,data);
#else
    (void)m;(void)out;(void)transform;(void)collision;(void)fp;(void)o;throw std::runtime_error("Native camera collision backend unavailable");
#endif
}
} // namespace halo2
