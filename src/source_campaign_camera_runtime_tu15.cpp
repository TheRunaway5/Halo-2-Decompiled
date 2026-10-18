#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t at) {return Bytes(m.view(at,4)).u32(0);}
void put(SourceRuntimeMemory &m,std::uint32_t at,std::uint32_t value) {
    m.write(at,std::as_bytes(std::span(&value,1)));
}
}
void source_campaign_camera_control_tu15(PhysicsLiveWorldTu15 &world,bool active) {
    world.call_actual(0x158560u,std::array{std::uint32_t(active)});
}
void source_campaign_camera_point_tu15(PhysicsLiveWorldTu15 &world,std::int16_t point,std::int16_t ticks) {
    auto abi=world.prepare_source_call(std::array{std::uint32_t(std::uint16_t(ticks)),null_datum});
    abi.gp[2]=std::uint32_t(std::uint16_t(point));world.run_actual(0x16f150u,abi);
}
void source_campaign_camera_animation_tu15(PhysicsLiveWorldTu15 &world,
    std::uint32_t graph,std::uint32_t name,std::uint32_t object,std::int16_t flag) {
    auto abi=world.prepare_source_call(std::array{object,std::uint32_t(std::uint16_t(flag))});
    abi.gp[0]=graph;abi.gp[3]=name;world.run_actual(0x16edd0u,abi);
}
void source_campaign_camera_field_of_view_tu15(PhysicsLiveWorldTu15 &world,float degrees,std::int16_t ticks) {
    world.call_actual(0x16f5a0u,std::array{std::bit_cast<std::uint32_t>(degrees),std::uint32_t(std::uint16_t(ticks))});
}
void source_campaign_camera_frame_tu15(PhysicsLiveWorldTu15 &world,float seconds) {
    if(!std::isfinite(seconds) || seconds<0)
        throw FormatError("Source campaign camera frame requires finite nonnegative elapsed time");
    const std::array argument{std::bit_cast<std::uint32_t>(seconds)};
    world.call_actual(0x157fe0u,argument);
    world.call_actual(0x1720e0u,argument);
}
std::optional<SourceCampaignCameraViewTu15> source_campaign_camera_view_tu15(
    const SourceRuntimeMemory &memory,unsigned view) {
    if(view>=4)throw FormatError("Source campaign camera view exceeds the actual local bank");
    const auto users=word(memory,0x4f54f0u),control=word(memory,0x4f5504u);
    if(!users || !control || memory.view(control,1)[0]==std::byte{} ||
       word(memory,users+12u+view*4u)==null_datum)return {};
    const auto at=0x4f64a4u+view*0x358u;
    const Bytes state(memory.view(at,0x358u));
    const auto vector=[&](unsigned p){return Vec3{state.f32(p),state.f32(p+4u),state.f32(p+8u)};};
    SourceCampaignCameraViewTu15 result{vector(0xb8u),vector(0xd8u),vector(0xe4u),
        state.f32(0xf0u),state.f32(0xf4u),state.f32(0x100u)};
    for(const auto value:std::array{result.position.x,result.position.y,result.position.z,
        result.forward.x,result.forward.y,result.forward.z,result.up.x,result.up.y,result.up.z,
        result.horizontal_field_of_view,result.aspect,result.aim_displacement})
        if(!std::isfinite(value))throw FormatError("Source campaign observer output is not finite");
    if(result.horizontal_field_of_view<=0 || result.horizontal_field_of_view>=3.14159265358979323846f ||
       result.aspect<=0 || length(result.forward)==0 || length(result.up)==0)
        throw FormatError("Source campaign observer output lacks its completed camera");
    return result;
}
std::int16_t source_campaign_camera_time_value_tu15(std::int32_t ticks,float delta) {
    volatile float seconds=float(ticks)*delta;
    volatile float product=seconds*30.f;const float value=product;
    std::uint32_t result=0x80000000u; // Masked original FISTP invalid result.
    if(std::isfinite(value) && value>=-2147483648.f && value<2147483648.f) {
        const float lower=std::floor(value),fraction=value-lower;
        const auto integer=std::int32_t(lower);
        result=std::uint32_t(integer)+std::uint32_t(fraction>.5f || (fraction==.5f && (integer&1)));
    }
    return std::bit_cast<std::int16_t>(std::uint16_t(result));
}
std::int16_t source_campaign_camera_time_tu15(PhysicsLiveWorldTu15 &world) {
    const auto &m=world.memory();const auto director=word(m,0x51df6cu),clock=word(m,0x51df54u);
    if(!director || !clock)throw FormatError("Source camera query lacks its original director/clock");
    return source_campaign_camera_time_value_tu15(std::bit_cast<std::int32_t>(word(m,director+0x14u)),
        std::bit_cast<float>(word(m,clock+4u)));
}
void source_campaign_camera_game_tick_tu15(SourceRuntimeMemory &m) {
    const auto director=word(m,0x51df6cu),clock=word(m,0x51df54u);
    if(!director || !clock)throw FormatError("Source camera tick lacks its original director/clock");
    const auto remaining=word(m,director+0x14u)-1u;
    put(m,director+0x14u,std::bit_cast<std::int32_t>(remaining)<0?0u:remaining);
    volatile float elapsed=std::bit_cast<float>(word(m,clock+4u))+
        std::bit_cast<float>(word(m,director+4u));
    put(m,director+4u,std::bit_cast<std::uint32_t>(float(elapsed)));
}
}
