#include "halo2/mcc_model_node_map.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <array>

namespace halo2 {
std::vector<MccModelNodeMap> plan_mcc_model_node_maps(const Cache& cache) {
    if(!cache.is_mcc())throw FormatError("Model node-map projection requires MCC ownership");
    std::vector<MccModelNodeMap> result;
    for(const auto& tag:cache.tags())if(tag.group=="mode" && tag.address && tag.size) {
        const auto owner=cache.load_source_tag_metadata(tag.datum);
        const Bytes root(owner.bytes_at(tag.address,0x4c));
        const auto count=root.u32(0x24),sections=root.u32(0x28),nodes=root.u32(0x48);
        if(count>512 || nodes>256)throw FormatError("MCC model node-map owner exceeds source bounds");
        if(!count)continue;
        const Bytes rows(owner.bytes_at(sections,std::size_t(count)*0x5c));
        for(unsigned n=0;n<count;++n) {
            const auto at=n*0x5c;
            const auto resources=rows.u32(at+0x48),pointer=rows.u32(at+0x4c);
            if(resources>128)throw FormatError("MCC model node-map resource count exceeds source bounds");
            if(!resources)continue;
            const Bytes descriptors(owner.bytes_at(pointer,std::size_t(resources)*16));
            bool found=false;
            for(unsigned i=0;i<resources;++i)if(descriptors.u16(i*16+4)==104) {
                if(found || descriptors.u16(i*16+6)!=1)
                    throw FormatError("MCC model has ambiguous or unsupported node-map ownership");
                found=true;
                const auto bytes=rows.u32(at+0x3c),tail=rows.u32(at+0x44);
                if(bytes<132 || tail>bytes-132 || bytes-tail-4!=128)
                    throw FormatError("MCC node map requires its retained120-byte model header");
                const auto node_count=descriptors.u32(i*16+8);
                const auto offset=std::uint64_t(128)+descriptors.u32(i*16+12);
                const auto encoded=rows.u32(at+0x38);
                if(node_count>256 || (node_count && !nodes) || offset>bytes || node_count>bytes-offset ||
                   std::uint64_t(encoded&0x3fffffffu)+bytes>0x40000000ull)
                    throw FormatError("MCC node map escapes its declared allocation or hierarchy");
                result.push_back({pointer+i*16+4,encoded,bytes,std::uint32_t(offset),node_count,nodes,tag.name});
            }
        }
    }
    return result;
}

void project_mcc_model_node_map_range(std::span<const MccModelNodeMap> maps,
    std::uint32_t encoded,std::span<const std::byte> container,std::span<std::byte> delivered) {
    const auto location=encoded>>30,offset=encoded&0x3fffffffu;
    if(delivered.size()>0x40000000ull-offset)
        throw FormatError("MCC model node-map read crosses its cache selector");
    if(!std::ranges::equal(Bytes(container).slice(offset,delivered.size()),delivered))
        throw FormatError("MCC model resource read differs from its immutable source");
    const auto end=std::uint64_t(offset)+delivered.size();
    std::vector<std::pair<std::uint64_t,std::array<std::byte,8>>> patches;
    for(const auto& map:maps) {
        if((map.encoded_resource>>30)!=location)continue;
        const auto begin=map.encoded_resource&0x3fffffffu;
        // The only changed file fields are raw+6C/+70, which become the
        // original resident+64 count and resident+68 unloaded pointer.
        const auto first=std::uint64_t(begin)+0x6c,last=first+8;
        if(offset>=last || end<=first)continue;
        const Bytes raw(Bytes(container).slice(begin,map.resource_bytes));
        if(raw.u32(0x6c)!=0 || raw.u32(0x70)!=map.node_count || raw.u32(0x74)!=0)
            throw FormatError("MCC serialized node-map header differs from its typed resource: "+map.model);
        for(auto node:raw.slice(map.node_offset,map.node_count))
            if(std::to_integer<unsigned>(node)>=map.model_nodes)
                throw FormatError("MCC node-map entry escapes its actual model hierarchy: "+map.model);
        std::array<std::byte,8> projected{};
        for(unsigned i=0;i<4;++i)projected[i]=std::byte(map.node_count>>(8*i));
        patches.emplace_back(first,projected);
    }
    // Validate every owner before publishing even a partial projected range.
    // Identical aliases and overlapping I/O windows retain the same source.
    for(const auto& [first,projected]:patches)
        for(auto at=std::max<std::uint64_t>(offset,first);at<std::min(end,first+8);++at)
            delivered[at-offset]=projected[at-first];
}
}
