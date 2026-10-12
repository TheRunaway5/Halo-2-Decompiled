#include "halo2/frontend.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("maps argument required");
    const std::filesystem::path maps(argv[1]);
    if (!std::filesystem::exists(maps / "mainmenu.map")) return 77;
    Cache cache(maps / "mainmenu.map");
    auto content = cache.load_frontend();
    MenuState state(content);
    state.accept(); state.accept(); state.text("Lobby Test"); state.commit_text(); state.back();
    state.select(2); state.accept(); state.accept();
    if (state.frame().screen != 14 || state.signed_in_profile() != "Lobby Test")
      throw std::runtime_error("normal Split Screen route missed signed-in lobby");
    Mesh mesh;
    FrontendPainter painter(content, maps / "fonts", mesh);
    unsigned failures = 0;
    auto check = [&](bool pass, const char *message) {
      std::cout << (pass ? "PASS " : "FAIL ") << message << '\n';
      failures += !pass;
    };
    auto material_quads = [&](const UiDraw &draw, const std::string &name) {
      unsigned count = 0;
      for (std::size_t i = 0; i < draw.vertices.size(); i += 6)
        count += mesh.materials.at(draw.vertices[i].material).name == name;
      return count;
    };
    const auto draw = painter.draw(state, 10);
    check(draw.scene_color_filter,
          "normal lobby retains the original mainmenu world color filter");
    const std::string path = "ui\\screens\\game_shell\\pregame_lobby\\";
    // Original25042D binds one occupied row, and hides every unused speaker.
    check(material_quads(draw, path + "speaker") <= 1,
          "unused lobby speaker slots are hidden");
    // Original250332 reads the selected map's matg descriptor, not unknown_map.
    check(material_quads(draw, path + "unknown_map") == 0,
          "selected Lockout preview replaces the unknown-map placeholder");
    check(material_quads(draw, path + "item_background") == 1,
          "one signed-in player produces one occupied roster row");
    // Original24FF0A chooses by session type, independent of button selection.
    state.select(1);
    const auto setup = painter.draw(state, 10);
    check(material_quads(draw, path + "mp_games2") == 1 &&
          material_quads(setup, path + "mp_games2") == 1,
          "Game Setup focus keeps the multiplayer pane");
    state.accept();
    check(state.frame().screen == 23 && state.frame().entries.size() == 4,
          "Split Screen Game Setup has the original four choices");
    const std::array<std::uint32_t, 4> setup_labels{
        0x0a0005d3, 0x0e0005d4, 0x0d00042a, 0x0e0005db};
    bool ordered = state.frame().entries.size() == setup_labels.size();
    for (unsigned i = 0; ordered && i < setup_labels.size(); ++i)
      ordered = state.frame().entries[i].label == content.localized(content.screens.at(23), setup_labels[i]);
    check(ordered, "Game Setup choices retain original order and localized labels");
    check(painter.draw(state, 10).backdrops.size() == 1,
          "Game Setup applies the original backdrop instead of a flat black overlay");
    bool setup_fade = true;
    for (const auto elapsed : {0.f, .125f, .25f}) {
      const auto transition = painter.draw(state, elapsed);
      setup_fade &= transition.backdrops.size() == 1 &&
          transition.backdrops[0].fade == elapsed * 4;
    }
    check(setup_fade, "Game Setup backdrop follows authored 250ms transition");
    state.accept();
    check(state.frame().screen == 206 && state.frame().entries.size() == 11 &&
              state.frame().entries[3].label == "Ivory Tower",
          "new-profile map picker uses original order and unlock admission");
    state.select(3);
    const auto focused = state.focused_lobby_map(state.frame());
    const auto preview = painter.draw(state, 10);
    check(preview.backdrops.size() == 1,
          "map picker retains the original popup backdrop");
    bool replacement_fade = true;
    for (const auto elapsed : {0.f, .125f, .25f}) {
      const auto transition = painter.draw(state, elapsed);
      replacement_fade &= transition.backdrops.size() == 1 &&
          transition.backdrops[0].fade == 1;
    }
    check(replacement_fade, "replacing Game Setup preserves full backdrop strength");
    check(focused && focused->name == "Ivory Tower" && !focused->description.empty() &&
              material_quads(preview, content.images.at(focused->bitmap).name) == 1,
          "map focus binds the actual preview and description before acceptance");
    state.back();
    check(state.frame().screen == 14 && state.lobby_view()->map->name == "Lockout",
          "map cancel reveals lobby and retains the accepted map");
    state.select(1); state.accept(); state.accept(); state.select(3); state.accept();
    check(state.frame().screen == 14 && state.lobby_view()->map->name == "Ivory Tower",
          "map accept returns to lobby with the selected map");
    state.select(1); state.accept(); state.select(3); state.accept();
    check(state.frame().screen == 14 && state.lobby_view()->input.session_enum == 0 &&
              state.lobby_view()->map->map_id == 105 &&
              state.lobby_view()->input.status == 7 && state.frame().selected == 0,
          "co-op switch selects the original no-save campaign lobby");
    state.accept();
    check(state.frame().screen == 14 && !state.launch && state.frame().selected == 0,
          "one-player co-op retains Start focus but ignores activation until ready");
    state.select(1); state.accept(); state.select(1); state.accept();
    check(state.frame().screen == 12 && state.frame().context == 1,
          "lobby difficulty uses the alternate original selector");
    state.select(2); state.accept();
    check(state.frame().screen == 14 && !state.launch && state.lobby_view()->difficulty == 2,
          "lobby difficulty accepts without starting a campaign");
    state.select(1); state.accept(); state.select(2); state.accept();
    check(state.frame().screen == 14 && state.lobby_view()->input.session_enum == 1,
          "Switch to Custom Game restores the multiplayer lobby");
    // Probe only the authored leader text while keeping the actual route/state.
    auto &screen = content.screens.at(14);
    screen.flags |= 4;
    screen.button_key = 0;
    content.screens.erase(5);
    for (auto &pane : screen.panes) {
      pane.bitmaps.clear(); pane.grids.clear(); pane.buttons.clear();
      for (unsigned n = 0; n < pane.texts.size(); ++n) {
        if (n != 43) pane.texts[n].bounds = {};
        pane.texts[n].string = 0xff000000u + n;
        screen.strings[pane.texts[n].string] = "A";
      }
    }
    state.select(0);
    const auto isolated = painter.draw(state, 10);
    check(std::none_of(isolated.vertices.begin(), isolated.vertices.end(),
        [&](const auto &v) { return mesh.materials.at(v.material).name.starts_with("font/"); }),
        "host leader text is hidden instead of overlapping Start Game");
    return failures ? 1 : 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
