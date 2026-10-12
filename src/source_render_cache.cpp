#include "halo2/source_render_cache.hpp"
#include "halo2/object_lighting.hpp"
#include <bit>
#include <cstring>
#include <limits>

namespace halo2 {
namespace {
void byte(std::span<std::uint32_t> words,unsigned offset,std::uint8_t value) {
    const unsigned shift=(offset&3)*8;
    words[offset/4]=(words[offset/4]&~(0xffu<<shift))|(std::uint32_t(value)<<shift);
}
bool occupied(const SourceRenderCachePool& p,unsigned i) {
    return (p.occupied[i/32]>>(i&31))&1;
}
void validate(const SourceRenderCachePool& p) {
    if(p.highwater>p.capacity || p.cursor>p.capacity || p.active_count>p.capacity)
        throw FormatError("source render cache pool input exceeds its 256 records");
}
}
void source_render_context_initialize(SourceRenderContext& c) {
    // Original fixed initializer with FPCW027F, including source static
    // direction constants. Unwritten +0C/colors/+75/high-half +60 survive.
    constexpr std::array<std::uint32_t,20> lighting={
        0x3db851ec,0x3db851ec,0x3db851ec,0x3e9e3779,0,0xbf737871,
        0x3ecccccd,0x3ecccccd,0x3df5c28f,0x3df5c28f,0x3df5c28f,
        0x3e9e3779,0,0xbf737871,0x3db851ec,0x3db851ec,0x3db851ec,
        0xbe9e3779,0,0x3f737871};
    c.words[0]=0xffffffff;c.words[1]=0;c.words[2]=0xffffffff;
    std::copy(lighting.begin(),lighting.end(),c.words.begin()+4);
    c.words[24]|=0xffff;
    byte(c.words,0x74,0);byte(c.words,0x76,0);byte(c.words,0x77,0);
}
void SourceRenderCachePool::activate(bool debug_fill) {
    active=true;cursor=highwater=active_count=0;next_generation=0xe163;
    for(auto& record:records) {
        if(debug_fill)record.words.fill(0xbabababa);
        record.words[0]&=0xffff0000;
    }
    occupied.fill(0);
}
SourceRenderCacheRecord* SourceRenderCachePool::owner_record(std::uint32_t owner,std::uint32_t token) {
    if(token==0xffffffff)return nullptr;
    const unsigned index=token&0xffff;
    if(index>=capacity)throw FormatError("source render cache token exceeds its allocated pool");
    return records[index].owner()==owner?&records[index]:nullptr;
}
std::uint32_t SourceRenderCachePool::acquire(std::uint32_t owner,std::uint32_t& token,
    std::span<const std::uint8_t> regions,std::span<const Vec3> rgb,std::uint32_t frame) {
    validate(*this);
    if(owner==0xffffffff || regions.size()>16 || rgb.size()>4)
        throw FormatError("source render cache owner/regions/colors outside original bounds");
    if(owner_record(owner,token))return token;
    std::uint32_t index=0xffffffff;
    for(auto i=cursor;i<highwater;++i)if(!halo2::occupied(*this,i)){index=i;break;}
    if(index==0xffffffff && highwater<capacity)index=highwater;
    if(index!=0xffffffff) {
        occupied[index/32]|=1u<<(index&31);++active_count;cursor=index+1;
        if(highwater<=index)highwater=index+1;
        records[index].words.fill(0);records[index].words[0]=next_generation;
        ++next_generation;if(next_generation==0xffff)next_generation=0x8000;
    } else {
        float oldest=-std::numeric_limits<float>::max();
        for(unsigned i=0;i<highwater;++i)if(halo2::occupied(*this,i)) {
            float age=float(std::bit_cast<std::int32_t>(frame-records[i].last_render_frame()));
            if(age<0)age=1000.0f;
            if(age>oldest){oldest=age;index=i;}
        }
        if(index==0xffffffff)return 0xffffffff;
    }
    auto& record=records[index];
    const auto result=((record.words[0]&0xffff)<<16)|index;
    record.words[0]&=0xffff;record.words[1]=owner;
    record.words[2]=record.words[3]=0xffffffff;
    SourceRenderContext context;
    std::copy_n(record.words.begin()+0x44/4,30,context.words.begin());
    source_render_context_initialize(context);
    context.words[0]=owner;context.words[1]=0xbe8b0;
    std::copy(context.words.begin(),context.words.end(),record.words.begin()+0x44/4);
    record.words[0xbc/4]=0xffffffff;
    for(unsigned view=0;view<4;++view)
        for(unsigned r=0;r<regions.size();++r)byte(record.words,0xc0+view*16+r,regions[r]);
    byte(record.words,0xb8,0);
    for(unsigned color=0;color<rgb.size();++color)
        record.words[0xa8/4+color]=pack_object_lighting_rgb(rgb[color]);
    byte(record.words,0xb8,std::uint8_t(rgb.size()));
    token=result;return result;
}
void SourceRenderCachePool::mark_rendered(std::uint32_t owner,std::uint32_t token,std::uint32_t frame) {
    if(auto* r=owner_record(owner,token))r->words[3]=frame;
}
std::int8_t SourceRenderCachePool::view_lod(std::uint32_t owner,std::uint32_t token,std::int32_t view) {
    if(view<0 || view>=4)return -1;
    if(auto* r=owner_record(owner,token))
        return std::bit_cast<std::int8_t>(std::uint8_t(r->words[0xbc/4]>>(view*8)));
    return -1;
}
void SourceRenderCachePool::set_view_lod(std::uint32_t owner,std::uint32_t& token,
    std::span<const std::uint8_t> regions,std::span<const Vec3> rgb,
    std::uint32_t frame,std::int32_t view,std::int8_t lod) {
    if(view<0 || view>=4)return;
    const auto result=acquire(owner,token,regions,rgb,frame);
    if(result==0xffffffff)throw FormatError("source 4C640 requires an allocated render-cache record");
    byte(records[result&0xffff].words,0xbc+unsigned(view),std::uint8_t(lod));
}
std::uint32_t source_render_geometry_stamp(std::uint32_t frame,std::int32_t view) {
    return (frame&0x3fffffff)|(std::uint32_t(view)<<30);
}
void source_render_frame_begin(std::uint32_t& frame,std::uint32_t& cursor) { ++frame;cursor=0; }
SourceRenderResourceDecision source_render_resource_decision(std::int16_t requested,std::int8_t prior,
    bool initial,bool fallback) {
    SourceRenderResourceDecision out;out.actual_lod=requested;
    out.fallback_requested=!initial && prior!=-1;
    out.succeeded=initial || (out.fallback_requested && fallback);
    if(out.fallback_requested) {
        if(fallback)out.actual_lod=prior;
        else out.cache_lod_writes[out.cache_lod_write_count++]=-1;
    } else {
        out.cache_lod_writes[out.cache_lod_write_count++]=std::int8_t(requested);
        if(!initial)out.cache_lod_writes[out.cache_lod_write_count++]=-1;
    }
    return out;
}
SourceRenderGeometryBegin source_render_geometry_begin(bool present,std::uint32_t token,
    std::uint32_t stamp,std::int8_t prior,std::int8_t requested,std::uint32_t frame,std::int32_t view) {
    const bool reuse=prior!=-1 && present && token!=0xffffffff && stamp==source_render_geometry_stamp(frame,view);
    return {reuse,reuse?std::int16_t(prior):std::int16_t(requested)};
}
SourceRenderGeometryResult source_render_geometry_allocate(const SourceRenderGeometryInput& in,std::uint32_t& cursor) {
    SourceRenderGeometryResult out;
    out.context_token=in.context_token;out.context_stamp=in.context_stamp;
    out.reused=in.begin.reuse_candidate && !in.region_changed && in.actual_lod==in.requested_lod;
    if(!in.resource_succeeded || (in.source_model_ordinal>0 && in.actual_lod!=in.requested_lod))return out;
    out.node_count=std::uint8_t(in.node_count);
    if(out.reused && !in.first_person) {
        out.continuation=SourceRenderGeometryContinuation::reuse_tail;
        if(in.view!=-1)out.token=in.context_token;
        return out;
    }
    std::uint32_t bytes=in.node_count>0 ? std::uint32_t(in.node_count)*48+68 :0;
    // 4B88A aligns the signed byte count to four. The node formula is already
    // divisible by four, including wrapping 32-bit cases.
    const auto end=cursor+bytes;
    if(std::bit_cast<std::int32_t>(end)>0x27000)return out;
    out.token=cursor&0x0fffffff;cursor=end;
    out.continuation=SourceRenderGeometryContinuation::build_nodes;
    if(in.context_present && !in.first_person && !in.region_changed && in.actual_lod==in.requested_lod) {
        out.context_written=true;out.context_token=out.token;
        out.context_stamp=source_render_geometry_stamp(in.frame,in.view);
    }
    return out;
}
SourceRenderGeometryTail source_render_geometry_tail(bool reused,bool geometry_success,
    bool context,bool prt,bool prt_success) {
    if(reused)return {false,false,false,geometry_success};
    if(!geometry_success)return {};
    return {prt,true,context,!prt || prt_success};
}
}
