#pragma once
#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
struct FrontendLobbyMapProgress {
  int best = -1;
  unsigned key_index = 1;
};
// Original1A0750 initializes all four keys to0. Actual1A06F0 rejects key0
// because it is absent from the literal campaign level table, yielding NONE.
inline constexpr int frontend_lobby_new_profile_map_progress = -1;

// Original1A06F0, including the last equal-key index. Only actual campaign
// definitions admit a key; a large unknown integer cannot unlock Foundation.
inline FrontendLobbyMapProgress frontend_lobby_map_progress(
    std::span<const UiLobbyMap> campaign, const std::array<int, 4> &keys) {
  FrontendLobbyMapProgress output;
  for (unsigned i = 0; i < keys.size(); ++i)
    if (keys[i] != -1 && output.best <= keys[i] &&
        std::any_of(campaign.begin(), campaign.end(),
            [&](const UiLobbyMap &map) { return map.map_id == keys[i]; })) {
      output.best = keys[i]; output.key_index = i;
    }
  return output;
}
// Original19C4E0(1) queries the maximum map key of campaign1 from the
// source scenario path table. Literal mainmenu campaign definitions contain
// the identical key pairs, verified by the fixture (maximum805).
inline int frontend_lobby_map_unlock_threshold(std::span<const UiLobbyMap> campaign) {
  int threshold = -1;
  for (const auto &map : campaign)
    if (map.campaign_id == 1 && map.map_id != -1 && threshold <= map.map_id)
      threshold = map.map_id;
  return threshold;
}
struct FrontendLobbyMaps {
  // Values index the original catalog, not a separate hardcoded name array.
  std::vector<unsigned> catalog_indices;
  bool admission_known{};
  std::optional<unsigned> last_selected_row;
};
// Original19C7E0/19C9A0 order followed by2C9928 flag admission and2C9A73
// last-map focus. Unknown progress is explicit: retain the existing catalog
// without claiming source unlock parity. Known new-profile progress is NONE.
inline FrontendLobbyMaps frontend_lobby_maps(std::span<const UiLobbyMap> maps,
    std::span<const UiLobbyMap> campaign, std::optional<int> best_key,
    int last_map_id = -1) {
  FrontendLobbyMaps output;
  output.admission_known = best_key.has_value();
  const auto threshold = frontend_lobby_map_unlock_threshold(campaign);
  for (unsigned i = 0; i < maps.size(); ++i)
    if (maps[i].map_id != -1 && (!(maps[i].flags & 1) || !best_key || *best_key >= threshold))
      output.catalog_indices.push_back(i);
  // All literal retail orders are unique. qsort's equal-order arrangement
  // is not established; retaining input ties is an explicit wider limit.
  std::stable_sort(output.catalog_indices.begin(), output.catalog_indices.end(),
      [&](unsigned a, unsigned b) { return maps[a].sort_order < maps[b].sort_order; });
  if (last_map_id != -1)
    for (unsigned row = 0; row < output.catalog_indices.size(); ++row)
      if (maps[output.catalog_indices[row]].map_id == last_map_id) {
        output.last_selected_row = row; break;
      }
  return output;
}
struct FrontendLobbyMapPreview {
  const UiLobbyMap *map{};
  unsigned bitmap_index = 1, description_text = 0;
};
// Original2C9B7C binds runtime bitmap1 and text2 (raw pane text0), using
// focused data, not the accepted lobby map. Missing datum/map retains prior
// bindings; callers distinguish no new binding from an empty description.
inline std::optional<FrontendLobbyMapPreview> frontend_lobby_map_preview(
    std::span<const UiLobbyMap> maps, const FrontendLobbyMaps &list,
    std::optional<unsigned> focused_row) {
  if (!focused_row || *focused_row >= list.catalog_indices.size()) return {};
  const auto index = list.catalog_indices[*focused_row];
  if (index >= maps.size()) throw FormatError("frontend focused map escapes its catalog");
  return FrontendLobbyMapPreview{&maps[index]};
}
} // namespace halo2
