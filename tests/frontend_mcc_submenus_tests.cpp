#define SDL_MAIN_HANDLED
#include "halo2/frontend.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
unsigned checks{};
void require(bool value, const std::string &message) {
  ++checks;
  if (!value) throw FormatError(message);
}
void choose(MenuState &state, unsigned row, unsigned destination) {
  require(row < state.frame().entries.size(), "Missing menu row before route");
  const auto source = state.frame().screen;
  state.select(row);
  state.accept();
  require(state.frame().screen == destination, "Route " + std::to_string(source) + " row " +
          std::to_string(row) + " returned " + std::to_string(state.frame().screen) +
          "; expected " + std::to_string(destination));
}
void local_profile(MenuState &state) {
  require(state.frame().screen == 9, "New local profile did not start at title");
  state.accept();
  require(state.frame().screen == 10, "Title did not open MCC profile picker");
  choose(state, 0, 167);
  state.text("MCC Submenu Test");
  state.commit_text();
  require(state.frame().screen == 38, "Profile creation missed customization");
  state.back();
  require(state.frame().screen == 6 && state.signed_in_profile() == "MCC Submenu Test",
          "MCC profile could not reach the main menu");
}
std::filesystem::path selected_cache(const std::filesystem::path &root) {
  std::ifstream input(root / "assets/mcc-source.tsv");
  std::string signature, key, path;
  std::getline(input, signature);
  if (signature != "H2_MCC_ASSET_SOURCE_V1") return {};
  while (input >> key >> path) if (key == "cache") return root / path;
  return {};
}
void source_screen(const UiScreen &actual, const UiScreen &raw) {
  require(actual.id == raw.id && actual.name == raw.name && actual.header == raw.header &&
          actual.button_key == raw.button_key && actual.shape == raw.shape &&
          actual.panes.size() == raw.panes.size(), "MCC submenu changed authored screen identity");
  for (unsigned i = 0; i < raw.panes.size(); ++i) {
    const auto &a = actual.panes[i], &b = raw.panes[i];
    require(a.animation == b.animation && a.buttons.size() == b.buttons.size() &&
            a.texts.size() == b.texts.size() && a.bitmaps.size() == b.bitmaps.size() &&
            a.lists.size() == b.lists.size(), "MCC submenu changed authored children");
    for (unsigned j = 0; j < a.texts.size(); ++j)
      require(a.texts[j].string == b.texts[j].string && a.texts[j].font == b.texts[j].font,
              "MCC submenu remapped an authored text SID/font");
    for (unsigned j = 0; j < a.buttons.size(); ++j)
      require(a.buttons[j].text.string == b.buttons[j].text.string &&
              a.buttons[j].bitmap == b.buttons[j].bitmap,
              "MCC submenu remapped an authored button SID/resource");
  }
}
}
int main(int argc, char **argv) try {
  if (argc != 4) throw FormatError("Imported MCC cache root (or auto), project root and report path required");
  const auto cache_root = std::string_view(argv[1]) == "auto" ? selected_cache(argv[2]) : std::filesystem::path(argv[1]);
  const auto maps = cache_root / "halo2/h2_maps_win64_dx11";
  const auto fonts = cache_root / "halo2/h2_fonts";
  if (!std::filesystem::is_regular_file(maps / "mainmenu.map")) return 77;
  const auto *scratch_value = std::getenv("HALO2_TEST_SCRATCH");
  require(scratch_value, "Test requires owned scratch");
  const auto preferences = std::filesystem::path(scratch_value) / "mcc-submenus.cfg";
  Cache cache(maps / "mainmenu.map");
  require(cache.is_mcc(), "Submenu regression requires actual MCC cache input");
  auto content = cache.load_frontend();
  const auto raw_screens = content.screens.size();
  content.vista = load_vista_settings(maps / "mainmenu.map");
  require(content.vista && content.vista->source_fonts == fonts,
          "MCC submenu adapter did not retain canonical imported font owner");
  const auto font_source = load_gameplay_font_source(content.vista->source_fonts);
  require(font_source.domain == GameplayFontDomain::Mcc && font_source.names.size() == 12,
          "MCC submenus selected a legacy font bank");
  for (const auto &[id, screen] : content.vista->screens)
    source_screen(screen, content.screens.at(id));
  require(content.screens.size() == raw_screens, "Extension changed the native screen registry");
  for (const auto &[datum, image] : content.vista->images) {
    const auto &raw = content.images.at(datum);
    require(image.name == raw.name && image.images.size() == raw.images.size(),
            "MCC submenu image owner differs from native cache");
    for (unsigned i = 0; i < image.images.size(); ++i)
      require(image.images[i].width == raw.images[i].width && image.images[i].height == raw.images[i].height &&
              image.images[i].format == raw.images[i].format && image.images[i].pixels == raw.images[i].pixels,
              "MCC submenu image pixels differ from their native owner");
  }
  Mesh mesh;
  FrontendPainter painter(content, fonts, mesh);
  nlohmann::json report{{"schema", "H2_MCC_SUBMENU_REGRESSION_V1"}, {"status", "RUNNING"},
      {"cache", cache_root.string()}, {"mcc_only", true}, {"network_services_created", false}};
  std::set<unsigned> drawn;
  nlohmann::json observations = nlohmann::json::array();
  std::size_t vertices{};
  const auto draw = [&](MenuState &state, bool entry_hit = false,
                        FrontendPointerAction expected_action = FrontendPointerAction::Entry) {
    std::optional<FrontendPointerTarget> selected_hit;
    for (const auto viewport : {std::array<unsigned, 2>{640, 480}, {1280, 720}}) {
      const auto output = painter.draw(state, 5.f, -1, 20000, viewport);
      require(!output.vertices.empty(), "Submenu produced no painter output on " + std::to_string(state.frame().screen));
      for (const auto &v : output.vertices)
        require(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
                std::isfinite(v.uv.x) && std::isfinite(v.uv.y) && v.material < mesh.materials.size(),
                "Submenu produced invalid vertex/material output");
      vertices += output.vertices.size();
      nlohmann::json observation{{"screen", state.frame().screen}, {"selected", state.frame().selected},
          {"entries", state.frame().entries.size()}, {"viewport", viewport},
          {"vertices", output.vertices.size()}, {"rendering_gaps", output.rendering_gaps}};
      if (entry_hit) {
        bool found = false;
        for (int y = 0; y < 480 && !found; y += 2)
          for (int x = 0; x < 640 && !found; x += 2)
            if (auto hit = painter.pointer_hit(float(x), float(y));
                hit && hit->action == expected_action &&
                (expected_action != FrontendPointerAction::Entry || hit->index == state.frame().selected)) {
              found = true; selected_hit = hit;
              observation["selected_pointer_hit"] = {x, y};
              observation["selected_pointer_action"] = int(hit->action);
            }
        require(found, "Expected submenu pointer action " + std::to_string(int(expected_action)) +
                " missing on " + std::to_string(state.frame().screen));
      }
      observations.push_back(std::move(observation));
    }
    drawn.insert(state.frame().screen);
    return selected_hit;
  };
  // Independently use the authored bitmap extent and sprite fraction. A PC
  // screen projects source pixels once at480/1200, even when its fonts are
  // imported MCC fonts. This catches the nested .4*.4 regression without
  // depending on the painter's private canvas flags or invented offsets.
  MenuState projection(content);
  projection.open(19);
  const auto projection_draw = painter.draw(projection, 5.f, -1, 20000, {640, 480});
  nlohmann::json projection_checks = nlohmann::json::array();
  for (const auto &bitmap : content.vista->screens.at(19).panes.front().bitmaps) {
    if (bitmap.fps || bitmap.scroll.x || bitmap.scroll.y || !content.vista->images.contains(bitmap.datum)) continue;
    const auto &image = content.vista->images.at(bitmap.datum);
    if (image.images.empty()) continue;
    const auto frame = unsigned(std::max(0, bitmap.frame));
    UiSprite sprite;
    if (!image.sequences.empty() && !image.sequences.front().empty())
      sprite = image.sequences.front().at(frame % image.sequences.front().size());
    else sprite.image = std::min(frame, unsigned(image.images.size() - 1));
    const auto &texture = image.images.at(sprite.image);
    const float expected_width = texture.width * (sprite.right - sprite.left) *
        (bitmap.scale.x ? bitmap.scale.x : 1) * (480.f / 1200.f);
    const float expected_height = texture.height * (sprite.bottom - sprite.top) *
        (bitmap.scale.y ? bitmap.scale.y : 1) * (480.f / 1200.f);
    unsigned matched{};
    for (std::size_t i = 0; i + 5 < projection_draw.vertices.size(); i += 6) {
      const auto &a = projection_draw.vertices[i], &b = projection_draw.vertices[i + 4];
      if (mesh.materials.at(a.material).name != image.name ||
          std::abs(a.uv.x - sprite.left) > .00001f || std::abs(a.uv.y - sprite.top) > .00001f ||
          std::abs(b.uv.x - sprite.right) > .00001f || std::abs(b.uv.y - sprite.bottom) > .00001f) continue;
      require(std::abs(b.position.x - a.position.x - expected_width) < .001f &&
              std::abs(b.position.y - a.position.y - expected_height) < .001f,
              "PC Settings bitmap extent was not projected exactly once: " + image.name);
      ++matched;
    }
    if (matched) projection_checks.push_back({{"image", image.name}, {"source_pixels", {texture.width, texture.height}},
        {"projected_extent", {expected_width, expected_height}}, {"matched_quads", matched}});
  }
  require(!projection_checks.empty(), "No authored PC Settings bitmap qualified for canvas regression");
  // The category list is a persistent screen child. Other panes replace
  // artwork/help only; their omitted/empty list coordinates must not move it.
  nlohmann::json selector_checks = nlohmann::json::array();
  for (const unsigned id : {60u, 207u}) {
    MenuState selector(content);
    selector.open(id);
    require(selector.frame().entries.size() == 7, "Category selector lost an engine");
    for (unsigned row = 0; row < 7; ++row) {
      selector.select(row);
      for (const auto viewport : {std::array<unsigned, 2>{640,480}, {1280,720}, {2560,1080}}) {
        const auto output = painter.draw(selector, 5, -1, 20000, viewport);
        const auto hit = painter.pointer_hit(210, 153 + 20.f * row);
        require(hit && hit->action == FrontendPointerAction::Entry && hit->index == row,
                "Category list moved while scrolling screen " + std::to_string(id) +
                " row " + std::to_string(row));
        bool category_art = false;
        for (std::size_t i=0;i+5<output.vertices.size();i+=6) {
          const auto &a=output.vertices[i], &b=output.vertices[i+4];
          const auto &name=mesh.materials.at(a.material).name;
          if (name == "ui\\list_skins\\default\\item_background")
            require(b.position.x < 349, "Category list overlaps its description/artwork column");
          if (name == "ui\\screens\\game_shell\\mp_game_select\\mp_games") category_art = true;
        }
        require(category_art, "Category change dropped its MCC illustration");
        selector_checks.push_back({{"screen",id},{"row",row},{"viewport",viewport},
            {"selected_hit",{210,153+20.f*row}}});
      }
    }
  }
  const auto original_header_font = load_gameplay_font(font_source, 2);
  const auto &header_s = original_header_font.glyphs.at('S');
  require(std::any_of(projection_draw.vertices.begin(), projection_draw.vertices.end(), [&](const auto &v) {
    return mesh.materials.at(v.material).name.ends_with("/original-role/2") &&
        std::abs(v.position.x - (50 + header_s.bearing_x)) < .001f &&
        std::abs(v.position.y - (36 + original_header_font.ascent - header_s.bearing_y)) < .001f;
  }), "PC Settings title lost original shared shell anchor/font");
  require(std::any_of(projection_draw.vertices.begin(), projection_draw.vertices.end(), [&](const auto &v) {
    return mesh.materials.at(v.material).name == "ui\\global_bitmaps\\framing_center" &&
        std::abs(v.position.y - 31) < .001f && std::abs(v.uv.y - 69.f/1303.f) < .00001f;
  }), "PC Settings selected a different shell frame from its title/footer anchors");
  // Original main-menu skin11 has one240x19 selection image; sign-in30
  // uses512/256-unit panels. MCC's higher-resolution pages remain the raster
  // owners, not the layout dimensions. See the authenticated numeric profile
  // in frontend_mcc_logical_layout_data.inc and the October19 visual receipt.
  nlohmann::json logical_checks = nlohmann::json::array();
  const auto extent = [&](const UiDraw &output, const std::string &name,
                          float width, float height) {
    unsigned matched{};
    for (std::size_t i = 0; i + 5 < output.vertices.size(); i += 6) {
      const auto &a = output.vertices[i], &b = output.vertices[i + 4];
      const auto &material = mesh.materials.at(a.material);
      if (material.name != name) continue;
      // The same bank also has a2x2 transparent inactive-row page. The
      // selected image is the source-qualified full-size page under test.
      if (material.diffuse.width == 2 && material.diffuse.height == 2) continue;
      require(std::abs(b.position.x - a.position.x - width) < .001f &&
              std::abs(b.position.y - a.position.y - height) < .001f,
              "MCC raster density changed original logical extent: " + name);
      ++matched;
    }
    require(matched == 1, "Original logical widget count differs: " + name);
  };
  nlohmann::json branding_checks = nlohmann::json::array();
  std::ifstream branding_reference(std::filesystem::path(argv[2]) / "resources/branding/title-forever.json");
  require(bool(branding_reference), "Missing website FOREVER artwork reference");
  const auto branding_artwork = nlohmann::json::parse(branding_reference);
  require(branding_artwork.at("font") == "Arial Regular 7.00" &&
          branding_artwork.at("text") == "F O R E V E R" &&
          branding_artwork.at("css").at("font_size") == 12 &&
          branding_artwork.at("css").at("letter_spacing") == 8,
          "Title branding no longer uses the website's Arial/CSS reference");
  const auto branding = [&](const MenuState &state, const UiDraw &output,
                            std::array<unsigned, 2> viewport) {
    const auto subtitle_vertex = [&](const Vertex &vertex) {
      return mesh.materials.at(vertex.material).name == "ui/branding/forever" &&
          std::abs(vertex.color.x - 182.f / 255) < .00001f &&
          std::abs(vertex.color.y - 217.f / 255) < .00001f &&
          std::abs(vertex.color.z - 247.f / 255) < .00001f;
    };
    std::array<float, 4> logo{640, 480, 0, 0}, subtitle{640, 480, 0, 0};
    unsigned logo_vertices{}, subtitle_vertices{};
    float logo_alpha{}, subtitle_alpha{};
    const auto include = [](std::array<float, 4> &bounds, const Vertex &vertex) {
      bounds[0] = std::min(bounds[0], vertex.position.x);
      bounds[1] = std::min(bounds[1], vertex.position.y);
      bounds[2] = std::max(bounds[2], vertex.position.x);
      bounds[3] = std::max(bounds[3], vertex.position.y);
    };
    for (const auto &vertex : output.vertices) {
      if (mesh.materials.at(vertex.material).name ==
          "ui\\screens\\game_shell\\start_screen\\start_screen") {
        include(logo, vertex);
        logo_alpha = vertex.position.z;
        ++logo_vertices;
      } else if (subtitle_vertex(vertex)) {
        include(subtitle, vertex);
        subtitle_alpha = vertex.position.z;
        ++subtitle_vertices;
      }
    }
    require(logo_vertices == 6 && subtitle_vertices == 6,
            "Title branding lost its logo or frozen Arial FOREVER artwork on screen " +
                std::to_string(state.frame().screen));
    const float scale = (logo[2] - logo[0]) / 365.f;
    const float left = branding_artwork.at("logical_left").get<float>();
    const float top = branding_artwork.at("logical_top_after_logo").get<float>();
    const float width = branding_artwork.at("width").get<float>() / 8.f;
    const float height = branding_artwork.at("height").get<float>() / 8.f;
    require(subtitle[0] > logo[0] && subtitle[2] < logo[2] &&
            std::abs(subtitle[0] - (logo[0] + left * scale)) < .01f &&
            std::abs(subtitle[1] - (logo[3] + top * scale)) < .01f &&
            std::abs(subtitle[2] - subtitle[0] - width * scale) < .01f &&
            std::abs(subtitle[3] - subtitle[1] - height * scale) < .01f &&
            subtitle[1] > logo[3] && subtitle[3] < 480 &&
            std::abs(logo_alpha - subtitle_alpha) < .00001f,
            "FOREVER subtitle differs from the website's measured Arial layout");
    float following_text = 480;
    for (const auto &vertex : output.vertices)
      if (!subtitle_vertex(vertex) &&
          mesh.materials.at(vertex.material).name.starts_with("font/") &&
          vertex.position.y >= logo[3])
        following_text = std::min(following_text, vertex.position.y);
    require(subtitle[3] < following_text,
            "FOREVER subtitle overlaps title options or the Start prompt on screen " +
                std::to_string(state.frame().screen));
    branding_checks.push_back({{"screen", state.frame().screen}, {"viewport", viewport},
        {"logo_bounds", logo}, {"forever_bounds", subtitle},
        {"following_text_top", following_text}});
  };
  MenuState title(content);
  for (const auto viewport : {std::array<unsigned, 2>{640, 480}, {1280, 720}, {2553, 1380}})
    branding(title, painter.draw(title, 5.f, -1, 20000, viewport), viewport);
  const auto visual_preferences = std::filesystem::path(scratch_value) / "visual.cfg";
  for (unsigned species : {0u, 1u}) {
    {
      std::ofstream file(visual_preferences);
      file << "halo2-native-menu-v1\nprofile \"Visual Test\"\nactive \"Visual Test\"\n"
              "option \"Visual Test\" 52 " << species << '\n';
    }
    MenuState visual(content, visual_preferences);
    visual.accept();
    choose(visual, 1, 6);
    for (unsigned screen : {6u, 30u}) {
      if (screen == 30) choose(visual, 2, 30);
      for (const auto viewport : {std::array<unsigned, 2>{640, 480}, {1280, 720}, {2553, 1380}}) {
        const auto output = painter.draw(visual, 5.f, -1, 20000, viewport);
        if (screen == 6) {
          extent(output, "ui\\list_skins\\main_menu\\list_bkd", 240, 19);
          require(std::any_of(output.vertices.begin(), output.vertices.end(), [&](const auto &v) {
            return mesh.materials.at(v.material).name.ends_with("/original-role/10");
          }), "Main-menu text lost original logical font role");
          branding(visual, output, viewport);
          require(visual.frame().entries.size() == 6 &&
                  visual.frame().entries.front().destination == 218 &&
                  visual.frame().entries.back().label == "EXIT GAME",
                  "Main menu lost Campaign or its final Exit Game option");
          visual.select(unsigned(visual.frame().entries.size() - 1));
          const auto selected_exit = painter.draw(visual, 5.f, -1, 20000, viewport);
          branding(visual, selected_exit, viewport);
          std::array<bool, 6> visible_entries{};
          for (int y = 0; y < 480; ++y)
            for (int x = 0; x < 640; x += 2)
              if (const auto hit = painter.pointer_hit(float(x), float(y));
                  hit && hit->action == FrontendPointerAction::Entry &&
                  hit->index < visible_entries.size())
                visible_entries[hit->index] = true;
          require(std::all_of(visible_entries.begin(), visible_entries.end(), [](bool hit) { return hit; }),
                  "Selecting Exit Game scrolls Campaign or another main-menu option off screen");
          visual.select(0);
        } else {
          extent(output, "ui\\screens\\game_shell\\4way_multiplayer_signin\\mp_signin", 512, 512);
          extent(output, "ui\\global_bitmaps\\ul_09", 256, 256);
          extent(output, "ui\\global_bitmaps\\br_09", 256, 256);
          require(output.scenes.size() == 1, "Single signed-in player has unexpected model count");
          const auto &scene = output.scenes.front();
          const auto &definition = *scene.definition;
          const auto &authored = content.screens.at(30).panes.front().scenes.at(species ? 4 : 0);
          require(!scene.vista_viewport && definition.viewport.top == 240 &&
                  definition.viewport.left == -320 && definition.viewport.bottom == 0 &&
                  definition.viewport.right == 320, "Player preview lost original model viewport");
          require(definition.objects == authored.objects && definition.lights == authored.lights &&
                  definition.field_of_view == authored.field_of_view &&
                  definition.camera.x == authored.camera.x && definition.camera.y == authored.camera.y &&
                  definition.camera.z == authored.camera.z && definition.scale.x == authored.scale.x &&
                  definition.scale.y == authored.scale.y && definition.scale.z == authored.scale.z &&
                  definition.flags == authored.flags && definition.animation == authored.animation &&
                  definition.delay == authored.delay, "Viewport projection changed an authored model field");
          // Original22E3B4 admits a model widget only once its authored
          // animation delay expires. Before that, its offset is still zero;
          // submitting it would flash the final pose before the slide begins.
          for (unsigned ms : {0u, 1u, 99u, 100u, 101u, 329u, 330u}) {
            const auto transition = painter.draw(visual, float(ms)/1000.f, -1, ms, viewport);
            if (ms < authored.delay) {
              require(transition.scenes.empty(), "Player preview flashed before its authored slide-in delay");
            } else {
              require(transition.scenes.size() == 1, "Player preview missed its authored slide-in admission");
              const float phase = std::clamp(float(ms-authored.delay)/230.f, 0.f, 1.f);
              require(std::abs(transition.scenes.front().animation_offset.x - (1024.f-1024.f*phase)) < .001f,
                      "Player preview changed the original230ms slide offset");
            }
          }
        }
        logical_checks.push_back({{"screen", screen}, {"species", species}, {"viewport", viewport}});
      }
    }
  }
  MenuState settings(content, preferences);
  local_profile(settings);
  choose(settings, 4, vista_menu(19));
  require(settings.frame().entries.size() == 6, "PC settings hub lost existing six categories");
  draw(settings, true);
  settings.select(2);
  draw(settings, true);
  choose(settings, 2, vista_menu(256));
  require(settings.frame().entries.size() == 14, "Video settings lost existing native controls");
  draw(settings, true);
  DisplaySettings applied;
  settings.set_pc_display_adapter([&](const DisplaySettings &value) { applied = value; return std::string{}; });
  settings.set_pc_video_modes({{800, 600}, {1280, 720}}, 960, 720, false);
  choose(settings, 9, vista_menu(272));
  require(settings.frame().context == video_fov_context && settings.frame().entries.size() == 51,
          "On-foot FOV choices changed");
  choose(settings, 50, vista_menu(256));
  require(applied.field_of_view == 120, "MCC video FOV choice did not reach display adapter");
  choose(settings, 10, vista_menu(272));
  require(settings.frame().context == video_vehicle_fov_context, "Vehicle FOV used on-foot setting");
  choose(settings, 20, vista_menu(256));
  require(applied.field_of_view == 120 && applied.vehicle_field_of_view == 90,
          "MCC vehicle FOV choice changed another setting");
  const auto before_toggle = settings.stack().size();
  choose(settings, 11, vista_menu(256));
  choose(settings, 12, vista_menu(256));
  choose(settings, 0, vista_menu(256));
  require(applied.crt_filter && applied.two_player_split == TwoPlayerSplit::Vertical &&
          applied.graphics_mode == pstencil::GraphicsMode::pStencil && settings.stack().size() == before_toggle,
          "MCC native video toggles lost existing behavior");
  choose(settings, 0, vista_menu(256));
  require(applied.graphics_mode == pstencil::GraphicsMode::PathTracing &&
          settings.pc_value_label(settings.frame(), 0) == "Path Tracing" &&
          MenuState(content, preferences).pc_display_settings().graphics_mode == pstencil::GraphicsMode::PathTracing &&
          settings.stack().size() == before_toggle,
          "MCC Path Tracing mode did not apply in place and persist");
  choose(settings, 0, vista_menu(256));
  require(applied.graphics_mode == pstencil::GraphicsMode::Original,
          "MCC graphics mode did not wrap to Original");
  settings.back();
  settings.select(3);
  draw(settings, true);
  choose(settings, 3, vista_menu(257));
  require(settings.frame().entries.size() == 4, "Audio settings lost choices");
  draw(settings, true);
  choose(settings, 0, vista_menu(263));
  require(settings.frame().entries.size() == 11, "Volume choices incomplete");
  draw(settings, true);
  choose(settings, 0, vista_menu(257));
  require(applied.volume == 0, "MCC audio choice did not reach display adapter");
  settings.open(38);
  // Match each native destination to its semantic MCC illustration. PC pane2
  // is Keyboard/Mouse; Voice Output must use the surviving headset artwork.
  const auto profile_art = std::find_if(content.images.begin(), content.images.end(),
      [](const auto &item) { return item.second.name ==
          "ui\\screens\\game_shell\\settings_screen\\player_profile\\edit_profile"; });
  require(profile_art != content.images.end(), "Missing MCC profile illustration bank");
  for (const auto [row, page] : std::array<std::pair<unsigned, unsigned>, 7>{{
      {0, 0}, {1, 1}, {2, 3}, {3, 3}, {4, 4}, {5, 5}, {6, 2}}}) {
    settings.select(row);
    const auto output = painter.draw(settings, 5, -1, 20000);
    const auto &expected = profile_art->second.images.at(page);
    bool matching_art = false;
    for (const auto &v : output.vertices) {
      const auto &material = mesh.materials.at(v.material);
      if (material.name == profile_art->second.name) {
        require(material.diffuse.pixels == expected.pixels,
                "Profile row " + std::to_string(row) + " uses another setting's MCC illustration");
        matching_art = true;
      }
    }
    require(matching_art, "Profile row lost its semantic MCC illustration");
  }
  settings.select(0);
  require(settings.frame().entries.back().destination == int(vista_menu(258)),
          "Profile cannot reach keyboard and mouse settings");
  choose(settings, unsigned(settings.frame().entries.size() - 1), vista_menu(258));
  require(settings.frame().entries.size() == 6, "Keyboard settings lost existing six routes");
  draw(settings, true);
  choose(settings, 0, vista_menu(243));
  choose(settings, 3, vista_menu(253));
  require(settings.frame().entries.size() == 43, "Advanced keyboard editor lost sections/actions");
  draw(settings, true);
  settings.select(1); settings.accept();
  require(settings.capturing_binding(), "MCC editor did not begin binding capture");
  settings.bind_pc_input(SDL_SCANCODE_UP);
  settings.auxiliary();
  settings.bind_pc_input(SDL_SCANCODE_I);
  const auto keyboard = settings.pc_keyboard_settings();
  require(keyboard.bindings[unsigned(PcAction::Forward)] == SDL_SCANCODE_UP &&
          keyboard.secondary[unsigned(PcAction::Forward)] == SDL_SCANCODE_I,
          "MCC editor lost primary/secondary bindings");
  MenuState reload(content, preferences);
  require(reload.pc_keyboard_settings() == keyboard && reload.pc_display_settings() == applied,
          "MCC submenu preferences did not persist");
  MenuState video(content);
  std::array<unsigned, 3> mock_mode{960, 720, 0};
  std::vector<std::array<unsigned, 3>> mode_calls;
  video.set_pc_video_adapter([&](unsigned width, unsigned height, FullscreenMode fullscreen) {
    mock_mode = {width, height, unsigned(fullscreen)};
    mode_calls.push_back(mock_mode);
    return std::string{};
  });
  video.set_pc_video_modes({{800, 600}, {1280, 720}}, 960, 720, false);
  video.open(vista_menu(256));
  video.ui_clock(1000);
  choose(video, 2, vista_menu(269));
  choose(video, 0, vista_menu(293));
  require(mock_mode == std::array<unsigned, 3>{800, 600, 0},
          "Resolution preview did not reach mock desktop adapter");
  draw(video, true, FrontendPointerAction::Accept);
  video.back();
  require(video.frame().screen == vista_menu(256) && mock_mode == std::array<unsigned, 3>{960, 720, 0},
          "Cancel did not restore pre-preview resolution");
  video.ui_clock(20000);
  choose(video, 2, vista_menu(269));
  choose(video, 0, vista_menu(293));
  require(video.frame().description.find("16") != std::string::npos &&
          video.frame().description.find("\xee\x92\x92") == std::string::npos,
          "MCC confirmation did not substitute source initial sixteen-second countdown");
  video.ui_clock(35999);
  require(video.frame().screen == vista_menu(293) && mock_mode == std::array<unsigned, 3>{800, 600, 0},
          "Resolution preview reverted before source sixteen-second deadline");
  video.ui_clock(36000);
  require(video.frame().screen == vista_menu(256) && mock_mode == std::array<unsigned, 3>{960, 720, 0},
          "Source sixteen-second timeout did not restore resolution");
  video.ui_clock(40000);
  choose(video, 2, vista_menu(269));
  choose(video, 2, vista_menu(293));
  const auto confirm_hit = draw(video, true, FrontendPointerAction::Accept);
  require(confirm_hit && video.activate_pointer(*confirm_hit) && video.frame().screen == vista_menu(256),
          "Visible resolution confirmation Accept did not keep mode");
  require(!video.activate_pointer(*confirm_hit), "Resolution confirmation admitted a stale pointer target");
  const auto confirmed_calls = mode_calls.size();
  video.ui_clock(100000);
  require(mock_mode == std::array<unsigned, 3>{1280, 720, 0} && mode_calls.size() == confirmed_calls,
          "Confirmed resolution was reverted by stale countdown");
  choose(video, 1, vista_menu(271));
  choose(video, 1, vista_menu(293));
  require(mock_mode == std::array<unsigned, 3>{1280, 720, 1}, "Display Mode did not preview fullscreen");
  video.back();
  require(video.frame().screen == vista_menu(256) && mock_mode == std::array<unsigned, 3>{1280, 720, 0},
          "Display Mode cancel did not restore prior confirmed mode");
  // Exercise each existing choice factory without account/session/network owners.
  constexpr std::array<std::pair<unsigned, unsigned>, 23> choices{{
      {42, 10}, {44, 2}, {242, 3}, {243, 4}, {245, 2}, {253, 43}, {256, 14}, {257, 4},
      {258, 6}, {263, 11}, {267, 5}, {268, 2}, {269, 3}, {271, 3}, {272, 10}, {273, 11},
      {274, 2}, {276, 2}, {285, 2}, {290, 3}, {294, 2}, {295, 2}, {296, 1}}};
  for (const auto [id, count] : choices) {
    MenuState state(content);
    state.set_pc_video_modes({{800, 600}, {1280, 720}}, 960, 720, false);
    state.open(vista_menu(id));
    require(state.frame().entries.size() == count, "Choice inventory differs on " + std::to_string(id));
    for (const auto &entry : state.frame().entries) require(!entry.label.empty(), "Empty MCC submenu label");
    draw(state, true);
  }
  for(unsigned context=1;context<=2;++context) {
    MenuState state(content);state.open(vista_menu(274),context);
    require(state.frame().entries.size()==(context==1?3u:6u),"Upscaling choices missing");draw(state,true);
  }
  MenuState categories(content);
  categories.open(19);
  choose(categories, 5, vista_menu(297));
  draw(categories);
  categories.back();
  require(categories.frame().screen == vista_menu(19), "About Back lost Settings hub");
  choose(categories, 4, vista_menu(296));
  draw(categories, true);
  categories.back();
  require(categories.frame().screen == vista_menu(19), "Network adapter Back lost Settings hub");
  MenuState split(content);
  local_profile(split);
  choose(split, 2, 30);
  require(!split.online_identity(), "Split-screen sign-in unexpectedly requires Online identity");
  const auto continue_hit = draw(split, true, FrontendPointerAction::Accept);
  require(continue_hit && split.activate_pointer(*continue_hit) && split.frame().screen == 14,
          "Visible split-screen A prompt did not enter lobby");
  draw(split, true);
  require(split.frame().entries.size() == 2, "Split-screen lobby lost Start/Setup");
  {
    const auto lobby_view = split.lobby_view();
    require(lobby_view && lobby_view->map, "Lobby lost its selected map");
    const auto &map_image = content.images.at(lobby_view->map->bitmap);
    const auto output = painter.draw(split, 5, -1, 20000);
    bool map_visible = false;
    for (std::size_t i = 0; i + 5 < output.vertices.size(); i += 6) {
      const auto &a = output.vertices[i], &b = output.vertices[i + 4];
      if (mesh.materials.at(a.material).name != map_image.name) continue;
      require(std::abs(b.position.x - a.position.x - 220) < .001f &&
              std::abs(b.position.y - a.position.y - 207) < .001f,
              "Lobby map art lost its original220x207 logical extent");
      map_visible = true;
    }
    require(map_visible, "Lobby projection dropped its MCC-owned selected map artwork");
  }

  for (const auto [x, row] : std::array<std::pair<float, unsigned>, 2>{{{124, 0}, {303, 1}}}) {
    const auto target = painter.pointer_hit(x, 77);
    require(target && target->action == FrontendPointerAction::Entry && target->index == row,
            "Lobby Start/Setup buttons lost their original bounds below the title bar");
  }

  require(split.controller_slot(41, true) == 0,
          "Split-screen host controller did not bind its existing local slot");
  const auto join = split.controller_slot(42, true);
  require(join && *join == 1 && split.frame().screen == 10, "Second split-screen controller cannot open profile picker");
  split.back();
  require(split.frame().screen == 14 && split.local_players().size() == 1,
          "Canceled second-player sign-in damaged lobby owner");
  require(split.controller_slot(43, true) == 1 && split.frame().screen == 10,
          "Canceled second-player slot could not be retried");
  choose(split, 0, 167);
  split.text("MCC Local Two");
  split.commit_text();
  require(split.frame().screen == 38, "Second-player creation missed customization");
  split.back();
  require(split.frame().screen == 14 && split.local_players().size() == 2,
          "Second local profile did not return to split-screen lobby");
  draw(split, true);
  choose(split, 1, 23);
  draw(split, true);
  MenuState online(content);
  local_profile(online);
  choose(online, 1, 20);
  require(!online.online_identity() && online.frame().entries.size() == 1,
          "Offline Online sign-in presentation lost its identity gate");
  draw(online, true);
  online.back();
  require(online.frame().screen == 6, "Online sign-in Back did not restore main menu");
  for (const auto id : {186u, 219u, 237u, 210u}) {
    online.open(id, online_server_browser_context);
    require(online.frame().screen == 20 && !online.online_identity(),
            "Online browser/matchmaking bypassed account gate");
    online.back();
  }
  report.update({{"status", "PASS"}, {"checks", checks}, {"native_screens", raw_screens},
      {"extension_screens", content.vista->screens.size()}, {"extension_images", content.vista->images.size()},
      {"drawn_screens", drawn}, {"vertices", vertices}, {"viewports", {{640, 480}, {1280, 720}}},
      {"observations", observations},
      {"pc_bitmap_projection", projection_checks},
      {"stationary_category_selectors", selector_checks},
      {"title_branding", branding_checks},
      {"original_logical_layout", logical_checks},
      {"mock_video_mode_calls", mode_calls}, {"host_display_changed", false},
      {"online_scope", "Offline sign-in presentation and browser/matchmaking access gate only; no services or remote actions"}});
  std::ofstream(argv[3]) << report.dump(2) << '\n';
  std::cout << report.dump() << '\n';
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
