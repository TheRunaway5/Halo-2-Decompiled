#pragma once
#include <array>
#include <cstdint>
namespace halo2 {
struct DamageHistoryEntry {
  std::uint32_t tick=0xffffffffu;
  float amount{};
  std::uint32_t object=0xffffffffu, player=0xffffffffu;
  std::uint8_t cause{};
};
using DamageHistory=std::array<DamageHistoryEntry,4>;
// TU CBA60 history writer only. The caller owns the identity admission gate
// and death-attribution-before-recording schedule; the death tail is separate.
void record_damage_history(DamageHistory &,std::uint32_t tick,float amount,
    std::uint32_t object,std::uint32_t player,std::uint8_t cause);
// TU CB410 records normalized shield then body amounts, including zero, when
// either full source identity is present. Call after lethal attribution.
void record_damage_result_history(DamageHistory &,std::uint32_t tick,
    float body_damage,float shield_damage,std::uint32_t object,
    std::uint32_t player,std::uint8_t cause);
}
