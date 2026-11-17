#include "halo2/pstencil/campaign_renderer.hpp"
#include "halo2/pstencil/campaign_lights.hpp"
#include "halo2/pstencil/projected_lights.hpp"
#include "halo2/pstencil/point_lights.hpp"
#include "halo2/pstencil/native_light_receiver_tu15.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/source_model_shader.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/path_tracing/source_lights.hpp"
#include <algorithm>
#include <bit>
#include <iterator>

namespace halo2::pstencil {
struct CampaignRenderer::Impl {
    CampaignLights lights;
    SourceBspAdmissionBsp geometry;
    BspAtmosphere atmosphere;
    std::unique_ptr<SourceBspEffectLights> spheres;
    std::unique_ptr<ProjectedLights> projected;
    std::map<std::uint32_t,SourceModelPaletteDefinition> palette_definitions;
    Impl(const Cache& cache,Mesh& mesh,unsigned bsp)
      :lights(cache,bsp),geometry(cache.load_source_bsp_admission(bsp)),
       atmosphere(cache.load_bsp_atmosphere(bsp)) {
        std::vector<std::uint32_t> models;
        for(const auto& tag:cache.tags())if(tag.group=="mode")models.push_back(tag.datum);
        const auto definitions=lights.definitions();
        auto shared=std::make_shared<SharedLightMaterials>(cache,mesh);
        spheres=std::make_unique<SourceBspEffectLights>(cache,mesh,bsp,definitions,models,shared);
        projected=std::make_unique<ProjectedLights>(cache,mesh,bsp,std::move(shared));
    }
};
CampaignRenderer::CampaignRenderer(const Cache& cache,Mesh& mesh,unsigned bsp)
    :impl_(std::make_unique<Impl>(cache,mesh,bsp)){}
CampaignRenderer::~CampaignRenderer()=default;
CampaignLightingFrame CampaignRenderer::draw(PhysicsLiveWorldTu15& world,
    const SourceCampaignViewFrameTu15& main,const CampaignLightingInput& input) {
    if(!main.main_commands_completed)
        throw FormatError("pStencil campaign lighting requires a completed actual main view");
    auto& i=*impl_;
    const auto frustums=main.visibility.sphere_bank();
    SourceEffectLightCollectionContext collection{};
    collection.visibility.frustums=&frustums;
    collection.containing_cluster=std::int16_t(input.fog.cluster);
    collection.owners={1,0,0,2,{}};
    SourceEffectLightCandidateContext candidates{};candidates.admission=collection.owners;
    const auto& inverse=input.camera.inverse_view;
    candidates.depth_plane={inverse.basis[0].z,inverse.basis[1].z,inverse.basis[2].z,inverse.position.z};
    candidates.pixel_scale=input.camera.viewport_half_metrics.y;
    auto lights=i.lights.view(world,input.eye,collection,candidates,input.frame,input.cinematic,true);
    CampaignLightingFrame out;out.lights=lights.records.size();
    if(input.path_tracing) {
        out.path_tracing_lights=path_tracing::source_lights(world,lights.records,
            [&](std::uint32_t owner,std::uint16_t node)->std::optional<Transform> {
                if(!input.presented_model_nodes)return {};
                const auto shown=input.presented_model_nodes->find(owner);
                if(shown==input.presented_model_nodes->end() || std::size_t(node)>=shown->second.size())return {};
                return shown->second.at(node);
            });
    }
    // Secondary native model queries update the same per-view LOD hysteresis
    // consumed by the next main view, including light-only/off-camera owners.
    // Every lighting backend must complete them; PT only omits raster packets.
    i.lights.select_native(world,lights,candidates,
        std::bit_cast<std::int32_t>(collection.local_player),input.frame,input.cinematic);
    include_active_point_lights(lights);
    const auto containing=source_bsp_fog_admission(i.atmosphere,{true,true,input.fog.planar_datum,0});
    if(!main.open_portals)throw FormatError("Campaign light receivers lost this main view's actual portal mask");
    const auto receivers=i.lights.receivers(lights,i.geometry,containing,&main.bsp,*main.open_portals);
    SourceEffectLightModelViewTu15 models{world,main.objects,{},containing};
    models.context.entry.distance_scale=input.distance_scale;
    models.context.entry.coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
    models.context.forward=input.forward;models.context.view_modifier=input.shaders.view_modifier;
    models.completed_campaign_main=&main;
    using ModelKey=std::pair<std::uint32_t,std::uint32_t>;
    std::map<ModelKey,unsigned> presented_lods;
    std::map<ModelKey,std::vector<std::array<float,12>>> presented_palettes;
    if(input.presented_model_nodes) {
        for(const auto& draw:main.base_draws) {
            if(draw.override_slot!=255 || !input.presented_model_nodes->contains(draw.object))continue;
            const auto [at,inserted]=presented_lods.emplace(ModelKey{draw.object,draw.unit.words[1]},draw.lod);
            if(!inserted && at->second!=draw.lod)
                throw FormatError("pStencil presented model changed its completed geometry LOD");
        }
        models.presented_palette=[&](std::uint32_t object,std::uint32_t model)
            ->std::span<const std::array<float,12>> {
            const ModelKey key{object,model};
            const auto lod=presented_lods.find(key);
            if(lod==presented_lods.end())return {};
            if(const auto palette=presented_palettes.find(key);palette!=presented_palettes.end())
                return palette->second;
            auto definition=i.palette_definitions.find(model);
            if(definition==i.palette_definitions.end())
                definition=i.palette_definitions.emplace(model,i.spheres->resident_cache().load_source_model_palette_definition(model)).first;
            auto palette=source_model_skin_palette(input.presented_model_nodes->at(object),definition->second.inverse_binds);
            append_source_model_compounds(palette,definition->second.compounds.at(lod->second));
            return presented_palettes.emplace(key,std::move(palette)).first->second;
        };
    }
    out.lights=lights.records.size();out.selected=lights.selected.size();
    for(const auto& row:lights.records)if(row.definition && Bytes(row.definition->bytes).u16(4)) {
        ++out.projected_records;
        const auto& p=row.runtime.position;out.placed_projected_records+=p.x!=0 || p.y!=0 || p.z!=0;
    }
    // Sphere queries run first while the current context still owns the main
    // frame. Both paths retain the same main prefix and independent casters.
    out.draws=i.spheres->draw(lights,receivers,main.bsp,input.camera,input.fog,input.eye,
        input.viewport,input.surface,1,&models,input.path_tracing?nullptr:&out.casters,2,
        input.submitting_constants,!input.path_tracing);
    unsigned shadow_targets=0; // Light shadow depth targets used this frame.
    for(const auto& selected:lights.selected) {
        const auto at=std::find_if(lights.records.begin(),lights.records.end(),
            [&](const auto& row){return row.runtime.identity==selected.identity;});
        if(at==lights.records.end() || !at->definition)
            throw FormatError("pStencil campaign selection lost its actual light");
        if(!Bytes(at->definition->bytes).u16(4))continue;
        const auto receiver=native_light_receiver_tu15(world,selected.identity);
        const auto geometry=native_light_geometry_tu15(world,receiver);
        const bool native_original=input.original || input.path_tracing;
        auto projected=i.projected->draw(world,*at,selected.requested_fade,receiver,geometry,
            input.camera,input.eye,input.viewport,input.surface,out.casters,2,input.submitting_constants,
            models.presented_palette,native_original,native_original?&shadow_targets:nullptr,!input.path_tracing);
        ++out.projected_lights;out.projected_draws+=unsigned(projected.draws.size());
        out.draws.insert(out.draws.end(),std::make_move_iterator(projected.draws.begin()),
            std::make_move_iterator(projected.draws.end()));
    }
    return out;
}
}
