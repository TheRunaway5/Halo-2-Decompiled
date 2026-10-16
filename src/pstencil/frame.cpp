#include "halo2/pstencil/frame.hpp"
#include "halo2/pstencil/caster.hpp"
#include <algorithm>
#include <bit>

namespace halo2::pstencil {
MenuWorldRasterPacket receiver_raster(MenuWorldRasterPacket raster) {
    // May2003 E88D0/E8CA0: KEEP,KEEP,KEEP,EQUAL,80,FF,0.
    raster.source_stencil_mode=0;
    raster.stencil.enabled=true;
    raster.stencil.values=MenuWorldRasterStencilValues{0x1e00,0x1e00,0x1e00,0x202,0x80,0xff,0};
    return raster;
}
LightBatch& Frame::light(std::size_t view,std::uint32_t identity,std::uint32_t domain,Vec3 position) {
    const auto same_position=[&](Vec3 value) {
        return std::bit_cast<std::array<std::uint32_t,3>>(value)==
            std::bit_cast<std::array<std::uint32_t,3>>(position);
    };
    auto& lights=views_.at(view);
    auto at=std::find_if(lights.begin(),lights.end(),[&](const auto& light) {
        return light.domain==domain && light.identity==identity && same_position(light.position);
    });
    if(at==lights.end()) {
        lights.push_back({identity,position,{},{},0,{}});at=std::prev(lights.end());at->domain=domain;
    }
    return *at;
}
void Frame::add_source(LightBatch& batch,const MenuWorldSourceBinding& source,unsigned stipple_level) {
    const auto already=std::any_of(batch.source_ranges.begin(),batch.source_ranges.end(),[&](const auto* previous) {
        const auto& other=*previous;
        return source.owner()==other.owner() && source.draw().geometry==other.draw().geometry &&
            source.draw().part==other.draw().part && source.draw().topology==other.draw().topology &&
            source.draw().index_range()==other.draw().index_range() &&
            source.draw().object_rows==other.draw().object_rows &&
            source.declaration().class_index==other.declaration().class_index;
    });
    if(shadow_casting_part(source.draw().geometry->parts().at(source.draw().part).type) && !already) {
        append_volume(batch.volume,source,batch.position,stipple_level);
        batch.source_ranges.push_back(&source);
    }
}
void Frame::add(std::size_t view,const EffectLightRawDraw& draw,std::size_t receiver,
    std::span<const std::size_t> specular,bool collect_caster) {
    if(!draw.point_light_position)throw FormatError("pStencil light is missing its actual source origin");
    auto& batch=light(view,draw.light_identity,draw.pstencil_light_domain,*draw.point_light_position);
    batch.receivers.push_back(receiver);
    batch.specular_receivers.insert(batch.specular_receivers.end(),specular.begin(),specular.end());
    if(collect_caster && draw.pstencil_casts_shadow)add_source(batch,draw.source);
}
void Frame::add_caster(std::size_t view,const PointCaster& caster) {
    if(!caster.opacity)return;
    shadow_only_+=caster.source.draw().geometry->parts().at(caster.source.draw().part).type==1;
    without_color_+=!caster.color_available;fractional_+=caster.opacity<255;
    auto& batch=light(view,caster.light_identity,caster.light_domain,caster.light_position);
    add_source(batch,caster.source,shadow_stipple_level(caster.opacity));
}
void Frame::add_casters(std::size_t view,std::span<const Vertex> vertices,
    std::span<const std::uint8_t> materials) {
    if(vertices.size()%3)throw FormatError("pStencil dynamic caster has incomplete triangles");
    for(std::size_t first=0;first<vertices.size();first+=3) {
        if(!shadow_casting_triangle(std::span<const Vertex,3>(vertices.data()+first,3),materials))continue;
        const std::array points{vertices[first].position,vertices[first+1].position,vertices[first+2].position};
        constexpr std::array<std::array<std::uint16_t,3>,1> faces{{{0,1,2}}};
        for(auto& light:views_.at(view)) {
            append_volume(light.volume,points,faces,light.position);++light.dynamic_triangles;
        }
    }
}
bool Frame::empty() const {
    const auto empty=[](const auto& views){return std::all_of(views.begin(),views.end(),[](const auto& lights){return lights.empty();});};
    return empty(views_) && empty(lightmaps_);
}
void Frame::add_lightmap(std::size_t view,const LightmapCaster& caster,std::span<const std::uint8_t> materials) {
    if(caster.vertices.size()%3)throw FormatError("pStencil lightmap caster has incomplete triangles");
    LightBatch batch{};batch.position=lightmap_light_origin(caster.incident);
    constexpr std::array<std::array<std::uint16_t,3>,1> face{{{0,1,2}}};
    for(std::size_t first=0;first<caster.vertices.size();first+=3) {
        if(!shadow_casting_triangle(std::span<const Vertex,3>(caster.vertices.data()+first,3),materials))continue;
        const std::array points{caster.vertices[first].position,caster.vertices[first+1].position,caster.vertices[first+2].position};
        append_volume(batch.volume,points,face,batch.position);++batch.dynamic_triangles;
    }
    if(!batch.volume.empty())lightmaps_.at(view).push_back(std::move(batch));
}
std::string Frame::describe() const {
    std::size_t lights{},vertices{},ranges{},dynamic{},specular{};
    for(const auto& view:views_)for(const auto& light:view) {
        ++lights;vertices+=light.volume.size();ranges+=light.source_ranges.size();
        dynamic+=light.dynamic_triangles;specular+=light.specular_receivers.size();
    }
    std::size_t baked{};for(const auto& view:lightmaps_)baked+=view.size();
    return "pStencil: lightmap_casters="+std::to_string(baked)+" lights="+std::to_string(lights)+" source_ranges="+std::to_string(ranges)+
        " shadow_only_parts="+std::to_string(shadow_only_)+" casters_without_color_draw="+std::to_string(without_color_)+
        " fractional_casters="+std::to_string(fractional_)+
        " volume_vertices="+std::to_string(vertices)+" dynamic_caster_triangles="+std::to_string(dynamic)+
        " specular_draws="+std::to_string(specular);
}
void Frame::prepare(Render& renderer) {
    std::vector<VolumeBatch> batches;
    for(auto& lights:views_)for(auto& light:lights) {
        light.upload=batches.size();batches.push_back({light.volume});
    }
    for(auto& lights:lightmaps_)for(auto& light:lights) {
        light.upload=batches.size();batches.push_back({light.volume});
    }
    renderer.prepare(batches);
}
} // namespace halo2::pstencil
