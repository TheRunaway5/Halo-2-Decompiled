#include "halo2/pstencil/campaign_lights.hpp"
#include "halo2/pstencil/native_lights.hpp"
#include "halo2/pstencil/native_light_receiver_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <algorithm>
#include <bit>
#include <set>
#include "halo2/source_tag_metadata.hpp"

namespace halo2::pstencil {
struct CampaignLights::Impl {
    unsigned bsp;
    BspAtmosphere locations;
    ObjectLightSpatialBsp memberships;
    std::map<std::uint32_t,EffectLightDefinition> definitions;
    std::set<std::uint32_t> supported;
    SourceEffectLightQueryState queries;
    std::array<SourceShadowSelectionCache,4> selection;
    std::vector<std::string> gaps;
    Impl(const Cache& cache,unsigned index):bsp(index),locations(cache.load_bsp_atmosphere(index)),
        memberships(cache.load_object_light_spatial_bsp(index)) {
        for(const auto& tag:cache.tags())if(tag.group=="ligh") {
            try {
                definitions.emplace(tag.datum,load_effect_light(cache,tag.datum));supported.insert(tag.datum);
            }catch(const FormatError& e) {
                gaps.push_back(tag.name+": "+e.what());
                // Literal metadata is sufficient for the common native
                // distance/PVS stages. It does not admit an unresolved shape,
                // HSV or animation branch to the sphere renderer below.
                const auto metadata=cache.load_source_tag_metadata(tag.datum);
                const auto raw=metadata.bytes_at(metadata.source_address(),228);const Bytes bytes(raw);
                EffectLightDefinition definition;std::copy(raw.begin(),raw.end(),definition.bytes.begin());
                definition.flags=bytes.u32(0);definition.lens=bytes.u32(0x94);
                definition.radius=bytes.f32(0x18);definition.specular_radius=bytes.f32(0x1c);definition.duration=bytes.f32(0xb0);
                definitions.emplace(tag.datum,std::move(definition));
            }
        }
    }
};
CampaignLights::CampaignLights(const Cache& cache,unsigned bsp):impl_(std::make_unique<Impl>(cache,bsp)){}
CampaignLights::~CampaignLights()=default;
std::vector<std::uint32_t> CampaignLights::definitions()const {
    return {impl_->supported.begin(),impl_->supported.end()};
}
const std::vector<std::string>& CampaignLights::unavailable()const{return impl_->gaps;}
SourceEffectLightView CampaignLights::view(PhysicsLiveWorldTu15& world,Vec3 eye,
    SourceEffectLightCollectionContext collection,SourceEffectLightCandidateContext candidates,
    std::int32_t frame,bool cinematic) {
    return view(world,eye,std::move(collection),std::move(candidates),frame,cinematic,false);
}
SourceEffectLightView CampaignLights::view(PhysicsLiveWorldTu15& world,Vec3 eye,
    SourceEffectLightCollectionContext collection,SourceEffectLightCandidateContext candidates,
    std::int32_t frame,bool cinematic,bool defer_native_selection) {
    auto& i=*impl_;auto snapshot=native_lights_tu15(world.memory(),i.memberships.clusters.size());
    const SourceEffectLightDefinitions definition=[&](auto id)->const EffectLightDefinition& {
        const auto at=i.definitions.find(id);
        if(at==i.definitions.end())throw FormatError("pStencil campaign consumed an unresolved native light definition");
        return at->second;
    };
    const auto priority=[&](std::uint32_t owner) {
        if(owner!=0xffffffffu)(void)world.source_object_body(owner);
        auto call=world.prepare_source_call();call.gp[6]=owner; // Whole3E9B0: ESI owner.
        world.run_actual(0x3e9b0u,call);return std::uint8_t(call.gp[0]);
    };
    collection.owners.local_priority=candidates.admission.local_priority=priority;
    candidates.root_owner=[&](std::uint32_t owner) {
        (void)world.source_object_body(owner);
        auto call=world.prepare_source_call();call.gp[1]=owner; // WholeBCFB0: ECX child.
        world.run_actual(0xbcfb0u,call);
        if(call.gp[0]!=0xffffffffu)(void)world.source_object_body(call.gp[0]);
        return call.gp[0];
    };
    candidates.owner_kind=[&](std::uint32_t owner) {
        (void)world.source_object_body(owner);
        const auto pool=Bytes(world.memory().view(0x4ecbd0u,4)).u32(0);
        const auto records=Bytes(world.memory().view(pool+0x44u,4)).u32(0);
        return std::to_integer<std::uint8_t>(world.memory().view(records+(owner&65535u)*12u+3u,1)[0]);
    };
    // Simulation's+C stamp belongs to its own native query domain. The
    // private pStencil view starts unvisited and never republishes that stamp.
    for(const auto id:snapshot.lights.identities())snapshot.lights.record(id)[3]=i.queries.epoch;
    source_effect_lights_update_view(snapshot.lights,definition,eye);
    auto out=source_effect_lights_query(snapshot.lights,snapshot.spatial,definition,i.queries,
        SourceEffectLightQueryKind::Main,collection);
    if(defer_native_selection)return out;
    // Keep complete native records and actual PVS order. Only the supported
    // sphere subset enters this renderer's admission/selection stage. Missing
    // frustum/animation consumers remain explicit gaps, never sphere aliases.
    auto admitted=out;
    std::erase_if(admitted.visible,[&](auto identity) {
        const auto row=std::find_if(out.records.begin(),out.records.end(),[&](const auto& r){return r.runtime.identity==identity;});
        if(row==out.records.end())throw FormatError("pStencil native light visibility lost its record");
        return !i.supported.contains(row->definition_identity) || row->runtime.scenario_placement!=-1;
    });
    source_effect_lights_select(admitted,i.selection,std::bit_cast<std::int32_t>(collection.local_player),
        candidates,frame,cinematic);
    out.candidates=std::move(admitted.candidates);out.selected=std::move(admitted.selected);
    return out;
}
void CampaignLights::select_native(PhysicsLiveWorldTu15& world,SourceEffectLightView& view,
    const SourceEffectLightCandidateContext& context,std::int32_t local_player,
    std::int32_t frame,bool cinematic) {
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto real=[&](std::uint32_t at){return std::bit_cast<float>(word(at));};
    const auto records=word(word(0x4ecbdcu)+0x44u);
    view.candidates.clear();view.selected.clear();
    for(const auto identity:view.visible) {
        const auto at=std::find_if(view.records.begin(),view.records.end(),[&](const auto& row){return row.runtime.identity==identity;});
        if(at==view.records.end() || !at->definition)
            throw FormatError("Native campaign candidate lost its collected light/definition");
        const Bytes definition(at->definition->bytes);
        if(definition.u32(0xc4) || definition.u32(0xcc) || definition.u32(0xd4))continue;
        if(!native_light_render_admission_tu15(world,identity) || view.candidates.size()>=128)continue;
        const auto record=records+(identity&65535u)*272u;
        if(Bytes(memory.view(record,2)).u16(0)!=std::uint16_t(identity>>16))
            throw FormatError("Native campaign candidate changed light generation");
        at->fades={real(record+0xf8u),real(record+0xfcu),real(record+0x104u),real(record+0x100u),real(record+0x108u)};
        at->runtime.overall_fade=at->fades.overall;
        const auto flags=at->definition->flags;const auto& runtime=at->runtime;
        SourceShadowCandidate candidate;candidate.identity=identity;candidate.owner=-1;
        if(runtime.owner!=0xffffffffu) {
            (void)world.source_object_body(runtime.owner);
            auto call=world.prepare_source_call();call.gp[1]=runtime.owner;world.run_actual(0xbcfb0u,call);
            candidate.owner=std::bit_cast<std::int32_t>(call.gp[0]);
        }
        candidate.requested_fade=runtime.overall_fade;
        candidate.coverage=source_shadow_coverage(runtime.spatial_center,runtime.spatial_radius,
            context.depth_plane,context.pixel_scale);
        candidate.flags=((flags>>5)&1u)<<8 | ((flags>>20)&1u)<<24;
        if(runtime.owner!=0xffffffffu) {
            const auto headers=word(word(0x4ecbd0u)+0x44u);
            candidate.flags|=std::uint32_t(std::to_integer<unsigned>(memory.view(headers+(runtime.owner&65535u)*12u+3u,1)[0])==2)<<16;
            auto call=world.prepare_source_call();call.gp[6]=std::bit_cast<std::uint32_t>(candidate.owner);
            world.run_actual(0x3e9b0u,call);candidate.flags|=call.gp[0]&255u;
        }
        view.candidates.push_back(candidate);
    }
    view.selected=source_shadow_select(impl_->selection[std::clamp(local_player,0,3)],view.candidates,
        frame,3,std::bit_cast<float>(0x3c23d70au),cinematic);
}
std::vector<WorldEffectLightReceiver> CampaignLights::receivers(const SourceEffectLightView& lights,
    const SourceBspAdmissionBsp& geometry,std::span<const std::uint8_t> fog,const SourceBspAdmission* main) {
    auto& i=*impl_;std::vector<WorldEffectLightReceiver> out;
    const SourceEffectLightReceiverWorld world{i.memberships,geometry,i.locations,std::int16_t(i.bsp)};
    for(const auto& selected:lights.selected) {
        if(!(selected.requested_fade>std::bit_cast<float>(0x3d8f5c29u)))continue;
        const auto at=std::find_if(lights.records.begin(),lights.records.end(),[&](const auto& row){return row.runtime.identity==selected.identity;});
        if(at==lights.records.end() || !at->definition)throw FormatError("pStencil campaign receiver lost its native light");
        // Native selection includes projected lights. This bridge owns only
        // the admitted sphere definitions; projected receivers use134C00.
        if(!i.supported.contains(at->definition_identity) || Bytes(at->definition->bytes).u16(4)!=0)continue;
        const auto& r=at->runtime;
        const auto shape=source_effect_light_sphere(*at->definition,r.position,r.forward,r.up,r.state.size);
        auto view=source_effect_light_receiver_view(world,r.cluster,r.spatial_center,r.spatial_radius,shape,fog,main);
        out.push_back({selected.identity,++i.queries.epoch,selected.requested_fade,std::move(view)});
    }
    return out;
}
}
