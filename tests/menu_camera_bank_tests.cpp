#include "halo2/menu_projection.hpp"
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
void unsupported(const Camera &camera) {
  bool rejected = false;
  try { (void)menu_render_view_bank(camera); }
  catch (const FormatError &) { rejected = true; }
  require(rejected, "unsupported title camera bank was silently reconstructed");
}
Camera observer_camera(std::uint32_t degrees,
                       const std::array<std::uint32_t, 9> &words) {
  const auto value = [&](unsigned i) { return std::bit_cast<float>(words[i]); };
  Camera camera;
  camera.position = {value(0), value(1), value(2)};
  camera.forward = {value(3), value(4), value(5)};
  camera.up = {value(6), value(7), value(8)};
  camera.field_of_view = std::bit_cast<float>(degrees);
  camera.horizontal_field_of_view = true;
  camera.oriented = true;
  return camera;
}
void unsupported_cameras(Camera valid) {
  auto changed = valid;
  changed.projection_vertical_angle.reset(); unsupported(changed);
  changed = valid; changed.oriented = false; unsupported(changed);
  changed = valid; changed.horizontal_field_of_view = false; unsupported(changed);
  changed = valid; changed.projection_clip_scale.x = 1; unsupported(changed);
  changed = valid; changed.projection_clip_scale.y = 1; unsupported(changed);
  changed = valid; changed.projection_clip_offset.x = .125f; unsupported(changed);
  changed = valid; changed.projection_clip_offset.y = .125f; unsupported(changed);
  for (const float angle : {0.f, -1.f, 4.f,
       std::numeric_limits<float>::infinity(),
       std::numeric_limits<float>::quiet_NaN()}) {
    changed = valid; changed.projection_vertical_angle = angle; unsupported(changed);
  }
  changed = valid; changed.position.x = std::numeric_limits<float>::quiet_NaN(); unsupported(changed);
  changed = valid; changed.forward.y = std::numeric_limits<float>::infinity(); unsupported(changed);
  changed = valid; changed.up.z = std::numeric_limits<float>::quiet_NaN(); unsupported(changed);
}
}
int main(int argc, char **argv) {
  try {
    require(argc == 2, "title camera-bank test requires its original fixture");
    std::ifstream input(argv[1]);
    require(bool(input), "missing original title camera-bank fixture");
    std::string line;
    unsigned source_cases = 0, gpu_cases = 0;
    Camera valid;
    while (std::getline(input, line)) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream row(line);
      char kind;
      unsigned tick, widescreen;
      std::uint32_t degrees, angle;
      std::array<std::uint32_t, 9> camera_words;
      row >> kind >> tick >> degrees >> widescreen;
      for (auto &word : camera_words) row >> word;
      row >> angle;
      require(bool(row) && (kind == 'C' || kind == 'G') && widescreen <= 1,
              "invalid original title camera-bank input");
      const auto observer = observer_camera(degrees, camera_words);
      const auto render = menu_render_camera(observer);
      require(render.field_of_view == observer.field_of_view &&
              render.horizontal_field_of_view && render.oriented,
              "menu render conversion changed the observer camera policy");
      require(std::bit_cast<std::array<std::uint32_t, 3>>(render.position) ==
              std::bit_cast<std::array<std::uint32_t, 3>>(observer.position) &&
              std::bit_cast<std::array<std::uint32_t, 3>>(render.forward) ==
              std::bit_cast<std::array<std::uint32_t, 3>>(observer.forward) &&
              std::bit_cast<std::array<std::uint32_t, 3>>(render.up) ==
              std::bit_cast<std::array<std::uint32_t, 3>>(observer.up),
              "menu render conversion changed the raw observer eye or orientation");
      require(render.projection_vertical_angle &&
              std::bit_cast<std::uint32_t>(*render.projection_vertical_angle) == angle,
              "menu render conversion differs from the original title render angle");
      const auto bank = menu_render_view_bank(render);
      const auto actual_bank = std::bit_cast<std::array<std::uint32_t, 48>>(bank);
      if (kind == 'C') {
        std::array<std::uint32_t, 48> expected_bank;
        for (auto &word : expected_bank) row >> word;
        require(bool(row), "truncated original title render bank");
        require(actual_bank == expected_bank,
                "production title camera-bank helper differs from original instructions");
        ++source_cases;
      } else {
        require(tick == 181 && std::bit_cast<float>(degrees) == 70 && !widescreen,
                "unexpected retained original title GPU camera");
        ++gpu_cases;
      }
      std::array<std::uint32_t, 48> expected_shader;
      for (auto &word : expected_shader) row >> word;
      require(bool(row), "truncated original title shader camera bank");
      const auto actual_shader = std::bit_cast<std::array<std::uint32_t, 48>>(
          source_view_shader_bank(bank, 1024.f, render.position, {0, 0, 640, 480}, false));
      require(actual_shader == expected_shader,
              "production title camera c0..11 differs from original instructions or GPU capture");
      std::string extra;
      require(!(row >> extra), "trailing original title camera-bank data");
      valid = render;
    }
    require(input.eof(), "invalid original title camera-bank fixture");
    require(source_cases == 48 && gpu_cases == 1, "incomplete original title camera-bank fixture");
    unsupported_cameras(valid);
    std::cout << source_cases << " production title render banks and c0..11 / "
              << source_cases * 96 << " words exact; " << gpu_cases
              << " retained original GPU bank / 48 words exact; 15 unsupported inputs rejected\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
