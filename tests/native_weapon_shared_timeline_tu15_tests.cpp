#define SDL_MAIN_HANDLED
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/effect_presentation_step.hpp"
#include "halo2/effect_sound.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/world_effects.hpp"
#include <SDL.h>
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
  NativeWorldEffects world_effects;
  const Cache &cache;
  NativeMuzzleEffects effects;
  unsigned audio_requests{}, primary_pcm{}, world_sound_requests{};
  std::unique_ptr<MultiplayerAudio> audio;
  std::uint32_t weapon;
  NativeMuzzleEffects::Markers markers;
  MuzzleEffectView view;
  std::map<std::uint32_t, std::vector<ParticleDraw>> draws;
  std::map<std::uint32_t, std::array<std::size_t, 2>> pools;
  std::uint32_t source = 0xffffffffu;
  unsigned skipped{}, reuses{}, sounds{}, firing_sounds{}, observed_tick{};
  Json trace = Json::array();
  std::optional<std::uint32_t> advanced;
  Pipeline(const Cache &cache, const CollisionWorld &collision, std::uint32_t a,
           std::uint32_t b, std::uint32_t f, std::uint32_t w,
           NativeMuzzleEffects::Markers m, const MuzzleEffectView &v)
      : primary(a), auxiliary(b), fire_sound(f),
        world_effects(cache, mesh, collision,
                      SourceEffectRandom{primary, auxiliary}),
        cache(cache), effects(cache, SourceEffectRandom{primary, auxiliary}),
        weapon(w), markers(std::move(m)), view(v) {
    {
      audio = std::make_unique<MultiplayerAudio>(
          cache, 0x105240u,
          SdlSoundPlatformConfiguration{
              SdlSoundPlatformConfiguration::Delivery::Manual, 256, {}},
          [this](const auto &packet) {
            primary_pcm += packet.sound == fire_sound;
          },
          &auxiliary);
    }
  }
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
      if (audio) {
        ++audio_requests;
        if (cue.position)
          audio->impulse_at(cue.cue.sound, *cue.position, cue.cue.gain,
                            MultiplayerAudioKind::Impulse, cue.owner);
        else
          audio->impulse(cue.cue.sound, cue.cue.gain,
                         MultiplayerAudioKind::Impulse, cue.owner);
      }
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
    NativeWorldEffects::ObjectMarkers locations;
    for (const auto &location : event.source_locations) {
      const auto &t = location.frame;
      locations[cache.string_id(location.name)].push_back(
          {t.position, t.basis[2], t.basis[0], t.basis[0], t.basis[2], false, 0,
           0});
    }
    world_effects.request_weapon(
        event.effect, event.weapon, event.tick, event.colors, event.velocity,
        std::bit_cast<float>(event.a_bits), std::bit_cast<float>(event.b_bits),
        event.source_instance, false,
        event.source_locations_observed ? &locations : nullptr);
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
    const auto before_random = std::array{primary, auxiliary};
    const auto before_particles = effects.particle_count();
    const auto before_locations = effects.location_count();
    const auto before_tick = effects.completed_tick();
    effects.stage_pose(weapon, markers, view);
    need(before_random == std::array{primary, auxiliary} &&
             before_particles == effects.particle_count() &&
             before_locations == effects.location_count() &&
             before_tick == effects.completed_tick(),
         "Staging received pose inputs advanced an emission or RNG owner");
    catch_up_effect_presentation(world_effects, effects, tick, 1.f / 30.f,
                                 {[&](auto at) {
                                    observed_tick = at;
                                    collect_world();
                                  },
                                  [&](auto at) {
                                    observed_tick = at;
                                    collect();
                                  }});
    observed_tick = tick;
    world_effects.advance(tick, 1.f / 30.f);
    collect_world();
    effects.pose(weapon, markers, view, tick, 1.f / 30.f);
    effects.advance(tick, 1.f / 30.f);
    collect();
    advanced = tick;
    draws[tick] = effects.draw(weapon, view, tick, 0, 1.f / 30.f);
    pools[tick] = {effects.particle_count(), effects.location_count()};
  }
  void collect_world() {
    for (const auto &sound : world_effects.take_sounds()) {
      ++world_sound_requests;
      trace.push_back({{"kind", "world_sound"},
                       {"sound", sound.sound},
                       {"owner", sound.owner},
                       {"tick", sound.tick},
                       {"gain_bits", std::bit_cast<std::uint32_t>(sound.gain)},
                       {"observed_tick", observed_tick}});
      audio->impulse_at(sound.sound, sound.position, sound.gain,
                        MultiplayerAudioKind::Impulse, sound.owner);
    }
  }
  void contact(const NativeGrenadeDetonation &event) {
    if (!event.source_locations.empty())
      world_effects.spawn_source(event.effect, event.projectile, event.tick,
                                 event.source_locations, event.colors,
                                 event.controls, event.replica_render);
    else
      world_effects.spawn(event.effect, event.projectile, event.tick,
                          {event.position, event.up, event.forward,
                           event.forward, event.up, false});
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
  SDL_SetMainReady();
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
    const auto initial_primary = word(memory, random),
               initial_auxiliary = word(memory, random + 4),
               initial_tick = state.tick;
    Pipeline sequential(*world.cache, world.match.simulation().collision(),
                        initial_primary, initial_auxiliary, fire_sound, weapon,
                        markers, ready.view),
        batched(*world.cache, world.match.simulation().collision(),
                initial_primary, initial_auxiliary, fire_sound, weapon, markers,
                ready.view);
    sequential.advance(state.tick);
    batched.advance(state.tick);
    std::uint32_t serial =
        state.weapon_effects.empty() ? 0 : state.weapon_effects.back().serial;
    std::uint64_t contact_serial =
        state.grenade_detonations.empty()
            ? 0
            : state.grenade_detonations.back().serial;
    std::vector<NativeGrenadeDetonation> contacts;
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
      for (const auto &contact : state.grenade_detonations)
        if (contact.serial > contact_serial) {
          contact_serial = contact.serial;
          contacts.push_back(contact);
          sequential.contact(contact);
        }
      sequential.advance(state.tick);
    }
    need(events.size() > 1 &&
             events.size() ==
                 unsigned(before_loaded - state.players[slot].loaded),
         "Probe lacks multiple actual native shot requests");

    Json minimal = Json::array();
    for (unsigned n : {1u, 2u}) {
      Pipeline left(*world.cache, world.match.simulation().collision(),
                    initial_primary, initial_auxiliary, fire_sound, weapon,
                    markers, ready.view),
          right(*world.cache, world.match.simulation().collision(),
                initial_primary, initial_auxiliary, fire_sound, weapon, markers,
                ready.view);
      left.advance(initial_tick);
      right.advance(initial_tick);
      const auto end = events.at(n - 1).tick + 1;
      for (auto t = initial_tick + 1; t <= end; ++t) {
        for (unsigned j = 0; j < n; ++j)
          if (events[j].tick == t)
            left.deliver(events[j]);
        left.advance(t);
      }
      for (unsigned j = 0; j < n; ++j)
        right.deliver(events[j]);
      right.advance(end);
      bool equal = true;
      std::string reason;
      try {
        same_particles(left, right, end);
      } catch (const std::exception &e) {
        equal = false;
        reason = e.what();
      }
      minimal.push_back(
          {{"requests", n},
           {"particle_bytes_equal", equal},
           {"reason", reason},
           {"sequential_cues", left.firing_sounds},
           {"coalesced_cues", right.firing_sounds},
           {"sequential_draws", left.draws.at(end).size()},
           {"coalesced_draws", right.draws.at(end).size()},
           {"final_rng_equal", left.primary == right.primary &&
                                   left.auxiliary == right.auxiliary}});
    }
    report.data["minimal_subsets"] = minimal;
    for (const auto &row : minimal)
      need(row.at("particle_bytes_equal").get<bool>(),
           "Minimal shared timeline changed particle bytes");
    for (const auto &event : events)
      batched.deliver(event);
    for (const auto &contact : contacts)
      batched.contact(contact);
    const auto particle_tick = events.back().tick + 1;
    batched.advance(particle_tick);
    sequential.advance(state.tick + 2);
    batched.advance(state.tick + 2);
    report.data["native_generic_contacts"] = contacts.size();
    report.data["world_sound_requests"] = Json::array(
        {sequential.world_sound_requests, batched.world_sound_requests});
    std::set<std::uint32_t> contact_effects;
    Json authored_contacts=Json::array();
    for(const auto &contact:contacts)if(contact_effects.insert(contact.effect).second) {
      const auto tag=world.cache->query_tag({.handle=TagHandle{contact.effect}});
      authored_contacts.push_back({{"effect",contact.effect},{"name",tag?tag->name:""},
          {"authored_sound_parts",authored_effect_sound_parts(*world.cache,contact.effect).size()}});
    }
    report.data["authored_contacts"]=authored_contacts;
    need(!contacts.empty() && sequential.world_sound_requests == batched.world_sound_requests,
         "Shared timeline changed actual world contact audio count");
    report.data["native_audio_enabled"] = bool(sequential.audio);
    report.data["native_audio_secondary_seed"] = 0x105240u;
    report.data["audio_requests"] =
        Json::array({sequential.audio_requests, batched.audio_requests});
    report.data["primary_native_pcm"] =
        Json::array({sequential.primary_pcm, batched.primary_pcm});
    report.data["native_ammunition_consumed"] =
        before_loaded - state.players[slot].loaded;
    report.data["native_events"] = events.size();
    report.data["original_requests"] = original;
    report.data["sequential"] = {
        {"sounds", sequential.sounds},
        {"firing_sounds", sequential.firing_sounds},
        {"primary", sequential.primary},
        {"auxiliary", sequential.auxiliary},
        {"active_draws", sequential.draws.at(particle_tick).size()},
        {"trace", sequential.trace}};
    report.data["coalesced"] = {
        {"sounds", batched.sounds},
        {"firing_sounds", batched.firing_sounds},
        {"primary", batched.primary},
        {"auxiliary", batched.auxiliary},
        {"active_draws", batched.draws.at(particle_tick).size()},
        {"trace", batched.trace}};
    report.data["RNG_streams_equal"] =
        sequential.primary == batched.primary &&
        sequential.auxiliary == batched.auxiliary;
    report.data["scope"] =
        "Same genuine native selected SMG firing journal through world and "
        "first-person recipients sharing their production RNG pair; "
        "individual/coalesced scheduling, exact FP particle GPU bytes. Fixed "
        "authenticated FP marker pose, not live placement/Xbox output proof.";
    need(events.size() > 1 &&
             events.size() ==
                 unsigned(before_loaded - state.players[slot].loaded),
         "Missing actual native shot journal");
    need(sequential.audio_requests == events.size() * 2 &&
             batched.audio_requests == events.size() * 2 &&
             sequential.primary_pcm == events.size() &&
             batched.primary_pcm == events.size(),
         "Shared timeline lacks actual native primary PCM delivery");
    need(sequential.firing_sounds == events.size() &&
             batched.firing_sounds == events.size(),
         "Coupled recipients changed primary cue count");
    need(!sequential.draws.at(particle_tick).empty() &&
             !batched.draws.at(particle_tick).empty(),
         "Coupled recipients lack active particle draws");
    const auto sound_rows = [](const Json &rows) {
      Json out = Json::array();
      for (const auto &row : rows)
        if (row.at("kind") == "sound" || row.at("kind") == "world_sound")
          out.push_back(row);
      return out;
    };
    need(
        sound_rows(sequential.trace) == sound_rows(batched.trace),
        "Shared timeline changed native world/FP audio dispatch order or tick");
    same_particles(sequential, batched, particle_tick);
    need(report.data["RNG_streams_equal"].get<bool>(),
         "Coupled recipients changed final shared RNG");
    sequential.world_effects.advance(state.tick + 3, 1.f / 30.f);
    const auto completed_fp = sequential.effects.completed_tick();
    bool mismatched = false;
    try {
      catch_up_effect_presentation(sequential.world_effects, sequential.effects,
                                   state.tick + 4, 1.f / 30.f, {});
    } catch (const FormatError &e) {
      mismatched = std::string_view(e.what()).find("do not share") !=
                   std::string_view::npos;
    }
    need(mismatched && sequential.effects.completed_tick() == completed_fp,
         "Mismatched effect clocks were advanced instead of refused");
    report.data["mismatched_clock_refused_without_advancing_FP"] = true;
    report.data["passed"] = true;
    return 0;
  } catch (const std::exception &e) {
    report.data["error"] = e.what();
    std::cerr << e.what() << '\n';
    return 1;
  }
}
