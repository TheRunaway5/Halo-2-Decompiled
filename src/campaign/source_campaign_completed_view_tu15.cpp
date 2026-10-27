#include "halo2/source_campaign_environment_view_tu15.hpp"
#include "halo2/source_model_shader.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_campaign_completed_view_tu15.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/menu_world_depth.hpp"
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
std::vector<std::uint32_t> source_campaign_resident_definitions_tu15(
    const Cache &cache,std::size_t bsp,PhysicsLiveWorldTu15 &world) {
    if(world.source_bsp_index()!=bsp)throw FormatError("Secondary residency census belongs to another native BSP");
    const auto &memory=world.memory();const auto pool=world.source_object_pool();
    const Bytes header(memory.view(pool,76));
    if(header.u32(0x20u)!=2048u || header.u32(0x24u)!=12u || !header.u32(0x44u) || !header.u32(0x48u))
        throw FormatError("Secondary residency census lost the actual original object data-array shape");
    const Bytes rows(memory.view(header.u32(0x44u),2048u*12u));
    const Bytes occupied(memory.view(header.u32(0x48u),256u));std::set<std::uint32_t> definitions;
    for(unsigned slot=0;slot<2048;++slot)if(occupied.u32((slot/32u)*4u)&(1u<<(slot%32u))) {
        const auto owner=(std::uint32_t(rows.u16(slot*12u))<<16u)|slot;
        const auto body=rows.u32(slot*12u+8u);
        if(!body || rows.u16(slot*12u+6u)<0xacu || world.source_object_body(owner)!=body)
            throw FormatError("Secondary residency census lost a complete salted live object");
        const auto definition=word(memory,body);
        if(definition!=0xffffffffu) {
            if(!cache.query_tag({.handle=TagHandle{definition},.require_address=true}))
                throw FormatError("Secondary live object definition has no actual immutable Cache owner");
            definitions.insert(definition);
        }
    }
    return {definitions.begin(),definitions.end()};
}

