#pragma once
#include "halo2/math.hpp"
#include "halo2/physics_body_factory.hpp"

namespace halo2 {
// Current source-owned banks. TU1.1/TU1.5 have identical record fields;
// their image globals and instruction addresses are separately authenticated.
struct SourceCampaignCoopContext {
  std::uint32_t users,game,clock,players,objects,cinematic;
};
struct SourceCampaignCoopRespawnServices {
  // CD5C0 and1CE060 inspect actual animation/action and AI safety owners.
  // These services must be bound to live world state, not a safe default.
  std::function<bool()> action_unsafe,combat_unsafe;
  // E5680 examines the selected biped's source state.
  std::function<bool(std::uint32_t)> biped_unsafe;
  // Complete1509C0 player birth and1500B0 placement near the selected survivor.
  // A failed birth/placement leaves the pending request alive for another tick.
  std::function<bool(std::uint32_t)> spawn;
  std::function<bool(std::uint32_t,std::uint32_t,Vec3)> place;
};
// Complete13A010 predicate. The override is original4FCDBF, whose producer
// is still external; it must not be inferred from a native debug setting.
bool source_campaign_coop_respawn_allowed(std::int32_t game_type,
    std::int16_t difficulty,bool source_override);
// Complete14E4C0. Starts once, clears bypass and uses the original clock rate
// and3-second operand. Existing requests keep their timer and bypass state.
void source_campaign_coop_request_respawn(SourcePhysicsContactMemory &,
    const SourceCampaignCoopContext &,float source_delay_seconds,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver & = {});
// Complete14E510 ordering and retained state, with the named nested owners
// above explicit. Status+9C, pending+9E, bypass+9F, ticks+A0 are source fields.
// This kernel neither creates a cooperative session nor supplies a respawn
// location in place of1500B0's authored collision/location owner.
void source_campaign_coop_update_respawn(SourcePhysicsContactMemory &,
    const SourceCampaignCoopContext &,const SourceCampaignCoopRespawnServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
