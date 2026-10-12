#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
using VistaLobbyDetailsObject = std::uintptr_t;
struct VistaLobbyRuleChoice {
  std::int32_t value{};
  std::uint32_t string{};
};
struct VistaLobbyRuleParameter {
  std::uint32_t datum{}, parameter{}, locale{}, title{};
  std::vector<VistaLobbyRuleChoice> choices;
};
struct VistaLobbyRuleGroup {
  std::int32_t key{};
  // Authored order, including missing sily references. Never sort/deduplicate.
  std::vector<const VistaLobbyRuleParameter*> parameters;
};
struct VistaLobbyRuleCatalogue {
  std::vector<VistaLobbyRuleGroup> groups;
};
struct VistaLobbyDetailsScreen {
  std::uint32_t locale = 0xffffffffu;
  std::vector<std::vector<std::uint32_t>> values;
};
struct VistaLobbyDetailsIndices {
  std::uint32_t first_quick = 6, heading = 3, name = 4;
  std::uint32_t icon = 4, status = 47;
  std::int32_t value_group{}, value_index{};
};
struct VistaLobbyDetailsMap {
  std::int32_t campaign_id{}, map_id{};
  // Actual 61564E output3, session+4CBC. This is not a bitmap reference.
  VistaLobbyDetailsObject identifier{};
};
using VistaLobbyMapInfo = std::array<std::byte, 0xc58>;
struct VistaLobbyDetailsServices {
  std::function<VistaLobbyDetailsObject(std::uint32_t)> text; // 611973.
  std::function<VistaLobbyDetailsObject(std::uint32_t)> bitmap; // 6119F7.
  std::function<void(VistaLobbyDetailsObject, std::uint8_t)> visible; // child+6E.
  std::function<void(VistaLobbyDetailsObject, std::uint32_t)> string_id; // 61BF85.
  std::function<void(VistaLobbyDetailsObject, std::u16string_view)> string; // 61BAA9.
  std::function<void(VistaLobbyDetailsObject, std::uint32_t)> frame; // 61C300.
  std::function<std::uint32_t(std::uint32_t)> screen_tag; // 60C701.
  std::function<const VistaLobbyDetailsScreen*(std::uint32_t)> screen_metadata; // 639623.
  // 60BB1D supplies the actual wgtz/goof owner. Null includes an absent
  // globals/goof tag; the ordered source groups are otherwise retained.
  std::function<const VistaLobbyRuleCatalogue*()> catalogue;
  // 63AA54 consumes the actual selected variant, including +44/+48.
  std::function<std::int32_t(std::span<const std::byte>, std::uint32_t)> parameter;
  // 43E3AC must use this unic owner's range, never a flattened SID table.
  std::function<std::u16string(std::uint32_t, std::uint32_t)> localize;
  // Exact source %s%s%s UTF16 formatting boundary, with 512-unit buffers.
  std::function<std::u16string(std::u16string_view, std::u16string_view,
      std::u16string_view)> format;
  std::function<std::uint8_t()> status_visibility; // 40B8C6's exact AL.

  std::function<std::uint8_t(VistaLobbyDetailsMap&)> map_query; // 61564E.
  std::function<std::uint8_t()> campaign; // 615FF6.
  std::function<std::optional<std::uint32_t>(std::int32_t, std::int32_t)> campaign_definition; // 45948C -> +0C.
  std::function<std::optional<std::uint32_t>(std::int32_t)> multiplayer_definition; // 459A1A -> +08.
  std::function<VistaLobbyDetailsObject(std::uint32_t, std::uint32_t)> resolve_image; // 77D482.
  std::function<void(VistaLobbyDetailsObject, VistaLobbyDetailsObject)> image; // 61CF73.
  // 44CF41 returns the actual metadata record. Successful records require
  // through +A80; the complete 459369 copy owner below derives output0.
  std::function<std::uint8_t(VistaLobbyDetailsObject, std::span<const std::byte>&)> identifier_record;
  std::function<std::uint8_t()> identifier_provider; // actual context -> 61AA42.
};
// Complete 6104E8 plus ordered 63ACDC/63AD57 tag lookup. Missing category,
// row, title or child leaves retained widget text/visibility untouched.
void vista_lobby_quick_detail(std::span<const std::byte> variant,
    VistaLobbyDetailsObject text, std::int32_t index, std::u16string_view separator,
    const VistaLobbyDetailsServices&);
void vista_lobby_details_game_icon(std::span<const std::byte> variant,
    VistaLobbyDetailsObject bitmap, const VistaLobbyDetailsServices&); // 60F217.
// Complete 611243. PC14 uses indices6..10/3/4/4/47 and value block0, entry0.
void vista_lobby_details(std::uint32_t screen_id, std::span<const std::byte> variant,
    const VistaLobbyDetailsIndices&, const VistaLobbyDetailsServices&);
// Complete 459369: always clears the full output and never assigns dword0.
bool vista_lobby_map_info(VistaLobbyDetailsObject identifier, VistaLobbyMapInfo&,
    const VistaLobbyDetailsServices&);
// Complete 643150, including the campaign early return and MP NULL override.
void vista_lobby_map_bitmap(const VistaLobbyDetailsServices&);
} // namespace halo2
