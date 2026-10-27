#pragma once
#include "halo2/source_decal_frame_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <optional>

namespace halo2 {
enum class SourceDecalWireAnchorTu15 : unsigned { frame,record,allocation,head };
inline constexpr std::size_t source_decal_wire_record_bytes_tu15=43;
// A present empty cache is eleven bytes; absence is one byte. Allocations own
// exact original XYZ/CMP pages once within this frame, never a historical bank.
// Writer supplies word(value,width), real(float), anchor(kind,full_identity).
template<class Writer>
void encode_source_decal_frame_wire_tu15(Writer& out,const std::optional<SourceDecalFrameTu15>& frame) {
    if(frame)validate_source_decal_frame_tu15(*frame);
    out.anchor(SourceDecalWireAnchorTu15::frame,0xffffffffu);
    out.word(frame.has_value(),1);if(!frame)return;
    out.word(frame->tick,4);out.word(frame->records.size(),2);
    out.word(frame->allocations.size(),2);out.word(frame->heads.size(),2);
    for(const auto& record:frame->records) {
        out.anchor(SourceDecalWireAnchorTu15::record,record.datum);
        for(auto value:{record.datum,record.definition,record.cache_datum,record.tick,record.tint})out.word(value,4);
        out.word(std::bit_cast<std::uint16_t>(record.cluster),2);out.word(record.layer,1);out.word(record.flags,1);
        out.real(record.lifetime);out.real(record.decay);
        out.word(record.primary_datum,4);out.word(record.next_cluster_datum,4);
        out.word(std::bit_cast<std::uint8_t>(record.bitmap_index),1);out.word(record.quad_count,2);
    }
    for(const auto& allocation:frame->allocations) {
        out.anchor(SourceDecalWireAnchorTu15::allocation,allocation.cache_datum);
        out.word(allocation.cache_datum,4);out.word(allocation.quads.size(),2);
        for(const auto& quad:allocation.quads)for(const auto& vertex:quad) {
            out.real(vertex.position.x);out.real(vertex.position.y);out.real(vertex.position.z);
            out.word(vertex.packed_projection,4);
        }
    }
    for(const auto& head:frame->heads) {
        out.anchor(SourceDecalWireAnchorTu15::head,(unsigned(head.layer)<<16)|head.cluster);
        out.word(head.cluster,2);out.word(head.layer,1);out.word(head.datum,4);
    }
}
// Reader supplies word(width), finite real(), remaining(). Decode into a local
// owning frame and validate it before the ordinary snapshot's atomic adoption.
template<class Reader>
std::optional<SourceDecalFrameTu15> decode_source_decal_frame_wire_tu15(Reader& in) {
    const auto present=in.word(1);
    if(present>1)throw FormatError("Invalid source decal cache presence");
    if(!present)return {};
    SourceDecalFrameTu15 frame;frame.tick=in.word(4);
    const auto records=in.word(2),allocations=in.word(2),heads=in.word(2);
    if(records>source_decal_record_capacity_tu15 || allocations>source_decal_record_capacity_tu15 ||
       heads>source_decal_head_layers_tu15*source_decal_cluster_capacity_tu15 ||
       records*source_decal_wire_record_bytes_tu15+allocations*6u+heads*7u>in.remaining())
        throw FormatError("Source decal wire extent exceeds its original capacities or snapshot");
    frame.records.reserve(records);frame.allocations.reserve(allocations);frame.heads.reserve(heads);
    for(unsigned n=0;n<records;++n) {
        SourceDecalFrameRecordTu15 record;
        record.datum=in.word(4);record.definition=in.word(4);record.cache_datum=in.word(4);
        record.tick=in.word(4);record.tint=in.word(4);
        record.cluster=std::bit_cast<std::int16_t>(std::uint16_t(in.word(2)));
        record.layer=in.word(1);record.flags=in.word(1);
        record.lifetime=in.real();record.decay=in.real();
        record.primary_datum=in.word(4);record.next_cluster_datum=in.word(4);
        record.bitmap_index=std::bit_cast<std::int8_t>(std::uint8_t(in.word(1)));
        record.quad_count=in.word(2);frame.records.push_back(record);
    }
    std::size_t pages{};
    for(unsigned n=0;n<allocations;++n) {
        SourceDecalFrameAllocationTu15 allocation;allocation.cache_datum=in.word(4);
        const auto count=in.word(2);
        if(count>source_decal_page_capacity_tu15-pages || count*64u>in.remaining())
            throw FormatError("Source decal wire pages exceed their original allocation budget or snapshot");
        pages+=count;allocation.quads.reserve(count);
        for(unsigned q=0;q<count;++q) {
            std::array<SourceDecalCacheVertex,4> quad;
            for(auto& vertex:quad) {
                const Vec3 position{in.real(),in.real(),in.real()};
                vertex=source_decal_frame_vertex_tu15(position,std::uint32_t(in.word(4)));
            }
            allocation.quads.push_back(quad);
        }
        frame.allocations.push_back(std::move(allocation));
    }
    for(unsigned n=0;n<heads;++n) {
        SourceDecalFrameHeadTu15 head;head.cluster=in.word(2);head.layer=in.word(1);head.datum=in.word(4);
        frame.heads.push_back(head);
    }
    validate_source_decal_frame_tu15(frame);return frame;
}
}
