#pragma once
#include "halo2/source_result_statistics.hpp"
#include <optional>

namespace halo2 {
struct SourceResultCaptureConstants {
  std::array<SourceResultStatDescriptor,7> weapon_descriptors{};
  // Recovered41-case compiled reporting-type switch and its original default.
  std::array<std::uint8_t,64> cause_weapons{};
  float multikill_seconds{};
};
struct SourceResultCapture {
  SourceResultPublication& publication;
  const SourceResultCaptureConstants& constants;
};
void source_result_weapon_add(SourceResultCapture&,unsigned player,unsigned kind,
    unsigned weapon,std::int32_t delta);
std::int32_t source_result_multikill_cutoff(std::uint32_t tick,std::int16_t rate,
    const SourceResultCaptureConstants&);
// Original1F2890 retained weapon, best-spree, running-streak and multikill
// mutation. Caller owns live attribution, engine/team gates and replication.
// A missing cause explicitly leaves the weapon producer unavailable.
std::int16_t source_result_credited_kill(SourceGameEndStats&,unsigned player,
    std::uint32_t tick,std::int32_t cutoff,std::optional<std::uint8_t> cause,
    SourceResultCapture* = nullptr,int team = -1);
// Original1F2A50 counter/weapon mutation and streak reset. Selected credit is
// independent of incoming-self. Source player+1B0/betrayal UI services remain
// caller-owned; no service return is supplied by these functions.
void source_result_victim_death(SourceGameEndStats&,unsigned victim,int credited,
    bool incoming_self,bool betrayal,std::optional<std::uint8_t> cause,
    SourceResultCapture* = nullptr,int victim_team = -1,int credited_team = -1);
struct SourceResultLifetimeInput {
  std::uint32_t tick{};
  std::int16_t clock_rate=30;
  std::int8_t raw_mode{};
  bool current_unit{},engine_present{},lifecycle_one{},status_one{};
};
// Stat13 branch of15BB20 /15D160 /15E540; original player phase also owns
// other fields/services. Tick is sampled before world objects/clock increment.
bool source_result_lifetime_tick(SourceResultPublication&,unsigned player,
    const SourceResultLifetimeInput&);
} // namespace halo2
