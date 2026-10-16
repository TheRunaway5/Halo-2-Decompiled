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
void source_campaign_camera_animation_tu15(PhysicsLiveWorldTu15 &world,
    std::uint32_t graph,std::uint32_t name,std::uint32_t object,std::int16_t flag) {
    auto abi=world.prepare_source_call(std::array{object,std::uint32_t(std::uint16_t(flag))});
    abi.gp[0]=graph;abi.gp[3]=name;world.run_actual(0x16edd0u,abi);
}
void source_campaign_camera_field_of_view_tu15(PhysicsLiveWorldTu15 &world,float degrees,std::int16_t ticks) {
    world.call_actual(0x16f5a0u,std::array{std::bit_cast<std::uint32_t>(degrees),std::uint32_t(std::uint16_t(ticks))});
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
