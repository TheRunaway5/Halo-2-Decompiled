#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_live_shadow_main_inputs_tu15.hpp"
#include "halo2/source_live_shadow_main_draws_tu15.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_texture_camera_scene_prefix_tu15.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
SourceCampaignViewTu15::SourceCampaignViewTu15(const Cache &cache,std::size_t bsp)
    :portals_(cache.load_source_portal_bsp(bsp)),geometry_(cache.load_source_bsp_admission(bsp)),
     particles_(cache,std::span<const ScenarioObject>{}),group_parts_(cache),clusters_(geometry_.clusters.size()),
     definitions_(geometry_.definitions.size()),instances_(geometry_.instances.size()),transforms_(geometry_.instances.size()) {
    // Same authored geometry/instance fields consumed by the existing MP
    // SourceBspEffectLights collector. Actual native130980 still owns each
    // admitted geometry's resource publication; no +50 pointer is supplied.
    const auto metadata=cache.load_source_bsp_metadata(bsp);
    const Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
    const auto read=[&](auto p,auto n){return Bytes(metadata.bytes_at(p,n));};
    const auto clusters=root.u32(16u+0xa0u),definitions=root.u32(16u+0x13cu),instances=root.u32(16u+0x144u);
    for(unsigned n=0;n<clusters_.size();++n) {
        const auto row=read(clusters+n*176u,0x6cu);
        clusters_[n]={row.u16(0x14u),row.u16(0x24u),false};
    }
    for(unsigned n=0;n<definitions_.size();++n) {
        const auto row=read(definitions+n*200u,0x6cu);
        definitions_[n]={row.u16(0x14u),row.u16(0x24u),false};
    }
    const auto lighting=cache.load_bsp_lighting(bsp);
    lightmap_matches_=lighting.structure_matches && lighting.bitmap_datum!=0xffffffffu;
    for(unsigned n=0;n<instances_.size();++n) {
        const auto row=read(instances+n*88u,88u);
        std::memcpy(&transforms_[n],row.slice(0,sizeof(Transform)).data(),sizeof(Transform));
        instances_[n].lighting_policy=row.u16(0x56u);
        instances_[n].transform_address=0x71002000u+n*88u;
        if(lightmap_matches_) {
            const auto bucket=lighting.instance_buckets.at(n).bucket;
            instances_[n].lightmap_resident=bucket>=0 && unsigned(bucket)<lighting.buckets.size();
        }
    }
}

void SourceCampaignViewTu15::prepare_bsp(PhysicsLiveWorldTu15 &world,const SourceBspAdmission &main) {
    const auto &memory=world.memory();
    const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
    const auto bsp=word(0x4ecc18u);
    if(!bsp || word(bsp+0x9cu)!=clusters_.size() || word(bsp+0x138u)!=definitions_.size() ||
       word(bsp+0x140u)!=instances_.size())
        throw FormatError("Campaign main draw resources differ from the actual current BSP");
    const auto ready=[&](std::uint32_t row) {
        if(!Bytes(memory.view(row+0x24u,2)).u16(0))return false;
        auto completed=world.request_render_geometry(2u,row+0x28u);
        if(!completed)completed=world.request_render_geometry(0u,row+0x28u);
        if(!completed || !word(row+0x50u))
            throw FormatError("Campaign original BSP resource publication did not finish");
        return true;
    };
    for(const auto &cluster:main.clusters) {
        if(cluster.id>=clusters_.size())throw FormatError("Campaign main cluster escapes the retained BSP");
        clusters_[cluster.id].resident=ready(word(bsp+0xa0u)+cluster.id*176u);
    }
    for(const auto &instance:main.instances) {
        if(instance.id>=instances_.size())throw FormatError("Campaign main instance escapes the retained BSP");
        const auto row=word(bsp+0x144u)+instance.id*88u;
        const auto definition=std::bit_cast<std::int16_t>(Bytes(memory.view(row+0x34u,2)).u16(0));
        if(definition<0 || unsigned(definition)>=definitions_.size() || definition!=geometry_.instances[instance.id].definition)
            throw FormatError("Campaign main instance lost its authored definition");
        definitions_[definition].resident=ready(word(bsp+0x13cu)+unsigned(definition)*200u);
    }
}

