#include "halo2/frontend.hpp"
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
}
int main(int argc, char **argv) try {
  if (argc != 2 || !std::filesystem::exists(std::filesystem::path(argv[1]) / "mainmenu.map"))
    return 77;
  const std::filesystem::path maps = argv[1];
  const Cache cache(maps / "mainmenu.map");
  const auto content = cache.load_frontend();
  MenuState state(content);
  // Keep the reported dialog reproducible even when the native biped
  // admission gate is expanded: map validation uses the same failure path.
  state.accept(); state.accept(); state.text("Overlap Test"); state.commit_text();
  check(state.frame().screen == 38, "profile creation missed customization");
  state.select(1); state.accept(); state.select(0); state.accept();
  state.select(1); state.accept(); state.back(); state.back();
  check(state.frame().screen == 6, "Elite customization missed the main menu");
  state.select(2); state.accept(); state.accept();
  check(state.frame().screen == 14, "Split Screen route missed the lobby");
  state.set_multiplayer_map_validator([](const FrontendLaunch &) {
    throw FormatError("The selected player representation has no admitted native biped path");
  });
  state.select(0); state.accept();
  check(state.frame().screen == 8 && state.frame().description ==
      "The selected player representation has no admitted native biped path",
      "reported Elite Start Game failure was not reproduced");
  Mesh mesh;
  FrontendPainter painter(content, maps / "fonts", mesh);
  const auto &animation = content.animations.at(content.screens.at(8).panes.front().animation - 1);
  for (const float elapsed : {0.f, .125f, .25f}) {
    const auto entering = painter.draw(state, elapsed, -1, 5000);
    const auto expected = std::min(1.f, elapsed * 1000.f / animation.intro.milliseconds);
    check(entering.backdrops.size() == 1 && entering.backdrops.front().fade == expected,
        "error dialog backdrop does not follow the authored entry animation");
  }
  const auto draw = painter.draw(state, 1, -1, 5000);
  check(draw.backdrops.size() == 1,
      "error dialog leaves the lobby's sharp text visible beneath its translucent panel");
  check(draw.backdrops[0].fade == 1,
      "settled error dialog does not obscure the lower UI with the source backdrop");
  check(draw.backdrops[0].before_vertex > 0 &&
        draw.backdrops[0].before_vertex < draw.vertices.size(),
      "error dialog backdrop does not separate lower UI from foreground widgets");
  // Body widgets precede the separately constructed header and subtitle.
  // Preserve the source body clip even when a diagnostic is much longer
  // than this particular biped error; it must never run into Continue.
  std::string diagnostic = std::string(80, 'W') + "\n";
  for (unsigned i = 0; i < 30; ++i)
    diagnostic += "Long diagnostic message with multiple lines.\n";
  MenuState long_state(content);
  long_state.accept(); long_state.accept(); long_state.text("Long Error");
  long_state.commit_text(); long_state.back();
  long_state.select(2); long_state.accept(); long_state.accept();
  long_state.set_multiplayer_map_validator([&](const FrontendLaunch &) {
    throw FormatError(diagnostic);
  });
  long_state.select(0); long_state.accept();
  check(long_state.frame().screen == 8 && long_state.frame().description == diagnostic,
      "long diagnostic did not reach the production unavailable dialog");
  const auto long_draw = painter.draw(long_state, 1, -1, 5000);
  const auto &body = content.screens.at(8).panes.front().texts.front().bounds;
  bool body_glyphs = false;
  for (std::size_t i = long_draw.backdrops.front().before_vertex; i < long_draw.vertices.size(); i += 6) {
    const auto &v = long_draw.vertices[i];
    if (!mesh.materials.at(v.material).name.starts_with("font/")) continue;
    // Once the header begins the source body submission has ended.
    if (v.position.y < 240 - body.top) break;
    for (unsigned j = 0; j < 6; ++j) {
      const auto &p = long_draw.vertices[i+j].position;
      check(p.x >= 320 + body.left && p.x <= 320 + body.right + 1 &&
            p.y >= 240 - body.top && p.y <= 240 - body.bottom + 1,
            "long error text escaped its source clip and overlaps another dialog control");
    }
    body_glyphs = true;
  }
  check(body_glyphs, "long error clip check did not exercise any body glyphs");
  state.accept();
  check(state.frame().screen == 14 && !state.launch,
      "Continue failed to restore the lobby after an unavailable launch");
  check(painter.draw(state, 1, -1, 6000).backdrops.empty(),
      "dismissed error dialog left a backdrop over the lobby");
  std::cout << "Elite Start Game error: lower lobby obscured, foreground separated, Continue restores lobby\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
