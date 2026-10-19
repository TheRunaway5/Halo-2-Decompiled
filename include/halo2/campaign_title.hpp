#pragma once
#include "halo2/cache.hpp"
#include "halo2/player_training.hpp"
#include <bit>
#include <cmath>
#include <string_view>

namespace halo2 {
// Decode the complete scnr title bank at both content and text-loading seams.
inline std::vector<CampaignTitleDefinition> read_campaign_titles(
    std::span<const std::byte> rows, bool mcc, std::string_view scenario) {
  if (rows.size() % 0x24)
    throw FormatError("Incomplete cinematic title bank");
  const auto word = [&](std::size_t p, unsigned bytes) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < bytes; ++i)
      value |= std::to_integer<unsigned>(rows[p + i]) << (8 * i);
    return value;
  };
  const auto real = [&](std::size_t p) {
    const auto value = std::bit_cast<float>(word(p, 4));
    if (!std::isfinite(value) || std::abs(value) > 1e7f)
      throw FormatError("Invalid cinematic title float");
    return value;
  };
  std::vector<CampaignTitleDefinition> result;
  for (std::size_t p = 0; p < rows.size(); p += 0x24) {
    CampaignTitleDefinition title;
    title.text = word(p, 4);
    title.color = word(p + 0x10, 4);
    title.shadow = word(p + 0x14, 4);
    for (unsigned i = 0; i < 4; ++i)
      title.bounds[i] = std::int16_t(word(p + 4 + i * 2, 2));
    title.justification = std::int16_t(word(p + 0xc, 2));
    title.font = std::int16_t(word(p + 0xe, 2));
    title.fade_in = real(p + 0x18);
    title.up_time = real(p + 0x1c);
    title.fade_out = real(p + 0x20);
    result.push_back(title);
  }
  // The selected MCC retail opening authors different title coordinates.
  // Using them directly as Xbox rect16 edges gives width -944.
  // Match the entire two-record donor bank before projecting logical bounds,
  // like the original-metric projection for MCC font rasters. Keep source
  // bytes, MCC string IDs, font, colors, alignment and timing unchanged.
  // Reference: original 00a SHA256 5093e2f8...561c02a28, titles1000087B/C;
  // MCC 00a SHA256 f6916833...4329b8, titles100009A2/3. Full evidence and
  // byte records: tests/fixtures/campaign_opening_titles_{mcc,xbox}.tsv.
  constexpr std::array<std::uint32_t, 18> opening_mcc{
      0x100009a2u, 0x040000e1u, 0x00500028u, 0x00020001u,
      0x00ffffffu, 0u, 0x3f800000u, 0x40a00000u, 0x3f800000u,
      0x100009a3u, 0x040000e1u, 0x00500028u, 0x00020001u,
      0x00ffffffu, 0u, 0x3f800000u, 0x40a00000u, 0x3f800000u};
  bool opening = mcc &&
      scenario == "scenarios\\solo\\00a_introduction\\00a_introduction" &&
      rows.size() == opening_mcc.size() * 4;
  if (opening)
    for (std::size_t i = 0; i < opening_mcc.size(); ++i)
      if (word(i * 4, 4) != opening_mcc[i]) {opening = false;break;}
  if (opening)
    for (auto &title : result)
      title.bounds = {350, 0, 480, 595};
  return result;
}
} // namespace halo2
