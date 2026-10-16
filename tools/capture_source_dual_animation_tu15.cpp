#include "halo2/bsp_atmosphere.hpp"
#include "halo2/model.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using Json = nlohmann::json;
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw FormatError(message);
}
std::uint32_t word(const SourceRuntimeMemory &m, std::uint32_t p) {
  return Bytes(m.view(p, 4)).u32(0);
}
unsigned palette_check(MultiplayerPreparedWorld &w, PhysicsLiveWorldTu15 &s,
                       std::uint32_t owner) {
  const auto &m = s.memory();
  const auto rd = [&](auto p) { return word(m, p); };
  const auto physical = w.source_objects->world_nodes(owner);
  const auto collector = s.build_world_model(owner, 0, 1, false, 0, 0);
  const Bytes c(collector.bytes);
  const auto completed = s.rendered_world_nodes(owner);
  require(completed && completed->size() == physical.size(),
          "Completed body/held-weapon render nodes missing");
  const auto meta = [&](std::uint32_t t) {
    return rd(rd(0x4f0414u) + (t & 65535u) * 16u + 8u);
  };
  const auto definition = w.source_objects->object_definition(owner);
  const auto mode = rd(meta(rd(meta(definition) + 0x38u)) + 4u),
             rm = meta(mode);
  const auto count = rd(rm + 0x48u), records = rd(rm + 0x4cu);
  std::vector<Transform> inverses(count);
  for (unsigned n = 0; n < count; ++n)
    std::memcpy(&inverses[n], m.view(records + n * 96u + 0x28u, 52).data(), 52);
  auto palette = source_model_skin_palette(*completed, inverses);
  const auto lod = c.u16(0x168);
  require(lod < 6, "Invalid original model LOD");
  append_source_model_compounds(
      palette,
      w.cache->load_source_model_palette_definition(mode).compounds[lod]);
  const auto token = c.u32(4);
  require(token != null_datum, "Original render builder omitted its palette");
  const auto output = 0x49433cu + (token & 0x0fffffffu);
  const auto regions = Bytes(m.view(output + 2, 2)).u16(0);
  unsigned checked = 0;
  for (unsigned region = 0; region < regions; ++region) {
    const auto selected =
        std::to_integer<unsigned>(c.slice(0x75u + region, 1).front());
    if (selected == 255)
      continue;
    const auto section = rd(rm + 0x28u) + selected * 92u,
               resident = rd(section + 0x34u);
    const auto maps = rd(resident + 0x64u), mapping = rd(resident + 0x68u);
    const Bytes range(m.view(output + 4u + region * 4u, 4));
    const auto first = range.u16(0), n = range.u16(2);
    for (unsigned i = 0; i < n; ++i) {
      const auto node =
          maps ? std::to_integer<unsigned>(m.view(mapping + i, 1)[0]) : i;
      require(node < palette.size(),
              "Original region node map escapes its palette");
      require(
          !std::memcmp(m.view(output + 0x44u + (first + i) * 48u, 48).data(),
                       palette[node].data(), 48),
          "Completed dual pose differs from original uploaded palette");
      ++checked;
    }
  }
  require(checked > 0, "No original palette matrices checked");
  return checked;
}
} // namespace
int main(int argc, char **argv) try {
  require(argc >= 6 && argc <= 8,
          "Supply MAP SHARED TU15 OUTPUT human|elite [PRIMARY] [SECONDARY]");
  if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))
    return 77;
  const bool elite = std::string_view(argv[5]) == "elite";
  require(elite || std::string_view(argv[5]) == "human",
          "Invalid source species");
  const auto root = std::filesystem::absolute(argv[4]);
  std::filesystem::create_directories(root);
  std::filesystem::path out;
  for (unsigned n = 0;; ++n) {
    out = root / ("capture-" + std::to_string(n));
    if (std::filesystem::create_directory(out))
      break;
  }
  MultiplayerOptions o;
  o.map = argv[1];
  o.shared_maps = argv[2];
  o.gameplay_xbe = argv[3];
  o.starting_weapon = argc >= 7 ? std::stoul(argv[6]) : 5;
  const auto secondary_choice =
      argc == 8 ? std::stoul(argv[7]) : o.starting_weapon;
  o.source_object_savegame_directory = out / "source-savegames";
  o.session_random_seed = 0x12345678;
  o.defer_initial_player_spawn = true;
  const bool source_particles = std::getenv("HALO2_SOURCE_FP_PARTICLE_PROBE");
  std::array<std::uint8_t, 9> appearance{};
  appearance[4] = elite ? 1 : 0;
  o.selected_profiles = {native_selected_player_configuration(
      "Source dual animation", appearance, -1)};
  Cache cache(o.map, o.shared_maps, OriginalData::load_xbe(o.gameplay_xbe));
  const auto content = cache.load_content(0, true);
  const auto secondary_definition =
      cache.slayer_starting_weapon(secondary_choice);
  require(
      content.weapons.at(secondary_definition).dual_wieldable &&
          content.weapons.at(cache.slayer_starting_weapon(o.starting_weapon))
              .dual_wieldable,
      "Requested weapon pair is not authored for dual wield");
  CollisionWorld geometry(content.collision);
  Spawn spawn{};
  bool placed = false;
  unsigned pickup_row = 0;
  for (unsigned row = 0; row < content.netgame_equipment.size(); ++row) {
    const auto &equipment = content.netgame_equipment[row];
    if (std::none_of(equipment.items.begin(), equipment.items.end(),
                     [&](const auto &item) {
                       return item.definition == secondary_definition;
                     }))
      continue;
    const auto floor =
        geometry.raycast(equipment.position + Vec3{0, 0, .5f}, {0, 0, -1}, 3.f);
    if (!floor || floor->normal.z < .9f)
      continue;
    spawn.position = floor->position + Vec3{0, 0, .002f};
    spawn.facing = 0;
    pickup_row = row;
    placed = true;
    break;
  }
  require(placed, "No original matching weapon pickup placement");
  o.native_spawn_inputs = {spawn};
  std::vector<std::uint32_t> players;
  o.player_birth_owner = [&](MultiplayerPlayer &p, unsigned) {
    players.push_back(p.source_datum);
    p.source_flags = 8;
  };
  o.pre_player_update_owner = [&](MultiplayerMatch &match) {
    for (const auto h : players)
      if (const auto *p = match.player_from_datum(h);
          p && !(p->source_flags & 1u))
        match.apply_source_player_activation(h, p->source_flags,
                                             p->source_flags | 1u);
  };
  MultiplayerPreparedWorld w(o);
  require(w.source_objects && w.source_objects->owns_physics(),
          "Actual original world owner missing");
  auto &source = *w.source_objects;
  const auto source_world = word(source.memory(), 0x4dc04cu);
  require(source_world && word(source.memory(), source_world + 8u) == 1u,
          "Original local simulation-world constructor was not completed");
  auto &m = w.match;
  std::array<Controls, multiplayer_capacity> controls{};
  controls[0].yaw = spawn.facing;
  m.step(controls);
  // Wait for the source map timer to admit the real pickup. No object,
  // pickup target or inventory result is supplied by this fixture.
  for (unsigned n = 0;
       n < 120 && source.netgame_object(pickup_row) == null_datum; ++n)
    m.step(controls);
  const auto initial = m.unit(0);
  const auto primary = initial.weapon.source_datum, unit = initial.source_datum;
  const auto pickup = source.netgame_object(pickup_row);
  require(pickup != null_datum &&
              source.object_definition(pickup) == secondary_definition,
          "Original map pickup row was not admitted");
  const std::array<unsigned, 2> local{0, 1};
  m.configure_local_views(std::span{local}.first(o.local_players), w.cache->load_first_person_binding(
                                     initial.weapon.definition, elite ? 3 : 2));
  SourceWorldParticleDrawsTu15 particle_graphics(*w.cache, w.content.objects);
  SourcePortalMaskState portal_mask;
  portal_mask.initialize(w.cache->load_source_portal_mask_definition(0), {});
  if (source_particles)
    for (const auto definition :
         {initial.weapon.definition, secondary_definition}) {
      const auto &weapon = w.content.weapons.at(definition);
      for (const auto &row : weapon.firing_effects)
        for (const auto effect : row.effects)
          if (effect != null_datum) {
            particle_graphics.preload_first_person(effect);
            particle_graphics.preload_world(effect);
          }
    }
  std::array<unsigned, 2> source_draws{};
  // Magnum's authored WORLD casing lifespan is3..5 seconds. Check drain
  // after that maximum from the final actual request, not a fixed tick that
  // rejects a legitimately retained casing. The six dual-compatible firing
  // banks in this diagnostic have no longer authored particle lifespan.
  constexpr unsigned maximum_particle_lifetime_ticks=150;
  unsigned last_effect_tick{};
  unsigned late_source_draws{};
  unsigned own_world_commands{};
  std::array<unsigned, 2> remote_world_draws{};
  std::ofstream frames(out / "frames.jsonl"), effects(out / "effects.jsonl");
  std::array<unsigned, 2> effect_counts{}, fp_models{}, fire_animations{},
      reload_animations{};
  std::array<std::set<std::uint32_t>, 2> fire_clips, reload_clips;
  unsigned dual_ticks = 0, palette_matrices = 0;
  std::uint64_t serial = 0;
  std::uint32_t offhand = null_datum;
  std::array<ModelNodeInterpolation,2> fp_interpolation;
  std::array<RenderModel,2> fp_hierarchies;
  std::array<unsigned,2> interpolated_fp_samples{},changed_fp_samples{};
  for(unsigned hand=0;hand<2;++hand) {
    const auto binding=cache.load_first_person_binding(hand?secondary_definition:initial.weapon.definition,elite?3:2);
    fp_hierarchies[hand]=cache.load_model(binding.weapon_model,false);
  }
  for (unsigned tick = 0; tick < 320; ++tick) {
    if (source_particles && tick == 60) {
      require(m.join("Source particle observer", -1, o.selected_profiles[0]) == 1,
              "Original second player did not join slot1");
      m.configure_local_views(local, w.cache->load_first_person_binding(
                                         initial.weapon.definition,
                                         elite ? 3 : 2));
    }
    auto &input = controls[0];
    input = {};
    input.yaw = initial.yaw;
    input.pitch = tick >= 180 ? .25f : 0.f;
    input.dual_wield = tick >= 10 && tick < 30;
    input.dual_wield_milliseconds = input.dual_wield ? 1000 : 0;
    input.fire = (tick >= 100 && tick < 116) || (tick >= 140 && tick < 156);
    input.fire_secondary =
        (tick >= 120 && tick < 136) || (tick >= 140 && tick < 156);
    input.reload = tick == 170;
    input.crouch = tick >= 220 && tick < 250;
    input.forward = tick >= 230 && tick < 250 ? .5f : 0.f;
    input.jump = tick == 260;
    m.step(controls);
    if (source_particles) {
      // Actual12E142 game update precedes12E151/13AA28 particle update.
      // Prepare the completed FP marker bank before advancing, as the live
      // renderer does; retain the preceding render union until that advance.
      const auto eye = m.unit(0).body.position + Vec3{0, 0, content.player.camera_height};
      const auto forward = direction(input.yaw, input.pitch);
      source.render_model_view([&](auto &original) {
        particle_graphics.publish_observer(original,0,eye,forward,{0,0,1},
            content.player_representations[elite ? 3 : 2].biped.camera_field_of_view);
      });
      source.prepare_first_person_models(0, eye, forward, {0, 0, 1});
      source.render_model_view([&](auto &original) {
        particle_graphics.advance(original, 1.f / 30.f);
      });
    }
    const auto state = m.state();
    const auto &u = m.unit(0);
    const auto &p = state.players[0];
    if (p.secondary_weapon.datum != null_datum) {
      offhand = p.secondary_weapon.datum;
      ++dual_ticks;
    }
    if (tick >= 60)
      require(offhand == pickup && p.secondary_weapon.datum == pickup,
              "Normal original dual pickup did not retain the actual second "
              "weapon");
    const auto fp = m.first_person_view(0);
    Json hands = Json::array();
    for (unsigned hand = 0; hand < 2; ++hand) {
      const auto &h = fp.hands[hand];
      if (tick >= 60)
        require(h.weapon == (hand ? offhand : primary) && h.dual_wield,
                "Original FP registration lost its actual dual hand");
      if (tick >= 100 && tick < 160) {
        fire_clips[hand].insert(h.animation_identifier);
        if (h.activity != null_datum &&
            cache.string_id(h.activity).starts_with("fire"))
          ++fire_animations[hand];
      }
      if (tick >= 170 && tick < 220) {
        reload_clips[hand].insert(h.animation_identifier);
        if (h.activity != null_datum &&
            cache.string_id(h.activity).starts_with("reload"))
          ++reload_animations[hand];
      }
      hands.push_back({{"weapon", h.weapon},
                       {"graph", h.graph},
                       {"packed", h.animation_identifier},
                       {"frame", h.frame},
                       {"activity", h.activity},
                       {"options", h.channel_options}});
    }
    for (const auto &e : state.weapon_effects)
      if (e.serial > serial) {
        serial = e.serial;
        if (e.weapon == primary || e.weapon == offhand) {
          const unsigned hand = e.weapon == primary ? 0 : 1;
          require(
              e.source_scheduled && e.source_presentation &&
                  e.source_locations_observed &&
                  e.source_instance != null_datum,
              "Dual firing lost its original completed effect/location owner");
          ++effect_counts[hand];
          last_effect_tick=tick;
          effects << Json{{"tick", tick},
                          {"hand", hand},
                          {"weapon", e.weapon},
                          {"effect", e.effect},
                          {"instance", e.source_instance},
                          {"locations", e.source_locations.size()}}
                         .dump()
                  << '\n';
        }
      }
    if (tick >= 60) {
      for (const auto owner : {unit, primary, offhand}) {
        const auto actual = source.world_nodes(owner);
        const auto published = std::find_if(
            state.source_body_poses.begin(), state.source_body_poses.end(),
            [&](const auto &pose) { return pose.object == owner; });
        require(published != state.source_body_poses.end() &&
                    published->nodes.size() == actual.size() &&
                    !std::memcmp(published->nodes.data(), actual.data(),
                                 actual.size() * sizeof(Transform)),
                "Published dual body/weapon WORLD nodes differ from source");
      }
      source.begin_model_frame();
      source.render_model_view([&](PhysicsLiveWorldTu15 &s) {
        for (const auto owner : {unit, primary, offhand})
          palette_matrices += palette_check(w, s, owner);
      });
      const float yaw = input.yaw, pitch = input.pitch;
      const Vec3 view_forward{std::cos(yaw) * std::cos(pitch),
                              std::sin(yaw) * std::cos(pitch), std::sin(pitch)};
      const auto position =
          u.body.position + Vec3{0, 0, content.player.camera_height};
      if(source_particles)source.render_model_view([&](auto &original) {
        particle_graphics.publish_observer(original,0,position,view_forward,{0,0,1},
            content.player_representations[elite ? 3 : 2].biped.camera_field_of_view);
      });
      else source.publish_native_observer_position(0, position);
      const auto draws = source.prepare_first_person_models(
          0, position, view_forward, {0, 0, 1});
      for (unsigned hand = 0; hand < 2; ++hand) {
        const auto owner = hand ? offhand : primary;
        const auto draw =
            std::find_if(draws.begin(), draws.end(), [&](const auto &d) {
              return d.owner == owner && d.mode == 1;
            });
        require(draw != draws.end() && !draw->nodes.empty(),
                "Original FP builder omitted a dual weapon model");
        ++fp_models[hand];
        const auto frame=camera_transform(position,view_forward,{0,0,1});
        auto &history=fp_interpolation[hand];history.capture(state.tick,draw->nodes,frame);
        require(history.sample(fp_hierarchies[hand],1,frame).size()==draw->nodes.size(),
                "Dual FP interpolation changed its completed model extent");
        const auto endpoint=history.sample(fp_hierarchies[hand],1,frame);
        require(!std::memcmp(endpoint.data(),draw->nodes.data(),endpoint.size()*sizeof(Transform)),
                "Dual FP interpolation changed original endpoint words");
        for(float fraction:{.25f,.5f,.75f}) {
          const auto sampled=history.sample(fp_hierarchies[hand],fraction,frame);
          for(const auto &node:sampled)for(const auto v:{node.basis[0],node.basis[1],node.basis[2],node.position})
            for(float value:{v.x,v.y,v.z})require(std::isfinite(value),"Dual FP interpolation emitted nonfinite geometry");
          if(history.paired()) {
            ++interpolated_fp_samples[hand];
            changed_fp_samples[hand]+=std::memcmp(sampled.data(),draw->nodes.data(),sampled.size()*sizeof(Transform))!=0;
          }
        }
        for (const auto &node : draw->nodes)
          for (const auto v :
               {node.basis[0], node.basis[1], node.basis[2], node.position})
            for (float f : {v.x, v.y, v.z})
              require(std::isfinite(f), "Nonfinite dual FP source node");
      }
    }
    if (source_particles && tick >= 60) {
      const Vec3 forward{std::cos(input.yaw) * std::cos(input.pitch),
                         std::sin(input.yaw) * std::cos(input.pitch),
                         std::sin(input.pitch)};
      const auto eye =
          u.body.position + Vec3{0, 0, content.player.camera_height};
      const auto angle =
          source_camera_angles(content.player_representations[elite ? 3 : 2]
                                   .biped.camera_field_of_view);
      SourceFrustumProjectionInput pi;
      pi.projection_angle = angle.projection_angle;
      pi.outer = {0, 0, 480, 640};
      pi.safe = pi.outer;
      pi.scenario_present = true;
      pi.scenario_type = w.cache->load_source_geometry_scenario_type();
      const auto projection = source_frustum_projection(pi);
      const auto camera =
          source_frustum_camera({eye,
                                 forward,
                                 {0, 0, 1},
                                 .015625f,
                                 1024.f,
                                 projection.clipping_rectangle()});
      const auto render =
          source_view_projection({eye,
                                  forward,
                                  {0, 0, 1},
                                  .015625f,
                                  1024.f,
                                  {},
                                  projection.projection_program});
      const auto visibility = source_portal_visibility(
          w.cache->load_source_portal_bsp(0), std::span{&camera, 1},
          w.cache->load_bsp_atmosphere(0).cluster_at(eye), portal_mask.words);
      // Observe the original WORLD emitter position. A newly allocated
      // location's bounding sphere still has its constructor value until
      // the source update completes; it cannot define the diagnostic camera.
      auto world_center = source.world_nodes(primary).front().position;
      const auto &memory = source.memory();
      const auto read = [&](std::uint32_t p) { return Bytes(memory.view(p, 4)).u32(0); };
      const auto pool = read(0x51df74u), first = read(pool + 0x44);
      bool located = false;
      for (unsigned n = 0; n < read(pool + 0x38) && !located; ++n) {
        if (!(read(read(pool + 0x48) + n / 32 * 4) & (1u << (n % 32))))continue;
        const auto s = first + n * 84, instance = read(s + 0x18);
        if (instance == null_datum)continue;
        if (!particle_graphics.world_effect(read(s + 0x10)))continue;
        const auto e = read(read(0x4f720cu) + 0x44) + (instance & 65535u) * 400;
        if (read(e + 0x48) != primary)continue;
        for (auto id = read(s + 0x30); id != null_datum;) {
          const auto l = read(read(0x52d71cu) + 0x44) + (id & 65535u) * 52;
          const auto radius = std::bit_cast<float>(read(l + 0x2c));
          if (memory.view(l + 2, 1)[0] == std::byte{} && radius > 0 && std::isfinite(radius)) {
            world_center = {std::bit_cast<float>(read(l + 0x10)),
                std::bit_cast<float>(read(l + 0x14)),std::bit_cast<float>(read(l + 0x18))};
            located = true;break;
          }
          id = read(l + 0xc);
        }
      }
      const auto remote_eye = world_center - forward * .5f + Vec3{0, 0, .1f};
      const auto remote_forward = normalized(world_center - remote_eye);
      const auto remote_camera = source_frustum_camera(
          {remote_eye, remote_forward, {0, 0, 1}, .015625f, 1024.f,
           projection.clipping_rectangle()});
      const auto remote_render = source_view_projection(
          {remote_eye, remote_forward, {0, 0, 1}, .015625f, 1024.f, {},
           projection.projection_program});
      const auto remote_visibility = source_portal_visibility(
          w.cache->load_source_portal_bsp(0), std::span{&remote_camera, 1},
          w.cache->load_bsp_atmosphere(0).cluster_at(remote_eye), portal_mask.words);
      source.render_model_view([&](auto &original) {
        particle_graphics.publish_observer(original,1,remote_eye,remote_forward,{0,0,1},
            content.player_representations[elite ? 3 : 2].biped.camera_field_of_view);
      });
      MuzzleEffectView view{};
      view.width = 640;
      view.height = 480;
      view.position = eye;
      view.forward = forward;
      view.up = {0, 0, 1};
      view.camera =
          source_view_shader_bank(render, 1024.f, eye, {0, 0, 640, 480}, true);
      const auto &basis = render.camera_to_world.basis;
      view.orientation = {basis[0].x, basis[0].y, basis[0].z,
                          basis[1].x, basis[1].y, basis[1].z,
                          basis[2].x, basis[2].y, basis[2].z};
      auto remote_view = view;
      remote_view.position = remote_eye;
      remote_view.forward = remote_forward;
      remote_view.camera = source_view_shader_bank(
          remote_render, 1024.f, remote_eye, {0, 0, 640, 480}, false);
      const auto &remote_basis = remote_render.camera_to_world.basis;
      remote_view.orientation = {remote_basis[0].x, remote_basis[0].y, remote_basis[0].z,
          remote_basis[1].x, remote_basis[1].y, remote_basis[1].z,
          remote_basis[2].x, remote_basis[2].y, remote_basis[2].z};
      source.render_model_view([&](auto &original) {
        // Inspect the original queue independently of the native resource
        // registry. Own first-person firing must not also enter WORLD.
        particle_graphics.draw(original, visibility, view, 0,
                               angle.projection_angle,
                               angle.lighting_distance_scale);
        const auto &memory = original.memory();
        const auto read = [&](std::uint32_t p) {
          return Bytes(memory.view(p, 4)).u32(0);
        };
        const auto commands = read(0x4c2aa8u);
        for (unsigned n = 0; n < read(0x4c2aa0u); ++n) {
          const auto command = commands + n * 112;
          if (read(command) != 3 || read(command + 0x20) != 0x176b30u)
            continue;
          const auto system = read(command + 0x24);
          const auto instance = read(system + 0x18);
          if (instance == null_datum)
            continue;
          const auto effects = read(0x4f720cu);
          const auto owner = read(read(effects + 0x44) +
                                  (instance & 65535u) * 400 + 0x48);
          own_world_commands += owner == primary || owner == offhand;
        }
        const auto draws = particle_graphics.draw_first_person(
            original, visibility, view, 0, angle.projection_angle,
            angle.lighting_distance_scale);
        for (const auto &draw : draws) {
          if (draw.effect_owner == primary)
            ++source_draws[0];
          else if (draw.effect_owner == offhand)
            ++source_draws[1];
          if (tick > last_effect_tick+maximum_particle_lifetime_ticks)
            ++late_source_draws;
        }
        // View1 has no local FP owner and observes the real WORLD systems.
        // Both hands must remain visible to another player's camera.
        for (const auto &draw : particle_graphics.draw(
                 original, remote_visibility, remote_view, 1, angle.projection_angle,
                 angle.lighting_distance_scale)) {
          if (draw.effect_owner == primary)
            ++remote_world_draws[0];
          else if (draw.effect_owner == offhand)
            ++remote_world_draws[1];
          if (tick > last_effect_tick+maximum_particle_lifetime_ticks)
            ++late_source_draws;
        }

      });
    }
    frames << Json{{"tick", tick},
                   {"primary", primary},
                   {"offhand", offhand},
                   {"hands", hands},
                   {"grounded", u.body.grounded},
                   {"crouch", source.biped_motion(unit).crouch}}
                  .dump()
           << '\n';
    frames.flush();
  }
  Json report{
      {"species", argv[5]},
      {"primary_choice", o.starting_weapon},
      {"secondary_choice", secondary_choice},
      {"primary", primary},
      {"secondary", offhand},
      {"dual_ticks", dual_ticks},
      {"palette_matrices", palette_matrices},
      {"effects", effect_counts},
      {"fp_models", fp_models},
      {"interpolated_fp_samples",interpolated_fp_samples},
      {"changed_fp_samples",changed_fp_samples},
      {"source_draws", source_draws},
      {"late_source_draws", late_source_draws},
      {"expiry_check_from", last_effect_tick+maximum_particle_lifetime_ticks+1},
      {"own_world_commands", own_world_commands},
      {"remote_world_draws", remote_world_draws},
      {"fire_animations", fire_animations},
      {"reload_animations", reload_animations},
      {"fire_clip_counts", {fire_clips[0].size(), fire_clips[1].size()}},
      {"reload_clip_counts", {reload_clips[0].size(), reload_clips[1].size()}},
      {"pixel_acceptance", false},
      {"scope",
       "Actual original pickup, dual FP registration, firing effect "
       "journal, completed FP model nodes, physical WORLD publication "
       "and original render palettes. Authored map pickup and declared "
       "position/controller inputs; no framebuffer parity."}};
  const bool reloads =
      (content.weapons.at(initial.weapon.definition).capacity <= 0 ||
       reload_animations[0]) &&
      (content.weapons.at(secondary_definition).capacity <= 0 ||
       reload_animations[1]);
  report["complete"] = reloads && fire_animations[0] && fire_animations[1] &&
                       dual_ticks > 250 && effect_counts[0] &&
                       effect_counts[1] && fp_models[0] > 200 &&
                       fp_models[1] > 200 && interpolated_fp_samples[0]>700 && interpolated_fp_samples[1]>700;
  if (source_particles)
    report["complete"] = report["complete"].get<bool>() && source_draws[0] &&
                         source_draws[1] && !late_source_draws &&
                         !own_world_commands && remote_world_draws[0] &&
                         remote_world_draws[1] &&
                         last_effect_tick+maximum_particle_lifetime_ticks+1<320;
  std::ofstream(out / "receipt.json") << report.dump(2) << '\n';
  std::cout << report.dump(2) << std::endl;
  require(report["complete"].get<bool>(),
          "Dual animation/effect history incomplete");
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
