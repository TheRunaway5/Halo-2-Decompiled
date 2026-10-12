#pragma once
#include "halo2/vista_lobby_details.hpp"
#include <array>
#include <map>
#include <memory>

namespace halo2 {
// PC mainmenu source records only. These never select a world/cache donor.
struct VistaLobbyMetadataBytes {
  std::size_t file_offset{};
  std::vector<std::byte> bytes;
};
struct VistaLobbyMetadataTag {
  std::uint32_t datum = 0xffffffffu;
  std::string group, name;
  VistaLobbyMetadataBytes source;
};
struct VistaLobbyMetadataMap {
  VistaLobbyMetadataBytes source;
  std::int32_t campaign_id = -1, map_id = -1;
  std::uint32_t bitmap = 0xffffffffu;
  std::u16string english_name, english_description;
  std::string scenario_path;
  std::int32_t sort_order{};
  std::uint8_t flags{};
  std::array<std::uint8_t, 16> team_counts{};
};
struct VistaLobbyMetadataPath {
  VistaLobbyMetadataBytes source;
  std::int32_t campaign_id{}, map_id{};
  std::string scenario_path;
};
struct VistaLobbyMetadataParameter {
  VistaLobbyMetadataTag owner;
  VistaLobbyRuleParameter details;
  std::int32_t default_value{};
  std::uint32_t header{}, description{};
  std::vector<VistaLobbyMetadataBytes> choice_records;
};
struct VistaLobbyMetadataGroup {
  VistaLobbyMetadataBytes source;
  std::int32_t key{};
  std::vector<std::uint32_t> parameters;
  std::vector<VistaLobbyMetadataBytes> reference_records;
};
struct VistaLobbyMetadataVariant {
  VistaLobbyMetadataBytes source;
  std::uint32_t name{}, engine{};
  std::vector<std::pair<std::uint32_t, std::int32_t>> values;
  std::vector<VistaLobbyMetadataBytes> value_records;
};
struct VistaLobbyMetadataLocaleEntry {
  std::uint32_t string{};
  std::string utf8;
  std::u16string utf16;
};
struct VistaLobbyMetadataLocale {
  VistaLobbyMetadataTag owner;
  std::uint16_t first{}, count{};
};
struct VistaLobbyMetadataScreen {
  VistaLobbyMetadataTag owner;
  unsigned id{};
  VistaLobbyDetailsScreen details;
  std::vector<std::uint32_t> value_names;
  std::vector<VistaLobbyMetadataBytes> value_records;
  std::vector<std::vector<VistaLobbyMetadataBytes>> string_records;
};
struct VistaLobbyMetadata {
  VistaLobbyMetadataTag globals, scenario, ui_globals, game_settings;
  std::int16_t scenario_type{};
  std::uint32_t shared_ui_globals = 0xffffffffu, quick_settings_screen = 0xffffffffu,
                variant_locale = 0xffffffffu;
  std::vector<VistaLobbyMetadataBytes> level_headers, path_headers;
  // Complete source order, including every map-ID NONE slot.
  std::vector<VistaLobbyMetadataMap> campaign, multiplayer;
  std::vector<VistaLobbyMetadataPath> paths;
  std::vector<std::uint32_t> preview_datums;
  std::map<std::uint32_t, VistaLobbyMetadataTag> preview_tags;
  std::map<std::uint32_t, VistaLobbyMetadataParameter> parameters;
  std::vector<VistaLobbyMetadataGroup> groups;
  std::vector<VistaLobbyMetadataVariant> variants;
  std::vector<VistaLobbyMetadataLocaleEntry> locale_entries;
  std::map<std::uint32_t, VistaLobbyMetadataLocale> locales;
  std::map<std::uint32_t, VistaLobbyMetadataScreen> screens;

  // Original45948C ignores campaign ID;459526 uses both IDs. Never share
  // their predicates or filter the raw records before these queries.
  const VistaLobbyMetadataMap* campaign_definition(std::int32_t map_id) const;
  const VistaLobbyMetadataMap* multiplayer_definition(std::int32_t map_id) const;
  const VistaLobbyMetadataPath* campaign_path(std::int32_t campaign_id,
      std::int32_t map_id) const;
  std::u16string localize(std::uint32_t owner, std::uint32_t string) const;
  // No stored pointers: copy/move of this bank cannot leave dangling groups.
  // The returned borrowed catalogue is valid while this bank remains alive.
  VistaLobbyRuleCatalogue catalogue() const;
};
std::u16string vista_lobby_metadata_utf16(std::string_view utf8);
// Bind only proven metadata leaves. Widgets, source screen-tag lookup,
// selected map/variant, image resolution and context remain caller-owned.
// Callbacks retain the complete bank and stable catalogue backing.
VistaLobbyDetailsServices vista_lobby_metadata_services(
    std::shared_ptr<const VistaLobbyMetadata>);
} // namespace halo2
