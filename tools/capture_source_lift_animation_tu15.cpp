#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_model_palette.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using Json = nlohmann::json;
void check(bool v, const char *message) {
  if (!v)
    throw FormatError(message);
}
// This fixture declares only a starting position and controller input. The
// original source world owns entry, field membership, forces and landing.
Spawn lift_entrance(const Cache &cache, const LevelContent &content,
                    Vec3 &target) {
  constexpr auto name = "scenarios\\objects\\multi\\lockout\\lockout_lift_"
                        "tube\\lockout_lift_tube";
  const auto tag =
      std::find_if(cache.tags().begin(), cache.tags().end(), [&](const Tag &t) {
        return t.group == "bloc" && t.name == name;
      });
  check(tag != cache.tags().end(), "Authored Lockout lift absent");
  const auto at =
      std::find_if(content.objects.begin(), content.objects.end(),
                   [&](const auto &o) { return o.definition == tag->datum; });
  check(at != content.objects.end(), "Authored Lockout lift placement absent");
  const auto object = cache.load_source_tag_metadata(at->definition);
  const auto model = cache.load_source_tag_metadata(
      Bytes(object.bytes_at(object.source_address(), object.size())).u32(0x38));
  const auto physics = cache.load_source_tag_metadata(
      Bytes(model.bytes_at(model.source_address(), model.size())).u32(0x24));
  const Bytes p(physics.bytes_at(physics.source_address(), physics.size()));
  check(p.u32(0x70) == 2 && p.u32(0x108) == 2,
        "Original Lockout lift geometry changed");
  const auto basis = authored_basis(at->rotation);
  const auto root = source_object_transform(at->position, basis.forward,
                                            basis.up, at->scale, false);
  bool selected = false;
  float radius{};
  for (unsigned n = 0; n < p.u32(0x108); ++n) {
    const Bytes phantom(physics.bytes_at(p.u32(0x10c) + n * 0x20u, 0x20));
    check(phantom.u16(0xc) == 4 && phantom.u16(0xe) < p.u32(0x70),
          "Original Lockout lift is not an authored hull");
    // Whole1F31E0 type4: phmo+70, stride100, shape+20.
    const Bytes hull(
        physics.bytes_at(p.u32(0x74) + phantom.u16(0xe) * 0x100u, 0x100));
    Transform local;
    local.position = {hull.f32(0x40), hull.f32(0x44), hull.f32(0x48)};
    const auto center = compose_transform(root, local).position;
    if (!selected || center.z < target.z) {
      target = center;
      radius = hull.f32(0x2c);
      selected = true;
    }
  }
  check(selected && std::isfinite(radius) && radius >= 0,
        "Invalid lift entrance geometry");
  if (radius == 0)
    target.z = std::min(target.z, root.position.z);
  const CollisionWorld terrain(content.collision);
  const float distance = radius > 0 ? radius + 2 * content.player.radius
                                    : 2 * content.player.height;
  for (const auto direction :
       {basis.forward, basis.left, basis.forward * -1.f, basis.left * -1.f,
        normalized(basis.forward + basis.left),
        normalized(basis.forward - basis.left),
        normalized(basis.left - basis.forward),
        normalized((basis.forward + basis.left) * -1.f)}) {
    auto position = target + direction * distance;
    const auto floor = terrain.raycast(
        position + Vec3{0, 0, 2 * content.player.height}, {0, 0, -1},
        (radius > 0 ? 4.f : 8.f) * content.player.height);
    if (!floor || floor->normal.z <= 0)
      continue;
    position.z = floor->position.z + content.player.radius;
    if (terrain.overlaps(position, content.player.height,
                         content.player.radius))
      continue;
    Spawn spawn;
    spawn.position = position;
    spawn.facing = std::atan2(target.y - position.y, target.x - position.x);
    return spawn;
  }
  throw FormatError("No clear authored lift-approach floor");
}
int main(int argc, char **argv) try {
  check(argc == 6 || argc == 7,
        "Supply MAP SHARED TU15 OUTPUT human|elite [STARTING_WEAPON]");
  if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))
    return 77;
  const auto root = std::filesystem::absolute(argv[4]);
  std::filesystem::create_directories(root);
  std::filesystem::path output;
  for (unsigned attempt = 0;; ++attempt) {
    output = root / ("capture-" + std::to_string(attempt));
    if (std::filesystem::create_directory(output))
      break;
  }
  std::cout << "Capture directory=" << output << std::endl;
  MultiplayerOptions o;
  o.map = argv[1];
  o.shared_maps = argv[2];
  o.gameplay_xbe = argv[3];
  o.source_object_savegame_directory = output / "source-savegames";
  o.starting_weapon = argc == 7 ? std::stoul(argv[6]) : 5;
  o.session_random_seed = 0x12345678;
  o.defer_initial_player_spawn = true;
  const bool elite = std::string_view(argv[5]) == "elite";
  check(elite || std::string_view(argv[5]) == "human",
        "Invalid player species");
  std::array<std::uint8_t, 9> appearance{};
  appearance[4] = elite ? 1 : 0;
  o.selected_profiles = {native_selected_player_configuration(
      "Source lift animation", appearance, -1)};
  Cache cache(o.map, o.shared_maps, OriginalData::load_xbe(o.gameplay_xbe));
  const auto content = cache.load_content(0, true);
  Vec3 target;
  const auto spawn = lift_entrance(cache, content, target);
  o.native_spawn_inputs = {spawn};
  std::vector<std::uint32_t> players;
  o.player_birth_owner = [&](MultiplayerPlayer &p, unsigned) {
    players.push_back(p.source_datum);
    p.source_flags = 8;
  };
  o.pre_player_update_owner = [&](MultiplayerMatch &m) {
    for (const auto h : players)
      if (const auto *p = m.player_from_datum(h); p && !(p->source_flags & 1u))
        m.apply_source_player_activation(h, p->source_flags,
                                         p->source_flags | 1u);
  };
  MultiplayerPreparedWorld w(o);
  check(w.source_objects && w.source_objects->owns_physics(),
        "Actual original physics owner missing");
  auto &source = *w.source_objects;
  auto &m = w.match;
  std::ofstream frames(output / "frames.jsonl"),
      forces(output / "forces.jsonl");
  unsigned tick = 0, deliveries = 0;
  std::uint32_t unit = null_datum;
  source.observe_phantom_phase([&](bool entering, const auto &abi,
                                   const SourceRuntimeMemory &bank) {
    if (!entering)
      return;
    const auto word = [&](auto p) { return Bytes(bank.view(p, 4)).u32(0); };
    const auto component = word(abi.gp[4] + 4u), owner = word(component + 8u);
    const auto *tag =
        w.cache->find_tag(TagHandle{source.object_definition(owner)});
    if (!tag || tag->name != "scenarios\\objects\\multi\\lockout\\lockout_lift_"
                             "tube\\lockout_lift_tube")
      return;
    bool member = false;
    if (const auto header = word(component + 0x94u)) {
      const auto rows = word(header), count = word(header + 4u);
      check(count <= 4096, "Invalid original phantom membership extent");
      for (unsigned n = 0; n < count; ++n)
        member |= word(rows + n * 8u + 4u) == unit;
    }
    if (member)
      ++deliveries;
    forces << Json{{"tick", tick},
                   {"lift", owner},
                   {"component", component},
                   {"player_member", member},
                   {"phase", "Original scheduled1DA750 entry"}}
                  .dump()
           << '\n';
  });
  struct Unobserve {
    MultiplayerObjectRuntime &s;
    ~Unobserve() { s.observe_phantom_phase({}); }
  } unobserve{source};
  std::array<Controls, multiplayer_capacity> controls{};
  float initial = spawn.position.z, peak = initial;
  bool entered = false, air = false;
  unsigned ascent_airborne = 0, landing_contacts = 0, landing_animations = 0;
  std::set<std::uint32_t> activities, ascent_frames;
  std::set<std::uint64_t> air_poses;
  const auto word = [&](auto p) {
    return Bytes(source.memory().view(p, 4)).u32(0);
  };
  for (tick = 0; tick < 300; ++tick) {
    auto &input = controls[0];
    input.yaw = spawn.facing;
    input.forward = tick >= 10 ? 1.f : 0.f;
    input.pitch = entered ? .25f : 0.f;
    m.step(controls);
    const auto &u = m.unit(0);
    unit = u.source_datum;
    check(source.object_definition(unit) ==
              content.player_representations[elite ? 3 : 2].biped.datum,
          "Source biped differs from requested species");
    check(u.weapon.definition ==
              w.cache->slayer_starting_weapon(o.starting_weapon),
          "Held source weapon differs from requested starting weapon");
    const auto motion = source.biped_motion(unit);
    peak = std::max(peak, motion.position.z);
    if (peak - initial > content.player.height)
      entered = true;
    const auto body = source.source_object_body(unit);
    const Bytes b(source.memory().view(body, 0x350));
    const auto extension = std::bit_cast<std::int16_t>(b.u16(0x12a));
    check(extension > 0, "Missing original unit animation extension");
    const auto manager = body + extension;
    const auto activity = word(manager + 8u), requested = word(manager + 0x7cu);
    const bool supported = (b.u32(0x348) & 1u) == 0;
    check(supported == u.body.grounded, "Source support was not published");
    if (entered && !supported) {
      air = true;
      if (motion.velocity.z > 0 && activity == 0x0800001e) {
        ++ascent_airborne;
        ascent_frames.insert(word(manager + 0x1cu));
      }
    }
    if (air && supported) {
      ++landing_contacts;
      air = false;
    }
    activities.insert(activity);
    if (entered && supported &&
        (activity == 0x0900001fu || activity == 0x09000020u))
      ++landing_animations;
    auto state = m.state();
    Json models = Json::array();
    source.begin_model_frame();
    source.render_model_view([&](PhysicsLiveWorldTu15 &s) {
      for (const auto owner : {unit, u.weapon.source_datum}) {
        const auto physical = source.world_nodes(owner);
        const auto published = std::find_if(
            state.source_body_poses.begin(), state.source_body_poses.end(),
            [&](const auto &p) { return p.object == owner; });
        check(published != state.source_body_poses.end() &&
                  published->nodes.size() == physical.size() &&
                  !std::memcmp(published->nodes.data(), physical.data(),
                               physical.size() * sizeof(Transform)),
              "Published pose differs from current source WORLD nodes");
        const auto collector = s.build_world_model(owner, 0, 1, false, 0, 0);
        const Bytes c(collector.bytes);
        const auto completed = s.rendered_world_nodes(owner);
        check(completed && completed->size() == physical.size(),
              "Completed render nodes missing");
        for (const auto &node : *completed)
          for (const auto v :
               {node.basis[0], node.basis[1], node.basis[2], node.position})
            for (float f : {v.x, v.y, v.z})
              check(std::isfinite(f), "Nonfinite completed source node");
        std::uint64_t hash = 14695981039346656037ull;
        const auto root = inverse_transform(completed->front());
        for (const auto &node : *completed) {
          const auto local = compose_transform(root, node);
          for (const auto v :
               {local.basis[0], local.basis[1], local.basis[2], local.position})
            for (float f : {v.x, v.y, v.z})
              hash =
                  (hash ^ std::bit_cast<std::uint32_t>(f)) * 1099511628211ull;
        }
        if (owner == unit && entered && !supported)
          air_poses.insert(hash);
        const auto meta = [&](std::uint32_t t) {
          return word(word(0x4f0414u) + (t & 65535u) * 16u + 8u);
        };
        const auto definition = source.object_definition(owner);
        const auto mode = word(meta(word(meta(definition) + 0x38u)) + 4u),
                   rm = meta(mode);
        const auto count = word(rm + 0x48u), records = word(rm + 0x4cu);
        std::vector<Transform> inverses(count);
        for (unsigned n = 0; n < count; ++n)
          std::memcpy(&inverses[n],
                      s.memory().view(records + n * 96u + 0x28u, 52).data(),
                      52);
        auto palette = source_model_skin_palette(*completed, inverses);
        const auto lod = c.u16(0x168);
        check(lod < 6, "Invalid original geometry LOD");
        append_source_model_compounds(
            palette,
            w.cache->load_source_model_palette_definition(mode).compounds[lod]);
        const auto token = c.u32(4);
        check(token != null_datum, "Original builder omitted palette");
        const auto output = 0x49433cu + (token & 0x0fffffffu);
        const auto regions = Bytes(s.memory().view(output + 2u, 2)).u16(0);
        unsigned matrices = 0;
        for (unsigned region = 0; region < regions; ++region) {
          const auto selected =
              std::to_integer<unsigned>(c.slice(0x75u + region, 1).front());
          if (selected == 255)
            continue;
          const auto section = word(rm + 0x28u) + selected * 92u,
                     resident = word(section + 0x34u);
          const auto maps = word(resident + 0x64u),
                     mapping = word(resident + 0x68u);
          const Bytes range(s.memory().view(output + 4u + region * 4u, 4));
          const auto first = range.u16(0), n = range.u16(2);
          for (unsigned i = 0; i < n; ++i) {
            const auto node = maps ? std::to_integer<unsigned>(
                                         s.memory().view(mapping + i, 1)[0])
                                   : i;
            check(node < palette.size(),
                  "Original palette node map escapes completed palette");
            check(!std::memcmp(s.memory()
                                   .view(output + 0x44u + (first + i) * 48u, 48)
                                   .data(),
                               palette[node].data(), 48),
                  "Completed render nodes differ from actual original uploaded "
                  "palette");
            ++matrices;
          }
        }
        check(matrices > 0, "No completed palette matrices checked");
        models.push_back({{"object", owner},
                          {"nodes", completed->size()},
                          {"lod", lod},
                          {"palette_matrices", matrices},
                          {"local_pose_hash", hash}});
      }
    });
    frames << Json{{"tick", tick},
                   {"unit", unit},
                   {"position",
                    {motion.position.x, motion.position.y, motion.position.z}},
                   {"velocity",
                    {motion.velocity.x, motion.velocity.y, motion.velocity.z}},
                   {"supported", supported},
                   {"activity", activity},
                   {"requested", requested},
                   {"graph", word(manager)},
                   {"packed", word(manager + 4u)},
                   {"mode", word(manager + 0x70u)},
                   {"weapon_class", word(manager + 0x74u)},
                   {"weapon_type", word(manager + 0x78u)},
                   {"frame",
                    Bytes(source.memory().view(manager + 0x1cu, 4)).f32(0)},
                   {"models", models}}
                  .dump()
           << '\n';
    frames.flush();
    if (tick % 30 == 0)
      std::cout << "tick=" << tick << " z=" << motion.position.z
                << " vz=" << motion.velocity.z << " support=" << supported
                << " activity=" << std::hex << activity
                << " requested=" << requested << std::dec << '\n';
  }
  Json report{{"species", argv[5]},
              {"starting_weapon", o.starting_weapon},
              {"spawn", {spawn.position.x, spawn.position.y, spawn.position.z}},
              {"target", {target.x, target.y, target.z}},
              {"rise", peak - initial},
              {"lift_member_force_phases", deliveries},
              {"ascent_airborne", ascent_airborne},
              {"landing_contacts", landing_contacts},
              {"landing_animations", landing_animations},
              {"ascent_frames", ascent_frames.size()},
              {"air_poses", air_poses.size()},
              {"activities", activities},
              {"scope",
               "Original lift and source WORLD/render nodes; diagnostic "
               "position and controls only. No framebuffer parity."}};
  report["complete"] = deliveries && ascent_airborne &&
                       ascent_frames.size() > 8 && air_poses.size() > 1 &&
                       landing_contacts && landing_animations;
  report["pixel_acceptance"] = false;
  check(bool(frames) && bool(forces),
        "Could not retain completed diagnostic traces");
  std::ofstream(output / "receipt.json") << report.dump(2) << '\n';
  std::cout << report.dump(2) << '\n';
  check(deliveries && ascent_airborne && ascent_frames.size() > 8 &&
            air_poses.size() > 1,
        "Lift ascent did not deliver animated airborne pose");
  check(landing_contacts && landing_animations,
        "Lift ride did not reach an authored landing animation");
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
