#include "halo2/vista_lobby_details.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F, class... A> decltype(auto) call(const F& f, A&&... a) {
  if (!f) throw FormatError("Missing consumed Vista lobby details service");
  return f(std::forward<A>(a)...);
}
std::uint32_t word(std::span<const std::byte> b, std::size_t at) {
  if (at > b.size() || b.size() - at < 4)
    throw FormatError("Truncated Vista lobby details source field");
  std::uint32_t result{};
  for (unsigned i = 0; i < 4; ++i)
    result |= std::to_integer<std::uint32_t>(b[at + i]) << (i * 8);
  return result;
}
std::int32_t group(std::span<const std::byte> variant) {
  switch (word(variant, 0x44)) {
  case 1: return 22;
  case 2: return 23;
  case 3: return 24;
  case 4: return 25;
  case 7: return 28;
  case 8: return 29;
  case 9: return 30;
  default: return -1;
  }
}
std::uint32_t icon(std::span<const std::byte> variant) {
  switch (word(variant, 0x44)) {
  case 1: return 6;
  case 2: return 0;
  case 3: return 3;
  case 4: return 1;
  case 7: return 4;
  case 8: return 8;
  case 9: return 7;
  default: return 10;
  }
}
std::u16string source_string(std::u16string result) {
  // Source callers initialize 512-unit UTF16 buffers; 44BCB1 checks at
  // most511 units. Preserve bounded, terminated text at this service seam.
  const auto end = result.find(u'\0');
  if (end != std::u16string::npos) result.resize(end);
  if (result.size() > 511) throw FormatError("Unowned Vista localized details buffer");
  return result;
}
}
void vista_lobby_quick_detail(std::span<const std::byte> variant,
    VistaLobbyDetailsObject text, std::int32_t index, std::u16string_view separator,
    const VistaLobbyDetailsServices& s) {
  const auto key = group(variant);
  const auto catalogue = call(s.catalogue);
  const VistaLobbyRuleGroup* selected{};
  if (catalogue)
    for (const auto& candidate : catalogue->groups)
      if (candidate.key == key) { selected = &candidate; break; }
  std::u16string title, value;
  if (selected && index >= 0 && std::size_t(index) < selected->parameters.size()) {
    if (const auto parameter = selected->parameters[std::size_t(index)]) {
      const auto current = call(s.parameter, variant, parameter->parameter);
      const VistaLobbyRuleChoice* choice{};
      for (const auto& candidate : parameter->choices)
        if (candidate.value == current) { choice = &candidate; break; }
      title = source_string(call(s.localize, parameter->locale, parameter->title));
      if (choice) value = source_string(call(s.localize, parameter->locale, choice->string));
    }
  }
  if (text && !title.empty()) {
    const auto result = source_string(call(s.format, std::u16string_view(title), separator,
        std::u16string_view(value)));
    call(s.string, text, std::u16string_view(result));
    call(s.visible, text, std::uint8_t{1});
  }
}
void vista_lobby_details(std::uint32_t screen_id, std::span<const std::byte> variant,
    const VistaLobbyDetailsIndices& indices, const VistaLobbyDetailsServices& s) {
  std::array<VistaLobbyDetailsObject, 5> quick{};
  for (unsigned i = 0; i < quick.size(); ++i) quick[i] = call(s.text, indices.first_quick + i);
  const auto heading = call(s.text, indices.heading);
  const auto name = call(s.text, indices.name);
  const auto bitmap = indices.icon == 0xffffffffu ? 0 : call(s.bitmap, indices.icon);
  const auto status = call(s.text, indices.status);
  if (!variant.empty()) {
    const auto datum = call(s.screen_tag, screen_id);
    const auto screen = datum == 0xffffffffu ? nullptr : call(s.screen_metadata, datum);
    if (heading) call(s.visible, heading, std::uint8_t{1});
    if (name) call(s.string_id, name, 0x0f0001e0u);
    if (bitmap) vista_lobby_details_game_icon(variant, bitmap, s);
    if (screen) {
      std::u16string separator;
      if (screen->locale != 0xffffffffu) {
        std::uint32_t sid{};
        if (indices.value_group >= 0 && std::size_t(indices.value_group) < screen->values.size()) {
          const auto& values = screen->values[std::size_t(indices.value_group)];
          if (indices.value_index >= 0 && std::size_t(indices.value_index) < values.size())
            sid = values[std::size_t(indices.value_index)];
        }
        separator = source_string(call(s.localize, screen->locale, sid));
      }
      for (unsigned i = 0; i < quick.size(); ++i)
        vista_lobby_quick_detail(variant, quick[i], std::int32_t(i), separator, s);
    }
  }
  if (status) call(s.visible, status, call(s.status_visibility));
}
void vista_lobby_details_game_icon(std::span<const std::byte> variant,
    VistaLobbyDetailsObject bitmap, const VistaLobbyDetailsServices& s) {
  const auto frame = icon(variant); // Source reads +44 even for a missing child.
  if (bitmap) {
    call(s.frame, bitmap, frame);
    call(s.visible, bitmap, std::uint8_t{1});
  }
}
bool vista_lobby_map_info(VistaLobbyDetailsObject identifier, VistaLobbyMapInfo& output,
    const VistaLobbyDetailsServices& s) {
  output.fill(std::byte{});
  std::span<const std::byte> record;
  if (!call(s.identifier_record, identifier, record)) return false;
  if (record.size() < 0xa80) throw FormatError("Truncated Vista identifier metadata record");
  std::copy_n(record.begin() + 0x20, 0x40, output.begin() + 4);
  std::copy_n(record.begin() + 0x980, 0x100, output.begin() + 0xb44);
  std::copy_n(record.begin() + 0x968, 0x10, output.begin() + 0xc48);
  if (call(s.identifier_provider))
    std::copy_n(record.begin() + 0x60, 0x900, output.begin() + 0x244);
  return true;
}
void vista_lobby_map_bitmap(const VistaLobbyDetailsServices& s) {
  VistaLobbyDetailsMap map;
  if (!call(s.map_query, map)) return;
  VistaLobbyDetailsObject image{}, bitmap{};
  if (call(s.campaign)) {
    const auto reference = call(s.campaign_definition, map.campaign_id, map.map_id);
    if (!reference) return;
    if (*reference) image = call(s.resolve_image, *reference, 0u);
    bitmap = call(s.bitmap, 2u);
  } else {
    bitmap = call(s.bitmap, 2u);
    if (!map.identifier) {
      const auto reference = call(s.multiplayer_definition, map.map_id);
      if (reference && *reference) image = call(s.resolve_image, *reference, 0u);
    } else {
      VistaLobbyMapInfo info;
      if (vista_lobby_map_info(map.identifier, info, s)) image = word(info, 0);
    }
  }
  if (bitmap) call(s.image, bitmap, image);
}
} // namespace halo2
