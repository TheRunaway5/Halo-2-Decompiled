#include "halo2/object_light_contacts.hpp"
#include <algorithm>
#include <limits>
namespace halo2 {
bool ObjectLightContactDefinition::model_attribution_available() const {
    return model!=0xffffffffu && render_model!=0xffffffffu && collision!=0xffffffffu &&
        std::int32_t(prt_count)>0 && std::int32_t(section_render_leaves_count)>0;
}
ObjectLightContactAction source_object_light_contact_action(std::int16_t type,
    std::int16_t policy,bool available) {
    if(type==6 && policy!=2)return ObjectLightContactAction::reject_scenery;
    return available?ObjectLightContactAction::attribute_model:ObjectLightContactAction::fallback_map;
}
std::uint16_t source_object_light_spatial_flags(const ObjectLightSpatialState& state) {
    std::uint16_t flags{};
    if((state.handle_flags&0x10) || (state.object_flags&1))flags|=1;
    if(state.object_flags&0x00100000)flags|=2;
    if(!(state.object_flags&0x00400000))flags|=4;
    if(!(state.object_flags&0x80000000))flags|=8;
    if(state.object_type==0) {
        if(state.unit_flags_10a&4)flags|=0x10;
        if(state.biped_flags_348&8)flags|=0x20;
    } else if(state.object_type==11 && (state.type11_flags_12c&2))flags|=0x40;
    return flags;
}
std::uint16_t source_object_light_collision_exclusion_mask(std::uint32_t query) {
    std::uint16_t flags=1;
    if(query&0x00080000)flags|=2;
    if(query&0x00040000)flags|=4;
    if(query&0x40000000)flags|=8;
    if(query&0x00100000)flags|=0x10;
    if(query&0x00400000)flags|=0x20;
    if(query&0x00200000)flags|=0x40;
    return flags;
}
bool source_object_light_spatial_contact_admitted(std::uint32_t query,std::uint8_t type,
    std::uint16_t flags,std::uint32_t object,std::uint32_t excluded_a,std::uint32_t excluded_b) {
    if(!(query&8))return false;
    if(!(query&0x1fff0u))query|=0x1fff0u;
    return ((query>>4)&0x1fffu)&(1u<<(type&31u)) &&
        !(flags&source_object_light_collision_exclusion_mask(query)) &&
        object!=excluded_a && object!=excluded_b;
}
std::optional<ObjectLightRenderReference> source_object_light_render_reference(
    const ObjectLightRenderLeaves::Section& section,std::int16_t node,
    std::uint32_t collision_leaf_count,std::uint32_t leaf,std::uint32_t plane,
    std::span<const std::uint16_t> planes,
    const std::function<bool(const ObjectLightRenderReference&)>& accept) {
    const auto found=std::find(section.nodes.begin(),section.nodes.end(),node);
    if(found==section.nodes.end())return {};
    const auto ordinal=std::size_t(found-section.nodes.begin());
    if(ordinal>=section.maps.size())throw FormatError("missing authored object render node map");
    const auto& map=section.maps[ordinal];
    if(map.leaves.size()!=collision_leaf_count)return {};
    if(leaf>=map.leaves.size())throw FormatError("object contact leaf escapes authored bank");
    const auto& l=map.leaves[leaf];
    const auto end=std::int64_t(l.first)+l.count;
    if(end<std::numeric_limits<std::int32_t>::min() || end>std::numeric_limits<std::int32_t>::max())
        throw FormatError("object render reference range overflows original signed count");
    for(std::int64_t index=l.first;index<end;++index) {
        if(index<0 || std::size_t(index)>=map.references.size())continue;
        const auto& r=map.references[std::size_t(index)];
        if(r.collision_node==-1)return {};
        if(r.collision_node<0)throw FormatError("invalid negative object render collision node");
        if(std::size_t(r.collision_node)>=planes.size() ||
           planes[std::size_t(r.collision_node)]!=(plane&0x7fffu))continue;
        const ObjectLightRenderReference candidate{std::int32_t(index),r.strip,r.lightmap_triangle};
        if(accept(candidate))return candidate;
    }
    return {};
}
std::optional<ObjectLightContactDefinition> Cache::load_object_light_contact_definition(
    std::uint32_t datum) const {
    const auto find=[&](std::uint32_t id,const char* group)->const Tag* {
        const auto tag=query_tag({.handle=TagHandle{id}, .group=group ? std::optional<TagGroup>{TagGroup::from_string(group)} : std::nullopt, .require_address=true});
        return !tag?nullptr:&*tag;
    };
    const auto object=find(datum,nullptr);
    if(!object)return {};
    if(object->size<0x3c)throw FormatError("truncated object light contact definition");
    const Bytes data(storage_);
    const auto root=metadata_offset(object->address,object->size);
    ObjectLightContactDefinition out;out.datum=datum;out.type=std::int16_t(data.u16(root));
    if(out.type<0 || out.type>12)throw FormatError("light contact definition is not an object");
    if(out.type==6) {
        if(object->size<0xc2)throw FormatError("truncated scenery light contact policy");
        out.scenery_policy=std::int16_t(data.u16(root+0xc0));
    }
    out.model=data.u32(root+0x38);
    if(out.model==0xffffffffu)return out;
    const auto model=find(out.model,"hlmt");
    if(!model || model->size<0x10)throw FormatError("missing object light contact model");
    const auto base=metadata_offset(model->address,model->size);
    out.render_model=data.u32(base+4);out.collision=data.u32(base+0xc);
    if(out.render_model==0xffffffffu || out.collision==0xffffffffu)return out;
    const auto render=find(out.render_model,"mode");
    if(!render || render->size<0x84)throw FormatError("missing object light contact render model");
    const auto geometry=metadata_offset(render->address,render->size);
    out.prt_count=data.u32(geometry+0x74);
    out.section_render_leaves_count=data.u32(geometry+0x7c);
    return out;
}
ObjectLightRenderLeaves Cache::load_object_light_render_leaves(std::uint32_t datum) const {
    const auto render=query_tag({.handle=TagHandle{datum}, .group=tag_groups::render_model, .require_address=true});
    if(!render || render->size<0x84)throw FormatError("missing object render leaf model");
    const Bytes data(storage_);
    const auto root=metadata_offset(render->address,render->size);

    ObjectLightRenderLeaves out;out.render_model=datum;
    const auto geometry=metadata_block(root+0x24, 0x5c, 1000000),sections=metadata_block(root+0x7c, 8, 1000000);
    if(sections.count>geometry.count)throw FormatError("object render leaf section escapes geometry");
    for(std::size_t section=0;section<sections.count;++section) {
        ObjectLightRenderLeaves::Section decoded;
        const auto record=geometry.offset+section*0x5c;
        const auto raw=raw_data(data.u32(record+0x38),data.u32(record+0x3c));
        const Bytes bytes(raw);const auto header=data.u32(record+0x44);
        if(std::uint64_t(header)+4>bytes.size())throw FormatError("invalid object render raw header");
        const auto base=bytes.size()-header-4;
        const auto resources=metadata_block(record+0x48, 16, 1000000);
        const auto count=bytes.u16(0x6c);
        for(std::size_t i=0;i<resources.count;++i) {
            const auto p=resources.offset+i*16;
            if(data.u16(p+4)!=100)continue;
            const auto start=std::uint64_t(base)+data.u32(p+12);
            const auto size=data.u32(p+8);
            if(start>bytes.size() || size>bytes.size()-start || size<count)
                throw FormatError("invalid object render node stream");
            decoded.nodes.clear();
            for(auto b:bytes.slice(std::size_t(start),count))
                decoded.nodes.push_back(std::to_integer<std::uint8_t>(b));
        }
        if(decoded.nodes.size()!=count)throw FormatError("missing object render node stream");
        const auto maps=metadata_block(sections.offset+section*8, 16, 1000000);
        for(std::size_t node=0;node<maps.count;++node) {
            ObjectLightRenderLeaves::NodeMap map;
            const auto p=maps.offset+node*16;
            const auto leaves=metadata_block(p, 8, 1000000),references=metadata_block(p+8, 8, 1000000);
            for(std::size_t i=0;i<leaves.count;++i) {
                const auto q=leaves.offset+i*8;
                map.leaves.push_back({std::int16_t(data.u16(q)),std::int16_t(data.u16(q+2)),
                    std::int32_t(data.u32(q+4))});
            }
            for(std::size_t i=0;i<references.count;++i) {
                const auto q=references.offset+i*8;
                map.references.push_back({data.u16(q),data.u16(q+2),std::int32_t(data.u32(q+4))});
            }
            decoded.maps.push_back(std::move(map));
        }
        out.sections.push_back(std::move(decoded));
    }
    return out;
}
} // namespace halo2
