#include "halo2/native_campaign_object_markers_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned width=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));m.write(p,std::span(bytes).first(width));
}
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::runtime_error("Native marker geometry requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 load(const SourcePhysicsContactMemory &m,std::uint32_t p){return scalar(read(m,p));}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 div(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 packed_add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 packed_mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(__m128 a,__m128 b){unsigned char result{};asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
__m128 broadcast(__m128 value){return _mm_shuffle_ps(value,value,0);}
// MOVSS followed by MOVHPS: X,zero,Y,Z, including the real eight-byte read.
__m128 vector(const SourcePhysicsContactMemory &m,std::uint32_t p) {
    const auto x=read(m,p);std::array<std::byte,8> yz{};m.read(p+4u,yz);
    std::array<std::uint32_t,4> lanes{x,0,0,0};
    for(unsigned n=0;n<8;++n)lanes[2u+n/4u]|=std::to_integer<unsigned>(yz[n])<<((n%4u)*8u);
    return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(lanes.data())));
}
void publish_vector(SourcePhysicsContactMemory &m,std::uint32_t p,__m128 value) {
    std::array<std::uint32_t,4> lanes{};_mm_storeu_si128(reinterpret_cast<__m128i*>(lanes.data()),_mm_castps_si128(value));write(m,p,lanes[0]);
    std::array<std::byte,8> yz{};for(unsigned n=0;n<8;++n)yz[n]=std::byte(lanes[2u+n/4u]>>((n%4u)*8u));m.write(p+4u,yz);
}
}
NativeCampaignObjectMarkerRetainedTu15::NativeCampaignObjectMarkerRetainedTu15(SourcePhysicsContactMemory &m,std::uint32_t base):memory_(m),base_(base),record_(base){}
void NativeCampaignObjectMarkerRetainedTu15::select(std::uint32_t n){record_=base_+n*0x70u;}
std::uint16_t NativeCampaignObjectMarkerRetainedTu15::node()const{return std::uint16_t(read(memory_,record_,2));}
void NativeCampaignObjectMarkerRetainedTu15::node(std::uint16_t value){write(memory_,record_,value,2);}
std::uint32_t NativeCampaignObjectMarkerRetainedTu15::scalar(NativeCampaignObjectMarkerPoseTu15 pose,unsigned field)const{return read(memory_,record_+(pose==NativeCampaignObjectMarkerPoseTu15::local?4u:0x38u)+field*4u);}
void NativeCampaignObjectMarkerRetainedTu15::scalar(NativeCampaignObjectMarkerPoseTu15 pose,unsigned field,std::uint32_t value){write(memory_,record_+(pose==NativeCampaignObjectMarkerPoseTu15::local?4u:0x38u)+field*4u,value);}
void NativeCampaignObjectMarkerRetainedTu15::vector(NativeCampaignObjectMarkerPoseTu15 pose,unsigned first,const std::array<std::uint32_t,3>&value){
    const auto packed=_mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(value[2]),std::bit_cast<std::int32_t>(value[1]),0,std::bit_cast<std::int32_t>(value[0])));
    publish_vector(memory_,record_+(pose==NativeCampaignObjectMarkerPoseTu15::local?4u:0x38u)+first*4u,packed);
}
void NativeCampaignObjectMarkerRetainedTu15::radius(std::uint32_t value){write(memory_,record_+0x6cu,value);}
NativeCampaignObjectMarkerLocalTu15::NativeCampaignObjectMarkerLocalTu15(NativeCampaignObjectMarkerValueTu15 &value):value_(value){}
void NativeCampaignObjectMarkerLocalTu15::select(std::uint32_t n){if(n)throw std::logic_error("A private marker value requires the real one-record capacity");}
std::uint16_t NativeCampaignObjectMarkerLocalTu15::node()const{return value_.node;}
void NativeCampaignObjectMarkerLocalTu15::node(std::uint16_t value){value_.node=value;}
std::uint32_t NativeCampaignObjectMarkerLocalTu15::scalar(NativeCampaignObjectMarkerPoseTu15 pose,unsigned field)const{return (pose==NativeCampaignObjectMarkerPoseTu15::local?value_.local:value_.world).at(field);}
void NativeCampaignObjectMarkerLocalTu15::scalar(NativeCampaignObjectMarkerPoseTu15 pose,unsigned field,std::uint32_t value){(pose==NativeCampaignObjectMarkerPoseTu15::local?value_.local:value_.world).at(field)=value;}
void NativeCampaignObjectMarkerLocalTu15::vector(NativeCampaignObjectMarkerPoseTu15 pose,unsigned first,const std::array<std::uint32_t,3>&value){for(unsigned n=0;n<3;++n)scalar(pose,first+n,value[n]);}
void NativeCampaignObjectMarkerLocalTu15::radius(std::uint32_t value){value_.radius=value;}
std::uint32_t native_campaign_object_marker_index_tu15(const SourcePhysicsContactMemory &m,
    std::uint32_t model,std::uint32_t marker) {
    if(model==none || marker==none || !marker)return none;
    const auto tags=read(m,0x4f0414u),tag=read(m,tags+(model&0xffffu)*16u+8u);
    const auto count=std::bit_cast<std::int32_t>(read(m,tag+0x58u));
    if(count<=0)return none;
    auto group=read(m,tag+0x5cu);
    for(std::int32_t n=0;n<count;++n,group+=12u)if(read(m,group)==marker)return std::uint32_t(n);
    return none;
}
namespace {
template<class Publish> void quaternion_basis(const SourcePhysicsContactMemory &m,
    std::uint32_t q,NativeTu15PhysicsFrameFp &fp,Publish &publish) {
    const auto y=load(m,q+4u),z=load(m,q+8u),w=load(m,q+12u),x=load(m,q);
    SimdEnvironment environment(fp);
    const auto yy0=mul(y,y),xx0=mul(x,x),zz0=mul(z,z);
    auto norm=add(xx0,yy0);norm=add(norm,zz0);const auto ww0=mul(w,w);norm=add(norm,ww0);
    auto factor=scalar(0);
    if(above(norm,load(m,0x46a0ecu)))factor=div(load(m,0x46a11cu),norm);
    const auto qx=load(m,q),qy=load(m,q+4u),qy_again=load(m,q+4u),one=load(m,0x46a0d0u);
    const auto scaled_x=mul(qx,factor),scaled_y=mul(qy,factor),scaled_z=mul(factor,load(m,q+8u));
    const auto zw=mul(scaled_z,load(m,q+12u)),xw=mul(scaled_x,load(m,q+12u));
    const auto xx=mul(load(m,q),scaled_x),xy=mul(load(m,q),scaled_y),xz=mul(load(m,q),scaled_z);
    const auto yy=mul(qy_again,scaled_y);const auto fresh_y=load(m,q+4u);
    const auto yw=mul(scaled_y,load(m,q+12u)),yz=mul(fresh_y,scaled_z),zz=mul(scaled_z,load(m,q+8u));
    const auto yy_zz=add(zz,yy),xx_yy=add(yy,xx);
    publish(0u,bits(sub(one,yy_zz)));
    const auto xy_minus_zw=sub(xy,zw),xy_plus_zw=add(xy,zw),fresh_one=load(m,0x46a0d0u);
    publish(12u,bits(xy_minus_zw));publish(4u,bits(xy_plus_zw));
    const auto xx_zz=add(zz,xx),xz_plus_yw=add(xz,yw);
    publish(24u,bits(xz_plus_yw));
    const auto middle=sub(fresh_one,xx_zz),yz_minus_xw=sub(yz,xw),xz_minus_yw=sub(xz,yw),yz_plus_xw=add(yz,xw),last=sub(fresh_one,xx_yy);
    publish(16u,bits(middle));publish(28u,bits(yz_minus_xw));publish(8u,bits(xz_minus_yw));publish(20u,bits(yz_plus_xw));publish(32u,bits(last));
}
}
void native_campaign_object_quaternion_basis_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t q,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp) {
    auto publish=[&](unsigned offset,std::uint32_t value){write(m,destination+offset,value);};
    quaternion_basis(m,q,fp,publish);
}
namespace {
template<class Right,class Store,class StoreVector> void transform_product(
    const SourcePhysicsContactMemory &m,std::uint32_t left,Right &right,
    NativeTu15PhysicsFrameFp &fp,Store &store,StoreVector &store_vector) {
    const auto left_scale=load(m,left),right_scale=scalar(right(0)),px=scalar(right(0x28u)),py=scalar(right(0x2cu)),pz=scalar(right(0x30u));
    SimdEnvironment environment(fp);const auto scale=mul(right_scale,left_scale);
    const auto forward=vector(m,left+4u),side=vector(m,left+0x10u),up=vector(m,left+0x1cu);
    auto position=packed_mul(broadcast(px),forward);const auto side_position=packed_mul(broadcast(py),side),up_position=packed_mul(broadcast(pz),up);
    position=packed_add(position,side_position);position=packed_add(position,up_position);position=packed_mul(position,broadcast(left_scale));
    position=packed_add(position,vector(m,left+0x28u));
    // The right Z component is read before the preceding translation store.
    auto z=scalar(right(0x24u));store_vector(0x28u,position);
    for(const auto offset:{0x1cu,0x10u,4u}) {
        const auto y=scalar(right(offset+4u)),x=scalar(right(offset));
        const auto z_term=packed_mul(broadcast(z),up),y_term=packed_mul(broadcast(y),side),x_term=packed_mul(broadcast(x),forward);
        const auto result=packed_add(x_term,packed_add(y_term,z_term));
        if(offset!=4u)z=scalar(right(offset-4u));
        store_vector(offset,result);
    }
    store(0,bits(scale));
}
std::array<std::uint32_t,3> vector_bits(__m128 value) {
    std::array<std::uint32_t,4> lanes{};_mm_storeu_si128(reinterpret_cast<__m128i*>(lanes.data()),_mm_castps_si128(value));
    return {lanes[0],lanes[2],lanes[3]};
}
}
void native_campaign_object_transform_product_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t left,std::uint32_t right,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp) {
    auto source=[&](unsigned offset){return read(m,right+offset);};
    auto store=[&](unsigned offset,std::uint32_t value){write(m,destination+offset,value);};
    auto store_vector=[&](unsigned offset,__m128 value){publish_vector(m,destination+offset,value);};
    transform_product(m,left,source,fp,store,store_vector);
}
std::uint32_t native_campaign_object_marker_transforms_tu15(const SourcePhysicsContactMemory &m,
    std::uint32_t model,std::uint32_t group,std::uint32_t variants,std::uint32_t node_indices,
    std::uint32_t nodes,bool mirrored,NativeCampaignObjectMarkerStorageTu15 &target,std::int32_t capacity,NativeTu15PhysicsFrameFp &fp) {
    if(group==none)return 0;
    const auto tags=read(m,0x4f0414u),tag=read(m,tags+(model&0xffffu)*16u+8u);
    const auto groups=read(m,tag+0x5cu),block=groups+group*12u;
    if(std::bit_cast<std::int32_t>(read(m,block+4u))<=0)return 0;
    std::uint32_t count{},offset{},candidate{};
    do {
        const auto row=read(m,block+8u)+offset;
        bool admitted=true;
        if(variants) {
            const auto region=read(m,row,1);
            if(region!=0xffu) {
                const auto signed_region=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(region)));
                const auto selected_variant=read(m,variants+std::uint32_t(signed_region),1);
                admitted=selected_variant==read(m,row+1u,1);
            }
        }
        if(admitted) {
            if(std::bit_cast<std::int32_t>(count)>=capacity)break;
            auto node=read(m,row+2u,1);if(node_indices)node=read(m,node_indices+node*4u);
            target.select(count);++count;target.node(std::uint16_t(node));
            constexpr auto local=NativeCampaignObjectMarkerPoseTu15::local;
            constexpr auto world=NativeCampaignObjectMarkerPoseTu15::world;
            auto publish_basis=[&](unsigned offset,std::uint32_t value){target.scalar(local,1u+offset/4u,value);};
            quaternion_basis(m,row+0x10u,fp,publish_basis);
            target.scalar(local,10,0);target.scalar(local,11,0);target.scalar(local,12,0);
            target.scalar(local,0,read(m,0x46a0d0u));
            for(unsigned n=0;n<3;++n)target.scalar(local,10u+n,read(m,row+4u+n*4u));
            const auto selected=std::int32_t(std::bit_cast<std::int16_t>(target.node()));
            auto right=[&](unsigned offset){return target.scalar(local,offset/4u);};
            auto store=[&](unsigned offset,std::uint32_t value){target.scalar(world,offset/4u,value);};
            auto store_vector=[&](unsigned offset,__m128 value){target.vector(world,offset/4u,vector_bits(value));};
            transform_product(m,nodes+std::uint32_t(selected)*0x34u,right,fp,store,store_vector);
            target.radius(read(m,row+0x20u));
            if(mirrored) {
                SimdEnvironment environment(fp);
                for(const auto field:{4u,5u,6u})target.scalar(world,field,bits(sub(scalar(0),scalar(target.scalar(world,field)))));
            }
        }
        ++candidate;offset+=0x24u;
    }while(std::bit_cast<std::int32_t>(candidate)<std::bit_cast<std::int32_t>(read(m,block+4u)));
    return count;
}
std::uint32_t native_campaign_object_marker_transforms_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t model,std::uint32_t group,std::uint32_t variants,std::uint32_t node_indices,
    std::uint32_t nodes,bool mirrored,std::uint32_t destination,std::int32_t capacity,NativeTu15PhysicsFrameFp &fp) {
    NativeCampaignObjectMarkerRetainedTu15 target(m,destination);
    return native_campaign_object_marker_transforms_tu15(m,model,group,variants,node_indices,nodes,mirrored,target,capacity,fp);
}

}
