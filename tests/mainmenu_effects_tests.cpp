#include "halo2/world_effects.hpp"
#include "halo2/math.hpp"
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw FormatError("Supply original mainmenu.map");
    Cache cache(argv[1]);
    const auto content = cache.load_content(0);
    CollisionWorld collision(content.collision);
    Mesh mesh;
    NativeWorldEffects effects(cache, mesh, collision, 0x12345678);
    Mesh batched_mesh;
    NativeWorldEffects batched(cache, batched_mesh, collision, 0x12345678);
    const auto placements = effects.attach_scenery(content);
    check(batched.attach_scenery(content) == placements, "Title attachment import changed across owners");
    std::cout << "Title effect attachments=" << placements << '\n';
    check(placements == 10, "Original title lost its scenery effect attachments");
    MuzzleEffectView view{};
    view.width = 640; view.height = 480;
    view.forward = {1,0,0}; view.up = {0,0,1};
    view.orientation = {1,0,0,0,1,0,0,0,1};
    std::set<std::string> visible;
    std::size_t peak{};
    for (unsigned tick = 0; tick < 1800; ++tick) {
      effects.advance(tick, 1.f/30.f);
      peak = std::max(peak, effects.diagnostics().particles);
      if (tick % 30 == 0) {
        const auto draws = effects.draw(view, 0, 1.f/30.f);
        for (const auto &draw : draws)
          if (draw.material->bitmaps[0])
            for (const auto &tag : cache.tags())
              if (tag.datum == draw.material->bitmaps[0]->datum) visible.insert(tag.name);
        const auto repeated = effects.draw(view, 0, 1.f/30.f);
        check(draws.size() == repeated.size(), "Drawing title effects advanced their simulation");
        view.width = 1920; view.height = 1080;
        const auto wide = effects.draw(view, 0, 1.f/30.f);
        check(draws.size() == wide.size(), "Title resolution changed particle count");
        for (unsigned n = 0; n < draws.size(); ++n)
          check(draws[n].vertex.attributes == repeated[n].vertex.attributes &&
                draws[n].vertex.attributes == wide[n].vertex.attributes &&
                draws[n].scene_sort_depth == wide[n].scene_sort_depth,
                "Drawing title effects changed their live state or location sort depth");
        view.width = 640; view.height = 480;
      }
    }
    for (const auto &boundary : effects.diagnostics().unavailable) std::cerr << boundary << '\n';
    check(effects.diagnostics().unavailable.empty(), "Title flak has an unresolved authored component");
    for (const auto &name : visible) std::cout << "Rendered texture " << name << '\n';
    check(peak > 0, "Title has no live particles");
    check(effects.diagnostics().particles > 0, "Title flak stopped instead of looping");
    effects.advance(1799, 1.f/30.f);
    batched.advance(1799, 1.f/30.f);
    const auto single = effects.draw(view, 0, 1.f/30.f);
    const auto batch = batched.draw(view, 0, 1.f/30.f);
    check(single.size() == batch.size(), "Batched title ticks lost particles");
    for (unsigned n = 0; n < single.size(); ++n)
      check(single[n].vertex.attributes == batch[n].vertex.attributes &&
            single[n].scene_sort_depth == batch[n].scene_sort_depth,
            "Batched title ticks changed effect history or RNG consumption");
    for (const auto *name : {"effects\\bitmaps\\weapons_fire\\shrapnel_sparks",
          "effects\\bitmaps\\energy\\c_electric_discharge",
          "effects\\bitmaps\\energy\\smoke_white_c_generic"})
      check(visible.contains(name), "Title never rendered an authored flak death-explosion component");
    std::cout << "PASS five original scenery placements with ten flak attachments, repeated death explosions, peak particles=" << peak << '\n';
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
