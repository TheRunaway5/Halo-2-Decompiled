#include "halo2/source_effect_light_opaque.hpp"
#include <bit>
#include <cstring>
namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
Bytes read(const SourceTagMetadataProviders& p,std::uint32_t at,std::size_t n) {
    if(!p.read)throw FormatError("source opaque shader metadata reader missing");
    return Bytes(p.read(at,n));
}
const SourceTagMetadata& definition(const SourceTagMetadataProviders& p,std::uint32_t id,const char* group) {
    if(!p.definition)throw FormatError("source opaque shader definition service missing");
    const auto& t=p.definition(id);
    if(t.group()!=group)throw FormatError("source opaque shader consumes unresolved tag group");
    return t;
}
void byte(SourceOpaqueSortRecord& r,unsigned at,std::uint8_t value) {
    const auto shift=(at%4)*8;r.words[at/4]=(r.words[at/4]&~(255u<<shift))|(std::uint32_t(value)<<shift);
}
bool select(const SourcePartCommand& command,const SourceCullDrawUnit& unit,
    const SourceTagMetadataProviders& p,const SourceOpaqueGeometryService& geometry,
    unsigned unit_index,SourceOpaqueSortRecord& record) {
    const auto& c=command.words;const auto& u=unit.words;
    const auto& shad=definition(p,c[2],"shad");const auto root=read(p,shad.source_address(),0x28);
    const auto shader_post=read(p,root.u32(0x24),4);const auto& stem=definition(p,shader_post.u32(0),"stem");
    const auto template_root=read(p,stem.source_address(),0x60);const auto post=read(p,template_root.u32(0x5c),40);
    const auto lod=(c[5]>>8)&255u;
    if(lod>=post.u32(0))throw FormatError("source opaque shader LOD missing");
    const auto lod_record=read(p,post.u32(4)+lod*10,10);
    const auto layer_index=(lod_record.u16(0)&511u)+12;
    if(layer_index>=post.u32(8))throw FormatError("source opaque layer table missing");
    const auto range=read(p,post.u32(12)+layer_index*2,2).u16(0);
    //15D70 gates on the layer's pass count before30DA0 returns implementation1.
    byte(record,4,(range>>9)?1:255);
    if(!(range>>9))return false;
    const auto classification=u[3]>>29;
    byte(record,5,classification<=1?1:2);
    if(classification!=0)record.words[3]=(u[3]>>9&511u)|((u[1]&65535u)<<11);
    if(!geometry)throw FormatError("source sphere opaque geometry service missing");
    const auto g=geometry(unit_index);
    const auto part=c[3]&255u;
    if(part>=g.parts.size())throw FormatError("source sphere opaque part outside current geometry");
    const auto pass_index=(range&511u)+(c[5]&255u);
    if(pass_index>=post.u32(16))throw FormatError("source opaque pass table missing");
    const auto pass=read(p,post.u32(20)+pass_index*10,10);
    const auto& spas=definition(p,pass.u32(4),"spas");const auto pass_root=read(p,spas.source_address(),0x24);
    const auto pass_post=read(p,pass_root.u32(0x20),8);
    const auto implementation=pass_post.u32(4)+306; // Ordinary sphere1.
    record.words[4]=implementation;
    const auto compiled=read(p,implementation,306);const auto vrtx_id=compiled.u32(0x100);
    std::uint16_t type;std::memcpy(&type,g.parts[part].data(),2);
    const auto selector=g.metadata.selectors[(type==4 || type==5)?1:0];
    const auto class_count=[&](){
        const auto& vrtx=definition(p,vrtx_id,"vrtx");return read(p,vrtx.source_address(),12).u32(4);
    };
    const auto selected=std::uint16_t(source_opaque_vertex_class(g.metadata.geometry_class,selector,class_count));
    record.words[2]=(record.words[2]&0xffff0000u)|selected;record.words[5]=u[4];
    if(vrtx_id==none)return false;
    const auto& vrtx=definition(p,vrtx_id,"vrtx");const auto vr=read(p,vrtx.source_address(),12);
    if(selected>=vr.u32(4))return false;
    return (read(p,vr.u32(8)+unsigned(selected)*28,28).u32(12)>>4)!=0;
}
}
SourceOpaqueGeometryMetadata load_source_opaque_bsp_geometry_metadata(const Cache& cache,
    std::size_t bsp,MenuWorldSourceSectionKind kind,unsigned section) {
    if(kind!=MenuWorldSourceSectionKind::bsp_cluster && kind!=MenuWorldSourceSectionKind::bsp_instance_definition)
        throw FormatError("source opaque BSP metadata requires BSP geometry owner");
    const auto owned=cache.load_source_bsp_metadata(bsp);const auto base=owned.source_address();
    const Bytes root(owned.bytes_at(base,owned.size()));
    const auto field=16+(kind==MenuWorldSourceSectionKind::bsp_cluster?0x9c:0x138);
    if(section>=root.u32(field))throw FormatError("source opaque BSP metadata section missing");
    const auto stride=kind==MenuWorldSourceSectionKind::bsp_cluster?176:200;
    const Bytes m(owned.bytes_at(root.u32(field+4)+section*stride,0x16));
    const auto selectors=m.slice(0x10,2);
    return {{std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(selectors[0])),
             std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(selectors[1]))},m.u16(0x14)};
}
SourceOpaqueGeometryMetadata load_source_opaque_model_geometry_metadata(const Cache& cache,
    std::uint32_t model,unsigned section) {
    const auto owned=cache.load_source_tag_metadata(model);
    if(owned.group()!="mode")throw FormatError("source opaque model geometry requires mode metadata");
    const Bytes root(owned.bytes_at(owned.source_address(),0x2c));
    if(section>=root.u32(0x24))throw FormatError("source opaque model section escapes metadata");
    const Bytes geometry(owned.bytes_at(root.u32(0x28)+section*92,0x16));
    const auto selectors=geometry.slice(0x10,2);
    return {{std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(selectors[0])),
             std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(selectors[1]))},geometry.u16(0x14)};
}
std::uint32_t source_opaque_vertex_class(std::uint16_t classification,std::int32_t selector,
    const std::function<std::uint32_t()>& count) {
    switch(classification) {
    case 0:return 0;
    case 1:return 1;
    case 2:return 2;
    case 3: {
        if(!count)throw FormatError("source opaque vertex class count service missing");
        const auto candidate=std::uint32_t(selector)+2u;
        return candidate<count()?candidate:0;
    }
    default:throw FormatError("source opaque vertex class dispatcher outside original table");
    }
}
unsigned source_effect_light_opaque_select(const SourcePartCommandBank& bank,
    std::span<const SourceCullDrawUnit> units,const SourceTagMetadataProviders& metadata,
    const SourceOpaqueGeometryService& geometry,std::span<SourceOpaqueSortRecord> records) {
    unsigned count=0;const auto begin=std::bit_cast<std::int32_t>(bank.header[3]);
    const auto end=std::bit_cast<std::int32_t>(bank.header[1]);
    if(begin<0 || end<0 || unsigned(end)>bank.storage.size())
        throw FormatError("source sphere opaque command interval outside current bank");
    for(unsigned i=unsigned(begin);i<unsigned(end);++i) {
        const auto& command=bank.storage[i];if(!(command.words[4]&0x1000u))continue;
        if(count>=records.size())throw FormatError("source sphere opaque output storage missing");
        auto& record=records[count];record.words[0]=std::uint16_t(i)|(command.words[2]<<16);
        if(command.words[0]!=0x4cbf0)throw FormatError("source sphere opaque command callback unresolved");
        const auto index=std::uint16_t(command.words[3]>>8);
        if(index>=units.size())throw FormatError("source sphere opaque unit outside current bank");
        if(select(command,units[index],metadata,geometry,index,record))++count;
    }
    source_opaque_sort(records.first(count));return count;
}
SourceOpaquePartRanges source_effect_light_part_ranges(const SourceCullDrawUnit& unit,
    const SourceBspRangeGeometry& geometry,unsigned part,const SourceBspRangeArena& arena,
    std::uint32_t arena_address) {
    if(part>=geometry.parts.size())throw FormatError("source sphere range part missing");
    const auto part_flags=Bytes(geometry.parts[part]).u16(2);
    SourceOpaquePartRanges result;
    result.topology=(unit.words[0]&0x200u) && !(part_flags&8u)?
        MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
    result.filtered=(unit.words[0]&0x40u) && unit.words[5] && geometry.subpart_count!=0;
    std::span<const std::uint32_t> mask;
    if(result.filtered) {
        const auto address=unit.words[5];
        if(!arena_address || address<arena_address || (address-arena_address)%4)
            throw FormatError("source sphere range mask has no current arena owner");
        const auto offset=std::size_t(address-arena_address)/4;
        if(offset>arena.words.size())throw FormatError("source sphere range mask escapes current arena");
        mask=std::span<const std::uint32_t>(arena.words).subspan(offset);
    }
    result.ranges=source_bsp_part_ranges(geometry,part,result.topology,result.filtered,mask);
    return result;
}
std::vector<SourceOpaqueRangeDraw> source_effect_light_ordered_ranges(
    const SourcePartCommandBank& bank,std::span<const SourceCullDrawUnit> units,
    std::span<const SourceOpaqueSortRecord> sorted,const SourceBspRangeArena& arena,
    std::uint32_t arena_address,const SourceOpaqueRangeGeometryService& geometry) {
    if(!geometry && !sorted.empty())throw FormatError("source sphere range binding service missing");
    std::vector<SourceOpaqueRangeDraw> result;result.reserve(sorted.size());
    for(const auto& record:sorted) {
        const auto command=std::uint16_t(record.words[0]);
        if(command>=bank.header[1] || command>=bank.storage.size())
            throw FormatError("source sphere sorted command outside current bank");
        const auto& c=bank.storage[command].words;
        if(c[1]!=0x4d0b0)throw FormatError("source sphere range draw callback unresolved");
        const auto unit=std::uint16_t(c[3]>>8);
        const auto part=std::uint8_t(c[3]);
        const auto vertex=std::uint16_t(record.words[2]);
        if(unit>=units.size())throw FormatError("source sphere sorted unit missing");
        const auto bound=geometry(command,unit,vertex);
        if(!bound.vertex_bound)continue;
        if(!bound.geometry)throw FormatError("source sphere successful binding lacks geometry");
        result.push_back({command,unit,part,vertex,
            source_effect_light_part_ranges(units[unit],*bound.geometry,part,arena,arena_address)});
    }
    return result;
}
}
