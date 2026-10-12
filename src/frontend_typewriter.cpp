#include "halo2/frontend_typewriter.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::u16string_view terminated(std::u16string_view text) {
  const auto end = text.find(u'\0');
  return end == text.npos ? text : text.substr(0, end);
}
} // namespace

void frontend_typewriter_set_text(FrontendTypewriterState &state,
                                  std::u16string_view text) {
  state.length = std::bit_cast<std::int16_t>(
      static_cast<std::uint16_t>(terminated(text).size()));
  if (state.cursor >= 0)
    state.cursor = 0;
}

void frontend_typewriter_begin(FrontendTypewriterState &state) {
  state.cursor = 0;
}

std::u16string frontend_typewriter_draw(FrontendTypewriterState &state,
    std::u16string_view supplied, std::uint32_t clock, std::int16_t interval_ms,
    std::uint16_t cursor_glyph, bool nonzero_bounds) {
  const auto text = terminated(supplied);
  if (text.empty() || !nonzero_bounds)
    return {};
  // 13FAC0 copies at most 1023 code units into the original 1024-unit scratch.
  std::u16string displayed(text.substr(0, 1023));
  if (state.cursor < 0)
    return displayed;
  const auto cursor = std::min<std::int16_t>(state.cursor, 1022);
  displayed.resize(static_cast<std::size_t>(cursor) + 1, u'\0');
  displayed[static_cast<std::size_t>(cursor)] = static_cast<char16_t>(cursor_glyph);
  // An earlier input NUL remains earlier than an out-of-range cursor marker.
  displayed.resize(terminated(displayed).size());
  state.cursor = cursor;
  if (state.last_clock == 0) {
    state.last_clock = clock;
  } else if (clock - state.last_clock >=
             static_cast<std::uint32_t>(static_cast<std::int32_t>(interval_ms))) {
    state.last_clock = clock;
    state.cursor = static_cast<std::int16_t>(cursor + 1);
    if (state.cursor == state.length)
      state.cursor = -1;
  }
  return displayed;
}
} // namespace halo2
