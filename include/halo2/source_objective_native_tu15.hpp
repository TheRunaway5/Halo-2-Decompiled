#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>

namespace halo2 {
// Semantic integer kernels for original249866..249898 /24991C..24992C.
// The caller owns legality, tick delivery, publications, events and scoring.
// Keep the signed16 publication separate: thresholds can exceed that width.
struct SourceAssaultArmingStep {
    std::int32_t threshold{},progress{};
};
constexpr SourceAssaultArmingStep source_assault_arming_tick(
    std::int16_t progress,std::int16_t seconds,std::int16_t tick_rate) noexcept {
    const auto threshold=std::int32_t(seconds)*std::int32_t(tick_rate);
    return {threshold,std::min(std::int32_t(progress)+1,threshold)};
}
constexpr std::int32_t source_assault_arming_decay(std::int16_t progress) noexcept {
    return std::max(std::int32_t(progress)-3,-1);
}
constexpr std::int32_t source_objective_family(std::int32_t source_engine) noexcept {
    return source_engine==9?9:1;
}
constexpr bool source_objective_team_has_marker(std::int16_t spawn,std::int16_t goal) noexcept {
    return spawn!=-1 || goal!=-1;
}
constexpr bool source_objective_uses_default_relationship(std::int32_t relationship) noexcept {
    return relationship==0;
}
constexpr bool source_objective_carrier_trait(std::int32_t trait,
    std::uint32_t flags,std::int32_t weapon) noexcept {
    switch(trait) {
    case 1:return (flags&0x80u)!=0;
    case 2:return weapon==0;
    case 3:return (flags&0x40u)!=0;
    default:return false;
    }
}
constexpr bool source_objective_item_class(std::uint8_t object_class) noexcept {
    return (object_class&31u)==2;
}
constexpr bool source_objective_team_slot(std::int32_t team) noexcept {
    return team>=0 && team<9;
}
constexpr bool source_objective_team_eligible(std::int32_t team,std::int32_t kind,
    std::int32_t engine,bool active,std::int32_t selected) noexcept {
    if(team==8)return kind==2;
    if(team<0 || team>=8 || kind==2 || !active)return false;
    return kind!=1 || (engine==9?team!=selected:team==selected);
}
constexpr bool source_objective_pickup_allowed(std::int32_t engine,std::uint16_t classification,
    std::int16_t owner,std::int8_t player_team,std::uint8_t flags) noexcept {
    if(classification!=1 || owner==-1)return true;
    if(engine==9 && (flags&2u))return false;
    if(owner==8)return true;
    return engine==9?owner==player_team:owner!=player_team;
}
constexpr std::int32_t source_objective_rotated_team(std::int32_t previous,std::int32_t next) noexcept {
    return previous!=-1 && next==-1?previous:next;
}
constexpr std::uint32_t source_objective_entity_datum(std::uint32_t datum) noexcept {
    return datum==0xffffffffu?datum:datum&65535u;
}
constexpr std::uint32_t source_objective_player_datum(std::uint32_t index,std::int16_t salt) noexcept {
    return (std::uint32_t(std::uint16_t(salt))<<16u)|index;
}
constexpr std::int32_t source_objective_overtime_tick(std::int32_t previous,
    bool eligible,bool latched,std::int32_t window_ticks) noexcept {
    if(!eligible)return 0;
    if(latched)return window_ticks;
    return std::bit_cast<std::int32_t>(std::uint32_t(previous)-1u);
}
inline float source_objective_overtime_seconds(std::int32_t ticks,float seconds_per_tick) noexcept {
    const float seconds=static_cast<float>(ticks)*seconds_per_tick;
    return seconds;
}
constexpr std::uint32_t source_objective_carrier_event(std::int32_t engine) noexcept {
    return engine==9?40u:39u;
}
constexpr std::uint32_t source_objective_carrier_indicator(std::int32_t engine) noexcept {
    return engine==9?19u:11u;
}
constexpr std::uint32_t source_objective_event_audience(std::int32_t engine) noexcept {
    return engine==9?10u:3u;
}
constexpr bool source_objective_spawn_filter(bool require_home,bool require_away,bool away) noexcept {
    return !(require_home && away) && !(require_away && !away);
}
constexpr bool source_objective_marker_eligible(std::uint16_t flags,std::int32_t kind) noexcept {
    return ((flags&1u) && kind!=1 && kind!=2) || ((flags&2u) && kind==1) || ((flags&4u) && kind==2);
}
constexpr std::uint32_t source_objective_marker_type(std::int32_t engine,bool spawn) noexcept {
    return spawn?(engine==9?2u:0u):(engine==9?3u:1u);
}
struct SourceObjectiveDistance {
    float x{},y{},z{},x_squared{},y_squared{},z_squared{},squared{};
};
inline SourceObjectiveDistance source_objective_distance_zxy(
    const std::array<float,3>& marker,const std::array<float,3>& point) noexcept {
    SourceObjectiveDistance value;
    value.z=marker[2]-point[2];value.x=marker[0]-point[0];
    value.z_squared=value.z*value.z;value.x_squared=value.x*value.x;
    value.y=marker[1]-point[1];const float zx=value.z_squared+value.x_squared;
    value.y_squared=value.y*value.y;value.squared=zx+value.y_squared;
    return value;
}
template<class ReadMarker>std::int32_t source_objective_marker_team(
    std::int32_t marker,ReadMarker&& read) {
    for(std::int32_t team=0;team<9;++team)
        if(read(team,false)==marker || read(team,true)==marker)return team;
    return -1;
}
// Diagnostics only; never source state, clocks, scores or network payloads.
inline constexpr std::array<std::uint32_t,57> source_objective_native_owner_entries{
    0x249440u,0x24a6c0u,0x24b390u,0x24b2f0u,0x24be50u,0x24b4c0u,0x24a180u,0x249460u,0x24b300u,0x24cd70u,0x24a810u,0x249590u,0x24a2e0u,0x24c670u,0x24cd10u,0x24b500u,0x24b5c0u,0x24c5d0u,0x24b2b0u,0x24c680u,0x24c6f0u,0x24ca90u,0x24a620u,0x24a130u,0x24a290u,0x24a1e0u,0x24b3c0u,0x24b800u,0x249600u,0x24a6f0u,0x24ce00u,0x2496b0u,0x24c540u,0x24c0f0u,0x24abf0u,0x24d530u,0x24d920u,0x249a40u,0x24b6e0u,0x24a2f0u,0x24a4b0u,0x24c490u,0x24a070u,0x24c390u,0x24d390u,0x24d1c0u,0x24c1e0u,0x24d5f0u,0x24d970u,0x24dae0u,0x24cef0u,0x24be90u,0x24a910u,0x24aa90u,0x24b950u,0x249b20u,0x24acb0u};
struct SourceObjectiveNativeTu15Stats {
    std::uint64_t arming{},decay{};
    std::array<std::uint64_t,64> owners{};
};
inline thread_local SourceObjectiveNativeTu15Stats source_objective_native_tu15_stats;
}
