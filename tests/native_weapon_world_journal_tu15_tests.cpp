#include "halo2/bsp_atmosphere.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/world_effects.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <vector>
using namespace halo2;
namespace {
using Json = nlohmann::json;
void need(bool value, const char *why) {
  if (!value)
    throw FormatError(why);
}
struct Report {
  const char *path;
  Json data{{"passed", false}, {"full_goal_complete", false}};
  ~Report() { std::ofstream(path) << data.dump(2) << '\n'; }
};
std::uint32_t word(const SourceRuntimeMemory &memory, std::uint32_t at) {
  return Bytes(memory.view(at, 4)).u32(0);
}
struct Frame {
  MultiplayerPreparedWorld &world;
  SourceWorldParticleDrawsTu15 graphics;
  SourcePortalVisibilitySet visibility;
  MuzzleEffectView view{};
  SourceCameraAngles angles;
  Vec3 eye, forward, up{0, 0, 1};
  float fov;
  explicit Frame(MultiplayerPreparedWorld &value)
      : world(value), graphics(*world.cache, world.content.objects) {
    const auto state = world.match.state();
    const auto &player = state.players.at(world.local_slots.at(0));
    const auto &biped = multiplayer_biped(world.content, 2);
    eye = ordinary_unit_eye(player.position, biped.camera_height,
                            biped.crouch_camera_height, player.crouch);
    forward = direction(player.yaw, player.pitch);
    fov = biped.camera_field_of_view;
    angles = source_camera_angles(fov);
    SourceFrustumProjectionInput input;
    input.projection_angle = angles.projection_angle;
    input.outer = input.safe = {0, 0, 480, 640};
    input.scenario_present = true;
    input.scenario_type = world.cache->load_source_geometry_scenario_type();
    const auto projection = source_frustum_projection(input);
    const auto camera = source_frustum_camera(
        {eye, forward, up, .015625f, 1024.f, projection.clipping_rectangle()});
    SourcePortalMaskState mask;
    mask.initialize(world.cache->load_source_portal_mask_definition(0), {});
    visibility = source_portal_visibility(
        world.cache->load_source_portal_bsp(0), std::span{&camera, 1},
        world.cache->load_bsp_atmosphere(0).cluster_at(eye), mask.words);
    view.position = eye;
    view.forward = forward;
    view.up = up;
    view.width = 640;
    view.height = 480;
    const auto &basis = camera.camera_to_world.basis;
    view.orientation = {basis[0].x, basis[0].y, basis[0].z,
                        basis[1].x, basis[1].y, basis[1].z,
                        basis[2].x, basis[2].y, basis[2].z};
    const auto &weapon = world.content.weapons.at(player.weapon);
    for (const auto &row : weapon.firing_effects)
      for (auto effect : row.effects)
        if (effect != null_datum) {
          const auto tag =
              world.cache->query_tag({.handle = TagHandle{effect}});
          if (tag && tag->group == "effe") {
            graphics.preload_first_person(effect);
            graphics.preload_world(effect);
          }
        }
  }
  std::vector<ParticleDraw> advance(float seconds) {
    auto &native = *world.source_objects;
    native.render_model_view([&](auto &source) {
      graphics.publish_observer(source, 0, eye, forward, up, fov);
      graphics.prepare_storage(source);
    });
    native.prepare_first_person_models(0, eye, forward, up);
    native.advance_effect_frame(seconds);
    std::vector<ParticleDraw> draws;
    native.render_model_view([&](auto &source) {
      graphics.preload_live_world(source.memory());
      graphics.prepare_storage(source);
      source.begin_model_frame();
      draws = graphics.draw_first_person(source, visibility, view, 0,
                                         angles.projection_angle,
                                         angles.lighting_distance_scale);
    });
    return draws;
  }
};
} // namespace

