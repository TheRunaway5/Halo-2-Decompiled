#include "halo2/multiplayer.hpp"
#include "halo2/world_effects.hpp"
#include <iostream>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw FormatError(message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 4)
      throw FormatError("Supply actual MIDSHIP SHARED_MAPS UPDATE_XBE");
    auto cache = std::make_shared<Cache>(argv[1], argv[2],
                                         OriginalData::load_xbe(argv[3]));
    const auto content = cache->load_content(0, true);
    UiVariant variant{"Slayer", 0, {{89, 5}}};
    auto rules = slayer_rules(variant);
    rules.weapon = cache->slayer_starting_weapon(rules.starting_weapon);
    MultiplayerMatch match(content, 0, cache->spawns(), rules);
    match.configure_grenades(cache, *cache->original_data()->grenade_constants);
    const auto player = match.join("Plasma effects");
    std::cout << "initial yaw " << match.unit(player).yaw << '\n';
    check(match.unit(player).grenades == std::array<std::uint8_t, 2>{0, 2},
          "Actual Midship spawn must provide plasma inventory");
    Mesh mesh;
    NativeWorldEffects effects(*cache, mesh, match.simulation().collision(),
                               0x12345678);
    MuzzleEffectView view{};
    view.forward = {1, 0, 0};
    view.up = {0, 0, 1};
    view.width = 640;
    view.height = 480;
    view.orientation = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    std::array<Controls, 16> controls{};
    controls[player].yaw = match.unit(player).yaw;
    controls[player].pitch = -1.2f;
    std::uint64_t serial{};
    std::size_t peak_particles{}, peak_systems{},peak_lights{};
    bool directional{}, electric_burst{}, ring{};
    const auto particle = [&](std::string_view name) {
      const auto t = std::find_if(
          cache->tags().begin(), cache->tags().end(),
          [&](const auto &t) { return t.group == "prt3" && t.name == name; });
      check(t != cache->tags().end(), "Authored plasma particle missing");
      const auto material = cache->load_particle_material(t->datum, 15, 0, 0);
      check(material.bitmaps[0].has_value(), "Plasma texture missing");
      return material.bitmaps[0]->datum;
    };
    const auto electric_bitmap =
        particle("effects\\objects\\weapons\\grenade\\plasma_grenade\\plasma_"
                 "grenade_electric_burst");
    const auto ring_bitmap = particle("effects\\generic\\smoke\\smoke_fiery");
    for (unsigned tick = 0; tick < 200; ++tick) {
      controls[player].grenade = tick == 0;
      match.step(controls);
      const auto state = match.state();
      for (const auto &e : state.grenade_detonations)
        if (e.serial > serial) {
          serial = e.serial;
          std::cout << "detonation tick " << e.tick << " position "
                    << e.position.x << ',' << e.position.y << ','
                    << e.position.z << '\n';
          const auto projectile = cache->load_source_tag_metadata(e.definition);
          const Bytes root(projectile.bytes_at(projectile.source_address(),
                                               projectile.size()));
          check(e.effect == root.u32(0xfc),
                "Actual plasma detonation did not retain its ground effect");
          effects.spawn(e.effect, e.projectile, e.tick,
                        {e.position, e.up, e.forward, e.forward, e.up, false});
        }
      effects.advance(state.tick, tick_seconds);
      const auto d = effects.diagnostics();
      peak_systems = std::max(peak_systems, d.systems);
      peak_particles = std::max(peak_particles, d.particles);
      peak_lights=std::max(peak_lights,d.lights);
      for(const auto &light:effects.lights())
        check(light.owner==0xffffffffu && light.effect_owner!=0xffffffffu && (light.state.brightness==0 || (light.state.diffuse.x<light.state.diffuse.y && light.state.diffuse.y==light.state.diffuse.z)),
              "Actual plasma lost its cyan source light state");
      const auto draws = effects.draw(view, 0, tick_seconds);
      for (const auto &draw : draws) {
        directional |= draw.material->vertex.classification == 8;
        if (draw.material->bitmaps[0]) {
          electric_burst |= draw.material->bitmaps[0]->datum == electric_bitmap;
          ring |= draw.material->bitmaps[0]->datum == ring_bitmap;
        }
      }
      view.width = 1920;
      view.height = 1080;
      const auto large = effects.draw(view, 0, tick_seconds);
      check(large.size() == draws.size(), "Resolution changed plasma count");
      for (unsigned n = 0; n < draws.size(); ++n)
        check(large[n].vertex.attributes == draws[n].vertex.attributes,
              "Drawing a second view changed plasma state");
      view.width = 640;
      view.height = 480;
    }
    for (const auto &s : effects.diagnostics().unavailable)
      std::cerr << "BOUNDARY " << s << '\n';
    check(serial == 1, "Actual Midship throw never detonated");
    check(peak_systems == 8, "Plasma explosion lost an authored system");
    check(peak_lights==1,"Actual plasma lost its authored explosion light");
    check(peak_particles > 0 && directional && electric_burst && ring,
          "Actual plasma burst/ring/directional presentation missing");
    effects.advance(6000, tick_seconds);
    check(effects.diagnostics().particles == 0, "Plasma particles leaked");
    check(effects.lights().empty(),"Plasma light lifetime did not expire");
    std::cout << "PASS actual Midship plasma: systems=" << peak_systems
              << " peak particles=" << peak_particles
              << "; resolution, per-view drawing and retirement verified\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
