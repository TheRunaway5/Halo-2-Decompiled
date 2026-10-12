#include "halo2/frontend_exit_animation.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
std::array<std::uint32_t, 4> words(const UiKey &key) {
  return {std::bit_cast<std::uint32_t>(key.position.x),
          std::bit_cast<std::uint32_t>(key.position.y),
          std::bit_cast<std::uint32_t>(key.position.z),
          std::bit_cast<std::uint32_t>(key.alpha)};
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("maps and original exit fixture required");
    const std::filesystem::path maps(argv[1]);
    if (!std::filesystem::exists(maps / "mainmenu.map")) return 77;
    Cache cache(maps / "mainmenu.map");
    auto content = cache.load_frontend();
    std::ifstream fixture(argv[2]);
    require(bool(fixture), "missing original exit fixture");
    std::map<std::string, FrontendExitAnimation> timelines;
    unsigned count = 0;
    for (std::string line; std::getline(fixture, line);) {
      if (line.empty() || line.front() == '#') continue;
      std::istringstream input(line);
      std::string timeline;
      unsigned index, kind, time, frame, end, queued;
      std::array<std::uint32_t, 4> expected;
      require(bool(input >> timeline >> index >> kind >> time >> frame >> end >> queued >>
          std::hex >> expected[0] >> expected[1] >> expected[2] >> expected[3]),
          "invalid original exit fixture row");
      auto &native = timelines[timeline];
      const auto key = native.update(content.animations.at(index - 1).outro, time, kind == 0);
      if (words(key) != expected || native.frame != frame || native.end_ms != end ||
          native.queued != bool(queued))
        throw std::runtime_error("exit timing differs from original22E3CD: " + timeline +
                                 " at " + std::to_string(time) + "ms");
      ++count;
    }
    require(fixture.eof() && count == 252, "incomplete original exit fixture");
    std::array<unsigned, 4> active_markers{}, exit_markers{};
    std::ifstream marker_fixture(std::filesystem::path(argv[2]).parent_path() /
                                 "frontend_lobby_exit_markers_original.tsv");
    require(bool(marker_fixture), "missing original exit marker fixture");
    unsigned marker_count = 0;
    for (std::string line; std::getline(marker_fixture, line);) {
      if (line.empty() || line.front() == '#') continue;
      std::istringstream input(line);
      unsigned outgoing, row, selected, before, frame;
      require(bool(input >> outgoing >> row >> selected >> before >> frame) && outgoing < 2 && row < 4,
              "invalid original exit marker fixture");
      (outgoing ? exit_markers : active_markers)[row] = frame;
      ++marker_count;
    }
    require(marker_fixture.eof() && marker_count == 8, "incomplete original exit marker fixture");

    // Copies of the actual authored bitmaps give their submissions distinct
    //identities without inventing texture pixels, geometry or animation data.
    auto probe = [&](unsigned screen, unsigned index, std::uint32_t datum, const char *name) {
      auto &bitmap = content.screens.at(screen).panes.front().bitmaps.at(index);
      auto image = content.images.at(bitmap.datum);
      image.name = name;
      content.images.emplace(datum, std::move(image));
      bitmap.datum = datum;
    };
    probe(23, 0, 0xfffffffd, "test/previous-setup-probe");
    probe(23, 1, 0xfffffffc, "test/previous-null-probe");
    probe(206, 0, 0xfffffffe, "test/incoming-map-probe");
    //The authored Setup list uses skin0; seven is its visible row capacity.
    //Give its existing flag2 end-cap a distinct test identity. Preserve both
    //authored children, their definitions and their original image pages.
    const auto setup_skin = content.screens.at(23).panes.front().lists.front().skin;
    auto &setup_bitmaps = content.skins.at(setup_skin).bitmaps;
    require(setup_skin == 0 && setup_bitmaps.size() == 2,
            "Setup list differs from the retained original skin0 definition");
    const auto source_marker = std::find_if(setup_bitmaps.begin(), setup_bitmaps.end(),
        [](const UiBitmap &bitmap) { return bitmap.flags & 2; });
    require(source_marker != setup_bitmaps.end(), "missing authored Setup end-cap");
    auto &marker = *source_marker;
    auto marker_image = content.images.at(marker.datum);
    marker_image.name = "test/exit-marker-probe";
    marker.datum = 0xfffffffb;
    content.images.emplace(marker.datum, marker_image);
    Mesh mesh;
    FrontendPainter painter(content, maps / "fonts", mesh);
    auto first = [&](const UiDraw &draw, const char *name) {
      for (std::size_t i = 0; i < draw.vertices.size(); i += 6)
        if (mesh.materials.at(draw.vertices[i].material).name == name) return i;
      return draw.vertices.size();
    };
    std::vector<unsigned> marker_materials;
    for (unsigned i = 0; i < mesh.materials.size(); ++i)
      if (mesh.materials[i].name == marker_image.name) marker_materials.push_back(i);
    auto check_markers = [&](const UiDraw &draw, const std::array<unsigned, 4> &frames) {
      unsigned row = 0;
      for (std::size_t i = 0; i < draw.vertices.size(); i += 6) {
        const auto material = draw.vertices[i].material;
        if (mesh.materials.at(material).name != marker_image.name) continue;
        require(row < frames.size(), "unexpected outgoing marker row");
        const auto frame = frames[row++];
        const auto page = !marker_image.sequences.empty() && !marker_image.sequences.front().empty()
            ? marker_image.sequences.front().at(frame).image : frame;
        require(material == marker_materials.at(page), "row marker differs from original focus predicate");
      }
      require(row == frames.size(), "incomplete authored Setup end-cap submissions");
    };
    MenuState state(content);
    state.accept(); state.accept(); state.text("Exit Test"); state.commit_text(); state.back();
    state.select(2); state.accept(); state.accept();
    require(state.frame().screen == 14, "ordinary Split Screen route missed lobby");
    state.select(1); state.accept();
    check_markers(painter.draw(state, 10), active_markers);
    state.accept();
    require(state.frame().screen == 206 && state.frame().previous_lobby_setup &&
        state.frame().previous_lobby_setup->screen == 23 &&
        std::none_of(state.stack().begin(), state.stack().end(),
            [](const MenuFrame &frame) { return frame.screen == 23; }),
        "replacement must retain outgoing Setup without adding Back history");

    for (const auto time : {0u, 125u, 249u, 250u, 251u, 499u, 500u}) {
      const auto draw = painter.draw(state, float(time) / 1000.f);
      require(draw.backdrops.size() == 1 && draw.backdrops.front().fade == 1.f,
              "replacement must keep one full-strength original blur");
      const auto old = first(draw, "test/previous-setup-probe");
      const auto previous_null = first(draw, "test/previous-null-probe");
      const auto current = first(draw, "test/incoming-map-probe");
      const auto at_time = " at " + std::to_string(time) + "ms";
      if (time == 125) check_markers(draw, exit_markers);
      if ((current < draw.vertices.size()) != (time > 0))
        throw std::runtime_error("incoming Map bitmap must follow its authored intro alpha" + at_time);
      if (time < 250) {
        if (!(old >= draw.backdrops.front().before_vertex && old < current))
          throw std::runtime_error("outgoing Setup must follow blur and precede incoming Map" + at_time);
        const auto expected = std::max(0.f, 1.f - float(std::max(time, 1u)) / 250.f);
        if (draw.vertices[old].position.z != expected)
          throw std::runtime_error("outgoing Setup bitmap must use original exit alpha" + at_time);
      } else {
        if (old != draw.vertices.size())
          throw std::runtime_error("zero-alpha outgoing Setup bitmap must be omitted" + at_time);
      }
      //The authored2x2 null bitmap has no animation. Its transparent pixels
      //do not affect the frame; its vertex submission exposes screen lifetime
      //separately from the animated panel's zero alpha after250ms.
      if ((previous_null < draw.vertices.size()) != (time < 500))
        throw std::runtime_error("outgoing Setup removal queue differs" + at_time);
    }
    //Starting a new diagnostic timeline restarts the same source sampler;
    //late crossing at266ms shifts removal to516ms rather than a fixed500ms.
    for (const auto time : {0u, 17u, 124u, 249u, 266u, 499u, 500u, 515u, 516u}) {
      const auto draw = painter.draw(state, float(time) / 1000.f);
      require((first(draw, "test/previous-null-probe") < draw.vertices.size()) == (time < 516),
              "late source exit cadence was replaced by a fixed timeout");
    }
    const auto settled = painter.draw(state, 10);
    require(first(settled, "test/previous-setup-probe") == settled.vertices.size(),
            "settled Map dialog retained outgoing Setup widgets");
    state.back();
    require(state.frame().screen == 14, "Map cancellation restored disposed Setup history");
    const auto returned = painter.draw(state, .125f);
    require(first(returned, "test/previous-setup-probe") == returned.vertices.size(),
            "outgoing Setup leaked into lobby after cancellation");
    state.select(1); state.accept(); state.accept();
    painter.draw(state, .125f);
    state.select(3); state.accept();
    require(state.frame().screen == 14 && state.lobby_view()->map->name == "Ivory Tower",
            "Map acceptance during transition missed retained lobby or selected map");
    std::cout << count << " original exit samples and ordinary Setup/Map lifecycle checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
