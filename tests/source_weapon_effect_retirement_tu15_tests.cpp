#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
namespace {
void need(bool value, const char* why) { if (!value) throw FormatError(why); }
struct Birth {
  SourceEffectColors colors;
  std::uint32_t weapon{}, a_bits{}, b_bits{};
};
}

int main(int argc, char** argv) try {
  need(argc == 3, "Supply prepared MCC map and shared maps");
  const auto scratch = std::getenv("HALO2_TEST_SCRATCH");
  need(scratch && std::filesystem::path(scratch).is_absolute(), "Run through tools/run_test.py");
  MultiplayerOptions options;
  options.map = argv[1]; options.shared_maps = argv[2];
  options.local_players = 0; options.dedicated = true; options.mute = true;
  options.starting_weapon = 5;
  options.variant = UiVariant{"Slayer", 0, {{2, 0}, {10, 0}, {89, 5}}};
  options.source_object_world = true;
  options.source_object_savegame_directory = std::filesystem::path(scratch) / "source";
  MultiplayerPreparedWorld world(options);
  auto& match = world.match;
  auto& native = *world.source_objects;
  need(world.cache->is_mcc() && native.owns_physics() && match.source_engine_owned(),
       "Retirement acceptance requires canonical native MCC authority");
  const auto weapon = world.cache->slayer_starting_weapon(5);
  const auto& definition = world.content.weapons.at(weapon);
  need(definition.smg && !definition.firing_effects.empty(), "Authored SMG firing bank is absent");
  const auto effect = definition.firing_effects.front().effects[0];
  const auto metadata = world.cache->load_source_tag_metadata(effect);
  need(Bytes(metadata.bytes_at(metadata.source_address(), 4)).u32(0) & 2u,
       "Fixture must exercise the original one-second initial effect update");
  match.configure_replicated_first_person(world.cache->load_first_person_binding(weapon, 2));
  const auto& memory = native.memory();
  const auto word = [&](auto address) { return Bytes(memory.view(address, 4)).u32(0); };
  const SourceRawDatumPoolServices storage{.read = [&](auto address, auto size) {
    return memory.view(address, size);
  }};
  std::map<std::uint32_t, Birth> births;
  std::optional<Birth> active;
  unsigned requests{}, retired{};
  native.observe_weapon_effect_phase([&](bool entering, const auto& abi, const auto&,
                                        const auto& trace, const auto&) {
    if (entering) {
      active.reset();
      if (abi.gp[0] == effect)
        active = Birth{{}, abi.gp[1], word(abi.gp[4] + 8u), word(abi.gp[4] + 12u)};
      return;
    }
    if (!active) return;
    ++requests;
    const std::set<std::uint32_t> instructions(trace.begin(), trace.end());
    // 178E6F passed the allocation gate, 179063 ran the initial update,
    // and 179083..17908A returned NONE after the original retirement check.
    need(instructions.contains(0x178e75u) && instructions.contains(0x179063u) &&
         instructions.contains(0x17907au) && instructions.contains(0x179085u),
         "Native SMG did not complete its actual born-then-retired factory path");
    need(abi.gp[0] == null_datum, "Original retired effect return was changed");
    ++retired;
    active.reset();
  });
  native.tu15_world().observe_physics_stores([&](auto, const auto& store) {
    if (!active || store.instruction != 0x17b0a9u || store.written.size() != 4) return;
    const auto body = store.address - 0x108u;
    const auto pool = word(0x4f720cu), rows = word(pool + 0x44u);
    need(word(pool + 0x24u) == 400u && body >= rows && (body - rows) % 400u == 0,
         "Observed original color store escaped the real effect pool");
    const auto index = (body - rows) / 400u;
    const auto instance = ((word(body) & 0xffffu) << 16u) | index;
    need(index < 256u && source_raw_datum_lookup(pool, instance, storage) == body &&
         word(body + 4u) == effect, "Observed colors lost the live full birth generation");
    auto birth = *active;
    birth.colors = {word(body + 0x104u), word(body + 0x108u)};
    need(births.emplace(instance, birth).second, "Original effect birth was observed twice");
  });
  const auto player = match.join("Native Audio");
  std::array<Controls, multiplayer_capacity> controls{};
  for (unsigned tick = 0; tick < 100; ++tick) {
    controls[player].fire = tick >= 12 && tick < 28;
    controls[player].reload = tick == 40;
    match.step(controls);
  }
  native.observe_weapon_effect_phase({});
  native.tu15_world().observe_physics_stores({});
  const auto snapshot = match.state();
  unsigned published{};
  for (const auto& event : snapshot.weapon_effects) {
    if (event.effect != effect) continue;
    const auto found = births.find(event.source_instance);
    need(found != births.end(), "Weapon journal invented an unobserved native birth");
    const auto& birth = found->second;
    need(event.source_scheduled && event.source_presentation && event.source_locations_observed &&
         !event.source_locations.empty() && event.weapon == birth.weapon &&
         event.a_bits == birth.a_bits && event.b_bits == birth.b_bits &&
         event.colors.a == birth.colors.a && event.colors.b == birth.colors.b,
         "Weapon journal lost actual birth colors, controls, owner or WORLD locations");
    need(!source_raw_datum_lookup(word(0x4f720cu), event.source_instance, storage),
         "Fixture did not retire its original allocation");
    ++published;
  }
  std::cout << "native requests=" << requests << " actual births=" << births.size()
            << " original NONE returns=" << retired << " published births=" << published << '\n';
  need(requests > 0 && births.size() == requests && retired == requests && published == requests,
       "Born-then-retired original weapon effects were lost before replication");
  std::cout << "PASS actual native birth colors and locations survive original initial-update retirement\n";
  return 0;
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n';
  return 1;
}
