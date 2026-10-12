#pragma once
#include "halo2/frontend.hpp"
#include "halo2/full_scoreboard.hpp"
#include "halo2/full_scoreboard_paint_constants.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <functional>

namespace halo2 {
struct FullScoreboardPaintFrame {
  FullScoreboardRect clip;
  FullScoreboardClock clock;
  // Source font header+0A and text globals4F3CCE/4F3CD0. These are explicit
  // state inputs, not an inferred menu wrapper or font-startup claim.
  std::int16_t font_horizontal_offset{}, first_line_indent{}, next_line_indent{};
  bool shadow_enabled{};
  std::uint16_t x87_control_word{};
};
using FullScoreboardPulse = std::optional<float> (*)(FullScoreboardClock,
    float phase, float one, float half, float alpha, std::uint16_t control_word);
using FullScoreboardEmblem = std::function<std::optional<unsigned>(const FullScoreboardRow &)>;
// On x86 this executes the original x87 expression, including FSIN, under the
// supplied masked control word and restores the host control word afterwards.
// Other hosts report the unavailable math provider instead of substituting sin.
std::optional<float> full_scoreboard_host_pulse(FullScoreboardClock,
    float phase, float one, float half, float alpha, std::uint16_t control_word);

enum class FullScoreboardPaintStatus { Ready, Partial, InvalidInput };
struct FullScoreboardPaintResult {
  FullScoreboardPaintStatus status{FullScoreboardPaintStatus::InvalidInput};
  UiDraw draw;
  bool emblems_unavailable{}, connection_quality_unavailable{}, pulse_unavailable{};
  unsigned text_controls_unavailable{};
};
class FullScoreboardPainter {
public:
  // The caller retains actual font_table selector6 (conduit-13), its atlas
  // material, and an untextured rectangle material for this painter's lifetime.
  FullScoreboardPainter(const RetailFont &font6, unsigned font_material,
                        unsigned rectangle_material);
  FullScoreboardPaintResult draw(const FullScoreboardLayout &,
      const FullScoreboardPaintConstants &, const FullScoreboardPaintFrame &,
      FullScoreboardPulse = full_scoreboard_host_pulse,
      const FullScoreboardEmblem & = {}) const;
private:
  const RetailFont &font6_;
  unsigned font_material_, rectangle_material_;
};
} // namespace halo2