struct Pipeline {
  std::uint32_t primary, auxiliary, fire_sound;
  Mesh mesh;
  NativeWorldEffects effects;
  MuzzleEffectView view;
  std::map<std::uint32_t, std::vector<ParticleDraw>> draws;
  std::map<std::uint32_t, std::array<std::size_t, 2>> pools;
  unsigned sounds{}, firing_sounds{};
  Json trace = Json::array();
  Pipeline(const Cache &cache, const CollisionWorld &collision, std::uint32_t a,
           std::uint32_t b, std::uint32_t f, std::uint32_t normal,
           const MuzzleEffectView &v)
      : primary(a), auxiliary(b), fire_sound(f),
        effects(cache, mesh, collision, SourceEffectRandom{primary, auxiliary}),
        view(v) {
    effects.preload_attached(normal);
  }
  void collect() {
    for (const auto &cue : effects.take_sounds()) {
      ++sounds;
      firing_sounds += cue.sound == fire_sound;
      trace.push_back({{"sound", cue.sound},
                       {"owner", cue.owner},
                       {"tick", cue.tick},
                       {"gain_bits", std::bit_cast<std::uint32_t>(cue.gain)},
                       {"kind", int(cue.kind)}});
    }
  }
  void deliver(const Cache &cache, const NativeWeaponEffectEvent &e) {
    NativeWorldEffects::ObjectMarkers locations;
    for (const auto &location : e.source_locations) {
      const auto &t = location.frame;
      locations[cache.string_id(location.name)].push_back(
          {t.position, t.basis[2], t.basis[0], t.basis[0], t.basis[2], false, 0,
           0});
    }
    effects.request_weapon(e.effect, e.weapon, e.tick, e.colors, e.velocity,
                           std::bit_cast<float>(e.a_bits),
                           std::bit_cast<float>(e.b_bits), e.source_instance,
                           true,
                           e.source_locations_observed ? &locations : nullptr);
    collect();
  }
  void advance(std::uint32_t tick) {
    effects.advance(tick, 1.f / 30.f);
    collect();
    draws[tick] = effects.draw(view, 0, 1.f / 30.f);
    const auto d = effects.diagnostics();
    pools[tick] = {d.effects, d.particles};
  }
  void contact(const NativeGrenadeDetonation &event) {
    if (!event.source_locations.empty())
      effects.spawn_source(event.effect, event.projectile, event.tick,
                           event.source_locations, event.colors, event.controls,
                           event.replica_render);
    else
      effects.spawn(event.effect, event.projectile, event.tick,
                    {event.position, event.up, event.forward, event.forward,
                     event.up, false});
  }
  Json result() const {
    const auto d = effects.diagnostics();
    return {{"sounds", sounds},
            {"firing_sounds", firing_sounds},
            {"primary", primary},
            {"auxiliary", auxiliary},
            {"trace", trace},
            {"effects", d.effects},
            {"particles", d.particles},
            {"lights", d.lights},
            {"unavailable", d.unavailable}};
  }
};
void same_particles(const Pipeline &a, const Pipeline &b, std::uint32_t tick) {
  need(a.pools.at(tick) == b.pools.at(tick),
       "World journal changed effect/particle retention");
  const auto &left = a.draws.at(tick), &right = b.draws.at(tick);
  need(left.size() == right.size(),
       "World journal changed particle draw count");
  for (std::size_t n = 0; n < left.size(); ++n) {
    const auto &x = left[n], &y = right[n];
    need(x.effect_owner == y.effect_owner &&
             x.effect_definition == y.effect_definition &&
             std::memcmp(&x.vertex.attributes, &y.vertex.attributes,
                         sizeof(x.vertex.attributes)) == 0 &&
             std::memcmp(&x.vertex.constants, &y.vertex.constants,
                         sizeof(x.vertex.constants)) == 0,
         "World journal changed authored particle owner or GPU input bytes");
  }
}
int main(int argc, char **argv) {
  if (argc != 4)
    return 2;
  if (!std::filesystem::is_regular_file(argv[1]))
    return 77;
  Report report{argv[3]};
  try {
    need(std::getenv("HALO2_TEST_SCRATCH"), "Use owned storage");
    MultiplayerOptions options;
    options.map = argv[1];
    options.shared_maps = argv[2];
    options.local_players = 1;
    options.starting_weapon = 5;
    options.source_object_world = true;
    options.host_port = 1;
    options.mute = true;
    options.session_random_seed = 0x105240;
    options.variant = {"Slayer", 0, {{2, 0}, {10, 0}, {89, 5}}};
    options.source_object_savegame_directory =
        std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH")) /
        "coalesced-native-world";
    MultiplayerPreparedWorld world(options);
    world.bind_source_local_views(world.local_slots);
    need(world.cache->is_mcc(), "Use prepared MCC");
    std::array<Controls, multiplayer_capacity> controls{};
    for (unsigned i = 0; i < 60; ++i)
      world.match.step(controls);
    Frame ready(world);
    (void)ready.advance(1.f / 30.f);
    auto state = world.match.state();
    const auto slot = world.local_slots.front(),
               normal = world.content.weapons.at(state.players[slot].weapon)
                            .firing_effects.at(0)
                            .effects[0],
               weapon = state.players[slot].source_weapon;
    const int before_loaded = state.players[slot].loaded;
    const auto fire_sound =
        world.content.weapons.at(state.players[slot].weapon).sound;
    const auto &memory = world.source_objects->memory();
    const auto random = word(memory, 0x4f3cd8u);

    const auto initial_primary = word(memory, random),
               initial_auxiliary = word(memory, random + 4),
               initial_tick = state.tick;
    Pipeline sequential(*world.cache, world.match.simulation().collision(),
                        initial_primary, initial_auxiliary, fire_sound, normal,
                        ready.view),
        batched(*world.cache, world.match.simulation().collision(),
                initial_primary, initial_auxiliary, fire_sound, normal,
                ready.view);
    sequential.advance(state.tick);
    batched.advance(state.tick);
    Pipeline contact_sequential(
        *world.cache, world.match.simulation().collision(), initial_primary,
        initial_auxiliary, fire_sound, normal, ready.view),
        contact_batched(*world.cache, world.match.simulation().collision(),
                        initial_primary, initial_auxiliary, fire_sound, normal,
                        ready.view);
    contact_sequential.advance(state.tick);
    contact_batched.advance(state.tick);
    std::uint64_t impact_serial =
        state.bullet_impacts.empty() ? 0 : state.bullet_impacts.back().serial;
    std::vector<BulletImpact> impacts;
    std::uint64_t contact_serial =
        state.grenade_detonations.empty()
            ? 0
            : state.grenade_detonations.back().serial;
    std::vector<NativeGrenadeDetonation> contacts;
    std::uint32_t serial =
        state.weapon_effects.empty() ? 0 : state.weapon_effects.back().serial;
    std::vector<NativeWeaponEffectEvent> events;
    Json original = Json::array();
    for (unsigned tick = 0; tick < 24; ++tick) {
      controls[slot].fire = tick < 16;
      world.match.step(controls);
      state = world.match.state();
      for (const auto &e : state.weapon_effects)
        if (e.serial > serial) {
          serial = e.serial;
          if (e.effect != normal || e.weapon != weapon)
            continue;
          need(e.source_scheduled && e.source_presentation,
               "Producer lacks actual source request");
          events.push_back(e);
          original.push_back(
              {{"tick", e.tick},
               {"serial", e.serial},
               {"instance", e.source_instance},
               {"locations", e.source_locations.size()},
               {"locations_observed", e.source_locations_observed}});
          sequential.deliver(*world.cache, e);
          contact_sequential.deliver(*world.cache, e);
        }
      for (const auto &impact : state.bullet_impacts)
        if (impact.serial > impact_serial) {
          impact_serial = impact.serial;
          impacts.push_back(impact);
          contact_sequential.effects.impact(impact);
        }
      for (const auto &contact : state.grenade_detonations)
        if (contact.serial > contact_serial) {
          contact_serial = contact.serial;
          contacts.push_back(contact);
          contact_sequential.contact(contact);
        }
      sequential.advance(state.tick);
      contact_sequential.advance(state.tick);
    }
    for (const auto &e : events) {
      batched.deliver(*world.cache, e);
      contact_batched.deliver(*world.cache, e);
    }
    for (const auto &impact : impacts)
      contact_batched.effects.impact(impact);
    for (const auto &contact : contacts)
      contact_batched.contact(contact);
    const auto queued = batched.effects.checkpoint();
    Json minimal = Json::array();
    for (unsigned n : {1u, 2u}) {
      Pipeline left(*world.cache, world.match.simulation().collision(),
                    initial_primary, initial_auxiliary, fire_sound, normal,
                    ready.view),
          right(*world.cache, world.match.simulation().collision(),
                initial_primary, initial_auxiliary, fire_sound, normal,
                ready.view);
      left.advance(initial_tick);
      right.advance(initial_tick);
      for (unsigned j = 0; j < n; ++j) {
        left.deliver(*world.cache, events.at(j));
        left.advance(events[j].tick);
        right.deliver(*world.cache, events[j]);
      }
      left.advance(events[n - 1].tick + 2);
      right.advance(events[n - 1].tick + 2);
      minimal.push_back({{"requests", n},
                         {"sequential", left.result()},
                         {"coalesced", right.result()}});
    }
    report.data["minimal_subsets"] = minimal;
    for (const auto &row : minimal)
      need(row.at("sequential").at("firing_sounds") == row.at("requests") &&
               row.at("coalesced").at("firing_sounds") == row.at("requests"),
           "Minimal coalesced world request lost a native shot");
    const auto particle_tick = events.back().tick + 1;
    batched.advance(particle_tick);
    need(!batched.draws.at(particle_tick).empty(),
         "World journal lacks an active particle submission");
    same_particles(sequential, batched, particle_tick);
    report.data["active_particle_draws_compared"] =
        batched.draws.at(particle_tick).size();
    sequential.advance(state.tick + 2);
    batched.advance(state.tick + 2);
    contact_sequential.advance(state.tick + 2);
    contact_batched.advance(state.tick + 2);
    report.data["native_bullet_contacts"] = impacts.size();
    report.data["native_generic_projectile_contacts"] = contacts.size();
    need(!contacts.empty(),
         "World journal comparison lacks actual native projectile contacts");
    need(contact_sequential.result() == contact_batched.result(),
         "Coalesced weapon/contact births changed sound, RNG or retention");
    same_particles(contact_sequential, contact_batched, state.tick + 2);
    report.data["native_contact_interleaving_equal"] =
        !impacts.empty() || !contacts.empty();
    report.data["native_ammunition_consumed"] =
        before_loaded - state.players[slot].loaded;
    report.data["native_events"] = events.size();
    report.data["original_requests"] = original;
    report.data["sequential"] = sequential.result();
    report.data["coalesced"] = batched.result();
    report.data["RNG_streams_equal"] =
        sequential.primary == batched.primary &&
        sequential.auxiliary == batched.auxiliary;
    report.data["sound_rows_equal"] = sequential.trace == batched.trace;
    report.data["scope"] =
        "Actual native SMG source journal and actual imported locations "
        "through world recipient request_weapon; individual/coalesced cue "
        "chronology, RNG, particle GPU input bytes, actual generic projectile "
        "contacts and queued/active checkpoint/fork replay. No "
        "waveform/framebuffer claim";
    need(events.size() > 1 &&
             events.size() ==
                 unsigned(before_loaded - state.players[slot].loaded),
         "Missing actual multi-shot native journal");
    need(sequential.firing_sounds == events.size(),
         "Sequential world cue count differs from actual native requests");
    need(batched.firing_sounds == sequential.firing_sounds,
         "Coalesced world requests lose firing cues");
    need(sequential.trace == batched.trace,
         "Coalesced world requests alter sound chronology");
    need(report.data["RNG_streams_equal"].get<bool>(),
         "Coalesced world requests alter RNG");
    const auto expected = batched.result();
    const auto restore = [&](const NativeWorldEffects::Checkpoint &cut) {
      auto staged = batched.effects.prepare_restore(
          cut, SourceEffectRandom{batched.primary, batched.auxiliary}, {});
      const auto random = cut.random();
      batched.primary = random[0];
      batched.auxiliary = random[1];
      need(batched.effects.commit_restore(std::move(staged)),
           "World journal checkpoint did not publish");
    };
    restore(queued);
    batched.trace = Json::array();
    batched.sounds = batched.firing_sounds = 0;
    const auto partial_tick = events.front().tick + 1;
    batched.advance(partial_tick);
    need(batched.firing_sounds > 0 && batched.firing_sounds < events.size(),
         "World journal lacks active and queued checkpoint state");
    const auto middle = batched.effects.checkpoint();
    const auto prefix = batched.trace;
    const auto prefix_sounds = batched.sounds,
               prefix_firing = batched.firing_sounds;
    for (auto tick = partial_tick + 1; tick <= state.tick + 2; ++tick)
      batched.advance(tick);
    need(batched.result() == expected,
         "Queued world journal checkpoint changed sound/RNG/retention");
    for (auto tick = partial_tick + 1; tick <= state.tick; ++tick)
      same_particles(sequential, batched, tick);
    Pipeline replay(*world.cache, world.match.simulation().collision(),
                    initial_primary, initial_auxiliary, fire_sound, normal,
                    ready.view);
    replay.draws = batched.draws;
    replay.pools = batched.pools;
    restore(middle);
    batched.trace = prefix;
    batched.sounds = prefix_sounds;
    batched.firing_sounds = prefix_firing;
    for (auto tick = partial_tick + 1; tick <= state.tick + 2; ++tick) {
      batched.advance(tick);
      same_particles(replay, batched, tick);
    }
    need(batched.result() == expected,
         "Active world journal checkpoint changed sound/RNG/retention");
    report.data["queued_and_active_checkpoint_replay_equal"] = true;
    restore(queued);
    auto fork_random = queued.random();
    auto fork = batched.effects.fork(
        SourceEffectRandom{fork_random[0], fork_random[1]}, {});
    fork->advance(state.tick + 2, 1.f / 30.f);
    Json fork_sounds = Json::array();
    for (const auto &cue : fork->take_sounds())
      fork_sounds.push_back(
          {{"sound", cue.sound},
           {"owner", cue.owner},
           {"tick", cue.tick},
           {"gain_bits", std::bit_cast<std::uint32_t>(cue.gain)},
           {"kind", int(cue.kind)}});
    need(fork_sounds == expected.at("trace") &&
             fork_random[0] == expected.at("primary") &&
             fork_random[1] == expected.at("auxiliary"),
         "World journal fork lost pending requests or RNG order");
    report.data["fork_preserves_pending_requests"] = true;
    batched.trace = Json::array();
    batched.sounds = batched.firing_sounds = 0;
    batched.effects.remove_object(weapon);
    batched.advance(state.tick + 2);
    need(batched.sounds == 0 && batched.effects.diagnostics().effects == 0,
         "Object removal left queued world weapon requests");
    report.data["object_removal_cancels_pending"] = true;
    report.data["passed"] = true;
    return 0;
  } catch (const std::exception &e) {
    report.data["error"] = e.what();
    std::cerr << e.what() << '\n';
    return 1;
  }
}
