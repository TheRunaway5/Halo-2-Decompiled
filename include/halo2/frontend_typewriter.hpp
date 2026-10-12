#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace halo2 {
// Original c_class_22cc8e constructor fields +3C/+3E/+40. The caller owns
// widget lifetime and the already copied UTF-16 buffer (including its capacity).
struct FrontendTypewriterState {
  std::int16_t cursor{-1};
  std::int16_t length{};
  std::uint32_t last_clock{};
};

// 22F4DB: strlen of the already copied buffer; only an active cursor restarts.
// Neither this operation nor flag8 setup clears the retained clock.
void frontend_typewriter_set_text(FrontendTypewriterState &, std::u16string_view);
// 25387D: the standard-text flag8 setup activates the cursor after set_text.
void frontend_typewriter_begin(FrontendTypewriterState &);
// 22CD48..22CE32. Returns this draw's prefix BEFORE advancing the state. Clock
// is the cumulative UI clock, not elapsed screen time. A draw advances at most
// once. The source requires unequal X and Y bounds, even if either is reversed.
// Call only for source-admitted widgets, before alpha/offscreen rejection.
std::u16string frontend_typewriter_draw(FrontendTypewriterState &,
    std::u16string_view, std::uint32_t clock, std::int16_t interval_ms,
    std::uint16_t cursor_glyph, bool nonzero_bounds = true);
} // namespace halo2
