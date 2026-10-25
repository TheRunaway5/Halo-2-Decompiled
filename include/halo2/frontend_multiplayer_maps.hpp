#pragma once
#include "halo2/frontend_lobby_maps.hpp"
#include <limits>
#include <string_view>

namespace halo2 {
// MenuState stores a datum into the authored catalog, not a row in its sorted
// and unlock-filtered presentation. This leaves its public object layout intact.
inline const UiLobbyMap *frontend_multiplayer_map(
    std::span<const UiLobbyMap> maps, unsigned catalog_index) {
  return catalog_index < maps.size() ? &maps[catalog_index] : nullptr;
}

inline unsigned frontend_multiplayer_initial_map(
    std::span<const UiLobbyMap> maps, std::span<const UiLobbyMap> campaign) {
  const auto list = frontend_lobby_maps(maps, campaign,
                                       frontend_lobby_new_profile_map_progress);
  // Preserve the native menu's existing Lockout default by its authored ID50,
  // independently of the cache's physical record order.
  for (const auto index : list.catalog_indices)
    if (maps[index].map_id == 50) return index;
  return list.catalog_indices.empty() ? std::numeric_limits<unsigned>::max()
                                      : list.catalog_indices.front();
}

// A cache filename is the exact authored scenario leaf plus .map. Reject
// traversal, absolute paths, drive names and extensions instead of resolving
// them outside the launcher-selected map directory. Never alias DLC names.
inline std::string frontend_multiplayer_map_file(const UiLobbyMap &map) {
  const std::string_view path = map.scenario_path;
  if (map.map_id == -1 || path.empty() || path.size() > 255)
    throw FormatError("The selected multiplayer map has no authored scenario path.");
  std::size_t begin = 0;
  std::string_view leaf;
  while (begin < path.size()) {
    const auto end = path.find_first_of("\\/", begin);
    const auto part = path.substr(begin, end == path.npos ? path.size() - begin
                                                         : end - begin);
    if (part.empty() || std::any_of(part.begin(), part.end(), [](unsigned char c) {
          return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == '-');
        }))
      throw FormatError("The selected multiplayer map has an unsafe authored scenario path.");
    leaf = part;
    if (end == path.npos) break;
    begin = end + 1;
    if (begin == path.size())
      throw FormatError("The selected multiplayer map has an unsafe authored scenario path.");
  }
  return std::string(leaf) + ".map";
}

inline std::string frontend_multiplayer_launch_file(
    std::span<const UiLobbyMap> maps, unsigned catalog_index) {
  const auto map = frontend_multiplayer_map(maps, catalog_index);
  if (!map) throw FormatError("The selected multiplayer map is absent from its authored catalog.");
  if (map->name.empty())
    throw FormatError("The selected multiplayer map has no authored title; its catalog identity is unqualified.");
  const auto leaf = frontend_multiplayer_map_file(*map);
  if (map->cache_file.empty()) return leaf;
  const std::filesystem::path bound(map->cache_file);
  if (!bound.is_absolute() || bound.lexically_normal() != bound || bound.filename() != leaf)
    throw FormatError("The selected custom map lost its exact prepared cache binding.");
  return map->cache_file;
}

inline bool frontend_multiplayer_energy_start(const UiLobbyMap &map) {
  // Actual catalog/scenario map IDs: Midship20 and Waterworks40. Catalog order
  // differs between retail and MCC and must not change this existing gate.
  return map.map_id == 20 || map.map_id == 40;
}
} // namespace halo2
