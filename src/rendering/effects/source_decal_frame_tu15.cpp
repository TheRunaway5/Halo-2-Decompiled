#include "halo2/source_decal_frame_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <set>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
bool same_vertex(const SourceDecalCacheVertex& a,const SourceDecalCacheVertex& b) {
    return a.packed_projection==b.packed_projection &&
        std::bit_cast<std::uint32_t>(a.position.x)==std::bit_cast<std::uint32_t>(b.position.x) &&
        std::bit_cast<std::uint32_t>(a.position.y)==std::bit_cast<std::uint32_t>(b.position.y) &&
        std::bit_cast<std::uint32_t>(a.position.z)==std::bit_cast<std::uint32_t>(b.position.z);
}
bool same_quad(const std::array<SourceDecalCacheVertex,4>& a,
               const std::array<SourceDecalCacheVertex,4>& b) {
    for(unsigned v=0;v<4;++v)if(!same_vertex(a[v],b[v]))return false;
    return true;
}
}
SourceDecalCacheVertex source_decal_frame_vertex_tu15(Vec3 position,std::uint32_t packed) {
    const auto component=[&](unsigned shift,unsigned width,float scale) {
        const auto raw=(packed>>shift)&((1u<<width)-1u),sign=1u<<(width-1u);
        return float(std::bit_cast<std::int32_t>((raw^sign)-sign))/scale;
    };
    return {position,{component(0,11,1023.f),component(11,11,1023.f),component(22,10,511.f)},packed};
}
void validate_source_decal_frame_tu15(const SourceDecalFrameTu15& frame) {
    if(frame.records.size()>source_decal_record_capacity_tu15 ||
       frame.allocations.size()>source_decal_record_capacity_tu15 ||
       frame.heads.size()>source_decal_head_layers_tu15*source_decal_cluster_capacity_tu15)
        throw FormatError("Source decal frame exceeds its original storage capacities");
    std::map<std::uint32_t,const SourceDecalFrameAllocationTu15*> allocations;
    std::set<unsigned> allocation_slots;
    std::size_t pages{};
    for(const auto& allocation:frame.allocations) {
        if(allocation.cache_datum==none || !(allocation.cache_datum>>16) ||
           (allocation.cache_datum&65535u)>=source_decal_record_capacity_tu15 ||
           !allocations.emplace(allocation.cache_datum,&allocation).second ||
           !allocation_slots.insert(allocation.cache_datum&65535u).second ||
           allocation.quads.size()>source_decal_page_capacity_tu15-pages)
            throw FormatError("Source decal frame has an invalid or duplicated cache allocation");
        pages+=allocation.quads.size();
        for(const auto& quad:allocation.quads)for(const auto& vertex:quad) {
            for(const auto f:{vertex.position.x,vertex.position.y,vertex.position.z})
                if(!std::isfinite(f))
                    throw FormatError("Source decal frame has an invalid world vertex");
            const auto decoded=source_decal_frame_vertex_tu15(vertex.position,vertex.packed_projection);
            for(unsigned component=0;component<3;++component)
                if(std::bit_cast<std::uint32_t>(decoded.projection[component])!=
                   std::bit_cast<std::uint32_t>(vertex.projection[component]))
                    throw FormatError("Source decal frame packed projection differs from its decoded value");
        }
    }
    std::map<std::uint32_t,const SourceDecalFrameRecordTu15*> records;
    std::set<unsigned> slots;
    std::set<std::uint32_t> used_allocations;
    for(const auto& record:frame.records) {
        const auto allocation=allocations.find(record.cache_datum);
        if(record.datum==none || !(record.datum>>16) ||
           (record.datum&65535u)>=source_decal_record_capacity_tu15 ||
           !records.emplace(record.datum,&record).second || !slots.insert(record.datum&65535u).second ||
           record.definition==none || record.layer>=source_decal_head_layers_tu15 ||
           record.cluster < -1 || record.cluster>=std::int16_t(source_decal_cluster_capacity_tu15) ||
           !std::isfinite(record.lifetime) || !std::isfinite(record.decay) ||
           allocation==allocations.end() || record.quad_count>allocation->second->quads.size())
            throw FormatError("Source decal frame has an invalid retained record");
        used_allocations.insert(record.cache_datum);
    }
    if(used_allocations.size()!=allocations.size())
        throw FormatError("Source decal frame contains unowned cache pages");
    std::set<unsigned> heads;
    for(const auto& head:frame.heads) {
        if(head.layer>=source_decal_head_layers_tu15 || head.cluster>=source_decal_cluster_capacity_tu15 ||
           head.datum==none || !heads.insert(head.layer*source_decal_cluster_capacity_tu15+head.cluster).second)
            throw FormatError("Source decal frame has an invalid or duplicated cluster head");
        auto handle=head.datum;unsigned visits{};
        while(handle!=none) {
            const auto record=records.find(handle);
            if(++visits>source_decal_record_capacity_tu15 || record==records.end() ||
               record->second->layer!=head.layer || record->second->cluster!=head.cluster)
                throw FormatError("Source decal frame cluster list is stale or cyclic");
            if(record->second->quad_count && !records.contains(record->second->primary_datum))
                throw FormatError("Source decal frame has no retained primary record");
            handle=record->second->next_cluster_datum;
        }
    }
}
SourceDecalFrameTu15 source_decal_frame_tu15(const SourceRuntimeMemory& memory) {
    const auto word=[&](std::uint32_t address) {
        std::uint32_t value{};const auto bytes=memory.view(address,4);
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8);
        return value;
    };
    const auto context=word(0x4f721cu),clock=word(0x51df54u);
    if(!context || !clock)throw FormatError("Source decal frame has no original context or clock");
    SourceDecalFrameTu15 frame;frame.tick=word(clock+8u);
    auto geometry=source_decal_vertex_cache_tu15(memory);
    std::map<std::uint32_t,std::size_t> by_allocation;
    for(auto& record:geometry) {
        if(record.quads.size()>source_decal_page_capacity_tu15)
            throw FormatError("Source decal record exceeds its original page capacity");
        frame.records.push_back({record.datum,record.definition,record.cache_datum,record.tick,record.tint,
            record.cluster,record.layer,record.flags,record.lifetime,record.decay,
            record.primary_datum,record.next_cluster_datum,record.bitmap_index,std::uint16_t(record.quads.size())});
        const auto [entry,inserted]=by_allocation.emplace(record.cache_datum,frame.allocations.size());
        if(inserted)frame.allocations.push_back({record.cache_datum,std::move(record.quads)});
        else {
            auto& retained=frame.allocations[entry->second].quads;
            for(std::size_t q=0;q<std::min(retained.size(),record.quads.size());++q)
                if(!same_quad(retained[q],record.quads[q]))
                    throw FormatError("Source decal allocation aliases different actual vertex pages");
            if(record.quads.size()>retained.size())retained=std::move(record.quads);
        }
    }
    for(unsigned layer=0;layer<source_decal_head_layers_tu15;++layer)
        for(unsigned cluster=0;cluster<source_decal_cluster_capacity_tu15;++cluster) {
            const auto datum=word(context+(layer*source_decal_cluster_capacity_tu15+cluster)*4u);
            if(datum!=none)frame.heads.push_back({std::uint16_t(cluster),std::uint8_t(layer),datum});
        }
    validate_source_decal_frame_tu15(frame);
    return frame;
}
}
