#include "halo2/frontend_text_style.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
UiColor color(const std::string &hex) {
  check(hex.size() == 32, "invalid oracle color");
  std::array<unsigned char, 16> bytes;
  for (unsigned i = 0; i < bytes.size(); ++i)
    bytes[i] = static_cast<unsigned char>(std::stoul(hex.substr(i*2, 2), nullptr, 16));
  UiColor result;
  std::memcpy(&result, bytes.data(), sizeof(result));
  return result;
}
void oracle(const std::filesystem::path &fixture) {
  std::ifstream file(fixture);
  check(bool(file), "missing original text style oracle");
  std::size_t colors = 0, glyphs = 0, packed = 0, setup = 0;
  for (std::string row; std::getline(file, row);) {
    std::istringstream in(row);
    char type;
    in >> type;
    if (type == 'C') {
      std::string name, supplied, expected;
      in >> name >> supplied >> expected;
      const auto actual = frontend_text_color(color(supplied));
      const auto want = color(expected);
      check(std::bit_cast<std::array<std::uint32_t, 4>>(actual) ==
                std::bit_cast<std::array<std::uint32_t, 4>>(want),
            "text normalization differs from original13EC70/13EB60");
      ++colors;
    } else if (type == 'G') {
      std::string character;
      bool want;
      in >> character >> want;
      check(frontend_text_texture_color(std::stoul(character, nullptr, 16)) == want,
            "private glyph color policy differs from original13F660");
      ++glyphs;
    } else if (type == 'P') {
      std::string name, supplied, character, foreground, shadow;
      bool enabled;
      in >> name >> supplied >> character >> enabled >> foreground >> shadow;
      const auto normalized = frontend_text_color(color(supplied));
      auto pixel = frontend_text_pixel(normalized);
      if (frontend_text_texture_color(std::stoul(character, nullptr, 16)))
        pixel |= 0xffffff;
      const auto shade = enabled ? frontend_text_pixel({normalized.alpha, 0, 0, 0}) : 0;
      check(pixel == std::stoul(foreground, nullptr, 16) &&
                shade == std::stoul(shadow, nullptr, 16),
            "text glyph/shadow packing differs from original13F470 draw");
      ++packed;
    } else if (type == 'B') {
      bool button, shadow;
      unsigned flags;
      in >> button >> flags >> shadow;
      check(frontend_text_shadow(flags, button) == shadow,
            "definition flags differ from original text/button setup callers");
      ++setup;
    } else
      throw std::runtime_error("unknown oracle row");
    check(!in.fail(), "invalid text style oracle row");
  }
  check(colors == 1032 && glyphs == 1031 && packed == 1088 && setup == 32,
        "incomplete original text style oracle");
  check(frontend_text_shadow(0, false) && frontend_text_shadow(8, true) &&
            !frontend_text_shadow(8, false), "text/button setup flag mapping differs");
  std::cout << "Original text oracle: " << colors << " colors, " << glyphs
            << " glyph classifications, " << packed << " packed draws\n";
}
void painter(const std::filesystem::path &maps) {
  Cache cache(maps / "mainmenu.map");
  const auto content = cache.load_frontend();
  MenuState state(content);
  state.accept();
  state.accept();
  check(state.frame().screen == 167, "text regression missed Profile Name");
  Mesh mesh;
  FrontendPainter painter(content, maps / "fonts", mesh);
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  const auto &glyph = font.glyphs.at('1');
  const auto &key = content.screens.at(167).panes.front().buttons.front().text;
  const auto draw = painter.draw(state, 10);
  std::size_t foreground = draw.vertices.size();
  for (std::size_t i = 0; i < draw.vertices.size(); i += 6) {
    const auto &v = draw.vertices[i];
    if (mesh.materials.at(v.material).name == "font/conduit-12" &&
        v.uv.x == float(glyph.x)/font.atlas.width &&
        v.uv.y == float(glyph.y)/font.atlas.height && v.color.x != 0 &&
        v.position.y == 240-key.bounds.top+font.ascent-glyph.bearing_y) {
      foreground = i;
      break;
    }
  }
  check(foreground >= 6 && foreground < draw.vertices.size(), "digit foreground absent");
  const auto &fg = draw.vertices[foreground];
  const auto &shade = draw.vertices[foreground-6];
  check(shade.material == fg.material && shade.color.x == 0 &&
            shade.color.y == 0 && shade.color.z == 0 &&
            shade.position.x == fg.position.x+1 &&
            shade.position.y == fg.position.y+1 && shade.uv.x == fg.uv.x && shade.uv.y == fg.uv.y,
        "keyboard digit is missing original black shadow before foreground");
  const float width = float(glyph.advance + glyph.bearing_x);
  const float origin = (key.flags & 1) ? 320 + key.bounds.left
                       : (key.flags & 2) ? 320 + key.bounds.right-width
                       : (640 + key.bounds.left + key.bounds.right-width)*.5f;
  check(fg.position.x == origin + glyph.bearing_x,
        "keyboard glyph centering omits original cumulative bearing-X measure");
  const auto expected = frontend_text_pixel_color(frontend_text_pixel(frontend_text_color(key.color)));
  check(fg.color.x == expected.red && fg.color.y == expected.green && fg.color.z == expected.blue,
        "keyboard text is missing original packed authored color");
  bool white_icon = false;
  for (std::size_t i = 0; i < draw.vertices.size(); i += 6) {
    const auto &v = draw.vertices[i];
    if (mesh.materials.at(v.material).name == "font/conduit-12" &&
        v.color.x == 1 && v.color.y == 1 && v.color.z == 1)
      white_icon = true;
  }
  check(white_icon, "keyboard texture-color controller glyphs are tinted");
  std::cout << "Retail keyboard color, shadow, and bearing geometry regression passed\n";
}
void keyboard_labels(const std::filesystem::path &fixture,
                     const std::filesystem::path &maps) {
  Cache cache(maps / "mainmenu.map");
  const auto content = cache.load_frontend();
  MenuState state(content);
  state.accept();
  state.accept();
  check(state.frame().screen == 167 && state.frame().selected == 40,
        "keyboard label fixture missed original default DONE focus");
  Mesh mesh;
  FrontendPainter painter(content, maps / "fonts", mesh);
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  const auto draw = painter.draw(state, 5, -1, 5000);
  std::ifstream file(fixture);
  check(bool(file), "missing original keyboard label oracle");
  unsigned count = 0;
  for (std::string row; std::getline(file, row);) {
    std::istringstream in(row);
    int label;
    unsigned codepoint;
    float x, y, width, height;
    std::string foreground, shadow;
    in >> label >> codepoint >> x >> y >> width >> height >> foreground >> shadow;
    check(!in.fail(), "invalid original keyboard glyph row");
    const auto &glyph = font.glyphs.at(codepoint);
    const auto want = frontend_text_pixel_color(std::stoul(foreground, nullptr, 16));
    bool found = false;
    for (std::size_t i = draw.backdrops.back().before_vertex;
         i < draw.vertices.size(); i += 6) {
      const auto &v = draw.vertices[i];
      if (mesh.materials.at(v.material).name == "font/conduit-12" &&
          v.position.x == x && v.position.y == y &&
          draw.vertices[i + 4].position.x == x + width &&
          draw.vertices[i + 4].position.y == y + height &&
          v.uv.x == float(glyph.x) / font.atlas.width &&
          v.uv.y == float(glyph.y) / font.atlas.height &&
          v.color.x == want.red && v.color.y == want.green &&
          v.color.z == want.blue && v.position.z == want.alpha) {
        found = true;
        break;
      }
    }
    if (!found)
      throw std::runtime_error("keyboard original glyph mismatch: label=" +
                               std::to_string(label) + " character=" +
                               std::to_string(codepoint) + " x=" +
                               std::to_string(x));
    ++count;
  }
  check(count == 77, "incomplete keyboard label oracle");
  std::cout << "Original keyboard action labels and subtitle: " << count
            << " glyph geometries/colors matched\n";
}
void font_atlas_surround(const std::filesystem::path &fixture,
                        const std::filesystem::path &maps) {
  const auto font = load_retail_font(maps / "fonts/conduit-12");
  std::ifstream file(fixture);
  check(bool(file), "missing original font atlas clear/copy fixture");
  unsigned words = 0;
  for (std::string row; std::getline(file, row);) {
    std::istringstream in(row);
    std::string character, packed;
    int x, y;
    in >> character >> x >> y >> packed;
    check(!in.fail(), "invalid original glyph atlas fixture row");
    const auto &glyph = font.glyphs.at(std::stoul(character, nullptr, 16));
    const auto pixel = unsigned((int(glyph.y) + y) * font.atlas.width +
                                int(glyph.x) + x) * 4;
    const auto want = std::stoul(packed, nullptr, 16);
    constexpr unsigned shifts[] = {8, 4, 0, 12};
    for (unsigned channel = 0; channel < 4; ++channel)
      if (std::to_integer<unsigned>(font.atlas.pixels.at(pixel + channel)) !=
          ((want >> shifts[channel]) & 15) * 17)
        throw std::runtime_error("font atlas differs from original1F5C0 clear/copy: " +
                                 character + " x=" + std::to_string(x) +
                                 " y=" + std::to_string(y));
    ++words;
  }
  check(words == 896, "incomplete original font atlas clear/copy fixture");
  std::cout << "Original font atlas copy and surround: " << words
            << " texels matched\n";
}
} // namespace
int main(int argc, char **argv) {
  if (argc != 3 || !std::filesystem::exists(argv[1]) ||
      !std::filesystem::exists(std::filesystem::path(argv[2]) / "mainmenu.map"))
    return 77;
  try {
    oracle(argv[1]);
    painter(argv[2]);
    keyboard_labels(std::filesystem::path(argv[1]).parent_path() /
                        "keyboard-labels-original.tsv", argv[2]);
    font_atlas_surround(std::filesystem::path(argv[1]).parent_path() /
                           "frontend-font-atlas-surround-original.tsv", argv[2]);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
