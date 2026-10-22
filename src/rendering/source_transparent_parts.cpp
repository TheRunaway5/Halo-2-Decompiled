#include "halo2/source_transparent_parts.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <bit>
#include <map>

namespace halo2 {
namespace {
SourceTransparentPartGeometry geometry(const SourceTransparentPart &p) {
    return {p.shader,{},0,std::nullopt,p.lod_scale>0?p.lod_scale:1.f};
}
}
std::vector<SourceTransparentCollectorSection> source_transparent_model_collector(
    const SourceInitialModelDefinition &d,const SourceInitialModelSelection &s,unsigned lod) {
    (void)source_initial_model_sections(d,s,lod); //same actual-selection bounds
    std::vector<SourceTransparentCollectorSection> out;
    for(unsigned region=0;region<d.render_sections.size();++region) {
        const auto permutation=s.permutations[region];if(permutation==255)continue;
        const int section=d.render_sections[region][permutation][lod];
        if(section>=0)out.push_back({region,unsigned(section)});
    }
    return out;
}
std::uint8_t source_transparent_model_bias_nibble(
    std::span<const std::int16_t> orders,unsigned region,unsigned model_count) {
    if(model_count!=1)return 0; //4C0D0 clears the whole parallel bank
    if(region>=orders.size())throw FormatError("source transparent sort region outside actual model");
    return std::uint8_t(std::uint16_t(orders[region])&15);
}
namespace {
SourceTransparentPart part(const Bytes &b,std::size_t at,unsigned ordinal) {
    SourceTransparentPart p;p.ordinal=std::uint16_t(ordinal);
    for(unsigned i=0;i<18;++i)p.authored_words[i]=b.u32(at+i*4);
    p.type=b.u16(at);p.flags=b.u16(at+2);p.material=std::int16_t(b.u16(at+4));
    p.index_first=b.u16(at+6);p.index_count=b.u16(at+8);
    p.subpart_first=b.u16(at+0xa);p.subpart_count=b.u16(at+0xc);
    p.center={b.f32(at+0x10),b.f32(at+0x14),b.f32(at+0x18)};p.lod_scale=b.f32(at+0x2c);
    return p;
}
}
SourceTransparentPartGeometry source_transparent_cluster_geometry(const SourceTransparentPart &p) {
    auto out=geometry(p);out.center=p.center;out.plane=p.plane;return out;
}
SourceTransparentPartGeometry source_transparent_instance_geometry(
    const SourceTransparentPart &p,const SourceTransparentInstance &instance) {
    auto out=geometry(p);out.center=instance.center;out.plane=p.plane;
    if(out.plane) {
        const auto old=*out.plane;const auto &b=instance.transform.basis;auto &n=*out.plane;
        n[0]=(b[2].x*old[2]+b[1].x*old[1])+b[0].x*old[0];
        n[1]=(b[2].y*old[2]+b[1].y*old[1])+b[0].y*old[0];
        n[2]=(b[2].z*old[2]+b[1].z*old[1])+b[0].z*old[0];
        const auto t=instance.transform.position;
        n[3]=((old[3]*instance.transform.scale+t.z*n[2])+t.y*n[1])+t.x*n[0];
    }
    return out;
}
SourceTransparentPartGeometry source_transparent_model_geometry(
    const SourceTransparentPart &p,std::int16_t rigid,std::uint16_t flags,
    std::span<const Vec3> nodes,std::optional<std::int16_t> mapped,std::uint8_t bias) {
    if(bias>15)throw FormatError("source transparent depth bias is not a nibble");
    const int node=rigid==-1?0:(flags&4)?
        (mapped?int(*mapped):throw FormatError("source transparent part needs actual context node remap")):int(rigid);
    if(node<0 || std::size_t(node)>=nodes.size())throw FormatError("source transparent rigid node outside actual pose bank");
    auto out=geometry(p);out.center=nodes[std::size_t(node)];
    if(bias)out.depth_bias=float(int(bias)-5)*std::bit_cast<float>(0x3a83126fu);
    return out;
}
SourceTransparentBspParts Cache::load_source_transparent_bsp_parts(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("source transparent BSP index outside scenario");
    const auto &bsp=bsps_[index];const Bytes file(cache_bytes());const Bytes m(file.slice(bsp.file_offset,bsp.size));
    
    
    const auto [material_count,materials]=TagAddressSpace(m, bsp.memory_address).block(16+0xa4, 32, 65535);
    std::map<std::uint32_t,SourceTransparentPart> shaders;
    const auto shader=[&](SourceTransparentPart &p) {
        if(p.material<0 || std::size_t(p.material)>=material_count)return;
        p.shader=m.u32(materials+std::size_t(p.material)*32+12);
        if(p.shader==0xffffffffu)return;
        if(!shaders.contains(p.shader))shaders.emplace(p.shader,load_source_transparent_shader(p.shader));
        const auto &s=shaders.at(p.shader);p.shader_priority_flags=s.shader_priority_flags;
        p.shader_command_flags=s.shader_command_flags;p.template_lod_layers=s.template_lod_layers;
    };
    SourceTransparentBspParts out;const auto [plane_count,planes]=TagAddressSpace(m, bsp.memory_address).block(16+0x158, 20, 2048);
    for(unsigned i=0;i<plane_count;++i) {
        const auto at=planes+i*20;SourceTransparentPlane p{m.u16(at),m.u16(at+2),{}};
        for(unsigned axis=0;axis<4;++axis)p.plane[axis]=m.f32(at+4+axis*4);out.planes.push_back(p);
    }
    const auto decode=[&](std::size_t field,std::size_t stride,bool instance) {
        std::vector<SourceTransparentPartSection> result;const auto [n,start]=TagAddressSpace(m, bsp.memory_address).block(field, stride, 2048);
        for(unsigned i=0;i<n;++i) {
            const auto s=start+i*stride;SourceTransparentPartSection section;section.section=i;
            if(!instance)for(unsigned axis=0;axis<6;++axis)section.cluster_bounds[axis]=m.f32(s+0x54+axis*4);
            const auto [resources,resource]=TagAddressSpace(m, bsp.memory_address).block(s+56, 16, 128);
            if(resources) {
                const auto raw=raw_data(m.u32(s+40),m.u32(s+44));const Bytes b(raw);
                const std::size_t at=std::size_t(m.u32(s+48))+8+m.u32(resource+12),size=m.u32(resource+8);
                b.require(at,size);if(size%72 || size/72>256)throw FormatError("source transparent BSP parts exceed bank");
                for(unsigned j=0;j<size/72;++j) {
                    auto p=part(b,at+j*72,j);shader(p);
                    const unsigned key=j*2+unsigned(instance);
                    //13E270 prepends to the bucket, so the last authored matching
                    //key is the first one returned by1E280/13E2D0.
                    for(const auto &plane:out.planes)if(plane.geometry==i && plane.part_and_instance==key)p.plane=plane.plane;
                    section.parts.push_back(std::move(p));
                }
            }
            result.push_back(std::move(section));
        }
        return result;
    };
    out.clusters=decode(16+0x9c,176,false);out.instance_definitions=decode(16+0x138,200,true);
    const auto [instances,instance]=TagAddressSpace(m, bsp.memory_address).block(16+0x140, 88, 2048);
    for(unsigned i=0;i<instances;++i) {
        const auto at=instance+i*88;SourceTransparentInstance p;p.ordinal=i;p.definition=m.u16(at+0x34);
        if(p.definition>=out.instance_definitions.size())throw FormatError("source transparent instance definition outside BSP");
        p.transform.scale=m.f32(at);for(unsigned column=0;column<3;++column)
            p.transform.basis[column]={m.f32(at+4+column*12),m.f32(at+8+column*12),m.f32(at+12+column*12)};
        p.transform.position={m.f32(at+0x28),m.f32(at+0x2c),m.f32(at+0x30)};
        p.center={m.f32(at+0x3c),m.f32(at+0x40),m.f32(at+0x44)};p.radius=m.f32(at+0x48);out.instances.push_back(p);
    }
    return out;
}
SourceTransparentPart Cache::load_source_transparent_shader(std::uint32_t datum) const {
    SourceTransparentPart out;out.shader=datum;
    const auto found=query_tag({.handle=TagHandle{datum}, .require_address=true});
    if(!found || found->group!="shad")throw FormatError("source transparent shader is not an authored ordinary shader");
    const Bytes m(cache_bytes());const auto root=metadata_offset(found->address,0x3e);
    out.shader_priority_flags=m.u16(root+0x16);out.shader_command_flags=m.u16(root+0x3c);
    
    const auto [definitions,definition]=metadata_block(root+32, 124, 64);
    if(!definitions)return out;
    const auto stem_datum=m.u32(definition);
    const auto stem=query_tag({.handle=TagHandle{stem_datum}, .group=tag_groups::shader_template, .require_address=true});
    if(!stem)throw FormatError("source transparent shader template missing");
    const auto [templates,tp]=metadata_block(metadata_offset(stem->address,96)+88, 40, 64);
    if(!templates)return out;
    const auto [lods,lod]=metadata_block(tp, 10, 64);
    for(unsigned i=0;i<lods;++i)out.template_lod_layers.push_back(m.u32(lod+i*10+2));
    return out;
}
SourceTransparentModelParts Cache::load_source_transparent_model_parts(
    std::uint32_t datum,std::span<const unsigned> collector) const {
    const auto mode=query_tag({.handle=TagHandle{datum}, .group=tag_groups::render_model, .require_address=true});
    if(!mode)throw FormatError("source transparent parts require actual render-model datum");
    const Bytes m(cache_bytes());const auto root=metadata_offset(mode->address,0x68);
    
    const auto [sections,section]=metadata_block(root+0x24, 0x5c, 512);const auto [material_count,materials]=metadata_block(root+0x60, 32, 65535);
    SourceTransparentModelParts out;out.render_model=datum;out.model_flags=m.u16(root+4);
    std::map<std::uint32_t,SourceTransparentPart> shaders;
    for(const auto index:collector) {
        if(index>=sections || index==255)throw FormatError("source transparent model section outside actual collector");
        const auto s=section+index*0x5c;SourceTransparentPartSection selected;selected.section=index;selected.rigid_node=std::int16_t(m.u16(s+0x2c));
        selected.no_map_node_count=std::to_integer<std::uint8_t>(m.slice(s+0x24,1)[0]);
        const auto [resources,resource]=metadata_block(s+0x48, 16, 128);
        if(resources) {
            const auto raw=raw_data(m.u32(s+0x38),m.u32(s+0x3c));const Bytes b(raw);const auto header=m.u32(s+0x44);
            if(std::uint64_t(header)+4>raw.size())throw FormatError("source transparent model resource base invalid");
            const std::size_t at=raw.size()-header-4+m.u32(resource+12),size=m.u32(resource+8);
            const auto node_count=b.u16(0x6c);bool nodes_found=false;
            for(unsigned i=0;i<resources;++i) {
                const auto r=resource+i*16;if(m.u16(r+4)!=100)continue;
                if(nodes_found || m.u32(r+8)<node_count)throw FormatError("source model resident node-map resource invalid");
                nodes_found=true;
                for(const auto v:b.slice(raw.size()-header-4+m.u32(r+12),node_count))
                    selected.resident_node_map.push_back(std::to_integer<std::uint8_t>(v));
            }
            if(node_count && !nodes_found)throw FormatError("source model resident node map missing");
            b.require(at,size);if(size%72 || size/72>256)throw FormatError("source transparent model parts exceed bank");
            for(unsigned j=0;j<size/72;++j) {
                auto p=part(b,at+j*72,j);
                if(p.material>=0 && std::size_t(p.material)<material_count) {
                    p.shader=m.u32(materials+std::size_t(p.material)*32+12);
                    if(p.shader!=0xffffffffu) {
                        if(!shaders.contains(p.shader))shaders.emplace(p.shader,load_source_transparent_shader(p.shader));
                        const auto &sp=shaders.at(p.shader);p.shader_priority_flags=sp.shader_priority_flags;
                        p.shader_command_flags=sp.shader_command_flags;p.template_lod_layers=sp.template_lod_layers;
                    }
                }
                selected.parts.push_back(std::move(p));
            }
        }
        out.sections.push_back(std::move(selected));
    }
    return out;
}
std::vector<std::int16_t> Cache::load_source_transparent_model_sort_orders(
    std::uint32_t datum,std::int32_t variant) const {
    const auto definition=load_initial_model_definition(datum);
    if(definition.model==0xffffffffu)return {};
    const auto hlmt=query_tag({.handle=TagHandle{definition.model}, .group=tag_groups::model, .require_address=true});
    if(!hlmt)throw FormatError("source transparent sort model missing");
    const Bytes b(cache_bytes());const auto root=metadata_offset(hlmt->address,0x78);
    const unsigned regions=b.u32(root+0x70);if(regions>16)throw FormatError("source transparent sort region bank exceeds source");
    std::vector<std::int16_t> out(regions,0);if(variant==-1)return out;
    const auto count=b.u32(root+0x50);if(variant<0 || std::uint32_t(variant)>=count)throw FormatError("source transparent actual variant outside model");
    const auto v=metadata_offset(b.u32(root+0x54),std::size_t(count)*0x38)+std::size_t(variant)*0x38;
    const auto n=b.u32(v+0x14);const auto list=n?metadata_offset(b.u32(v+0x18),std::size_t(n)*20):0;
    for(unsigned i=0;i<regions;++i) {
        const auto index=std::int8_t(std::to_integer<std::uint8_t>(b.slice(v+4+i,1)[0]));
        if(index==-1)continue;if(index<0 || unsigned(index)>=n)throw FormatError("source transparent variant sort region outside authored list");
        out[i]=std::int16_t(b.u16(list+unsigned(index)*20+0x10));
    }
    return out;
}
}
