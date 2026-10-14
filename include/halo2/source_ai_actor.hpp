#pragma once
#include "halo2/datum_pool.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
class SourceUnitControl;
struct SourceAiActorLifecycleServices;
struct SourceAiActorControlServices;
struct SourceAiActorLocationReferenceServices;
struct SourceAiActorMembershipServices;
//1E1A00 is a complete lifecycle owner, not an allocator-only release. Binding
// must have its real synchronous callback; an unprovided owner must fail.
struct SourceAiActorBindingServices {
  SourceUnitControl &units;
  std::function<void(std::uint32_t actor, bool)> release_actor;
};
// Requested character datum and its first inherited nonempty general block.
struct SourceAiActorDefinition {
  std::uint32_t datum = 0xffffffffu, general_flags{};
  std::int16_t type{};
};
struct SourceAiActorConstructorConstants {
  std::uint32_t empty_reference{};
  std::array<std::uint32_t, 3> point_bits{}, direction_bits{};
};
struct SourceAiActorCreateContext {
  std::int32_t game_tick;
  std::int16_t ticks_per_second, bsp_index;
  SourceAiActorCreateContext(std::int32_t tick_value, std::int16_t rate_value,
                             std::int16_t selected_bsp)
      : game_tick(tick_value), ticks_per_second(rate_value),
        bsp_index(selected_bsp) {}
};
// Immutable operands of base-retail 1DFB90 only. Clock and selected BSP are
// live creation inputs. Other images/layouts need independently verified
// adapters.
SourceAiActorConstructorConstants
source_retail_ai_actor_constructor_constants();
// The original 16B840 allocator has already zeroed the 0x888-byte record and
// assigned its salt. Complete cold1DFB90 writes and its NULL-callback slot
// initialization follow; unit/squad/perception/activation owners are separate.
void source_ai_actor_initialize(std::span<std::uint8_t>,
                                const SourceAiActorDefinition &,
                                const SourceAiActorConstructorConstants &,
                                const SourceAiActorCreateContext &,
                                std::uint16_t &flag3_counter);
// Base-retail actor records, cold allocation and explicit unit binding.
// Complete birth/admission remain separate owners. Control and teardown
// borrow actual linked owners; required callbacks never receive a fallback.
class SourceAiActorBank {
public:
  static constexpr std::uint32_t none = 0xffffffffu;
  SourceAiActorBank();
  // Borrow the already-constructed actual actor bank. No initialization,
  // allocation, copy or second authority; the memory owner must outlive this
  // view. Source-backed continuation belongs to the enclosing whole bank;
  // reconstruct this view after replacing that bank's backing.
  SourceAiActorBank(SourceRuntimeMemory &, std::uint32_t source_pool);
  SourceAiActorBank(SourceRuntimeMemory &, NativeRuntimeAddress source_pool);
  std::uint32_t allocate(const SourceAiActorDefinition &,
                         const SourceAiActorConstructorConstants &,
                         const SourceAiActorCreateContext &,
                         std::uint16_t &flag3_counter);
  std::span<const std::uint8_t> record(std::uint32_t full_actor_datum) const;
  // Base1E0D50/1E1250, including old-owner release and actual unit-control
  // dispatch. These do not supply full birth, admission or actor destruction.
  void bind_unit(std::uint32_t actor, std::uint32_t unit,
                 const SourceAiActorBindingServices &);
  void unbind_unit(std::uint32_t actor, const SourceAiActorBindingServices &);
  // Releases the brain and its owned references. The unit remains with its
  // actual object-lifetime owner. The flag is original203ED0 keep_count.
  void release_actor(std::uint32_t actor, bool keep_squad_count,
                      const SourceAiActorLifecycleServices &);
  bool can_activate(std::uint32_t actor, const SourceAiActorControlServices &) const;
  bool set_active(std::uint32_t actor, bool requested, const SourceAiActorControlServices &);
  void set_team(std::uint32_t actor, std::int16_t team, const SourceAiActorControlServices &);
  void reset_location_references(std::uint32_t actor, bool conditional,
                                 const SourceAiActorLocationReferenceServices &);
  void join_squad(std::uint32_t actor, std::int16_t squad, bool keep_team,
                   const SourceAiActorMembershipServices &);
  bool activate_squad(std::int16_t squad, const SourceAiActorMembershipServices &);
  void add_to_global(std::uint32_t actor, const SourceAiActorMembershipServices &);
  void remove_from_squad(std::uint32_t actor, bool keep_count, const SourceAiActorMembershipServices &);
  void remove_from_global(std::uint32_t actor, const SourceAiActorMembershipServices &);
  const DatumPool &pool() const { return actors_; }

private:
  DatumPool actors_;
};
} // namespace halo2
