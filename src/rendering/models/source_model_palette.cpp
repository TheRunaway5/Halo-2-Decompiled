#include "halo2/source_model_palette.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
std::vector<std::array<float,12>> source_model_skin_palette(
    std::span<const Transform> world,std::span<const Transform> inverse) {
    if(world.size()!=inverse.size() || world.size()>256)
        throw FormatError("source model WORLD/inverse palette size differs");
    std::vector<std::array<float,12>> out;out.reserve(world.size());
    for(unsigned i=0;i<world.size();++i) {
        const auto composed=compose_transform(world[i],inverse[i]);
        const auto &b=composed.basis;const auto &p=composed.position;
        out.push_back({b[0].x*composed.scale,b[1].x*composed.scale,b[2].x*composed.scale,p.x,
            b[0].y*composed.scale,b[1].y*composed.scale,b[2].y*composed.scale,p.y,
            b[0].z*composed.scale,b[1].z*composed.scale,b[2].z*composed.scale,p.z});
    }
    return out;
}
void append_source_model_compounds(std::vector<std::array<float,12>> &palette,
    std::span<const SourceModelCompoundNode> definitions) {
    for(const auto &definition:definitions) {
        std::array<float,12> blended{};float total=0;
        for(unsigned i=0;i<4;++i) {
            const auto node=definition.nodes[i];if(node==255)break;
            if(node>=palette.size())throw FormatError("compound model node escapes prior palette");
            const float weight=i<3?definition.weights[i]:1.f-total;
            for(unsigned k=0;k<12;++k) {
                const volatile float product=palette[node][k]*weight;
                blended[k]=product+blended[k];
            }
            total=weight+total;
        }
        if(palette.size()>=256)throw FormatError("compound model palette exceeds source capacity");
        palette.push_back(blended);
    }
}
SourceModelPalette source_model_palette(const SourceModelPaletteDefinition &definition,
    unsigned lod,std::span<const Transform> world,std::span<const SourceModelPaletteRegion> regions) {
    if(lod>=6 || regions.size()!=definition.region_count)
        throw FormatError("source model palette requires actual LOD/region bank");
    if(definition.compound_node_counts[lod]!=definition.compounds[lod].size())
        throw FormatError("source model palette compound node producer unresolved");
    auto base=source_model_skin_palette(world,definition.inverse_binds);
    append_source_model_compounds(base,definition.compounds[lod]);
    SourceModelPalette out;
    if(definition.model_flags&4) {
        if(base.empty())throw FormatError("source compact model palette has no first node");
        out.matrices.push_back(base.front());
    } else out.matrices=base;
    out.base_count=std::uint16_t(out.matrices.size());
    for(const auto &region:regions) {
        SourceModelPaletteRange range;
        if(region.selected_section!=255) {
            if(region.node_map.empty()) {
                range.count=region.no_map_node_count?region.no_map_node_count:
                    std::uint16_t(std::min<std::size_t>(base.size(),40));
            } else {
                if(out.matrices.size()+region.node_map.size()>std::numeric_limits<std::uint16_t>::max())
                    throw FormatError("source model compact palette exceeds index bank");
                range.first=std::uint16_t(out.matrices.size());range.count=std::uint16_t(region.node_map.size());
                for(const auto node:region.node_map) {
                    if(node>=base.size())throw FormatError("source model resident node map escapes base palette");
                    out.matrices.push_back(base[node]);
                }
            }
        }
        out.regions.push_back(range);
    }
    return out;
}
Vec3 source_model_palette_part_center(const SourceModelPalette &bank,
    unsigned region,std::int16_t rigid,std::uint16_t flags) {
    const int node=rigid==-1?0:(flags&4)?
        (region<bank.regions.size()?int(bank.regions[region].first):
            throw FormatError("source model part region escapes compact palette")):int(rigid);
    if(node<0 || std::size_t(node)>=bank.matrices.size())
        throw FormatError("source model part node escapes actual palette");
    const auto &m=bank.matrices[unsigned(node)];return {m[3],m[7],m[11]};
}
SourceModelPaletteDefinition Cache::load_source_model_palette_definition(std::uint32_t datum)const {
    const auto t=query_tag({.handle=TagHandle{datum}, .group=tag_groups::render_model, .require_address=true});
    if(!t)throw FormatError("source model palette render model missing");
    const Bytes b(cache_bytes());const auto p=metadata_offset(t->address,0x50);
    SourceModelPaletteDefinition out;out.model_flags=b.u16(p+4);out.region_count=b.u32(p+0x1c);
    const auto count=b.u32(p+0x48);if(count>256 || out.region_count>16)
        throw FormatError("source model palette node/region bank exceeds source");
    const auto nodes=count?metadata_offset(b.u32(p+0x4c),std::size_t(count)*0x60):0;
    for(unsigned i=0;i<count;++i) {
        const auto n=nodes+i*0x60;Transform inverse;inverse.scale=b.f32(n+0x28);
        for(unsigned c=0;c<3;++c)inverse.basis[c]={b.f32(n+0x2c+c*12),b.f32(n+0x30+c*12),b.f32(n+0x34+c*12)};
        inverse.position={b.f32(n+0x50),b.f32(n+0x54),b.f32(n+0x58)};out.inverse_binds.push_back(inverse);
    }
    const auto groups=b.u32(p+0x34);if(groups>256)throw FormatError("source model palette section groups exceed bank");
    const auto group=groups?metadata_offset(b.u32(p+0x38),std::size_t(groups)*12):0;
    for(unsigned lod=0;lod<6;++lod) {
        const auto selected=std::int8_t(std::to_integer<std::uint8_t>(b.slice(p+0x3c+lod,1)[0]));
        if(selected<0 || unsigned(selected)>=groups)throw FormatError("source model palette LOD section group absent");
        const auto at=group+unsigned(selected)*12;
        const auto count=b.u32(at+4);out.compound_node_counts[lod]=count;
        if(count>256)throw FormatError("compound node count exceeds source palette");
        const auto start=count?metadata_offset(b.u32(at+8),std::size_t(count)*16):0;
        for(unsigned i=0;i<count;++i) {
            SourceModelCompoundNode node;
            for(unsigned j=0;j<4;++j)node.nodes[j]=std::to_integer<std::uint8_t>(b.slice(start+i*16+j,1)[0]);
            for(unsigned j=0;j<3;++j)node.weights[j]=b.f32(start+i*16+4+j*4);
            out.compounds[lod].push_back(node);
        }
    }
    return out;
}
} // namespace halo2
