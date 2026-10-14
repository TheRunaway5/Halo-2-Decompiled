#pragma once
#include "halo2/source_ai_actor_control.hpp"
#include "halo2/source_ai_actor_lifecycle.hpp"

namespace halo2 {
// Borrow the same world banks/control/reference owners. Selected BSP and clock
// are live inputs. This does not create actors or repair unrelated membership.
struct SourceAiActorMembershipServices {
  const SourceAiActorControlServices &control;
  DatumPool &squads;
  const SourceAiActorLocationReferenceServices &references;
  std::int16_t bsp_index;
};
// Complete base203D70/203BF0/203F60/203ED0/203FB0 with actual linked order,
// control and1E22D0 reference reset. Engine callers use SourceAiActorBank.
// Raw remove operations are also shared by the lifecycle owner, so teardown
// and ordinary membership never maintain separate unlink implementations.
class SourceAiActorMembership {
public:
  static void join_squad(DatumPool &actors, std::uint32_t actor, std::int16_t squad,
                         bool keep_team, const SourceAiActorMembershipServices &);
  static bool activate_squad(DatumPool &actors, std::int16_t squad,
                             const SourceAiActorMembershipServices &);
  static void add_to_global(DatumPool &actors, std::uint32_t actor,
                            const SourceAiActorMembershipServices &);
  static void remove_from_squad(DatumPool &actors, std::uint32_t actor,
      bool keep_count, DatumPool &squads, std::span<std::uint8_t> ai_globals);
  static void remove_from_global(DatumPool &actors, std::uint32_t actor,
                                 std::span<std::uint8_t> ai_globals);
};
} // namespace halo2
