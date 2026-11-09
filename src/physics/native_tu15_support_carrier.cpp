#include "halo2/native_tu15_support_carrier.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include "halo2/native_tu15_contact_body_update.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t n{};
    for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(b.size());m.read(p,event.before);event.written.assign(b.begin(),b.end());}
    m.write(p,b);if(o)o(event);
}
void word(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t n){
    std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));store(m,o,pc,p,b);
}
class Simd {
    NativeTu15PhysicsFrameFp&fp_;unsigned host_;
public:
    explicit Simd(NativeTu15PhysicsFrameFp&fp):fp_(fp),host_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native support carrier requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~Simd(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
__m128 scalar(std::uint32_t n){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(n)));}
std::uint32_t bits(__m128 n){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(n)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 low_mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t difference(std::uint32_t a,std::uint32_t b,NativeTu15PhysicsFrameFp&fp){
    Simd environment(fp);auto x=scalar(a);const auto y=scalar(b);
    asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);
}
__m128 row(const SourcePhysicsContactMemory&m,std::uint32_t p){
    auto out=scalar(read(m,p));std::array<std::byte,8>b{};m.read(p+4u,b);
    std::uint64_t high{};for(unsigned k=0;k<8;++k)high|=std::uint64_t(std::to_integer<unsigned>(b[k]))<<(k*8u);
    return _mm_movelh_ps(out,_mm_castsi128_ps(_mm_cvtsi64_si128(std::bit_cast<std::int64_t>(high))));
}
void publish_row(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t first_pc,std::uint32_t second_pc,std::uint32_t p,__m128 value){
    word(m,o,first_pc,p,bits(value));std::array<std::byte,16>b{};_mm_storeu_ps(reinterpret_cast<float*>(b.data()),value);
    store(m,o,second_pc,p+4u,std::span(b).subspan(8u,8u));
}
__m128 broadcast(__m128 x){return _mm_shuffle_ps(x,x,0);}
void frame_data(SourcePhysicsContactMemory&m,std::uint32_t p,const NativeTu15ModelConstraintFrame&frame){
    const auto values=std::bit_cast<std::array<std::uint32_t,13>>(frame);
    for(unsigned n=0;n<values.size();++n)word(m,{},0,p+n*4u,values[n]);
}
void copy(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t source,std::uint32_t destination,unsigned count){
    for(unsigned n=0;n<count;++n)word(m,o,pc,destination+n*4u,read(m,source+n*4u));
}
void point_velocity(SourcePhysicsContactMemory&m,std::uint32_t component,std::uint32_t slot,
    std::uint32_t point,NativeTu15SupportCarrierScratch scratch,NativeTu15PhysicsFrameFp&fp,
    NativeTu15SupportCarrierStats*stats){
    if(stats)++stats->point_queries;
    const auto rows=read(m,component+0x70u),body=read(m,rows+slot*0x60u+0x40u);
    if(read(m,body+0x40u,1)){
        if(stats)++stats->disabled_bodies;const auto zero=read(m,0x474d04u);
        copy(m,{},0,zero,scratch.velocity,3);return;
    }
    for(unsigned n=0;n<3;++n)word(m,{},0,scratch.point+n*4u,read(m,point+n*4u));
    word(m,{},0,scratch.point+12u,0);
    const auto motion=read(m,body+0x3cu),table=read(m,motion),owner=read(m,table+0x58u);
    if(owner!=0x323a10u){
        if(stats)++stats->unknown_motion_owners;
        throw std::runtime_error("Native support carrier requires qualified motion point owner "+std::to_string(owner));
    }
    native_tu15_contact_motion_point_velocity(m,motion,scratch.point,scratch.velocity,fp);
}
}
void native_tu15_support_affine_product(SourcePhysicsContactMemory&m,std::uint32_t first,
    std::uint32_t second,std::uint32_t destination,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&o){
    const auto scale=broadcast(scalar(read(m,first))),second_scale=scalar(read(m,second));
    auto px=broadcast(scalar(read(m,second+0x28u))),py=broadcast(scalar(read(m,second+0x2cu))),pz=broadcast(scalar(read(m,second+0x30u)));
    Simd environment(fp);const auto product_scale=low_mul(second_scale,scale);
    const auto forward=row(m,first+4u),side=row(m,first+0x10u),up=row(m,first+0x1cu);
    px=mul(px,forward);py=mul(py,side);pz=mul(pz,up);auto position=mul(add(add(px,py),pz),scale);
    position=add(position,row(m,first+0x28u));
    auto third=scalar(read(m,second+0x24u));
    publish_row(m,o,0x1455cdu,0x1455d2u,destination+0x28u,position);
    const auto publish=[&](unsigned offset,bool following,unsigned next_offset,
        std::uint32_t out_pc1,std::uint32_t out_pc2){
        const auto middle=scalar(read(m,second+offset+4u)),first_value=scalar(read(m,second+offset));
        auto result=add(mul(broadcast(first_value),forward),add(mul(broadcast(middle),side),mul(broadcast(third),up)));
        if(following)third=scalar(read(m,second+next_offset));
        publish_row(m,o,out_pc1,out_pc2,destination+offset,result);
    };
    publish(0x1cu,true,0x18u,0x145600u,0x145605u);
    publish(0x10u,true,0xcu,0x145633u,0x145638u);
    publish(4u,false,0,0x145661u,0x145666u);
    word(m,o,0x14566au,destination,bits(product_scale));
}
bool native_tu15_update_support_carrier(SourcePhysicsContactMemory&m,
    const NativeTu15SupportCarrierRequest&r,NativeTu15SupportCarrierScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&o,NativeTu15SupportCarrierStats*stats){
    if(stats)++stats->calls;
    if(r.component_datum==0xffffffffu){if(stats)++stats->missing_components;return false;}
    const auto manager=read(m,0x52be5cu),bank=read(m,manager+0x44u),component=bank+(r.component_datum&0xffffu)*160u;
    const auto stored=std::bit_cast<std::int8_t>(std::uint8_t(read(m,component+0x18u,1)));
    std::int32_t slot=-1;
    if(stored>=0&&std::int32_t(stored)<std::bit_cast<std::int32_t>(read(m,component+0x74u)))slot=stored;
    if(slot<0||slot>std::bit_cast<std::int32_t>(read(m,component+0x74u)-1u)){
        if(stats)++stats->invalid_body_slots;return false;
    }
    const auto frame=native_tu15_model_constraint_body_frame(m,{component},std::uint32_t(slot),fp,stats?&stats->frames:nullptr);
    frame_data(m,scratch.frame,frame);native_tu15_camera_inverse_frame(m,scratch.frame,scratch.inverse,fp);
    native_tu15_support_affine_product(m,r.previous_frame,scratch.inverse,scratch.product,fp);
    native_tu15_camera_inverse_frame(m,scratch.product,scratch.product,fp);
    copy(m,o,0x184f77u,scratch.product+4u,r.basis,9);
    point_velocity(m,component,std::uint32_t(slot),r.point,scratch,fp,stats);
    std::array<std::uint32_t,3>velocity{};for(unsigned n=0;n<3;++n)velocity[n]=read(m,scratch.velocity+n*4u);
    const auto identity=read(m,0x474d30u);
    word(m,o,0x184fb0u,r.velocity,velocity[0]);word(m,o,0x184fb2u,r.velocity+4u,velocity[1]);word(m,o,0x184fb5u,r.velocity+8u,velocity[2]);
    copy(m,o,0x184fc0u,identity+4u,r.basis,9);
    word(m,o,0x184fd4u,r.delta,difference(velocity[0],read(m,r.previous_velocity),fp));
    word(m,o,0x184fe3u,r.delta+4u,difference(velocity[1],read(m,r.previous_velocity+4u),fp));
    word(m,o,0x184ff3u,r.delta+8u,difference(velocity[2],read(m,r.previous_velocity+8u),fp));
    word(m,o,0x184ff8u,r.previous_velocity,velocity[0]);word(m,o,0x184ffau,r.previous_velocity+4u,velocity[1]);word(m,o,0x185006u,r.previous_velocity+8u,velocity[2]);
    copy(m,o,0x185009u,scratch.frame,r.current_frame,13);
    if(stats)++stats->completed;return true;
}
}
