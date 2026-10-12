#pragma once
#include <array>
#include <cstdint>
namespace halo2 {
struct SourcePlayerCarryItem {
  std::uint32_t datum = 0xffffffffu;
  std::uint32_t definition_flags{}; // Actual weap+12C.
};
struct SourcePlayerCarryAdmission {
  std::array<SourcePlayerCarryItem,4> slots;
  std::array<std::uint32_t,2> current{0xffffffffu,0xffffffffu};
  std::array<std::uint32_t,2> requested{0xffffffffu,0xffffffffu};
  std::uint32_t parent = 0xffffffffu; // Actual unit+14.
  std::uint32_t target_definition_flags{};
};
// Complete 152080 player-only restriction, applied to CEC00's four bytes:
// carry-primary, carry-secondary, replace-primary, replace-secondary. This
// owner can only clear permissions; it cannot supply missing CEC00 capability,
// magazine, heat, item state or game-engine admission results.
void source_restrict_player_inventory(std::array<std::uint8_t,4> &,
                                     const SourcePlayerCarryAdmission &);
} // namespace halo2
