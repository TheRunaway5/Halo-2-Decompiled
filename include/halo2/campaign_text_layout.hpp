#pragma once
#include "halo2/frontend.hpp"
#include <string>
#include <vector>
namespace halo2::campaign_text_layout {
struct Escape {unsigned codepoint;bool found;};
// TU1.5 1426D0's six recognized output records. Unknown diagnostic/timer
// ownership is separate; unknown markers retain the literal bar and suffix.
constexpr Escape escape(unsigned c) {
  switch(c) {
  case '|':return {'|',true};case 'l':return {0xe405,true};
  case 'r':return {0xe406,true};case 'c':return {0xe407,true};
  case 'n':return {13,true};case 't':return {9,true};
  default:return {'|',false};
  }
}
inline std::vector<unsigned> characters(const std::string &s) {
  std::vector<unsigned> out;
  for (std::size_t i = 0; i < s.size() && out.size() < 255;) {
    const auto b = std::uint8_t(s[i++]);
    unsigned c = b, n = 0;
    if ((b & 0xe0) == 0xc0) {c = b & 31;n = 1;}
    else if ((b & 0xf0) == 0xe0) {c = b & 15;n = 2;}
    else if ((b & 0xf8) == 0xf0) {c = b & 7;n = 3;}
    if (i + n > s.size())break;
    while (n--)c = (c << 6) | (std::uint8_t(s[i++]) & 63);
    out.push_back(c);
  }
  std::vector<unsigned> decoded;decoded.reserve(out.size());
  for(std::size_t i=0;i<out.size();++i) {
    if(out[i]=='|') {const auto e=escape(i+1<out.size()?out[i+1]:0);
      decoded.push_back(e.codepoint);if(e.found)++i;}
    else decoded.push_back(out[i]);
  }
  return decoded;
}
struct Line {std::vector<unsigned> text;float width{};};
inline std::vector<Line> lines(const RetailFont &font,const std::string &text,float maximum) {
  std::vector<Line> result(1);unsigned previous = 0;
  for (auto c : characters(text)) {
    // The original token iterator ignores LF; CR is the newline token.
    if (c == '\n')continue;
    if (c == '\r') {result.emplace_back();previous = 0;continue;}
    const auto at = font.glyphs.find(c);if(at == font.glyphs.end())continue;
    float kern = 0;
    if (const auto k = font.kerning.find({c, previous});k != font.kerning.end())kern = k->second;
    const float advance = at->second.advance + at->second.bearing_x;
    if (result.back().width + kern + advance > maximum && !result.back().text.empty()) {
      result.emplace_back();kern = 0;
    }
    result.back().text.push_back(c);result.back().width += kern + advance;previous = c;
  }
  return result;
}
inline float center(int justification,float left,float right,float width) {
  return justification==0?left+width*.5f:justification==1?right-width*.5f:(left+right)*.5f;
}
inline Vec3 title_color(unsigned color) {
  // TU1.5 13F40B..13F4AC limits an all-white title to original456910 (0.8).
  // With eight-bit source channels, only255/255/255 passes its0.0001 test.
  if((color&0xffffffu)==0xffffffu)return {.8f,.8f,.8f};
  return {float((color>>16)&255)/255,float((color>>8)&255)/255,float(color&255)/255};
}
}
