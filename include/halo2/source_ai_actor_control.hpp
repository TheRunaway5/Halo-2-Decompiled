#pragma once
#include "halo2/source_ai_actor.hpp"

namespace halo2 {
// Actual object bodies, swarm/clump banks and live AI globals. The world owns
// their lifetimes/checkpoints. These operations neither create bodies nor
// substitute actor eligibility for complete birth/admission.
struct SourceAiActorControlServices {
  SourceUnitControl &objects;
  DatumPool &perceptions, &clumps;
  std::span<std::uint8_t> ai_globals;
  std::function<std::int32_t()> game_tick;
};
// Complete base1E13F0/BAF80,1E11B0/268810 and1E1150/290BF0.
// Source-backed adapters and the actor bank borrow the same records. Normal
// engine callers use the bank's can_activate/set_active/set_team methods.
class SourceAiActorControl {
public:
  static bool can_activate(const DatumPool &actors, std::uint32_t actor,
                           const SourceAiActorControlServices &);
  static bool set_active(DatumPool &actors, std::uint32_t actor, bool requested,
                         const SourceAiActorControlServices &);
  static void set_team(DatumPool &actors, std::uint32_t actor, std::int16_t team,
                       const SourceAiActorControlServices &);
};
} // namespace halo2
