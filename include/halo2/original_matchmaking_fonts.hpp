#pragma once

namespace halo2 {
struct RetailFont;

// Finite original DVD font owners used by the recovered TU1.5 matchmaking
// views. Selector6 is the same authored Conduit13 owner as selector4 and is
// exposed separately for the original in-game scoreboard. Callers scope the
// selection to those views; the general MCC font bank is unchanged.
bool original_matchmaking_font_selector(unsigned selector) noexcept;
RetailFont load_original_matchmaking_font(unsigned selector);
} // namespace halo2
