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
  NativeMuzzleEffects effects;
  std::uint32_t weapon;
  NativeMuzzleEffects::Markers markers;
  MuzzleEffectView view;
  std::map<std::uint32_t, std::vector<ParticleDraw>> draws;
  std::map<std::uint32_t, std::array<std::size_t, 2>> pools;
  std::uint32_t source = 0xffffffffu;
  unsigned skipped{}, reuses{}, sounds{}, firing_sounds{}, observed_tick{};
  Json trace = Json::array();
  Pipeline(const Cache &cache, std::uint32_t a, std::uint32_t b,
           std::uint32_t f, std::uint32_t w, NativeMuzzleEffects::Markers m,
           const MuzzleEffectView &v)
      : primary(a), auxiliary(b), fire_sound(f),
        effects(cache, SourceEffectRandom{primary, auxiliary}), weapon(w),
        markers(std::move(m)), view(v) {}
  void collect() {
    auto cues = effects.take_sounds();
    sounds += cues.size();
    for (const auto &cue : cues) {
      firing_sounds += cue.cue.sound == fire_sound;
      trace.push_back(
          {{"kind", "sound"},
           {"sound", cue.cue.sound},
           {"owner", cue.owner},
           {"gain_bits", std::bit_cast<std::uint32_t>(cue.cue.gain)},
           {"observed_tick", observed_tick}});
    }
  }
  void deliver(const NativeWeaponEffectEvent &event) {
    if (event.source_presentation && event.source_instance == null_datum) {
      ++skipped;
      return;
    }
    const bool reused = source == event.source_instance && source != null_datum;
    if (reused)
      ++reuses;
    effects.source_request({event.effect, event.weapon, event.source_instance,
                            event.tick, event.a_bits, event.b_bits,
                            event.colors, event.velocity, true});
    source = event.source_instance;
    trace.push_back({{"kind", "request"},
                     {"tick", event.tick},
                     {"serial", event.serial},
                     {"source_instance", source},
                     {"reused", reused}});
    collect();
  }
  void advance(std::uint32_t tick) {
    observed_tick = tick;
    effects.pose(weapon, markers, view, tick, 1.f / 30.f);
    effects.advance(tick, 1.f / 30.f);
    collect();
    draws[tick] = effects.draw(weapon, view, tick, 0, 1.f / 30.f);
    pools[tick] = {effects.particle_count(), effects.location_count()};
  }
};
void same_particles(const Pipeline &a, const Pipeline &b, std::uint32_t tick) {
  need(a.pools.at(tick) == b.pools.at(tick),
       "Source journal changed particle/location retention");
  const auto &left = a.draws.at(tick), &right = b.draws.at(tick);
  need(left.size() == right.size(),
       "Source journal changed particle draw count");
  for (std::size_t n = 0; n < left.size(); ++n) {
    const auto &x = left[n], &y = right[n];
    need(x.effect_owner == y.effect_owner &&
             x.effect_definition == y.effect_definition &&
             std::memcmp(&x.vertex.attributes, &y.vertex.attributes,
                         sizeof(x.vertex.attributes)) == 0 &&
             std::memcmp(&x.vertex.constants, &y.vertex.constants,
                         sizeof(x.vertex.constants)) == 0,
         "Source journal changed authored particle owner or GPU input bytes");
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
    // Hold an authenticated model-order bind pose fixed while comparing
    // delivery schedules. This tests chronology, not live FP placement parity.
    const auto first_person_model =
        world.content.weapons.at(state.players[slot].weapon).first_person_model;
    const auto model = world.cache->load_model(first_person_model, false);
    const auto nodes = pose_model_nodes_world(model, Transform{}, nullptr, 0);
    std::map<std::uint32_t, std::vector<Transform>> marker_bank;
    NativeMuzzleEffects definitions(*world.cache, 1);
    for (auto name : definitions.marker_names(normal)) {
      auto &bank = marker_bank[name];
      for (const auto &marker : pose_model_markers(
               source_model_markers(
                   world.cache->load_source_tag_metadata(first_person_model),
                   name),
               nodes))
        bank.push_back(marker.world);
      need(!bank.empty(), "Actual weapon particle marker is absent");
    }
    const NativeMuzzleEffects::Markers markers = [&](auto name) {
      return marker_bank.at(name);
    };
    Pipeline sequential(*world.cache, word(memory, random),
                        word(memory, random + 4), fire_sound, weapon, markers,
                        ready.view),
        batched(*world.cache, word(memory, random), word(memory, random + 4),
                fire_sound, weapon, markers, ready.view);
    sequential.advance(state.tick);
    batched.advance(state.tick);
    std::uint32_t serial =
        state.weapon_effects.empty() ? 0 : state.weapon_effects.back().serial;
    std::vector<NativeWeaponEffectEvent> events;
    Json original = Json::array();
    // The actual native producer advances gameplay. Recipient presentation owns
    // its later effect frames; no extra producer particle frame is fabricated.
    for (unsigned tick = 0; tick < 24; ++tick) {
      controls[slot].fire = tick < 16;
      world.match.step(controls);
      state = world.match.state();
      for (const auto &event : state.weapon_effects)
        if (event.serial > serial) {
          serial = event.serial;
          if (event.effect != normal || event.weapon != weapon)
            continue;
          need(event.source_scheduled && event.source_presentation,
               "Producer did not publish an actual scheduled native effect");
          events.push_back(event);
          original.push_back({{"serial", event.serial},
                              {"tick", event.tick},
                              {"source_instance", event.source_instance},
                              {"locations", event.source_locations.size()}});
          sequential.deliver(event);
        }
      sequential.advance(state.tick);
    }
    need(events.size() > 1 &&
             events.size() ==
                 unsigned(before_loaded - state.players[slot].loaded),
         "Probe lacks multiple actual native shot requests");
    for (const auto &event : events)
      batched.deliver(event);
    const auto queued = batched.effects.checkpoint();
    const auto requests_trace = batched.trace;
    const auto particle_tick = events.back().tick + 1;
    batched.advance(particle_tick);
    need(!batched.draws.at(particle_tick).empty(),
         "Coalesced request comparison has no actual particle draws");
    same_particles(sequential, batched, particle_tick);
    report.data["coalesced_particle_draws_compared"] =
        batched.draws.at(particle_tick).size();
    sequential.advance(state.tick + 2);
    batched.advance(state.tick + 2);
    const auto sound_rows = [](const Json &trace, bool timing) {
      Json out = Json::array();
      for (auto row : trace)
        if (row.at("kind") == "sound") {
          if (!timing)
            row.erase("observed_tick");
          out.push_back(std::move(row));
        }
      return out;
    };
    need(sound_rows(sequential.trace, false) ==
             sound_rows(batched.trace, false),
         "Coalesced native requests changed authored sound order, owner or "
         "gain bits");
    const auto restore = [&](const NativeMuzzleEffects::Checkpoint &cut) {
      auto staged = batched.effects.prepare_restore(
          cut, SourceEffectRandom{batched.primary, batched.auxiliary}, {});
      const auto random = cut.random();
      batched.primary = random[0];
      batched.auxiliary = random[1];
      need(batched.effects.commit_restore(std::move(staged)),
           "Same-owner source journal checkpoint failed to publish");
    };
    const auto expected_random = batched.effects.checkpoint().random();
    const auto expected_sounds = batched.firing_sounds;
    restore(queued);
    batched.trace = requests_trace;
    batched.sounds = batched.firing_sounds = 0;
    const auto partial_tick = events.front().tick + 1;
    batched.advance(partial_tick);
    need(batched.firing_sounds > 0 && batched.firing_sounds < expected_sounds,
         "Source journal checkpoint has no active binding and future requests");
    const auto middle = batched.effects.checkpoint();
    const auto prefix_trace = batched.trace;
    const auto prefix_sounds = batched.sounds,
               prefix_firing = batched.firing_sounds;
    for (auto tick = partial_tick + 1; tick <= state.tick + 2; ++tick)
      batched.advance(tick);
    for (auto tick = partial_tick + 1; tick <= state.tick; ++tick)
      same_particles(sequential, batched, tick);
    const auto timed_sounds = sound_rows(batched.trace, true);
    const auto replay_draws = batched.draws;
    const auto replay_pools = batched.pools;
    need(batched.effects.checkpoint().random() == expected_random &&
             batched.firing_sounds == expected_sounds,
         "Queued requests changed after incremental checkpoint replay");
    restore(middle);
    batched.trace = prefix_trace;
    batched.sounds = prefix_sounds;
    batched.firing_sounds = prefix_firing;
    for (auto tick = partial_tick + 1; tick <= state.tick + 2; ++tick)
      batched.advance(tick);
    need(sound_rows(batched.trace, true) == timed_sounds,
         "Checkpoint lost native request order, binding, sound timing or gain");
    need(batched.effects.checkpoint().random() == expected_random,
         "Checkpoint changed native request RNG streams");
    Pipeline replay(*world.cache, 1, 2, fire_sound, weapon, markers,
                    ready.view);
    replay.draws = replay_draws;
    replay.pools = replay_pools;
    for (auto tick = partial_tick + 1; tick <= state.tick + 2; ++tick)
      same_particles(replay, batched, tick);
    report.data["pending_and_active_checkpoint_replay_equal"] = true;
    report.data["native_ammunition_consumed"] =
        before_loaded - state.players[slot].loaded;
    report.data["native_events"] = events.size();
    report.data["original_requests"] = original;
    report.data["sequential"] = {{"sounds", sequential.sounds},
                                 {"firing_sounds", sequential.firing_sounds},
                                 {"skipped_NONE", sequential.skipped},
                                 {"reuses", sequential.reuses},
                                 {"trace", sequential.trace}};
    report.data["coalesced"] = {{"sounds", batched.sounds},
                                {"firing_sounds", batched.firing_sounds},
                                {"skipped_NONE", batched.skipped},
                                {"reuses", batched.reuses},
                                {"trace", batched.trace}};
    report.data["scope"] =
        "Actual native authored SMG journal through the client source-request "
        "interface: individual/coalesced delivery, exact sound "
        "owner/order/gain, both RNG streams and particle GPU input bytes at an "
        "authenticated fixed model pose, including queued and active "
        "checkpoint replay. No framebuffer or PCM waveform claim";
    need(events.size() > 1 &&
             events.size() ==
                 unsigned(before_loaded - state.players[slot].loaded),
         "Probe lacks multiple actual native shot requests");
    need(sequential.firing_sounds == events.size(),
         "Sequential client delivery already differs from native shot count");
    need(batched.firing_sounds == sequential.firing_sounds,
         "Coalesced native requests lost client firing sound cues");
    need(batched.primary == sequential.primary &&
             batched.auxiliary == sequential.auxiliary,
         "Coalesced native requests changed an event RNG stream");
    report.data["RNG_streams_equal"] = true;
    report.data["passed"] = true;
    return 0;
  } catch (const std::exception &e) {
    report.data["error"] = e.what();
    std::cerr << e.what() << '\n';
    return 1;
  }
}
