#include "halo2/hud.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
HudPresentationLayout hud_presentation_layout(HudViewport reference,
                                             unsigned width, unsigned height) {
  if (!width || !height)
    throw FormatError("Invalid HUD presentation dimensions");
  HudPresentationLayout result;
  result.scale = std::min(width / 640.f, height / 480.f);
  result.offset = {(width - 640 * result.scale) / 2,
                   (height - 480 * result.scale) / 2};
  auto &viewport = result.viewport;
  viewport = reference;
  const auto &old = reference.outer;
  viewport.outer = {(old.top * height / 480.f - result.offset.y) / result.scale,
                    (old.left * width / 640.f - result.offset.x) / result.scale,
                    (old.bottom * height / 480.f - result.offset.y) / result.scale,
                    (old.right * width / 640.f - result.offset.x) / result.scale};
  viewport.safe = {viewport.outer.top + reference.safe.top - old.top,
                   viewport.outer.left + reference.safe.left - old.left,
                   viewport.outer.bottom + reference.safe.bottom - old.bottom,
                   viewport.outer.right + reference.safe.right - old.right};
  result.center = {std::floor((viewport.safe.left + viewport.safe.right) * .5f),
                   std::floor((viewport.safe.top + viewport.safe.bottom) * .5f)};
  // Desktop displays need no television overscan. Give every local player's
  // four corners the same small inset, including edges along split seams.
  constexpr float inset = 8;
  viewport.safe = {viewport.outer.top + inset, viewport.outer.left + inset,
                   viewport.outer.bottom - inset, viewport.outer.right - inset};
  return result;
}
} // namespace halo2
