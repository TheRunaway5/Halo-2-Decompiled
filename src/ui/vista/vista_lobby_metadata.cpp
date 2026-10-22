#include "halo2/vista_lobby_metadata.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
std::u16string vista_lobby_metadata_utf16(std::string_view text) {
  std::u16string out;
  for (std::size_t i = 0; i < text.size();) {
    const auto lead = static_cast<unsigned char>(text[i++]);
    std::uint32_t code{}; unsigned extra{};
    if (lead < 0x80) code = lead;
    else if (lead >= 0xc2 && lead <= 0xdf) { code = lead & 0x1f; extra = 1; }
    else if (lead >= 0xe0 && lead <= 0xef) { code = lead & 0x0f; extra = 2; }
    else if (lead >= 0xf0 && lead <= 0xf4) { code = lead & 7; extra = 3; }
    else throw FormatError("invalid Vista source locale UTF8 lead");
    if (extra > text.size() - i) throw FormatError("truncated Vista source locale UTF8");
    for (unsigned j = 0; j < extra; ++j) {
      const auto value = static_cast<unsigned char>(text[i++]);
      if ((value & 0xc0) != 0x80) throw FormatError("invalid Vista source locale UTF8 continuation");
      code = (code << 6) | (value & 0x3f);
    }
    if ((extra == 1 && code < 0x80) || (extra == 2 && code < 0x800) ||
        (extra == 3 && code < 0x10000) || code > 0x10ffff ||
        (code >= 0xd800 && code <= 0xdfff))
      throw FormatError("invalid Vista source locale UTF8 scalar");
    if (code < 0x10000) out.push_back(static_cast<char16_t>(code));
    else {
      code -= 0x10000;
      out.push_back(static_cast<char16_t>(0xd800 + (code >> 10)));
      out.push_back(static_cast<char16_t>(0xdc00 + (code & 0x3ff)));
    }
  }
  return out;
}
const VistaLobbyMetadataMap* VistaLobbyMetadata::campaign_definition(std::int32_t id) const {
  const auto found = std::find_if(campaign.begin(), campaign.end(),
      [id](const auto& record) { return record.map_id == id; });
  return found == campaign.end() ? nullptr : &*found;
}
const VistaLobbyMetadataMap* VistaLobbyMetadata::multiplayer_definition(std::int32_t id) const {
  const auto found = std::find_if(multiplayer.begin(), multiplayer.end(),
      [id](const auto& record) { return record.map_id == id; });
  return found == multiplayer.end() ? nullptr : &*found;
}
const VistaLobbyMetadataPath* VistaLobbyMetadata::campaign_path(std::int32_t campaign_id,
    std::int32_t map_id) const {
  const auto found = std::find_if(paths.begin(), paths.end(), [&](const auto& record) {
    return record.campaign_id == campaign_id && record.map_id == map_id;
  });
  return found == paths.end() ? nullptr : &*found;
}
std::u16string VistaLobbyMetadata::localize(std::uint32_t owner, std::uint32_t string) const {
  const auto range = locales.find(owner);
  if (range == locales.end()) return {};
  const auto first = range->second.first, count = range->second.count;
  if (first > locale_entries.size() || count > locale_entries.size() - first)
    throw FormatError("retained Vista locale range outside source bank");
  //43DEFD searches the owning range in order and returns literal79BAD4
  // (the empty UTF8 string) on a miss. It never searches another unic.
  for (std::size_t i = first; i < std::size_t(first) + count; ++i)
    if (locale_entries[i].string == string) return locale_entries[i].utf16;
  return {};
}
VistaLobbyRuleCatalogue VistaLobbyMetadata::catalogue() const {
  VistaLobbyRuleCatalogue result;
  for (const auto& source : groups) {
    VistaLobbyRuleGroup group; group.key = source.key;
    for (const auto datum : source.parameters) {
      const auto found = parameters.find(datum);
      group.parameters.push_back(found == parameters.end() ? nullptr : &found->second.details);
    }
    result.groups.push_back(std::move(group));
  }
  return result;
}
VistaLobbyDetailsServices vista_lobby_metadata_services(
    std::shared_ptr<const VistaLobbyMetadata> bank) {
  if (!bank) throw FormatError("Vista lobby metadata adapter requires its retained PC bank");
  VistaLobbyDetailsServices result;
  auto catalogue = std::make_shared<const VistaLobbyRuleCatalogue>(bank->catalogue());
  result.catalogue = [bank, catalogue]() -> const VistaLobbyRuleCatalogue* {
    return bank->game_settings.datum == 0xffffffffu ? nullptr : catalogue.get();
  };
  result.screen_metadata = [bank](std::uint32_t datum) -> const VistaLobbyDetailsScreen* {
    const auto found = bank->screens.find(datum);
    return found == bank->screens.end() ? nullptr : &found->second.details;
  };
  result.localize = [bank](std::uint32_t owner, std::uint32_t string) {
    return bank->localize(owner, string);
  };
  result.campaign_definition = [bank](std::int32_t, std::int32_t map_id)
      -> std::optional<std::uint32_t> {
    const auto* record = bank->campaign_definition(map_id);
    return record ? std::optional(record->bitmap) : std::nullopt;
  };
  result.multiplayer_definition = [bank](std::int32_t map_id)
      -> std::optional<std::uint32_t> {
    const auto* record = bank->multiplayer_definition(map_id);
    return record ? std::optional(record->bitmap) : std::nullopt;
  };
  return result;
}
} // namespace halo2
