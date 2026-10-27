#include "halo2/source_decorator_decals_tu15.hpp"
#include "halo2/bsp_lighting.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>
#include <set>

namespace halo2 {
namespace {
std::int16_t signed_half(const Bytes& b,std::size_t p) {return std::bit_cast<std::int16_t>(b.u16(p));}
template<std::size_t N>std::array<std::byte,N> copy(const Bytes& b,std::size_t p) {
    std::array<std::byte,N> out;const auto bytes=b.slice(p,N);std::copy(bytes.begin(),bytes.end(),out.begin());return out;
}
Vec3 position(const Bytes& b,std::size_t p) {
    Vec3 out{b.f32(p),b.f32(p+4),b.f32(p+8)};
    if(!std::isfinite(out.x)||!std::isfinite(out.y)||!std::isfinite(out.z))throw FormatError("nonfinite decorator position");
    return out;
}
void put_half(std::array<std::byte,20>& b,std::size_t at,std::int16_t value) {
    const auto word=std::bit_cast<std::uint16_t>(value);b[at]=std::byte(word);b[at+1]=std::byte(word>>8);
}
}
Vertex source_decorator_decal_vertex_tu15(const SourceDecoratorDecalVertexTu15& value) {
    const Bytes b(value.packed);Vertex out;out.position=position(b,0);
    const auto normalize=[](std::int16_t code){return std::max(-1.f,float(code)/32767.f);};
    out.uv={normalize(signed_half(b,12)),normalize(signed_half(b,14))};
    // Original1C310 emits format0x40. Pinned Xemu vk/vertex.c binds UNORM4
    // and glsl/vsh.c applies.bgra for UB_D3D, distinct from PBYTE3 radiance.
    const auto color=b.u32(16);
    out.color={float((color>>16)&255u)/255.f,float((color>>8)&255u)/255.f,float(color&255u)/255.f};
    out.source_color_alpha=float(color>>24)/255.f;return out;
}
SourceDecoratorDecalVertexTu15 source_decorator_decal_vertex_mcc_tu15(std::span<const std::byte,32> input) {
    const Bytes b(input);(void)position(b,0);SourceDecoratorDecalVertexTu15 out;
    std::copy_n(input.begin(),12,out.packed.begin());
    put_half(out.packed,12,pack_bsp_lightmap_uv_tu15(b.f32(12)));
    put_half(out.packed,14,pack_bsp_lightmap_uv_tu15(b.f32(16)));
    std::copy_n(input.begin()+28,4,out.packed.begin()+16);return out;
}
SourceDecoratorDecalBspTu15 Cache::load_source_decorator_decals_tu15(std::size_t index) const {
    if(index>=bsps_.size() || vista_)throw FormatError("decorator requires original or prepared MCC BSP");
    const auto& bsp=bsps_[index];const Bytes file(cache_bytes());
    const Bytes m(file.slice(bsp.file_offset,bsp.size));const TagAddressSpace memory(m,bsp.memory_address);
    const auto definitions=memory.block(16+0x234,48,1);
    SourceDecoratorDecalBspTu15 out;out.bsp_index=unsigned(index);out.bsp=bsp.datum;
    if(!definitions.count)return out;
    const auto required=[&](std::uint32_t id,TagGroup group)->const Tag& {
        const auto* result=query_tag({.handle=TagHandle{id},.group=group,.require_address=true,.minimum_size=1});
        if(!result)throw FormatError("decorator metadata has no resident full salted tag identity");
        return *result;
    };
    const auto& scenario=required(scenario_tag_.value,tag_groups::scenario);
    const auto scenario_root=metadata_offset(scenario.address,scenario.size);
    const auto palette=TagAddressSpace(file,metadata_magic_).block(scenario_root+0x378,8,128);
    for(unsigned n=0;n<palette.count;++n)out.palette.push_back(file.u32(palette.offset+n*8+4));
    std::map<std::array<unsigned,3>,std::optional<SourceDecoratorDecalSelectionTu15>> selections;
    const auto selection=[&](const std::array<std::byte,24>& raw)->std::optional<SourceDecoratorDecalSelectionTu15> {
        const std::array<unsigned,3> key{std::to_integer<unsigned>(raw[0]),std::to_integer<unsigned>(raw[1]),std::to_integer<unsigned>(raw[2])};
        if(const auto found=selections.find(key);found!=selections.end())return found->second;
        const auto resolve=[&]()->std::optional<SourceDecoratorDecalSelectionTu15> {
            if(key[0]>=128 || key[0]>=out.palette.size() || out.palette[key[0]]==0xffffffffu)return {};
            const auto& set=required(out.palette[key[0]],TagGroup{"DECR"});const auto root=metadata_offset(set.address,set.size);
            const TagAddressSpace tags(file,metadata_magic_);const auto classes=tags.block(root+0x10,20,256);
            if(key[1]>=classes.count)return {};
            const auto c=classes.offset+key[1]*20;const auto permutations=tags.block(c+12,40,256);
            if(key[2]>=permutations.count)return {};
            const auto p=permutations.offset+key[2]*40;const auto shaders=tags.block(root,8,256);
            const auto shader_index=std::to_integer<unsigned>(file.slice(p+4,1)[0]);
            if(shader_index>=shaders.count)return {};
            const auto shader=file.u32(shaders.offset+shader_index*8+4);if(shader==0xffffffffu)return {};
            (void)required(shader,tag_groups::shader);
            return SourceDecoratorDecalSelectionTu15{set.datum,shader,std::uint8_t(key[1]),std::uint8_t(key[2]),
                std::to_integer<std::uint8_t>(file.slice(c+4,1)[0]),std::to_integer<std::uint8_t>(file.slice(p+8,1)[0]),
                std::to_integer<std::uint8_t>(file.slice(p+9,1)[0]),std::to_integer<std::uint8_t>(file.slice(p+10,1)[0]),copy<40>(file,p)};
        };
        auto result=resolve();selections.emplace(key,result);return result;
    };
    for(unsigned ordinal=0;ordinal<definitions.count;++ordinal) {
        const auto p=definitions.offset+ordinal*48;SourceDecoratorDecalDefinitionTu15 d;d.ordinal=ordinal;
        d.source_address=std::uint32_t(bsp.memory_address+p);
        d.grid_origin=position(m,p);d.cells_per_dimension=std::bit_cast<std::int32_t>(m.u32(p+12));
        const auto groups=memory.block(p+24,24,131072),cells=memory.block(p+32,24,65535),projected=memory.block(p+40,64,32768);
        std::set<unsigned> used;
        for(unsigned n=0;n<groups.count;++n) {
            const auto q=groups.offset+n*24;SourceDecoratorDecalGroupTu15 g;g.ordinal=n;g.authored=copy<24>(m,q);
            g.cluster=signed_half(m,q+4);g.cache_block=signed_half(m,q+6);g.placement_first=signed_half(m,q+8);
            g.placement_count=signed_half(m,q+10);g.vertex_first=signed_half(m,q+12);g.vertex_count=signed_half(m,q+14);
            g.index_first=signed_half(m,q+16);g.index_count=signed_half(m,q+18);g.selection=selection(g.authored);
            if(g.selection && (g.selection->type==1 || g.selection->type==2)) {
                if(g.cache_block<0 || g.placement_count<0 || g.vertex_count<0 || g.index_count<0)
                    throw FormatError("decorator decal group has negative consumed resource count");
                used.insert(unsigned(g.cache_block));
            }
            d.groups.push_back(std::move(g));
        }
        const unsigned stride=mcc_?52:44;const auto caches=memory.block(p+16,stride,4096);
        for(auto n:used)if(n>=caches.count)throw FormatError("decorator group cache index escapes authored bank");
        for(unsigned n=0;n<caches.count;++n) {
            const auto q=caches.offset+n*stride;SourceDecoratorDecalCacheTu15 bank;bank.ordinal=n;bank.header_bytes=stride;
            const auto header=m.slice(q,stride);std::copy(header.begin(),header.end(),bank.authored_header.begin());
            bank.resource_pointer=m.u32(q);bank.resource_bytes=m.u32(q+4);bank.section_bytes=m.u32(q+8);
            bank.storage=mcc_?SourceDecoratorDecalStorageTu15::Mcc32:SourceDecoratorDecalStorageTu15::Xbox20;
            const auto descriptors=memory.block(q+16,16,128);
            for(unsigned k=0;k<descriptors.count;++k) {
                const auto r=descriptors.offset+k*16;bank.resources.push_back({std::to_integer<std::uint8_t>(m.slice(r,1)[0]),m.u16(r+4),m.u16(r+6),m.u32(r+8),m.u32(r+12)});
            }
            if(used.contains(n) && bank.resource_bytes) {
                const auto raw=raw_data(bank.resource_pointer,bank.resource_bytes);const Bytes b(raw);b.require(0,40);
                if(b.u32(0)!=0x626c6b68u)throw FormatError("decorator cache lacks its actual resource block header");
                const auto placement_count=b.u32(8),vertex_count=b.u32(16),index_count=b.u32(24);
                if(placement_count>32768 || vertex_count>65536 || index_count>65536)
                    throw FormatError("decorator cache resource counts exceed original banks");
                const auto base=std::size_t(bank.section_bytes)+8;b.require(base,0);
                const auto resource=[&](unsigned type,unsigned locator)->std::optional<std::span<const std::byte>> {
                    std::optional<std::span<const std::byte>> result;
                    for(const auto& r:bank.resources)if(r.type==type && r.primary_locator==locator) {
                        if(result)throw FormatError("decorator cache has ambiguous resource locator");
                        result=b.slice(base+r.offset,r.bytes);
                    }
                    return result;
                };
                if(placement_count) {
                    const auto value=resource(0,0);if(!value || value->size()!=std::size_t(placement_count)*24)
                        throw FormatError("decorator placement resource does not match actual count");
                    const Bytes bytes(*value);for(unsigned k=0;k<placement_count;++k)bank.placements.push_back(copy<24>(bytes,k*24));
                }
                if(vertex_count) {
                    bank.stream_type=std::to_integer<std::uint8_t>(b.slice(32,1)[0]);bank.stream_stride=std::to_integer<std::uint8_t>(b.slice(33,1)[0]);
                    if(bank.stream_type!=(mcc_?51:53) || bank.stream_stride!=(mcc_?32:20))
                        throw FormatError("decorator compiled stream differs from source53/MCC51 contract");
                    const auto value=resource(2,24);if(!value || value->size()!=std::size_t(vertex_count)*bank.stream_stride)
                        throw FormatError("decorator compiled vertex resource does not match actual count");
                    const Bytes bytes(*value);bank.vertices.reserve(vertex_count);
                    for(unsigned k=0;k<vertex_count;++k) {
                        SourceDecoratorDecalVertexTu15 v;
                        if(mcc_)v=source_decorator_decal_vertex_mcc_tu15(std::span<const std::byte,32>{value->data()+k*32,32});
                        else v.packed=copy<20>(bytes,k*20);
                        (void)source_decorator_decal_vertex_tu15(v);bank.vertices.push_back(v);
                    }
                }
                if(index_count) {
                    const auto value=resource(0,16);if(!value || value->size()!=std::size_t(index_count)*2)
                        throw FormatError("decorator index resource does not match actual count");
                    const Bytes bytes(*value);for(unsigned k=0;k<index_count;++k)bank.indices.push_back(bytes.u16(k*2));
                }
            }
            d.caches.push_back(std::move(bank));
        }
        for(const auto& g:d.groups)if(g.selection && (g.selection->type==1 || g.selection->type==2)) {
            const auto& b=d.caches.at(unsigned(g.cache_block));
            const auto range=[](std::int16_t first,std::int16_t count,std::size_t size) {
                if(!count)return; //Zero-count authored starts can be retained residue.
                if(first<0 || std::size_t(first)>size || std::size_t(count)>size-std::size_t(first))
                    throw FormatError("decorator group consumes beyond actual cache resource");
            };
            range(g.placement_first,g.placement_count,b.placements.size());range(g.vertex_first,g.vertex_count,b.vertices.size());
            range(g.index_first,g.index_count,b.indices.size());
            if(g.selection->type==1 && g.vertex_count%4)throw FormatError("floating decorator draw has incomplete original QUADS");
        }
        for(unsigned n=0;n<cells.count;++n) {
            const auto q=cells.offset+n*24;SourceDecoratorDecalCellTu15 cell;cell.authored=copy<24>(m,q);
            for(unsigned child=0;child<8;++child)cell.children[child]=signed_half(m,q+child*2);
            cell.cache_block=signed_half(m,q+16);cell.group_count=signed_half(m,q+18);cell.group_first=std::bit_cast<std::int32_t>(m.u32(q+20));
            if(cell.group_count<0 || (cell.group_count && (cell.group_first<0 || std::size_t(cell.group_first)>d.groups.size() ||
                std::size_t(cell.group_count)>d.groups.size()-std::size_t(cell.group_first))))throw FormatError("decorator cell group range escapes actual bank");
            for(auto child:cell.children)if(child!=-1 && (child<0 || unsigned(child)>=cells.count))
                throw FormatError("decorator cell child escapes actual bank");
            d.cells.push_back(cell);
        }
        for(unsigned n=0;n<projected.count;++n)d.projected_placements.push_back(copy<64>(m,projected.offset+n*64));
        out.definitions.push_back(std::move(d));
    }
    return out;
}
}