SourceCampaignViewFrameTu15 SourceCampaignViewTu15::collect(PhysicsLiveWorldTu15 &world,
    const SourceCampaignViewInputTu15 &input,const BspAtmosphere &atmosphere,
    const BspFogSample &fog) {
    auto &memory=world.memory();
    if(input.view>=4 || !input.local_players || input.local_players>4 ||
       input.actual_local_user< -1 || input.actual_local_user>=4)
        throw FormatError("Campaign view has no actual source ordinal/user/count");
    const bool secondary=input.source_prefix && input.source_prefix->texture_camera;
    if(input.source_prefix) {
        const auto &prefix=*input.source_prefix;prefix.require_current(memory);
        if(!prefix.backend_requested || prefix.finished ||
           !std::ranges::equal(memory.view(0x491e20u,0x298u),prefix.packet) ||
           !std::ranges::equal(memory.view(0x4c65c0u,0x74u),prefix.raw_camera) ||
           !std::ranges::equal(memory.view(0x4c6634u,0xc0u),prefix.projection) ||
           !std::ranges::equal(memory.view(0x4c6710u,0x120u),prefix.environment) ||
           prefix.actual_local_user!=input.actual_local_user ||
           Bytes(prefix.packet).u16(0)!=input.view || std::to_integer<unsigned>(prefix.packet[7])!=unsigned(secondary) ||
           (secondary && input.actual_local_user!=-1) ||
           std::memcmp(prefix.raw_camera.data(),&input.eye,sizeof(Vec3)) ||
           std::memcmp(prefix.raw_camera.data()+12,&input.forward,sizeof(Vec3)) ||
           std::memcmp(prefix.projection.data(),&input.rendering,sizeof(input.rendering)))
            throw FormatError("Campaign view lost its actual completed source CPU-prefix context");
    } else if(input.actual_local_user<0)
        throw FormatError("A non-user campaign view requires its full native source prefix");
    if(input.particles && (!input.environment ||
       !std::ranges::equal(memory.view(0x4c6710u,0x120u),input.environment->bytes)))
        throw FormatError("Campaign particles require their completed full source environment publication");
    // Whole219080 allocates this64-byte bank;2190C0 binds actual machine
    // lifetimes and initializes it. Whole1179C0/117900 mutate that same bank.
    // None of these producers is repeated, reset or replaced by rendering.
    const auto pointer=Bytes(memory.view(0x505e7cu,4)).u32(0);
    if(!pointer)throw FormatError("Campaign view lost its original TU1.5 portal-mask owner");
    std::array<std::uint32_t,16> mask;
    std::memcpy(mask.data(),memory.view(pointer,sizeof(mask)).data(),sizeof(mask));
    SourceCampaignViewFrameTu15 frame;
    frame.visibility=source_portal_visibility(portals_,std::span{&input.clipping,1},fog.cluster,mask);
    const auto containing=source_bsp_fog_admission(atmosphere,{true,true,fog.planar_datum,0});
    SourceBspAdmissionInput bsp_input;bsp_input.containing_fog=containing;
    frame.bsp=source_bsp_admission(geometry_,frame.visibility,bsp_input);
    // Literal2B630 reset is once before both25800 and the ordinary view.
    // Each actual2B770 still has its own12D19→22830 per-view reset.
    if(input.begin_frame)world.begin_model_frame();
    world.begin_model_view();
    // Source2B68C→20700 belongs only to the ordinary view AFTER25800.
    // The secondary scene borrows the once-per-frame20680 gate and does not
    // mint a second local-user query identity for the same viewport ordinal.
    if(!secondary && input.prepare_lens_view)input.prepare_lens_view(world,input.view);
    bool consume_particles{};
    if(input.particles) {
        if(std::memcmp(&input.particles->camera.position,&input.eye,sizeof(Vec3)) ||
           std::memcmp(&input.particles->camera.forward,&input.forward,sizeof(Vec3)))
            throw FormatError("Campaign particles lost the actual submitting main camera");
        // The actual submitting camera is present even when this view has no
        // resident particle materials. Do not inherit the preceding view's
        // local user, recursion flag or near/far context in that empty case.
        if(!input.source_prefix) {
            source_live_shadow_main_user_tu15(world,input.actual_local_user);
            source_live_shadow_main_location_tu15(world,fog.cluster);
            source_live_shadow_main_camera_tu15(world,{input.eye,input.forward,input.particles->camera.up,
                input.clipping.near_distance,input.clipping.far_distance,{},{}},input.viewport,0,false);
        }
        frame.live_effect_definitions=particles_.preload_live_world(memory);
        consume_particles=particles_.active();
        if(consume_particles) {
            particles_.prepare_storage(world);
            //2B8B9..2B8F1 has no player/observer branch for source userNONE.
            if(input.actual_local_user>=0)particles_.publish_observer(world,unsigned(input.actual_local_user),
                input.eye,input.forward,input.particles->camera.up,input.particles->horizontal_angle);
            // A live query owner already ran20680 and prepares this actual
            // view above. Standalone CPU callers retain the original gates.
            if(!secondary && !input.prepare_lens_view) {
                if(input.begin_frame)world.call_actual(0x20680u);
                auto view=world.prepare_source_call();view.gp[0]=input.view;world.run_actual(0x20700u,view);
            }
            particles_.prepare_view(world,frame.visibility,input.particles->camera,input.view,
                input.particles->projection_angle,input.distance_scale,input.actual_local_user);
            source_live_shadow_main_user_tu15(world,input.actual_local_user);
            source_live_shadow_main_location_tu15(world,fog.cluster);
        }
    }
    if(!consume_particles)world.publish_render_visibility(frame.visibility);
    SourceObjectAdmissionInput object_input;object_input.containing_fog=containing;
    frame.objects=world.admit_render_objects(frame.visibility,object_input,frame.bsp.associations);
    source_draw_units_begin(frame.units,input.view);source_bsp_range_arena_begin(frame.units.arena);
    if(consume_particles) {
        prepare_bsp(world,frame.bsp);
        const SourceBspDrawWorld resources{geometry_,clusters_,definitions_,instances_,lightmap_matches_};
        source_bsp_collect_draw_units(frame.units,frame.bsp,resources,
            {input.view,0x71000000u,0,fog,source_shader_coverage_camera(input.rendering)});
    }
    SourceLiveModelDrawContextTu15 context;
    context.entry.eye=context.draw.eye=input.eye;context.forward=input.forward;
    context.entry.distance_scale=input.distance_scale;
    context.entry.coverage_multiplier=input.coverage_multiplier;
    context.entry.coverage=source_shader_coverage_camera(input.rendering);
    context.entry.flags={0,std::int32_t(fog.classification),fog.planar_datum,
        fog.eye_fraction,fog.planar_only,std::uint8_t(fog.planar_flags)};
    context.draw.rendering=input.rendering;context.draw.viewport=input.viewport;
    // Native range-arena token, not source allocation or a supplied result.
    // Every consumed range allocation remains owned by frame.units.arena.
    context.draw.draw.mask_arena_address=0x71000000u;
    context.view_modifier=input.view_modifier;context.view=input.view;context.local_players=input.local_players;
    SourceLiveModelBaseSelectionTu15 selected;
    context.built=[&](const auto &collector) {
        selected.remember(collector);
        frame.built.insert_or_assign(Bytes(collector.bytes).u32(0),collector);
    };
    const std::uint32_t main_context=0x556e18u;
    world.write_source_call_input(0x4cd3a0u,std::as_bytes(std::span(&main_context,1)));
    source_live_shadow_main_inputs_tu15(world,context);
    if(consume_particles)world.call_actual(0xc35d0u);
    source_live_model_draws_tu15(world,frame.objects,frame.units,context);
    frame.base_draws=selected.copy(world,frame.units);
    for(unsigned n=0;n<frame.units.draw_count;++n) {
        if(frame.units.units[n].words[1]==0xffffffffu)continue;
        const auto context_address=frame.units.units[n].words[4];
        if(context_address)frame.drawn.insert(Bytes(memory.view(context_address,4)).u32(0));
    }
    if(consume_particles) {
        source_live_shadow_main_draws_tu15(memory,frame.units,frame.bsp,frame.objects.associations,
            0x71000000u,0x71002000u,transforms_,
            [&](auto address,auto bytes){world.write_source_call_input(address,bytes);});
        source_live_shadow_main_queues_tu15(world,frame.bsp,frame.objects);
        auto cull=world.prepare_source_call();world.run_actual(0x44710u,cull);
        // Authenticated normal13660→13768→2BCB0 argument5=0; after the
        // real completed main handoff/cull, never a model-only boolean.
        auto commands=world.prepare_source_call();commands.gp[0]=0;world.run_actual(0x40e80u,commands);
        frame.main_commands_completed=true;
        frame.transparent_commands=source_live_transparent_queue_tu15(world);
        frame.transparent_groups=source_live_transparent_groups_tu15(world);
        for(const auto &group:frame.transparent_groups)
            frame.transparent_group_parts.push_back(group_parts_.resolve(group,frame.base_draws));
        frame.source_particle_commands=Bytes(memory.view(0x4c2aa0u,4)).u32(0);
        frame.world_particles=particles_.draw(world,frame.visibility,input.particles->camera,input.view,
            input.particles->projection_angle,input.distance_scale,true,true);
    }
    return frame;
}
}
