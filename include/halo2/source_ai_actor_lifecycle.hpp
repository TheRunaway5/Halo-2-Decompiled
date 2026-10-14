#pragma once
#include "halo2/source_ai_actor.hpp"
#include <array>
#include <optional>

namespace halo2 {
struct SourceAiActorSlotCallbacks {
  std::uint32_t handler{}, stop{}, release{}, forget{}, notify_location{};
};
// Immutable base46EEB8 data. Entries are source code identifiers for the real
// owner dispatcher, never host function pointers. A missing handler is distinct
// from a valid handler with an actual NULL callback.
const std::array<SourceAiActorSlotCallbacks, 0x83> &
source_retail_ai_actor_slot_callbacks();

using SourceAiFollowReferenceRelease = std::function<void(std::uint32_t reference)>;
using SourceAiSlotLocationNotify = std::function<void(std::uint32_t entry,
    std::uint32_t actor, std::span<std::uint8_t> slot, bool conditional, DatumPool &actors)>;
// Narrow borrowed services for complete1E22D0. Callback owners must outlive
// synchronous use; no function/record authority is copied or defaulted.
struct SourceAiActorLocationReferenceServices {
  std::span<const SourceAiActorSlotCallbacks> slot_handlers;
  std::uint32_t empty_reference;
  const SourceAiFollowReferenceRelease &release_follow_reference;
  const SourceAiSlotLocationNotify &notify_slot_location;
};

// Borrow actual shared banks and live globals, not a second perception/squad
// authority. Callback operations must be their complete source owners. When
// a required operation is reached without an owner, release throws and never
// invents its completion. Callback spans/pools are the actual mutable records.
struct SourceAiActorLifecycleServices {
  SourceUnitControl &units;
  DatumPool &squad_records, &joint_states, &props, &prop_references, &tracking;
  DatumPool &clumps, &locations, &perceptions;
  DatumPool &commands, &joint_commands, &script_threads;
  std::span<std::uint8_t> ai_globals, flag3_statistics;
  std::span<const SourceAiActorSlotCallbacks> slot_handlers;
  std::uint32_t empty_reference;
  std::function<std::int32_t()> game_tick;
  std::function<std::optional<SourceAiActorDefinition>(std::uint32_t)> character_general;
  std::function<void(std::uint32_t entry, std::uint32_t actor,
                     std::span<std::uint8_t> slot, DatumPool &actors)> stop_slot;
  std::function<void(std::uint32_t entry, std::uint32_t actor,
                     std::span<std::uint8_t> slot, std::uint32_t reference,
                     DatumPool &actors)> release_slot_reference;
  SourceAiFollowReferenceRelease release_follow_reference;
  SourceAiSlotLocationNotify notify_slot_location;
};
inline SourceAiActorLocationReferenceServices source_ai_actor_location_reference_services(
    const SourceAiActorLifecycleServices &services) {
  return {services.slot_handlers,services.empty_reference,
          services.release_follow_reference,services.notify_slot_location};
}

// Raw source-owner boundary used by the actor bank and original-bank adapters.
// Engine callers use SourceAiActorBank::release_actor. Complete base1E1A00,
// including203ED0/203FB0/1E1740/1E2150, clump2694D0/26AC60, location
//26DC20/1E22D0/1F86A0, swarm28FD90/28E160 and the empty2628F0 branch.
//258600/258540/258480 command/thread retirement. Followed-reference registry
// and non-NULL slot operations retain owners.
class SourceAiActorLifecycle {
public:
  static void reset_location_references(DatumPool &actors, std::uint32_t actor,
      bool conditional, const SourceAiActorLocationReferenceServices &);
  static void release(DatumPool &actors, std::uint32_t actor,
                      bool keep_squad_count,
                      const SourceAiActorLifecycleServices &);
};
} // namespace halo2
