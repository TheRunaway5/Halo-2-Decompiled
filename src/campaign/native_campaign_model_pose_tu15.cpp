#include "halo2/native_campaign_model_pose_tu15.hpp"
#include "halo2/native_campaign_object_markers_tu15.hpp"
#include <array>
#include <bit>
#include <vector>
#include <xmmintrin.h>
#include <emmintrin.h>

namespace halo2 {
NativeCampaignModelPoseCapacityRequiredTu15::NativeCampaignModelPoseCapacityRequiredTu15(
    std::uint32_t instruction):std::runtime_error("Native model pose reached its original private pending capacity"),instruction_(instruction){}
namespace {
using Transform=std::array<std::uint32_t,13>;
class Environment {
    NativeTu15PhysicsFrameFp &fp_;std::uint32_t host_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native model pose requires masked retained SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);return value;
}
std::array<std::uint32_t,2> pair(const SourcePhysicsContactMemory &memory,std::uint32_t at) {
    std::array<std::byte,8> raw{};memory.read(at,raw);std::array<std::uint32_t,2> out{};
    for(unsigned n=0;n<8;++n)out[n/4u]|=std::to_integer<unsigned>(raw[n])<<((n%4u)*8u);return out;
}
std::array<std::uint32_t,4> mask(const SourcePhysicsContactMemory &memory,std::uint32_t at) {
    std::array<std::byte,16> raw{};memory.read(at,raw);std::array<std::uint32_t,4> out{};
    for(unsigned n=0;n<16;++n)out[n/4u]|=std::to_integer<unsigned>(raw[n])<<((n%4u)*8u);return out;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t at,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,at,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(at,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(at,event.written);if(observe)observe(event);
}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
__m128 packed(const std::array<std::uint32_t,4>& value){return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(value.data())));}
std::array<std::uint32_t,4> lanes(__m128 value){std::array<std::uint32_t,4> out;_mm_storeu_si128(reinterpret_cast<__m128i*>(out.data()),_mm_castps_si128(value));return out;}
std::uint32_t bits(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 load(const SourcePhysicsContactMemory &m,std::uint32_t at){return scalar(read(m,at));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 packed_mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 packed_add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2,%1;seta %0":"=qm"(out):"x"(a),"x"(b):"cc","memory");return out!=0;}
__m128 reciprocal(__m128 value){__m128 out{};asm volatile("rcpss %1,%0":"+x"(out):"x"(value):"memory");return out;}
// Actual144BF0. Stores are private values, but source operands remain lazy:
// callers may alias their forward/up/position records with later outputs.
Transform root_basis(const SourcePhysicsContactMemory &m,std::uint32_t position,
    std::uint32_t forward,std::uint32_t up,NativeTu15PhysicsFrameFp &fp) {
    Environment environment(fp);Transform out{};
    out[0]=read(m,0x46a0d0u);
    out[1]=read(m,forward);out[2]=read(m,forward+4u);out[3]=read(m,forward+8u);
    auto side_z=load(m,forward+4u),first=load(m,forward);
    first=mul(first,load(m,up+4u));side_z=mul(side_z,load(m,up));
    auto second=load(m,forward+8u);second=mul(second,load(m,up));
    auto third=load(m,up+8u);third=mul(third,load(m,forward+4u));
    side_z=sub(side_z,first);
    auto side_y=load(m,up+8u);side_y=mul(side_y,load(m,forward));side_y=sub(side_y,second);
    auto side_x=load(m,forward+8u);side_x=mul(side_x,load(m,up+4u));
    out[6]=bits(side_z);out[5]=bits(side_y);out[4]=bits(sub(side_x,third));
    out[7]=read(m,up);out[8]=read(m,up+4u);out[9]=read(m,up+8u);
    out[10]=read(m,position);out[11]=read(m,position+4u);out[12]=read(m,position+8u);
    return out;
}
// Actual144CF0 local pose conversion. The normalization pair is the real
// previous product's forward side/up YZ contribution, consumed by the packed
// normalization arithmetic even though only its low-lane sum is a value.
Transform local_transform(SourcePhysicsContactMemory &m,std::uint32_t pose,
    const std::array<std::uint32_t,2>& normalization_pair,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    Environment environment(fp);Transform out{};
    if(!(read(m,0x56e684u,1)&1u)) {
        const auto flags=read(m,0x56e684u),epsilon=read(m,0x46a0ecu);
        publish(m,observe,0x144d12,0x56e684u,flags|1u);
        publish(m,observe,0x144d18,0x56e680u,epsilon);
    }
    const auto low=pair(m,pose),high=pair(m,pose+8u);
    const auto quaternion=packed({low[0],low[1],high[0],high[1]});
    const auto squares=packed_mul(quaternion,quaternion);const auto square=lanes(squares);
    auto norm=packed_add(squares,packed({square[2],square[3],normalization_pair[0],normalization_pair[1]}));
    norm=add(norm,_mm_shuffle_ps(norm,norm,0x55));
    if(!above(norm,load(m,0x56e680u))) {
        const auto one=read(m,0x46a0d0u);
        out[1]=one;out[5]=one;out[9]=one;
    } else {
        const auto estimate=reciprocal(norm);
        auto correction=mul(norm,estimate);correction=mul(correction,estimate);
        auto gain=add(estimate,estimate);gain=sub(gain,correction);gain=add(gain,gain);
        const auto normalized=packed_mul(quaternion,_mm_shuffle_ps(gain,gain,0));
        auto x=load(m,pose);x=_mm_shuffle_ps(x,x,0);x=packed_mul(x,normalized);
        auto w=load(m,pose+12u);w=_mm_shuffle_ps(w,w,0);w=packed_mul(w,normalized);
        auto y=load(m,pose+4u);y=_mm_shuffle_ps(y,y,0);y=packed_mul(y,normalized);
        const auto normalized_high=lanes(normalized);
        const auto zz=mul(scalar(normalized_high[2]),load(m,pose+8u));
        const auto signs=mask(m,0x45f490u);
        const auto cross=packed_add(_mm_shuffle_ps(x,x,0x99),
            _mm_xor_ps(_mm_shuffle_ps(w,w,0x66),packed(signs)));
        const auto mixed=lanes(cross);
        out[2]=mixed[2];out[3]=mixed[3];out[4]=mixed[0];out[6]=mixed[0];out[7]=mixed[1];
        const auto y_lanes=lanes(y);const auto repeated_y=_mm_shuffle_ps(y,y,0x55);
        const auto x_lanes=lanes(x),w_lanes=lanes(w);
        const auto one_minus_zz=sub(load(m,0x46a0d0u),zz);
        out[1]=bits(sub(one_minus_zz,repeated_y));
        out[5]=bits(sub(one_minus_zz,scalar(x_lanes[0])));
        auto last=sub(load(m,0x46a0d0u),scalar(x_lanes[0]));last=sub(last,repeated_y);out[9]=bits(last);
        out[8]=bits(sub(scalar(y_lanes[2]),scalar(w_lanes[0])));
        out[6]=bits(add(scalar(y_lanes[2]),scalar(w_lanes[0])));
    }
    out[0]=read(m,pose+28u);
    const auto translation=pair(m,pose+16u);out[10]=translation[0];out[11]=translation[1];
    out[12]=read(m,pose+24u);return out;
}
}
void native_campaign_model_pose_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t model,std::uint32_t destination,std::uint32_t poses,
    std::uint32_t position,std::uint32_t forward,std::uint32_t up,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    const auto root=root_basis(memory,position,forward,up,fp);
    if(std::bit_cast<std::int32_t>(read(memory,model+0x48u))<=0)return;
    std::vector<std::int32_t> pending{0};std::array<std::uint32_t,2> normalization_pair{};
    for(std::size_t visited=0;visited<pending.size();++visited) {
        const auto node=pending[visited];
        const auto records=read(memory,model+0x4cu),record=records+std::uint32_t(node)*0x60u;
        std::uint32_t parent{};
        if(node)parent=destination+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(memory,record+4u,2)))))*0x34u;
        const auto local=local_transform(memory,poses+std::uint32_t(node)*0x20u,normalization_pair,fp,observe);
        const NativeCampaignObjectTransformLocalTu15 right(local);
        const auto output=destination+std::uint32_t(node)*0x34u;
        if(node) {
            const NativeCampaignObjectTransformRetainedTu15 left(memory,parent);
            normalization_pair=native_campaign_object_transform_product_tu15(memory,left,right,output,fp,observe).forward_side_up_pair;
        } else {
            const NativeCampaignObjectTransformLocalTu15 left(root);
            normalization_pair=native_campaign_object_transform_product_tu15(memory,left,right,output,fp,observe).forward_side_up_pair;
        }
        const auto sibling=std::bit_cast<std::int16_t>(std::uint16_t(read(memory,record+8u,2)));
        if(sibling!=-1) {
            if(pending.size()==NativeCampaignModelPoseCapacityRequiredTu15::capacity())
                throw NativeCampaignModelPoseCapacityRequiredTu15(0x1708bfu);
            pending.push_back(sibling);
        }
        const auto child=std::bit_cast<std::int16_t>(std::uint16_t(read(memory,record+6u,2)));
        if(child!=-1) {
            if(pending.size()==NativeCampaignModelPoseCapacityRequiredTu15::capacity())
                throw NativeCampaignModelPoseCapacityRequiredTu15(0x1708d1u);
            pending.push_back(child);
        }
    }
}
}
