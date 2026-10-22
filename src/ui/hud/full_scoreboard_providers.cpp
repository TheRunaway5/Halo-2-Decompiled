#include "halo2/full_scoreboard_providers.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
FullScoreboardNameResult full_scoreboard_sanitize_name(std::u16string_view input,
    const FullScoreboardNameMappings& m, FullScoreboardGlyphReplacement callback) {
  FullScoreboardNameResult out;
  if (m.tokens.size()!=18 || m.glyphs.size()!=131) return out;
  for (const auto& t:m.tokens)
    if (t.text.empty() || t.text.size()>255 || t.text.find(u'\0')!=std::u16string::npos)
      return out;
  out.text=input.substr(0,std::min<std::size_t>(input.find(u'\0'),255));
  for (std::size_t at=0;at<out.text.size();++at) {
    const auto ch=out.text[at];
    std::u16string replacement;
    std::size_t consumed=0;
    if (ch>=0xe000 && ch<=0xf8ff) {
      const auto glyph=std::find_if(m.glyphs.begin(),m.glyphs.end(),
          [=](const auto& g){return g.glyph==ch;});
      if (glyph!=m.glyphs.end() && glyph->original_handler) {
        if (!callback || !callback(glyph->original_handler,ch,replacement)) {
          out.status=FullScoreboardNameStatus::UnavailableGlyphCallback;
          out.unavailable_handler=glyph->original_handler;
          return out;
        }
        replacement.resize(std::min<std::size_t>(replacement.find(u'\0'),255));
        consumed=1;
      }
    }
    if (!consumed) {
      for (const auto& token:m.tokens)
        if (std::u16string_view(out.text).substr(at).starts_with(token.text)) {
          if (replacement.size()<255) replacement+=token.glyph;
          consumed=token.text.size();
        }
    }
    // Original strict predicted-length <256 gate; the cursor still advances
    // one code unit after a rejected or successful replacement.
    const auto predicted=out.text.size()+replacement.size()-consumed;
    if (predicted<256) out.text.replace(at,consumed,replacement);
  }
  out.status=FullScoreboardNameStatus::Ready;
  return out;
}
FullScoreboardColorBits full_scoreboard_palette_color(std::int8_t index,
    const FullScoreboardPalette& p) {
  if (!p.scenario_present || index<0 || std::size_t(index)>=p.colors.size())
    return p.fallback;
  return p.colors[std::size_t(index)];
}
std::array<FullScoreboardColorBits,4> full_scoreboard_ffa_colors(
    const std::array<std::uint8_t,4>& a,const FullScoreboardPalette& p) {
  std::array<FullScoreboardColorBits,4> out;
  for (unsigned i=0;i<4;++i)
    out[i]=full_scoreboard_palette_color(std::bit_cast<std::int8_t>(a[i]),p);
  return out;
}
std::array<std::uint8_t,4> full_scoreboard_default_appearance() {
  return {10,0,0,10}; // original1AA36E/374/37A/380 stores
}
} // namespace halo2
