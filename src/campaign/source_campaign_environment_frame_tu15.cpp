#include "halo2/source_campaign_environment_frame_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <span>

namespace halo2 {
SourceCampaignEnvironmentFrameTu15 source_campaign_environment_frame_tu15(
    PhysicsLiveWorldTu15 &world,Vec3 eye,Vec3 forward,std::int32_t cluster,
    float near_distance,float far_distance) {
    for(const float value:{eye.x,eye.y,eye.z,forward.x,forward.y,forward.z,
        near_distance,far_distance})
        if(!std::isfinite(value))throw FormatError("Campaign environment requires its finite submitting camera");
    if(near_distance<0 || far_distance<=near_distance)
        throw FormatError("Campaign environment requires an ordered submitting camera depth range");
    auto &memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto bsp=word(0x4ecc18u);
    if(!bsp || cluster< -1 || (cluster>=0 && std::uint32_t(cluster)>=word(bsp+0x9cu)))
        throw FormatError("Campaign environment cluster escapes its actual BSP owner");
    // Caller scratch is above the prepared return/argument stack.131AC0's
    // original0x4820-byte downward stack frame cannot overlap these inputs.
    const auto scratch=world.prepare_source_call().gp[4]+0x1000u;
    const auto put=[&](std::uint32_t p,const auto &v){world.write_source_call_input(p,std::as_bytes(std::span(&v,1)));};
    put(scratch,eye);put(scratch+16u,forward);
    const std::array<std::byte,24> controls{};
    world.write_source_call_input(scratch+32u,controls);
    auto sky=world.prepare_source_call();
    sky.gp[1]=std::bit_cast<std::uint32_t>(cluster);sky.gp[2]=scratch+32u;
    sky.gp[3]=scratch+36u;sky.gp[7]=scratch+40u;
    world.run_actual(0x14de90u,sky);
    SourceCampaignEnvironmentFrameTu15 result;
    result.visible_sky=std::bit_cast<std::int32_t>(word(scratch+32u));
    result.sky_model_present=std::uint8_t(sky.gp[0]);
    result.clear_override=std::to_integer<std::uint8_t>(memory.view(scratch+36u,1)[0]);
    auto producer=world.prepare_source_call(std::array{scratch,scratch+16u,
        std::uint32_t(result.sky_model_present),1u});
    producer.gp[0]=scratch+0x100u;producer.gp[1]=std::bit_cast<std::uint32_t>(cluster);
    world.run_actual(0x1310f0u,producer);
    const auto environment=memory.view(scratch+0x100u,result.bytes.size());
    std::copy(environment.begin(),environment.end(),result.bytes.begin());
    result.far_distance=detail::source_campaign_environment_caller_tu15(result.bytes,
        result.clear_override,std::span<const std::byte,12>(memory.view(scratch+40u,12u)),
        near_distance,far_distance);
    //2BA96 copies all0x48 dwords. Particle admission reads fields beyond
    // the four ordinary model-collection inputs, including+28/+AC/+B4.
    world.write_source_call_input(0x4c6710u,result.bytes);
    return result;
}
}
