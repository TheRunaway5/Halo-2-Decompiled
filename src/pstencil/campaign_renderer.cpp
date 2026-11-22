#include "halo2/pstencil/campaign_renderer.hpp"
#include "campaign_lighting_timing.hpp"
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
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace halo2::pstencil {
namespace {
class TimingOutputEnvironment {
    int error_=errno;
    std::fenv_t fp_{};
    bool saved_=std::fegetenv(&fp_)==0;
public:
    ~TimingOutputEnvironment() {
        if(saved_)std::fesetenv(&fp_);
        errno=error_;
    }
};
std::ofstream* campaign_lighting_trace() noexcept {
    static auto* trace=[]() noexcept -> std::ofstream* {
        const auto* path=std::getenv("HALO2_CAMPAIGN_LIGHT_TIMINGS");
        if(!path || !*path)return nullptr;
        const TimingOutputEnvironment environment;
        try {
            static std::ofstream output(path);
            if(output) {
                output<<"source_frame\tbsp\tmode\twidth\theight\tcinematic\tlight_records\tselected_lights"
                    "\traw_packets\tpoint_casters\tprojected_lights\tprojected_draws\ttotal_ns";
                for(const auto phase:campaign_light_phase_names)output<<'\t'<<phase<<"_ns\t"<<phase<<"_calls";
                for(const auto counter:campaign_light_work_names)output<<'\t'<<counter;
                output<<'\n';
                if(output)return &output;
            }
        } catch(...) {} // Diagnostic failures must leave campaign lighting available.
        std::fputs("Cannot open HALO2_CAMPAIGN_LIGHT_TIMINGS\n",stderr);
        return nullptr;
    }();
    return trace;
}
void write_campaign_lighting_trace(std::ofstream& trace,const CampaignLightingInput& input,unsigned bsp,
    const CampaignLightingFrame& frame,const CampaignLightingPassTimings& timing,std::uint64_t total) noexcept {
    const TimingOutputEnvironment environment;
    try {
        trace<<std::bit_cast<std::uint32_t>(input.frame)<<'\t'<<bsp<<'\t'
            <<(input.path_tracing?"Path Tracing":input.original?"Original":"pStencil")
            <<'\t'<<input.surface[0]<<'\t'<<input.surface[1]<<'\t'<<input.cinematic<<'\t'<<frame.lights
            <<'\t'<<frame.selected<<'\t'<<frame.draws.size()<<'\t'<<frame.casters.size()
            <<'\t'<<frame.projected_lights<<'\t'<<frame.projected_draws<<'\t'<<total;
        for(const auto& phase:timing.phases)trace<<'\t'<<phase.ns<<'\t'<<phase.calls;
        trace<<'\t'<<timing.work.native_object_admissions<<'\t'<<timing.work.stamped_scans
            <<'\t'<<timing.work.stamped_bulk_scans<<'\t'<<timing.work.stamped_scalar_probes
            <<'\t'<<timing.work.stamped_bulk_probes;
        trace<<'\n';
    } catch(...) {} // Timing output is independent of the completed draw.
}
}
struct CampaignRenderer::Impl {
    CampaignLights lights;
    SourceBspAdmissionBsp geometry;
    BspAtmosphere atmosphere;
    unsigned bsp;
    std::unique_ptr<SourceBspEffectLights> spheres;
    std::unique_ptr<ProjectedLights> projected;
    std::map<std::uint32_t,SourceModelPaletteDefinition> palette_definitions;
    Impl(const Cache& cache,Mesh& mesh,unsigned bsp)
      :lights(cache,bsp),geometry(cache.load_source_bsp_admission(bsp)),
       atmosphere(cache.load_bsp_atmosphere(bsp)),bsp(bsp) {
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
    auto* trace=campaign_lighting_trace();
    CampaignLightingPassTimings timings;
    auto* timing=trace?&timings:nullptr;
    using Phase=CampaignLightPhase;
    const auto frame_begin=timing?campaign_lighting_now():CampaignLightingClock::time_point{};
    CampaignLightPhaseTimer collection_time(timing,Phase::collection);
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
    collection_time.finish();
    // Secondary native model queries update the same per-view LOD hysteresis
    // consumed by the next main view, including light-only/off-camera owners.
    // Every lighting backend must complete them; PT only omits raster packets.
    time_campaign_light_phase(timing,Phase::native_selection,[&] {
        i.lights.select_native(world,lights,candidates,
            std::bit_cast<std::int32_t>(collection.local_player),input.frame,input.cinematic);
        include_active_point_lights(lights);
    });
    CampaignLightPhaseTimer receivers_time(timing,Phase::point_receivers);
    const auto containing=source_bsp_fog_admission(i.atmosphere,{true,true,input.fog.planar_datum,0});
    if(!main.open_portals)throw FormatError("Campaign light receivers lost this main view's actual portal mask");
    const auto receivers=i.lights.receivers(lights,i.geometry,containing,&main.bsp,*main.open_portals);
    receivers_time.finish();
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
    static const bool reference_casters=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_ORIGINAL_POINT_CASTERS");
        return value && *value;
    }();
    const bool collect_point_casters=!input.path_tracing && (!input.original || reference_casters);
    // Sphere queries run first while the current context still owns the main
    // frame. The pStencil point-caster bank belongs to its stencil consumer.
    out.draws=time_campaign_light_phase(timing,Phase::point_overlays,[&] {
        return i.spheres->draw(lights,receivers,main.bsp,input.camera,input.fog,input.eye,
            input.viewport,input.surface,1,&models,collect_point_casters?&out.casters:nullptr,2,
            input.submitting_constants,!input.path_tracing);
    });
    static const bool reference_receiver_snapshot=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_PROJECTED_RECEIVER_SNAPSHOT");
        return value && *value;
    }();
    unsigned shadow_targets=0; // Light shadow depth targets used this frame.
    for(const auto& selected:lights.selected) {
        const auto at=std::find_if(lights.records.begin(),lights.records.end(),
            [&](const auto& row){return row.runtime.identity==selected.identity;});
        if(at==lights.records.end() || !at->definition)
            throw FormatError("pStencil campaign selection lost its actual light");
        if(!Bytes(at->definition->bytes).u16(4))continue;
        const auto receiver=time_campaign_light_phase(timing,Phase::projected_receiver,[&] {
            return native_light_receiver_tu15(world,selected.identity,
                !input.original || reference_receiver_snapshot,timing);
        });
        const auto geometry=time_campaign_light_phase(timing,Phase::projected_geometry,[&] {
            return native_light_geometry_tu15(world,receiver,timing);
        });
        const bool native_original=input.original || input.path_tracing;
        auto projected=i.projected->draw(world,*at,selected.requested_fade,receiver,geometry,
            input.camera,input.eye,input.viewport,input.surface,out.casters,2,input.submitting_constants,
            models.presented_palette,native_original,native_original?&shadow_targets:nullptr,
            !input.path_tracing,collect_point_casters,timing);
        ++out.projected_lights;
        for(const auto& draw:projected.draws)out.projected_draws+=draw.draw_count();
        out.draws.insert(out.draws.end(),std::make_move_iterator(projected.draws.begin()),
            std::make_move_iterator(projected.draws.end()));
    }
    if(trace)write_campaign_lighting_trace(*trace,input,i.bsp,out,timings,campaign_lighting_elapsed(frame_begin));
    return out;
}
}
