#include "halo2/menu_world_depth.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
std::array<std::uint32_t, 2> words(MenuWorldDepthClipPlanes planes) {
  return {std::bit_cast<std::uint32_t>(planes.minimum_w),
          std::bit_cast<std::uint32_t>(planes.maximum_w)};
}
unsigned rejected_inputs() {
  unsigned rejected = 0;
  const MenuWorldDepthClipInput valid{.015625f, 1024.f};
  const auto reject = [&](const MenuWorldDepthClipInput &input) {
    bool caught = false;
    try { (void)menu_world_depth_clip_planes(input); }
    catch (const FormatError &) { caught = true; }
    require(caught, "unsupported menu depth input was silently accepted");
    ++rejected;
  };
  const std::array nonfinite{std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity()};
  for (float value : nonfinite) {
    auto changed = valid; changed.near_distance = value; reject(changed);
    changed = valid; changed.far_distance = value; reject(changed);
  }
  for (float value : {0.f, -0.f, -1.f, std::numeric_limits<float>::denorm_min()}) {
    auto changed = valid; changed.far_distance = value; reject(changed);
  }
  for (auto value : {3u, std::numeric_limits<std::uint32_t>::max()}) {
    auto changed = valid;
    changed.boundary = static_cast<MenuWorldDepthBoundary>(value); reject(changed);
  }
  for (auto mode : {MenuWorldDepthBoundary::near_plane, MenuWorldDepthBoundary::far_plane}) {
    for (float value : nonfinite) {
      auto changed = valid;
      changed.boundary = mode; changed.boundary_distance = value; reject(changed);
    }
  }
  return rejected;
}
}
int main(int argc, char **argv) {
  try {
    require(argc == 2, "menu world depth test requires its unchanged-XBE fixture");
    std::ifstream input(argv[1]);
    require(bool(input), "missing original menu world depth fixture");
    std::string line;
    unsigned cases = 0, scaled_cases = 0;
    std::array<unsigned, 3> modes{};
    while (std::getline(input, line)) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream row(line);
      unsigned id, mode, scaled;
      std::uint32_t near, far, boundary;
      std::array<std::uint32_t, 2> expected_base, expected_submitted;
      row >> id >> near >> far >> mode >> boundary >> scaled;
      for (auto &word : expected_base) row >> word;
      for (auto &word : expected_submitted) row >> word;
      require(bool(row) && id == cases && mode <= 2 && scaled <= 1,
              "invalid original menu depth fixture row");
      std::string extra;
      require(!(row >> extra), "trailing original menu depth fixture data");
      MenuWorldDepthClipInput view{
          std::bit_cast<float>(near), std::bit_cast<float>(far),
          static_cast<MenuWorldDepthBoundary>(mode), std::bit_cast<float>(boundary)};
      require(words(menu_world_depth_clip_planes(view)) == expected_base,
              "menu world clip planes differ from original13450 arithmetic");
      view.scaled_projection = scaled != 0;
      require(words(menu_world_depth_clip_planes(view)) == expected_submitted,
              "menu submitted clip planes differ from original12D50 arithmetic");
      ++modes[mode];
      scaled_cases += scaled;
      ++cases;
    }
    require(input.eof() && cases == 450 && scaled_cases == 225 &&
            modes == std::array<unsigned, 3>{30, 210, 210},
            "incomplete original menu depth fixture coverage");
    const unsigned rejections = rejected_inputs();
    require(rejections == 18, "incomplete menu depth unsupported-input checks");
    std::cout << cases << " unchanged-XBE clip arithmetic cases / " << cases * 4
              << " words exact; " << rejections << " unsupported inputs rejected\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
