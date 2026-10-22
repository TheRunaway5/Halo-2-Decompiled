#include "halo2/source_transparent_group_parts.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include <algorithm>
#include <bit>
#include <cstring>

namespace halo2 {
SourceTransparentGroupParts SourceTransparentGroupPartBindings::resolve(
    const SourceLiveTransparentGroupTu15 &group,std::span<const SourceLiveModelBaseDrawTu15> units) {
    std::array<std::uint32_t,2> payload;
    std::memcpy(payload.data(),group.queue_command.payload.data(),sizeof(payload));
    if(group.queue_command.callback!=0x423b0u || payload!=std::array{group.section,group.flags} ||
       group.section>=31 || group.end_command<group.first_command || group.end_command>65536 ||
       group.commands.size()!=group.end_command-group.first_command)
        throw FormatError("transparent group lost its source callback/payload/command range");
    std::map<unsigned,unsigned> ordinals;
    for(unsigned i=0;i<units.size();++i)if(!ordinals.emplace(units[i].ordinal,i).second)
        throw FormatError("transparent group main unit ordinals are duplicated");
    constexpr std::array order{1u,3u,8u,7u,2u,16u,15u,21u};
    unsigned next_layer=0;SourceTransparentGroupParts result;
    for(const auto &layer:group.ordered_layers) {
        while(next_layer<order.size() && order[next_layer]!=layer.layer)++next_layer;
        if(next_layer==order.size() || !(group.layers&(1u<<layer.layer)))
            throw FormatError("transparent group lost its admitted original layer order");
        ++next_layer;
        auto &bound=result.ordered_layers.emplace_back();bound.layer=layer.layer;
        for(unsigned sorted=0;sorted<layer.descriptors.size();++sorted) {
            const auto &d=layer.descriptors[sorted];const auto ordinal=d[0]&65535u;
            if(ordinal<group.first_command || ordinal>=group.end_command)
                throw FormatError("transparent group descriptor escaped its own command range");
            const auto &c=group.commands[ordinal-group.first_command].words;
            const auto at=ordinals.find(c[3]>>8&65535u);
            if(at==ordinals.end())throw FormatError("transparent group descriptor has no retained main unit");
            const auto &base=units[at->second];const auto &u=base.unit.words;
            if(group.geometry) {
                if(base.ordinal>=group.geometry->units.size())
                    throw FormatError("transparent group part escaped its post-cull main unit bank");
                const auto &native=group.geometry->units[base.ordinal].words;
                // Culling can change admission/mask fields. These immutable
                // model/palette/packed section/context words must still join
                // the earlier owned pose/lighting handoff exactly.
                for(const unsigned word:{1u,2u,3u,4u})if(native[word]!=u[word])
                    throw FormatError("transparent group post-cull unit lost its retained base identity");
            }
            const auto geometry_class=u[3]>>29;const auto part=c[3]&255u;
            if(c[0]!=0x4cbe0u || c[1]!=0x4d0a0u || !(c[4]&(1u<<layer.layer)) ||
               (d[0]>>16)!=(c[2]&65535u) || ((u[3]>>4)&31u)!=group.section ||
               d[5]!=u[4] || ((d[1]>>8)&255u)!=(geometry_class<=1?1u:2u) ||
               (geometry_class && d[3]!=((u[3]>>9&511u)|((u[1]&65535u)<<11))) ||
               u[1]==0xffffffffu || base.object==0xffffffffu ||
               base.lighting_class!=(u[0]>>29) || !base.geometry.geometry.resident ||
               part>=base.geometry.geometry.parts.size())
                throw FormatError("transparent group descriptor lost its main owner/geometry/part identity");
            const Bytes original(base.geometry.geometry.parts[part]);
            const auto material=std::bit_cast<std::int16_t>(original.u16(4));
            if(!original.u16(0) || original.u16(0)==5 || material<0 ||
               unsigned(material)>=base.geometry.materials.size())
                throw FormatError("transparent group submitted an unadmitted source model part");
            const auto expected=base.override_slot==255?base.geometry.materials[unsigned(material)].shader:base.shader_override;
            if(expected==0xffffffffu || expected!=c[2] ||
               (layer.layer==21 && base.override_slot==255))
                throw FormatError("transparent group part lost its actual shader override owner");
            const auto lod=c[5]>>8&255u,pass=c[5]&255u,implementation=d[1]&255u;
            const Key key{c[2],lod,layer.layer,pass,implementation};
            auto selected=passes_.find(key);
            if(selected==passes_.end())selected=passes_.emplace(key,std::make_shared<const MenuWorldSourcePass>(
                load_menu_world_source_pass(cache_,c[2],{lod,layer.layer,pass,implementation}))).first;
            const auto &metadata=*selected->second;
            if(metadata.implementation_address()!=d[4])
                throw FormatError("transparent group descriptor changed its actual implementation address");
            const bool transparent=original.u16(0)==4 || original.u16(0)==5;
            const auto class_key=std::tuple{u[1],unsigned(u[3]>>9&511u),metadata.vertex_shader(),transparent};
            auto compiled=classes_.find(class_key);
            if(compiled==classes_.end()) {
                const auto geometry=load_source_opaque_model_geometry_metadata(cache_,u[1],u[3]>>9&511u);
                const auto vertex=cache_.load_source_tag_metadata(metadata.vertex_shader());
                const auto count=Bytes(vertex.bytes_at(vertex.source_address(),12)).u32(4);
                const auto cls=source_opaque_vertex_class(geometry.geometry_class,geometry.selectors[transparent?1:0],[&]{return count;});
                if(cls>=count || cache_.load_vertex_shader_code(metadata.vertex_shader(),cls).empty())
                    throw FormatError("transparent group source-selected vertex program is absent");
                compiled=classes_.emplace(class_key,cls).first;
            }
            if(compiled->second!=(d[2]&65535u))
                throw FormatError("transparent group descriptor changed its source-selected vertex class");
            bound.parts.push_back({at->second,ordinal,sorted,selected->second});
            if(std::ranges::find(result.owners,base.object)==result.owners.end())result.owners.push_back(base.object);
        }
    }
    return result;
}
}
