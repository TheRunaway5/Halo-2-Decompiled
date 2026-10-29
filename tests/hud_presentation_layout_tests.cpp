#include "halo2/hud.hpp"
#include <cmath>
#include <iostream>

using namespace halo2;
namespace {
void check(bool condition, const char *message) {
  if (!condition)
    throw FormatError(message);
}
void close(float a, float b, const char *message) {
  check(std::abs(a - b) < .001f, message);
}
} // namespace

int main() try {
  // Reference cells include the exterior Xbox overscan and four-pixel split
  // gutters. Test physical corner distances independently of those margins.
  const std::array references{
      HudViewport{{0, 0, 480, 640}, {36, 48, 444, 592}, 0},
      HudViewport{{0, 0, 240, 640}, {36, 48, 236, 592}, 1},
      HudViewport{{240, 0, 480, 640}, {244, 48, 444, 592}, 1},
      HudViewport{{0, 0, 480, 320}, {36, 48, 444, 316}, 1},
      HudViewport{{0, 320, 480, 640}, {36, 324, 444, 592}, 1},
      HudViewport{{0, 0, 240, 320}, {36, 48, 236, 316}, 2},
      HudViewport{{0, 320, 240, 640}, {36, 324, 236, 592}, 2},
      HudViewport{{240, 0, 480, 320}, {244, 48, 444, 316}, 2},
      HudViewport{{240, 320, 480, 640}, {244, 324, 444, 592}, 2}};
  unsigned cases{};
  for (const auto dimensions : {std::array{640u, 480u}, std::array{1280u, 720u},
                               std::array{3840u, 2160u}, std::array{2560u, 720u},
                               std::array{853u, 479u}, std::array{720u, 1280u}}) {
    for (const auto &reference : references) {
      const auto layout = hud_presentation_layout(reference, dimensions[0], dimensions[1]);
      const auto &v = layout.viewport;
      auto x = [&](float value) { return value * layout.scale + layout.offset.x; };
      auto y = [&](float value) { return value * layout.scale + layout.offset.y; };
      close(x(v.outer.left), reference.outer.left * dimensions[0] / 640,
            "HUD cell left no longer reaches its physical viewport");
      close(y(v.outer.top), reference.outer.top * dimensions[1] / 480,
            "HUD cell top no longer reaches its physical viewport");
      close(x(v.outer.right), reference.outer.right * dimensions[0] / 640,
            "HUD cell right no longer reaches its physical viewport");
      close(y(v.outer.bottom), reference.outer.bottom * dimensions[1] / 480,
            "HUD cell bottom no longer reaches its physical viewport");
      close(x(v.safe.left) - x(v.outer.left), 8 * layout.scale,
            "HUD left corner still uses the Xbox overscan inset");
      close(y(v.safe.top) - y(v.outer.top), 8 * layout.scale,
            "HUD top corner still uses the Xbox overscan inset");
      close(x(v.outer.right) - x(v.safe.right), 8 * layout.scale,
            "HUD right corner still uses the Xbox overscan inset");
      close(y(v.outer.bottom) - y(v.safe.bottom), 8 * layout.scale,
            "HUD bottom corner still uses the Xbox overscan inset");
      // The source center is deliberately asymmetric in split-screen.
      // Preserve its existing floor operation in adapted HUD coordinates.
      const float old_left = v.outer.left + reference.safe.left - reference.outer.left,
                  old_right = v.outer.right + reference.safe.right - reference.outer.right,
                  old_top = v.outer.top + reference.safe.top - reference.outer.top,
                  old_bottom = v.outer.bottom + reference.safe.bottom - reference.outer.bottom;
      close(layout.center.x, std::floor((old_left + old_right) * .5f),
            "Corner layout moved the source horizontal aim center");
      close(layout.center.y, std::floor((old_top + old_bottom) * .5f),
            "Corner layout moved the source vertical aim center");
      check(v.mode == reference.mode, "Corner layout changed an authored sprite slot");
      ++cases;
    }
  }
  for (const auto dimensions : {std::array{0u, 480u}, std::array{640u, 0u}}) {
    bool rejected{};
    try { (void)hud_presentation_layout(references[0], dimensions[0], dimensions[1]); }
    catch (const FormatError &) { rejected = true; }
    check(rejected, "Zero-size HUD presentation was accepted");
  }
  std::cout << cases << " full, half and quarter HUD corner layouts passed\n";
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
