#include "halo2/source_campaign_secondary_view_tu15.hpp"
#include "halo2/source_campaign_environment_view_tu15.hpp"
#include "halo2/source_model_shader.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_campaign_completed_view_tu15.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_decal_frame_tu15.hpp"
#include "halo2/source_water_reflection_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <map>
#include <set>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4> bytes{};m.read(p,std::span(bytes).first(n));
    std::uint32_t out{};for(unsigned k=0;k<n;++k)out|=std::to_integer<unsigned>(bytes[k])<<(8u*k);return out;
}
std::int16_t short_at(const Bytes &b,unsigned p){return std::bit_cast<std::int16_t>(b.u16(p));}
Vec3 vector_at(const Bytes &b,unsigned p){return {b.f32(p),b.f32(p+4u),b.f32(p+8u)};}
}
SourceCampaignSecondaryViewTu15::SourceCampaignSecondaryViewTu15(const Cache &cache,std::size_t bsp,
    std::span<const Vertex> geometry,SourceCampaignViewTu15 &collector,
    const BspAtmosphere &atmosphere,const BspSkyModels &skies,SourceCampaignWaterWeatherTu15 &families,
    SourceCampaignWaterMaterialBindingsTu15 water_materials,SourceCampaignSecondaryMaterialOwnerTu15 materials,
    pstencil::CampaignRenderer *lighting):cache_(cache),bsp_(bsp),bsp_vertices_(geometry),collector_(collector),
    atmosphere_(atmosphere),skies_(skies),families_(families),water_materials_(std::move(water_materials)),
    materials_(std::move(materials)),lighting_(lighting),decals_(std::make_shared<SourceDecalScene>(cache)) {
    if(!materials_.model || !materials_.presentation)
        throw FormatError("Campaign secondary view requires its actual model/material/presentation owners");
}
std::vector<std::uint32_t> SourceCampaignSecondaryViewTu15::resident_definitions(PhysicsLiveWorldTu15 &world)const {
    return source_campaign_resident_definitions_tu15(cache_,bsp_,world);
}
std::unique_ptr<SourceCampaignSecondaryPacketsTu15> SourceCampaignSecondaryViewTu15::collect(
    PhysicsLiveWorldTu15 &world,SourceCampaignWaterWeatherStateTu15 weather_state,
    SourceTextureCameraFrameTu15 &frame,const SourceTextureCameraScenePrefixTu15 &prefix,
    bool begin_frame,std::uint64_t frame_number,float seconds,bool cinematic) {
    auto &memory=world.memory();prefix.require_current(memory);
    if(world.source_bsp_index()!=bsp_ || !prefix.texture_camera || prefix.actual_local_user!=-1)
        throw FormatError("Secondary campaign collection lacks its actual world/BSP/NONE-user prefix");
    source_texture_camera_complete_scene_prefix_tu15(world,frame,prefix);
    auto out=std::make_unique<SourceCampaignSecondaryPacketsTu15>();
    auto completed=source_campaign_completed_view_tu15(world,prefix,atmosphere_,begin_frame);
    out->environment=completed.environment;out->fog=completed.fog;
    out->collection_rendering=completed.collection_rendering;out->submitting_rendering=completed.submitting_rendering;
    out->shaders=completed.shaders;out->camera=completed.submitting_camera;
    auto input=completed.input;input.environment=&out->environment;
    const auto &rectangle=completed.collection_rectangle;const auto &submitted_rectangle=completed.submitting_rectangle;
    const auto width=unsigned(int(submitted_rectangle[3])-int(submitted_rectangle[1]));
    const auto height=unsigned(int(submitted_rectangle[2])-int(submitted_rectangle[0]));
    const auto eye=input.eye,forward=input.forward;const auto policy=completed.policy;
    const Bytes packet(prefix.packet);
    out->source=std::make_unique<SourceCampaignViewFrameTu15>(collector_.collect(world,input,atmosphere_,out->fog));
    out->background=skies_.pose(out->fog,eye);
    // Source2BCB0 unconditionally emits ordinary cached decal layers0..5.
    // Borrow actual current cache pages and this view's completed GPU bank.
    out->decal_resources=decals_;
    if(decals_->active()) {
        std::array<ShaderVector,192> constants{};
        const auto &particles=completed.input.particles->camera;
        std::copy(particles.camera.begin(),particles.camera.end(),constants.begin());
        std::copy(particles.fog.begin(),particles.fog.end(),constants.begin()+12);
        out->decals=decals_->draw(source_decal_frame_tu15(memory),out->source->bsp,constants,
            {float(submitted_rectangle[1]),float(submitted_rectangle[0]),float(width),float(height)},particles.clip_range);
    }
    out->lens_phases_source_gated=true;
    std::set<unsigned> specialized;
    if(materials_.append_specialized)for(const auto ordinal:materials_.append_specialized(*out->source,
        out->shaders,out->poses,out->objects)) {
        if(!specialized.insert(ordinal).second || std::ranges::none_of(out->source->base_draws,
            [&](const auto &draw){return draw.ordinal==ordinal;}))
            throw FormatError("Secondary specialized material owner consumed an unknown/duplicate source ordinal");
    }
    std::map<std::uint32_t,std::vector<Transform>> poses;
    for(const auto &draw:out->source->base_draws) {
        if(specialized.contains(draw.ordinal))continue;
        if(draw.override_slot!=255)
            throw FormatError("Secondary source model requires its actual override material owner");
        if(!draw.geometry.geometry.resident)continue; // Exact449D0 no-resource return.
        if(!poses.contains(draw.object)) {
            auto nodes=world.rendered_world_nodes(draw.object);
            if(!nodes || nodes->empty())throw FormatError("Secondary source model lost its completed rendered WORLD pose");
            poses.emplace(draw.object,std::move(*nodes));
        }
        const auto &model=materials_.model(draw.definition);
        auto geometry=model.draw(draw.object,std::span{&draw,1},poses.at(draw.object),out->shaders,materials_.baked_lighting);
        if(geometry.empty())continue;
        auto presentation=materials_.presentation(draw);
        if(lighting_)out->shadows.add(world,draw.object,source_geometry_lod_distance(draw.sphere.center,eye,
            input.distance_scale),geometry);
        out->poses.push_back(std::move(geometry));
        out->objects.push_back({out->poses.back(),draw.lighting,std::move(presentation.colors),
            std::move(presentation.shader_input)});
    }
    if(lighting_) {
        pstencil::CampaignLightingInput lighting_input{out->collection_rendering,out->shaders,out->fog,eye,
            forward,input.distance_scale,rectangle,{width,height},std::bit_cast<std::int32_t>(std::uint32_t(frame_number)+1u),cinematic};
        lighting_input.submitting_constants=&completed.input.particles->camera.camera;
        out->lighting=lighting_->draw(world,*out->source,lighting_input);
    }
    out->families=families_.collect_view(world,weather_state,
        {prefix.packet,out->collection_rendering,out->submitting_rendering,out->source->visibility,atmosphere_,out->fog},water_materials_);
    out->camera.position=vector_at(packet,0x18u);out->camera.forward=vector_at(packet,0x24u);
    out->camera.up=vector_at(packet,0x30u);out->camera.oriented=true;
    out->camera.projection_vertical_angle=packet.f32(0x40u);
    auto &view=out->view;view.camera=out->camera;
    view.left=submitted_rectangle[1];view.top=submitted_rectangle[0];view.width=width;view.height=height;
    // These remain SOURCE coordinates. The Renderer owns exact14BA0 scale
    // and actual attachment mapping; no host/window viewport is fabricated.
    view.full_surface=false;
    if(out->families.water_vertices.empty())view.vertices=bsp_vertices_;
    else {
        out->vertices.reserve(bsp_vertices_.size()+out->families.water_vertices.size());
        out->vertices.insert(out->vertices.end(),bsp_vertices_.begin(),bsp_vertices_.end());
        out->vertices.insert(out->vertices.end(),out->families.water_vertices.begin(),out->families.water_vertices.end());
        view.vertices=out->vertices;
    }
    view.background=out->background;view.objects=out->objects;view.source_bsp_admission=&out->source->bsp;
    view.world_particles=out->source->world_particles;view.water=out->families.water;
    view.source_decals=out->decals;
    view.weather_points=out->families.weather_points;view.weather_lines=out->families.weather_lines;view.weather_plates=out->families.weather_plates;
    view.atmosphere=out->fog;view.atmosphere_source=&atmosphere_;view.effect_time=seconds;
    view.projection_clip_planes=Vec2{packet.f32(0x58u),packet.f32(0x5cu)};
    view.source_w_distance=Bytes(memory.view(0x4922f8u,4)).f32(0);view.source_render_policy=policy;
    view.source_view_shader_constants.emplace();
    view.source_view_shader_constants->world=source_view_shader_constants(out->submitting_rendering,packet.f32(0x5cu),false);
    view.source_view_shader_constants->reflection=view.source_view_shader_constants->world;
    view.pstencil_lightmap_casters=out->shadows.casters();
    if(out->lighting) {
        view.raw_effect_lights=out->lighting->draws;view.pstencil_point_casters=out->lighting->casters;
        view.pstencil_native_point_casters=true;
    }
    if(out->families.reflection_eye) {
        const auto &water=families_.water_definitions();
        if(water.definitions.empty())throw FormatError("Secondary reflection lost its actual first water definition");
        const auto reflection=source_water_reflection_tu15(
            std::span<const std::byte,0x74>(prefix.packet.data()+0x18u,0x74u),
            out->submitting_rendering,water.definitions.front().parameters[33]);
        view.water_reflection_source_rendering=reflection.rendering;
        std::copy_n(reflection.constants.begin(),7,view.source_view_shader_constants->reflection.begin());
        view.water_reflection_camera=out->camera;
        view.water_reflection_background=out->background;
    }
    prefix.require_current(memory);return out;
}
}
