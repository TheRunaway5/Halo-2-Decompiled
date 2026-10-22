#include "halo2/lethal_attribution.hpp"
#include <algorithm>
#include <bit>
#include <limits>
namespace halo2 {
namespace {
bool enemy(std::int16_t a, std::int16_t b) {
  // 1E2F50's NONE short returns true before the Slayer membership callback.
  return a == -1 || b == -1 || a != b;
}
}
std::optional<LethalAttribution> lethal_attribution(
    const LethalAttributionInput &input, const DamageHistory &history,
    std::span<const AttributionPlayer> players) {
  if (input.mode == 4 || input.unit_type > 1 || !(input.result_flags & 1u) ||
      (input.result_flags & 0x800u) || !input.engine_present ||
      !input.statistics_valid) return {};
  auto lookup = [&](std::uint32_t datum) -> const AttributionPlayer * {
    if (datum == 0xffffffffu) return nullptr;
    const auto found = std::find_if(players.begin(), players.end(),
        [datum](const auto &player) { return player.datum == datum; });
    return found == players.end() ? nullptr : &*found;
  };
  LethalAttribution result;
  result.cause = input.cause;
  const bool other_candidates = input.incoming_player == 0xffffffffu ||
      !enemy(input.victim_membership, input.incoming_designator);
  // Integer short * 6 is exactly representable in binary32; the original
  // nearest FISTP therefore yields this same integer for every short rate.
  const auto cutoff = input.tick - std::uint32_t(int(input.clock_rate) * 6);
  int selected = -1, direct_match = -1;
  float maximum = -std::numeric_limits<float>::max();
  for (unsigned i = 0; i < history.size(); ++i) {
    const auto &entry = history[i];
    if (entry.player == 0xffffffffu) continue;
    if (entry.player == input.incoming_player) direct_match = int(i);
    else {
      if (!other_candidates) continue;
      const auto *player = lookup(entry.player);
      if (!player || !enemy(input.victim_membership, player->membership)) continue;
    }
    if (entry.tick > cutoff && entry.amount > maximum) {
      selected = int(i);
      maximum = entry.amount;
    }
  }
  if (selected == -1) selected = direct_match;
  float threshold = 0;
  if (selected != -1) {
    const auto &entry = history[selected];
    result.cause = entry.cause;
    if (lookup(entry.player)) {
      result.credited_player = entry.player;
      threshold = entry.amount * std::bit_cast<float>(0x3ecccccdu);
      if (entry.player == input.incoming_player) result.cause = input.cause;
    }
  }
  if (result.credited_player == 0xffffffffu && lookup(input.incoming_player)) {
    result.credited_player = input.incoming_player;
    result.cause = input.cause;
  }
  if (const auto *credited = lookup(result.credited_player)) {
    result.relationship = !enemy(input.victim_membership, credited->membership);
    result.credited_kill = !result.relationship && input.victim_player != 0xffffffffu;
    result.nonenemy_credit = !result.credited_kill;
    // The source writer relationship byte is set by the nonenemy-credit path,
    // including its absent-victim branch.
    result.relationship = result.nonenemy_credit;
  }
  if (threshold > 0 && input.victim_player != 0xffffffffu) {
    for (const auto &entry : history) {
      const auto *player = lookup(entry.player);
      if (!player || entry.player == result.credited_player ||
          (entry.player != input.incoming_player && !(entry.amount > threshold))) continue;
      auto &identities = enemy(input.victim_membership, player->membership)
          ? result.assists : result.nonenemy_history;
      auto &count = enemy(input.victim_membership, player->membership)
          ? result.assist_count : result.nonenemy_history_count;
      if (std::find(identities.begin(), identities.begin() + count, entry.player) ==
          identities.begin() + count) identities[count++] = entry.player;
    }
  }
  result.victim_death = input.victim_player != 0xffffffffu;
  result.suicide = result.victim_death && input.incoming_player == input.victim_player;
  return result;
}
}
