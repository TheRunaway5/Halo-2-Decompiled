#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
// Literal executable data supplied by the selected-image importer. Token text
// and glyph callback tables are not embedded in this module.
struct FullScoreboardNameToken { std::u16string text; char16_t glyph{}; };
struct FullScoreboardNameGlyph { std::uint32_t glyph{}, original_handler{}; };
struct FullScoreboardNameMappings {
  std::vector<FullScoreboardNameToken> tokens;
  std::vector<FullScoreboardNameGlyph> glyphs;
};
enum class FullScoreboardNameStatus { Ready, InvalidMappings, UnavailableGlyphCallback };
struct FullScoreboardNameResult {
  FullScoreboardNameStatus status{FullScoreboardNameStatus::InvalidMappings};
  std::u16string text;
  std::uint32_t unavailable_handler{};
};
// An optional owning callback can deliver the actual original dynamic glyph
// replacement. Returning false preserves the explicit unavailable boundary.
using FullScoreboardGlyphReplacement = bool (*)(std::uint32_t original_handler,
    char16_t glyph, std::u16string& replacement);
// Source162E70 copies at most255 UTF16 units, then233CF1 sanitizes with capacity
//256. Widget truncation to19 units remains in full_scoreboard_layout.
FullScoreboardNameResult full_scoreboard_sanitize_name(std::u16string_view,
    const FullScoreboardNameMappings&, FullScoreboardGlyphReplacement = nullptr);

using FullScoreboardColorBits = std::array<std::uint32_t,3>;
struct FullScoreboardPalette {
  bool scenario_present{}; // original loaded-scenario pointer, not a ready flag
  std::span<const FullScoreboardColorBits> colors; // literal matg+160/+164
  FullScoreboardColorBits fallback; // literal [474C70] pointed-to RGB
};
// Complete original1AA2B0/ordinary830C0. Four signed profile bytes are explicit;
// no slot-derived color, team color or membership-NONE override is inferred.
FullScoreboardColorBits full_scoreboard_palette_color(std::int8_t,
    const FullScoreboardPalette&);
std::array<FullScoreboardColorBits,4> full_scoreboard_ffa_colors(
    const std::array<std::uint8_t,4>& appearance, const FullScoreboardPalette&);
// Complete original1AA360 constructor owns these four appearance bytes. Saved
// profile/preferences and their copy to player+84 remain caller-owned.
std::array<std::uint8_t,4> full_scoreboard_default_appearance();
} // namespace halo2
