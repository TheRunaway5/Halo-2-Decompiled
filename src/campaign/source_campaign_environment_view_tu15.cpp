#include "halo2/source_campaign_environment_view_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
BspFogSample source_campaign_environment_sample_tu15(
    const SourceCampaignEnvironmentFrameTu15 &frame,const BspAtmosphere &resources,
    std::int32_t cluster) {
    const Bytes bank(frame.bytes);
    const auto vector=[&](unsigned p){return Vec3{bank.f32(p),bank.f32(p+4),bank.f32(p+8)};};
    const auto atmospheric=[&](unsigned p){return AtmosphericFog{
        vector(p),bank.f32(p+12),bank.f32(p+16),bank.f32(p+20)};};
    BspFogSample out;
    out.cluster=cluster;out.visible_sky=frame.visible_sky;
    out.classification=bank.u32(0);out.clear=vector(4);
    out.primary=atmospheric(0x1c);out.secondary=atmospheric(0x34);
    out.sky_fog={vector(0x4c),bank.f32(0x58),0,1};
    out.sky_color_premultiplied=vector(0x5c);out.sky_opacity=bank.f32(0x68);
    out.patchy_datum=bank.u32(0x6c);
    out.planar_datum=bank.u32(0x9c);
    for(unsigned n=0;n<4;++n)out.planar.plane[n]=bank.f32(0xf4+n*4);
    out.planar.color=vector(0xa0);out.planar.maximum_density=bank.f32(0xac);
    out.planar.opaque_distance=bank.f32(0xb0);out.planar.opaque_depth=bank.f32(0xb4);
    out.planar.eye_offset=bank.f32(0xc4);
    out.eye_height=bank.f32(0x108);out.eye_fraction=bank.f32(0x10c);
    out.eye_offset=bank.f32(0x110);out.planar_only=frame.bytes[0x114]!=std::byte{};
    out.transition=bank.f32(0x118);out.interpolation=bank.f32(0x11c);
    for(unsigned n=0;n<4;++n)out.bloom[n]=bank.f32(0xe0+n*4);
    if(out.planar_datum!=0xffffffffu) {
        const auto palette=std::ranges::find(resources.fog_palette,out.planar_datum,
            &BspAtmosphere::FogPalette::datum);
        if(palette==resources.fog_palette.end())
            throw FormatError("Completed campaign environment lacks its authored planar fog resource");
        out.planar_flags=palette->flags;
    }
    if(out.patchy_datum!=0xffffffffu) {
        const auto definition=resources.patchy_definition(out.patchy_datum);
        if(!definition)throw FormatError("Completed campaign environment lacks its authored patchy fog resource");
        // PatchyFog currently has one color bank. Keep this missing consumer
        // explicit until the original secondary-color shader path is ported.
        if(bank.f32(0x7c)!=0 || bank.f32(0x80)!=0 || bank.f32(0x84)!=0)
            throw FormatError("Completed campaign environment requires its secondary patchy fog color consumer");
        out.patchy=definition->fog;
        out.patchy->color=vector(0x70);
        out.patchy->density={bank.f32(0x88),bank.f32(0x8c)};
        out.patchy->distance={bank.f32(0x90),bank.f32(0x94)};
    }
    return out;
}

SourceCampaignEnvironmentViewTu15 source_campaign_environment_view_tu15(
    PhysicsLiveWorldTu15 &world,const BspAtmosphere &resources,Vec3 eye,Vec3 forward,
    float near_distance,float configured_far_distance,unsigned local_view) {
    for(const float value:{eye.x,eye.y,eye.z,forward.x,forward.y,forward.z,
        near_distance,configured_far_distance})
        if(!std::isfinite(value))throw FormatError("Campaign environment requires its finite submitting camera");
    if(near_distance<0 || configured_far_distance<=near_distance || local_view>=4)
        throw FormatError("Campaign environment view requires its ordered depth range and actual local view");
    auto &memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto bsp=word(0x4ecc18u);
    if(!bsp || word(bsp+0x2cu)!=resources.leaves.size() || word(bsp+0x9cu)!=resources.clusters.size())
        throw FormatError("Campaign environment location differs from the actual current BSP");
    world.write_source_call_input(0x4c6870u,std::as_bytes(std::span(&local_view,1)));
    const auto cluster_owner=0x4c6874u+local_view*4u,leaf_owner=0x4c6884u+local_view*4u;
    auto cluster=std::bit_cast<std::int32_t>(word(cluster_owner));
    auto leaf=std::bit_cast<std::int32_t>(word(leaf_owner));
    auto query=world.prepare_source_call();const auto scratch=query.gp[4]+0x1000u;
    world.write_source_call_input(scratch,std::as_bytes(std::span(&eye,1)));
    query.gp[0]=0;query.gp[1]=word(bsp+0x18u);query.gp[2]=scratch;
    world.run_actual(0x14cdb0u,query);
    const auto selected=std::bit_cast<std::int32_t>(query.gp[0]);
    const bool found=selected!=-1;
    if(found) {
        leaf=selected;
        cluster=std::bit_cast<std::int16_t>(Bytes(memory.view(word(bsp+0x30u)+std::uint32_t(leaf)*8u,2)).u16(0));
        world.write_source_call_input(cluster_owner,std::as_bytes(std::span(&cluster,1)));
        world.write_source_call_input(leaf_owner,std::as_bytes(std::span(&leaf,1)));
    } else if(leaf<0 || std::uint32_t(leaf)>=word(bsp+0x2cu) ||
              cluster<0 || std::uint32_t(cluster)>=word(bsp+0x9cu)) {
        leaf=cluster=-1;
    }
    SourceCampaignEnvironmentViewTu15 out;
    out.leaf=leaf;out.location_found=found;
    out.source=source_campaign_environment_frame_tu15(world,eye,forward,cluster,
        near_distance,configured_far_distance);
    out.fog=source_campaign_environment_sample_tu15(out.source,resources,cluster);
    return out;
}
}
