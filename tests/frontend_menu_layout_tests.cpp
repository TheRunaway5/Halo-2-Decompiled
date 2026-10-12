#include "halo2/frontend.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace halo2;
int main(int argc, char **argv) try {
  if (argc < 3 || argc > 4 ||
      !std::filesystem::exists(std::filesystem::path(argv[1]) / "mainmenu.map") ||
      !std::filesystem::exists(argv[2]) ||
      (argc == 4 && !std::filesystem::is_directory(argv[3]))) return 77;
  const std::filesystem::path maps = argv[1];
  const Cache cache(maps / "mainmenu.map");
  auto content = cache.load_frontend();
  content.vista = load_vista_settings(argv[2]);
  const std::filesystem::path pack = argc == 4 ? argv[3] : "";
  Mesh mesh;
  FrontendPainter painter(content, maps / "fonts", mesh, maps / "ui/frame_edges",
      load_frontend_ui_assets(content, pack), pack.empty() ? pack : pack / "fonts");
  unsigned cases{}, dialogs{}, suppressed{}, glyph_vertices{};
  auto verify = [&](MenuState &state, int pane) {
    const auto id = state.frame().screen;
    const auto &definition = content.screen(id);
    const auto draw = painter.draw(state, 5, pane, 10000);
    auto check = [&](bool condition, const char *message) {
      if (!condition) throw std::runtime_error("screen " + std::to_string(id) +
          " pane " + std::to_string(pane) + " selection " +
          std::to_string(state.frame().selected) + ": " + message);
    };
    const bool dialog = (definition.flags & 25) && state.stack().size() > 1;
    if (dialog && !(definition.flags & 32)) {
      check(draw.backdrops.size() == 1 && draw.backdrops.front().fade == 1,
          "lower menu is left sharp beneath a translucent dialog");
      const auto boundary = draw.backdrops.front().before_vertex;
      check(boundary > 0 && boundary < draw.vertices.size() && boundary % 6 == 0,
          "backdrop does not separate complete lower and foreground widgets");
      ++dialogs;
    } else {
      check(draw.backdrops.empty(), "source backdrop suppression was ignored");
      suppressed += dialog;
    }
    for (const auto &v : draw.vertices) {
      check(std::isfinite(v.position.x) && std::isfinite(v.position.y),
          "invalid widget coordinate");
      const auto &material = mesh.materials.at(v.material);
      check(material.name != "ui/background-dim",
          "generic black dimmer replaced the authored dialog backdrop");
      if (!material.name.starts_with("font/")) continue;
      check(v.position.x >= 0 && v.position.x <= 640 &&
            v.position.y >= 0 && v.position.y <= 480,
          "text escapes the authored foreground canvas");
      ++glyph_vertices;
    }
    ++cases;
  };
  auto screens = [&](const FrontendContent &source, bool vista) {
    for (const auto &[id, definition] : source.screens) {
      // Vista5 is shared shell art; Vista38 supplies the PC Controls pane
      // inside the Xbox profile page. Neither is a standalone PC route.
      // PC16/185 require retained result owners. Their twelve actual-source
      // panes are covered by frontend_vista_pcr_painter_tests instead.
      // PC209 is retained transition data, not a MenuState navigation route.
      if (vista && (id == 5 || id == 38 || id == 16 || id == 185 || id == 209)) continue;
      MenuState sample(content);
      if (id != 9 || vista) {
        try { sample.open(vista ? vista_menu(id) : id); }
        catch (const std::exception &error) {
          throw std::runtime_error("opening screen " + std::to_string(id) +
              (vista ? " (Vista): " : ": ") + error.what());
        }
      }
      for (unsigned pane = 0; pane < std::max(1u, unsigned(definition.panes.size())); ++pane)
        for (unsigned selected = 0; selected < std::max(1u, unsigned(sample.frame().entries.size())); ++selected) {
          sample.select(selected);
          verify(sample, int(pane));
        }
    }
  };
  screens(content, false);
  screens(*content.vista, true);
  for (const auto &[category, params] : content.categories) {
    MenuState sample(content);
    sample.open(221, category);
    for (unsigned i = 0; i < sample.frame().entries.size(); ++i) {
      sample.select(i);
      verify(sample, -1);
    }
  }
  for (const auto &[id, parameter] : content.parameters) {
    MenuState sample(content);
    sample.open(223, id);
    for (unsigned i = 0; i < sample.frame().entries.size(); ++i) {
      sample.select(i);
      verify(sample, -1);
    }
  }
  std::cout << cases << " menu pane/selection/context draws; " << dialogs
            << " isolated dialogs; " << suppressed << " source-suppressed dialogs; "
            << glyph_vertices << " bounded text vertices; art="
            << (pack.empty() ? "stock" : "Vista") << '\n';
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
