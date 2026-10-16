#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <bit>
#include <cmath>
namespace halo2 {
void source_campaign_cinematic_control_tu15(PhysicsLiveWorldTu15 &world,bool active) {
    world.call_actual(active?0x13ead0u:0x13f560u);
}
void source_campaign_cinematic_lod_tu15(PhysicsLiveWorldTu15 &world,std::uint32_t object,bool active) {
    auto abi=world.prepare_source_call(std::array{std::uint32_t(active)});
    abi.gp[0]=object;world.run_actual(0xbdf70u,abi);
}
bool source_campaign_cinematic_animation_tu15(PhysicsLiveWorldTu15 &world,
    std::uint32_t object,std::uint32_t graph,std::uint32_t name,
    std::uint32_t anchor,bool unit,bool loop,bool interpolate) {
    if(unit) {
        auto abi=world.prepare_source_call(std::array{name,std::uint32_t(interpolate),anchor,std::uint32_t(loop)});
        abi.gp[0]=graph;abi.gp[1]=object;world.run_actual(0x11dc00u,abi);
        return (abi.gp[0]&255u)!=0;
    }
    // All four original scenery evaluators supply start frame0 and hold1.
    auto abi=world.prepare_source_call(std::array{object,name,0u,anchor,1u,std::uint32_t(loop)});
    abi.gp[0]=graph;world.run_actual(0x10f710u,abi);return (abi.gp[0]&255u)!=0;
}
void source_campaign_cinematic_attach_tu15(PhysicsLiveWorldTu15 &world,
    std::uint32_t parent,std::uint32_t parent_marker,
    std::uint32_t child,std::uint32_t child_marker) {
    auto abi=world.prepare_source_call(std::array{parent_marker,child_marker});
    abi.gp[7]=parent;abi.gp[6]=child;world.run_actual(0x10ffd0u,abi);
}
std::int16_t source_campaign_unit_animation_ticks_tu15(float seconds) {
    volatile float product=seconds*30.f;const float value=product;
    std::uint32_t rounded=0x80000000u; // Original masked FISTP invalid result.
    if(std::isfinite(value) && value>=-2147483648.f && value<2147483648.f) {
        const float lower=std::floor(value),fraction=value-lower;
        const auto integer=std::int32_t(lower);
        rounded=std::uint32_t(integer)+std::uint32_t(fraction>.5f || (fraction==.5f && (integer&1)));
    }
    const auto ticks=rounded-2u;
    return std::bit_cast<std::int16_t>(std::uint16_t(std::bit_cast<std::int32_t>(ticks)>0?ticks:0u));
}
std::int16_t source_campaign_cinematic_animation_time_tu15(
    PhysicsLiveWorldTu15 &world,std::uint32_t object,bool unit) {
    auto abi=world.prepare_source_call();abi.gp[unit?0:1]=object;
    world.run_actual(unit?0x11dd90u:0x10f510u,abi);
    return unit?source_campaign_unit_animation_ticks_tu15(std::bit_cast<float>(abi.xmm[0][0])):
        std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[0]));
}
}