SourceCampaignCompletedViewTu15 source_campaign_completed_view_tu15(PhysicsLiveWorldTu15 &world,
    const SourceTextureCameraScenePrefixTu15 &prefix,const BspAtmosphere &atmosphere_,
    bool begin_frame,std::optional<float> horizontal_angle) {
    auto &memory=world.memory();prefix.require_current(memory);
    SourceCampaignCompletedViewTu15 value;auto *out=&value;
    const Bytes packet(prefix.packet),raw(prefix.raw_camera);
    const auto viewport=unsigned(packet.u16(0));
    if(viewport>=4 || packet.u16(2)!=0 || prefix.completed_view.size()!=0x118)
        throw FormatError("Secondary campaign prefix is not its original world rendering domain");
    std::memcpy(&out->collection_rendering,prefix.projection.data(),sizeof(out->collection_rendering));
    std::memcpy(&out->submitting_rendering,prefix.packet.data()+0x8cu,sizeof(out->submitting_rendering));
    out->environment.bytes=prefix.environment;
    out->environment.visible_sky=std::bit_cast<std::int32_t>(word(memory,0x4c670cu));
    out->environment.sky_model_present=std::uint8_t(word(memory,0x4c6709u,1));
    // Exact2B770 stack byte passed to14DE90 as the RGB-override destination.
    // Prefix.original_abi is the genuine entry frame; no source call has
    // reused that retained stack before this value is copied.
    out->environment.clear_override=std::uint8_t(word(memory,prefix.original_abi.gp[4]-0x165u,1));
    out->environment.far_distance=raw.f32(0x44u);
    const auto cluster=std::bit_cast<std::int32_t>(word(memory,0x4c6704u));
    out->fog=source_campaign_environment_sample_tu15(out->environment,atmosphere_,cluster);
    const auto eye=vector_at(raw,0),forward=vector_at(raw,12u),up=vector_at(raw,24u);
    const auto clipping=source_frustum_camera({eye,forward,up,raw.f32(0x40u),raw.f32(0x44u),
        out->collection_rendering.clipping_rectangle});
    const std::array<std::int16_t,4> rectangle{short_at(raw,0x30u),short_at(raw,0x32u),
        short_at(raw,0x34u),short_at(raw,0x36u)};
    const auto auxiliary=word(memory,0x51df50u);const bool auxiliary_active=auxiliary && word(memory,auxiliary+5u,1);
    const auto policy=source_geometry_lod_policy(1);out->policy=policy;
    out->shaders={source_shader_coverage_camera(out->collection_rendering),
        source_shader_lod_view(rectangle[0],rectangle[2],bool(auxiliary),auxiliary_active),
        policy.coverage_multiplier,policy.effect_scale,word(memory,0x4c6839u,1)!=0};
    out->collection_rectangle=rectangle;
    MuzzleEffectView particles;
    const std::array<std::int16_t,4> submitted_rectangle{short_at(packet,0x48u),short_at(packet,0x4au),
        short_at(packet,0x4cu),short_at(packet,0x4eu)};
    out->submitting_rectangle=submitted_rectangle;
    const auto width=unsigned(int(submitted_rectangle[3])-int(submitted_rectangle[1]));
    const auto height=unsigned(int(submitted_rectangle[2])-int(submitted_rectangle[0]));
    if(!width || !height || width>32767 || height>32767)
        throw FormatError("Secondary submitting source rectangle is empty or outside its signed-short domain");
    particles.camera=source_view_shader_bank(out->submitting_rendering,packet.f32(0x5cu),eye,
        {float(submitted_rectangle[1]),float(submitted_rectangle[0]),float(width),float(height),
         std::bit_cast<float>(word(memory,0x473628u)),word(memory,0x491e27u,1)!=0,
         word(memory,0x515edcu,1)!=0,unsigned(std::bit_cast<std::int32_t>(word(memory,0x4920d8u)))},false);
    particles.fog=bsp_fog_constants(out->fog,eye,forward);
    const auto &basis=out->collection_rendering.camera_to_world.basis;
    particles.orientation={basis[0].x,basis[0].y,basis[0].z,basis[1].x,basis[1].y,basis[1].z,
        basis[2].x,basis[2].y,basis[2].z};
    particles.position=eye;particles.forward=forward;particles.up=up;particles.width=width;particles.height=height;
    // W-depth conversion retains its actual configured source planes, while
    // c0..11 above uses the submitting packet's independent W-distance.
    const auto depth=menu_world_depth_clip_planes({Bytes(memory.view(0x4922f4u,8)).f32(0),
        Bytes(memory.view(0x4922f4u,8)).f32(4),MenuWorldDepthBoundary::none,0,false});
    particles.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
    SourceCampaignViewInputTu15 input{out->collection_rendering,clipping,eye,forward,rectangle,
        raw.f32(0x2cu),policy.coverage_multiplier,out->shaders.view_modifier};
    input.begin_frame=begin_frame;input.view=viewport;input.local_players=1;input.actual_local_user=prefix.actual_local_user;
    input.source_prefix=&prefix;
    if(prefix.actual_local_user>=0 && !horizontal_angle)
        throw FormatError("Completed local-user view needs its actual admitted horizontal lens owner");
    input.particles=SourceCampaignParticleViewTu15{particles,raw.f32(0x28u),horizontal_angle.value_or(0.f)};
    // No local player/observer lens is published for sourceNONE. This unused
    // field cannot masquerade as an authored horizontal lens producer.
    input.environment=nullptr;out->input=std::move(input);
    out->submitting_camera.position=vector_at(packet,0x18u);out->submitting_camera.forward=vector_at(packet,0x24u);
    out->submitting_camera.up=vector_at(packet,0x30u);out->submitting_camera.oriented=true;
    out->submitting_camera.projection_vertical_angle=packet.f32(0x40u);
    out->submitting_clip={packet.f32(0x58u),packet.f32(0x5cu)};
    out->configured_clip={Bytes(memory.view(0x4922f4u,8)).f32(0),Bytes(memory.view(0x4922f4u,8)).f32(4)};
    out->submitting_constants=source_view_shader_constants(out->submitting_rendering,out->submitting_clip.y,false);
    prefix.require_current(memory);return value;
}
}
