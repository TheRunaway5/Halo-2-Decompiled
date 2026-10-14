#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/scene.hpp"
#include <iostream>
#include <set>

int main(int argc, char **argv) {
  try {
    if (argc != 2) throw halo2::FormatError("original map directory required");
    const auto map = std::filesystem::path(argv[1]) / "headlong.map";
    if (!std::filesystem::is_regular_file(map)) return 77;
    halo2::Cache cache(map, argv[1]);
    const auto content = cache.load_content(0, true);
    std::set<int> empty_vehicles;
    unsigned vehicle_templates = 0;
    for (const auto &object : content.objects) {
      if (object.type != 1) continue;
      if (object.definition == halo2::null_datum) {
        empty_vehicles.insert(object.placement);
      } else if (object.netgame_row >= 0 || object.model_attachment) {
        if (cache.tag_payload({object.definition}).empty())
          throw halo2::FormatError("vehicle template lost its resident definition");
        ++vehicle_templates;
      }
    }
    // The owned Headlong scenario retains seven unused vehicle rows with
    // palette index -1. Retail BF0F0 returns NONE before constructing them.
    if (empty_vehicles != std::set<int>{0, 1, 2, 3, 4, 5, 6} || !vehicle_templates)
      throw halo2::FormatError("authored empty placements or valid templates lost");
    halo2::SceneObjects scene(content, 0, halo2::SourceSceneGameContext{2, 2});
    for (const auto &object : scene.objects())
      if (object.handle != halo2::null_datum &&
          content.objects.at(object.placement).definition == halo2::null_datum)
        throw halo2::FormatError("empty vehicle placement acquired a live owner");
    bool rejected = false;
    try { (void)cache.tag_payload({halo2::null_datum}); }
    catch (const halo2::FormatError &) { rejected = true; }
    if (!rejected) throw halo2::FormatError("NONE became resident tag metadata");
    std::cout << "Headlong: seven authored empty vehicle placements retained, "
              << vehicle_templates << " resident templates; NONE has no live owner or payload\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
