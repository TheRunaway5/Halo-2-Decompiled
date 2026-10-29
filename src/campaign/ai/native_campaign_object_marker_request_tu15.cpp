#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include "halo2/native_campaign_object_markers_tu15.hpp"
#include "halo2/native_campaign_object_motion_refs_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned width=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
std::int32_t signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
void mirror(NativeCampaignObjectMarkerStorageTu15 &out,NativeTu15PhysicsFrameFp &fp) {
    if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))throw std::runtime_error("Native object marker fallback requires its SIMD trap owner");
    const auto previous=_mm_getcsr();_mm_setcsr(fp.mxcsr);
    try {
        for(const auto field:{4u,5u,6u}) {
            const auto value=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(out.scalar(NativeCampaignObjectMarkerPoseTu15::world,field))));
            auto negative=_mm_setzero_ps();asm volatile("subss %1,%0":"+x"(negative):"x"(value):"memory");
            out.scalar(NativeCampaignObjectMarkerPoseTu15::world,field,std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(negative))));
        }
    }catch(...){fp.mxcsr=_mm_getcsr();_mm_setcsr(previous);throw;}
    fp.mxcsr=_mm_getcsr();_mm_setcsr(previous);
}
}
namespace {
std::uint16_t request(const SourcePhysicsContactMemory &m,
    std::uint32_t object,bool direct,std::uint32_t marker,std::int16_t capacity,
    NativeCampaignObjectMarkerStorageTu15 &out,NativeTu15PhysicsFrameFp &fp) {
    const auto selected=direct?object:native_campaign_object_marker_owner_tu15(m,object);
    if(selected!=none) {
        const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),record=read(m,rows+(selected&0xffffu)*12u+8u);
        const auto definition=read(m,record),tags=read(m,0x4f0414u),tag=read(m,tags+(definition&0xffffu)*16u+8u),model=read(m,tag+0x38u);
        if(model!=none) {
            const auto variant_offset=signed16(read(m,record+0x11au,2)),node_offset=signed16(read(m,record+0x116u,2));
            const auto model_record=read(m,tags+(model&0xffffu)*16u+8u),flags=read(m,record+4u);
            // Original computes a quotient from this signed extent, then
            // overwrites it with the render-model tag before either use.
            (void)read(m,record+0x114u,2);
            const auto render_model=read(m,model_record+4u);
            const auto group=native_campaign_object_marker_index_tu15(m,render_model,marker);
            const auto count=native_campaign_object_marker_transforms_tu15(m,render_model,group,
                record+std::uint32_t(variant_offset),0,record+std::uint32_t(node_offset),
                (flags&0x400u)!=0,out,capacity,fp);
            if(std::uint16_t(count))return std::uint16_t(count);
        }
    }
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),one=read(m,0x46a0d0u),record=read(m,rows+(object&0xffffu)*12u+8u);
    out.select(0);out.node(0);
    constexpr auto local=NativeCampaignObjectMarkerPoseTu15::local;
    constexpr auto world=NativeCampaignObjectMarkerPoseTu15::world;
    out.scalar(local,0,one);
    for(unsigned field=1;field<13;++field)out.scalar(local,field,(field==1u||field==5u||field==9u)?one:0u);
    const auto current_rows=read(m,objects+0x44u),current=read(m,current_rows+(object&0xffffu)*12u+8u);
    const auto node_offset=signed16(read(m,current+0x116u,2));
    const auto source=current+std::uint32_t(node_offset);
    for(unsigned lane=0;lane<13;++lane)out.scalar(world,lane,read(m,source+lane*4u));
    out.radius(0);
    if(read(m,record+4u)&0x400u)mirror(out,fp);
    return marker==0u?1u:0u;
}
}
std::uint16_t native_campaign_object_marker_request_tu15(SourcePhysicsContactMemory &m,
    std::uint32_t object,bool direct,std::uint32_t marker,std::int16_t capacity,
    std::uint32_t destination,NativeTu15PhysicsFrameFp &fp) {
    NativeCampaignObjectMarkerRetainedTu15 target(m,destination);
    return request(m,object,direct,marker,capacity,target,fp);
}
NativeCampaignObjectMarkerRequestValueTu15 native_campaign_object_marker_request_value_tu15(
    const SourcePhysicsContactMemory &m,std::uint32_t object,bool direct,
    std::uint32_t marker,NativeTu15PhysicsFrameFp &fp) {
    NativeCampaignObjectMarkerRequestValueTu15 result;
    NativeCampaignObjectMarkerLocalTu15 target(result.value);
    result.count=request(m,object,direct,marker,1,target,fp);
    return result;
}
}
