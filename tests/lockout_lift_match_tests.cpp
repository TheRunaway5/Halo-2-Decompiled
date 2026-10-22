#include "helpers/gameplay_fixture.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 4 || !std::filesystem::is_regular_file(argv[1]))
      return 77;
    Cache cache(argv[1], argv[2], OriginalData::load_xbe(argv[3]));
    auto content = cache.load_content(0, true);
    const auto tag = std::find_if(
        cache.tags().begin(), cache.tags().end(), [](const Tag &t) {
          return t.group == "bloc" &&
                 t.name == "scenarios\\objects\\multi\\lockout\\lockout_lift_"
                           "tube\\lockout_lift_tube";
        });
    if (tag == cache.tags().end())
      throw FormatError("Authored Lockout lift tag absent");
    const auto at = std::find_if(
        content.objects.begin(), content.objects.end(),
        [&](const ScenarioObject &o) { return o.definition == tag->datum; });
    if (at == content.objects.end())
      throw FormatError("Authored Lockout lift placement absent");
    // Original resident phmo inputs: lower phantom's actual polyhedron
    // center. This is a diagnostic position input, never a supplied contact
    // or force. The original GJK/world response is a separate parity gate.
    const auto object = cache.load_source_tag_metadata(at->definition);
    const auto model = cache.load_source_tag_metadata(
        Bytes(object.bytes_at(object.source_address(), object.size()))
            .u32(0x38));
    const auto physics = cache.load_source_tag_metadata(
        Bytes(model.bytes_at(model.source_address(), model.size())).u32(0x24));
    const Bytes p(physics.bytes_at(physics.source_address(), physics.size()));
    if (p.u32(0x70) != 2 || p.u32(0x108) != 2)
      throw FormatError("Original Lockout phantom geometry changed");
    const Bytes hull(physics.bytes_at(p.u32(0x74), 0x100));
    Transform local;
    local.position = {hull.f32(0x40), hull.f32(0x44), hull.f32(0x48)};
    const auto basis = authored_basis(at->rotation);
    const auto root = source_object_transform(at->position, basis.forward,
                                              basis.up, at->scale, false);
    Spawn entrance{};
    entrance.position = compose_transform(root, local).position;
    auto rules = slayer_rules(UiVariant{"Slayer", 0, {{89, 5}}});
    rules.weapon = cache.slayer_starting_weapon(rules.starting_weapon);
    MultiplayerMatch match(content, 0, cache.spawns(), rules);
    const auto slot = match.join("Authored lift interior regression");
    test::place_player(match,slot,entrance);
    const auto initial = match.unit(slot).body.position;
    std::array<Controls, 16> controls{};
    controls[slot].yaw = match.unit(slot).yaw;
    float peak = initial.z, fastest = 0;
    for (unsigned n = 0; n < 30; ++n) {
      match.step(controls);
      peak = std::max(peak, match.unit(slot).body.position.z);
      fastest = std::max(fastest, match.unit(slot).body.velocity.z);
    }
    std::cout << "Authored lower hull center=" << initial.x << ',' << initial.y
              << ',' << initial.z << " peakZ=" << peak
              << " upward velocity=" << fastest << '\n';
    if (!(peak > initial.z && fastest > 0))
      throw FormatError("RED: live native Match::step never lifts the player "
                        "from Lockout's authored lower phantom interior");
    std::cout << "PASS native lift travel; exact original contact/force/world "
                 "parity remains separately required\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
