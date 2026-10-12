#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
// Consumed projection of actual455163, fed by449EBF ->4563EB. The native
// datum is the caller's genuinely allocated salted handle, not a PC pool cast.
// Configuration application and output/controller bindings remain separate.
struct VistaPlayerBirthConsumed {
  unsigned slot{};
  std::uint32_t native_datum{};
  std::uint16_t flags{},creation_user{};
  std::uint32_t creation_controller{};
  std::array<std::byte,8> identity;
  std::array<std::byte,6> machine;
  std::array<std::byte,132> configuration;
  bool complete_player_record=false;
};
// Actual5DDB4E modes1/3/5 (world kinds1/2/4) and5D484F initializer projection. The
// same sole-host machine publication is separately verified against the real
// session path. This object does not implement a complete simulation world.
struct VistaLocalActivationIdentity {
  std::array<std::byte,6> machine;
  unsigned local_index{};
  std::uint32_t world_kind{};
  bool complete_simulation_metadata=false;
};
std::uint16_t vista_player_birth_flags(std::uint8_t new_byte,std::uint8_t left_byte) noexcept;
std::uint16_t native_player_birth_flags(std::uint8_t new_byte,std::uint8_t left_byte) noexcept;
// Exact successful-spawn tails:455CCE..455D46 and the independently executed
// retail14F64F/TU1.1150D2F/u515210F tails. These masks are intentionally distinct.
std::uint16_t vista_player_spawn_completed_flags(std::uint16_t) noexcept;
std::uint16_t native_player_spawn_completed_flags(std::uint16_t) noexcept;

// Requires the actual1188 copied initializer options (449E4C commits them before
//449EBF creates players); cannot imply a fulfilled/ready world. Slot/handle
// come from the real native birth owner after Cache/content initialization.
VistaPlayerBirthConsumed vista_player_birth_from_initialized_options(
    std::span<const std::byte> actual_options,unsigned actual_slot,
    std::uint32_t actual_native_datum);
// Requires the actual machine identifier already published by the real session
// owner. Missing, stale, unmatched or ambiguous ownership rejects explicitly.
VistaLocalActivationIdentity vista_player_local_identity_from_initialized_options(
    std::span<const std::byte> actual_options,
    std::span<const std::byte,6> actual_published_machine);
// Local branch of5AE0CB only. Remote-view lookup and uninitialized simulation
// branches are not synthesized by this native adapter.
std::uint8_t vista_player_local_activation_query(const VistaLocalActivationIdentity&,
    const VistaPlayerBirthConsumed&);
} // namespace halo2
