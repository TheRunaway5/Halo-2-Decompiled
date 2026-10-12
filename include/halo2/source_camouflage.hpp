#pragma once
#include <array>
#include <cstdint>
#include <functional>

namespace halo2 {
struct SourceCamouflageAmounts {
  float primary{},secondary{},fade_speed{}; // unit+2B0/+2B4/+2B8
};
// Views keep the ordinary unit action word and the fade values authoritative
// in their existing storage. These owners do not define allocation defaults.
struct SourceCamouflagePlayerView {
  std::array<std::int16_t,2>& timers; // player+198/+19A
  std::uint32_t current_unit;        // player+2C
};
struct SourceCamouflageUnitView {
  std::uint32_t& flags; // unit+134, also used by other unit action owners
  float& primary;      // unit+2B0
  float& secondary;    // unit+2B4
  float& fade_speed;   // unit+2B8
  std::uint32_t replication_entity; // unit+D4
  std::uint8_t intrinsic_mode{}; // unit+2BC; distinct from the player timers
};
struct SourceCamouflageServices {
  std::function<bool(std::uint32_t,std::int32_t)> trait;
  std::function<SourceCamouflagePlayerView(std::uint32_t)> player;
  std::function<SourceCamouflageUnitView(std::uint32_t)> unit;
  std::function<void(std::uint32_t,std::uint32_t)> dirty;
  std::function<void(std::uint32_t,bool)> intrinsic_control;
};

// Whole TU15C8B0/1537F0/153990, including the original x87 rounding-control
// conversion of signed tick_rate*0.5 before narrowing the grant to signed16.
void source_camouflage_refresh(bool engine_present,std::uint32_t player,
    std::int16_t tick_rate,std::uint16_t x87_control_word,
    const SourceCamouflageServices&);
void source_powerup_grant(std::uint32_t player,std::int16_t index,
    std::int16_t ticks,const SourceCamouflageServices&);
// Whole TU153920; its caller owns the current-unit guard and scheduling.
void source_powerup_tick(std::uint32_t player,const SourceCamouflageServices&);
void source_camouflage_deactivate(std::uint32_t unit,float supplied_speed,
    const SourceCamouflageServices&);
// Whole TU C7B70's actor-NONE/multiplayer branch. Campaign debug override and
// world scheduling remain the caller's separate contracts.
void source_camouflage_fade(std::uint32_t unit,float clock_dt,
    const SourceCamouflageServices&);
// Whole D2310's intrinsic-zero arithmetic, or its actual intrinsic delegate.
// The caller supplies authored weap+27C/+280 or jpt!+30 and zero limit.
void source_camouflage_disrupt(std::uint32_t unit,float amount,float limit,
    const SourceCamouflageServices&);
}
