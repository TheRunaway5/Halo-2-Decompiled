#include "halo2/frontend.hpp"
#include "halo2/frontend_signin.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template <std::size_t N> std::string bits(const std::array<bool, N> &values) {
  std::string result;
  for (const auto value : values) result += value ? '1' : '0';
  return result;
}
void oracle(const std::filesystem::path &fixture) {
  std::ifstream file(fixture);
  check(bool(file), "missing original252554 fixture");
  unsigned cases = 0;
  for (std::string row; std::getline(file, row);) {
    std::istringstream input(row);
    unsigned index;
    FrontendSignInSlot slot;
    std::string texts, models, bitmaps, frames, strings;
    input >> index >> slot.connected >> slot.signed_in >> slot.live >> slot.gamepad_type >> slot.species
          >> texts >> models >> bitmaps >> frames >> strings;
    check(!input.fail() && index < 4, "invalid original sign-in fixture");
    std::array<FrontendSignInSlot, 4> slots{};
    slots[index] = slot;
    const auto actual = frontend_signin_presentation(slots);
    check(bits(actual.texts) == texts && bits(actual.models) == models &&
              bits(actual.bitmaps) == bitmaps,
          "screen30 visibility differs from original252554/2B10A3");
    std::ostringstream actual_frames;
    for (unsigned i = 0; i < 4; ++i)
      actual_frames << (i ? "," : "") << actual.controller_frames[i];
    check(actual_frames.str() == frames, "screen30 controller frames differ from original2B0A14");
    // The serialized IDs have leading zeroes; compare numeric records.
    std::istringstream expected(strings);
    for (std::string item; std::getline(expected, item, ',');) {
      const auto colon = item.find(':');
      check(colon != std::string::npos, "invalid original string binding");
      const auto text_index = std::stoul(item.substr(0, colon));
      check(text_index < 24 && actual.text_strings[text_index] == std::stoul(item.substr(colon+1), nullptr, 16),
            "screen30 prompt binding differs from original253B1A caller");
    }
    const auto count = std::count_if(strings.begin(), strings.end(), [](char c) { return c == ':'; });
    check(count == std::count_if(actual.text_strings.begin(), actual.text_strings.end(),
                                [](auto value) { return value != 0; }), "extra native sign-in text override");
    ++cases;
  }
  check(cases == 256, "incomplete original252554 cases");
  std::cout << cases << " original sign-in visibility cases matched\n";
}
void painter(const std::filesystem::path &maps) {
  Cache cache(maps / "mainmenu.map");
  auto content = cache.load_frontend();
  MenuState state(content);
  state.accept(); //9->10
  state.accept(); //10->167
  state.text("Default");
  state.commit_text(); //167->38
  state.back(); //38->6, signs in created profile.
  check(state.frame().screen == 6, "fourway fixture missed real profile sign-in");
  state.select(2);
  state.accept(); //6->30 Split Screen.
  check(state.frame().screen == 30, "fourway fixture missed Split Screen");
  Mesh mesh;
  FrontendPainter painter(content, maps / "fonts", mesh);
  const auto draw = painter.draw(state, 10);
  const auto &pane = content.screens.at(30).panes.front();
  const auto &controller = content.images.at(pane.bitmaps[7].datum);
  std::size_t controller_quads = 0;
  for (std::size_t i = 0; i < draw.vertices.size(); i += 6)
    if (mesh.materials.at(draw.vertices[i].material).name == controller.name)
      ++controller_quads;
  check(controller_quads == 3, "occupied screen30 player still draws its empty controller");
  check(draw.scenes.size() == 1 && draw.scenes.front().definition->objects.front() == "ui_player1",
        "single signed-in profile must show exactly its Spartan model");
  // Probe each original text widget in isolation, keeping its authored font,
  // animation and bounds. A visible ordinary glyph makes labels measurable
  // even when an unbound private profile field otherwise expands to empty.
  auto &screen = content.screens.at(30);
  const auto authored = screen.panes.front().texts;
  screen.flags |= 4; // Suppress header during individual-widget observations.
  screen.button_key = 0;
  content.screens.erase(5);
  for (unsigned i = 0; i < authored.size(); ++i) {
    for (unsigned j = 0; j < authored.size(); ++j) {
      auto &widget = screen.panes.front().texts[j];
      widget = authored[j];
      widget.bounds = j == i ? authored[j].bounds : UiRect{};
      widget.string = 0xff000000u+j;
      screen.strings[widget.string] = "A";
    }
    const auto individual = painter.draw(state, 10);
    const bool glyph = std::any_of(individual.vertices.begin(), individual.vertices.end(),
        [&](const auto &vertex) { return mesh.materials.at(vertex.material).name.starts_with("font/"); });
    const bool original_visible = i == 0 || i == 1 || i == 8 || i >= 21;
    check(glyph == original_visible,
          "normal-entry screen30 text visibility differs from original occupied/disconnected slots");
  }
  // Saved profiles alone do not make a direct preview a signed-in user.
  MenuState unsigned_state(content);
  unsigned_state.open(30);
  check(painter.draw(unsigned_state, 10).scenes.empty(),
        "direct screen30 preview invents an occupied signed-in slot");
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("fixture and maps arguments required");
    oracle(argv[1]);
    if (!std::filesystem::exists(std::filesystem::path(argv[2])/"mainmenu.map")) return 77;
    painter(argv[2]);
    std::cout << "Retail normal-entry screen30 presentation passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
