#include "halo2/bsp_atmosphere.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_native_particle_history_tu15.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
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
using State = std::map<std::string, std::vector<std::byte>>;
State native_state(const SourceRuntimeMemory &memory) {
  State out;
  const auto copy = [&](const std::string &name, auto at, unsigned size) {
    const auto bytes = memory.view(at, size);
    out[name] = {bytes.begin(), bytes.end()};
  };
  for (const auto [global, stride] :
       std::array<std::pair<std::uint32_t, unsigned>, 6>{{{0x4f7208u, 60},
                                                          {0x4f720cu, 400},
                                                          {0x51df74u, 84},
                                                          {0x52d71cu, 52},
                                                          {0x52d718u, 76},
                                                          {0x52d714u, 64}}}) {
    const auto pool = word(memory, global), capacity = word(memory, pool + 0x20u);
    need(capacity && capacity <= 65536 && word(memory, pool + 0x24u) == stride,
         "Native continuation lost a real pool layout");
    const auto label = std::to_string(global);
    copy(label + "/allocator", pool + 0x34u, 14);
    copy(label + "/records", word(memory, pool + 0x44u), capacity * stride);
    copy(label + "/occupancy", word(memory, pool + 0x48u), (capacity + 31) / 32 * 4);
  }
  copy("rng", word(memory, 0x4f3cd8u), 8);
  return out;
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
    eye = ordinary_unit_eye(player.position, biped.camera_height, biped.crouch_camera_height,
                            player.crouch);
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
    visibility =
        source_portal_visibility(world.cache->load_source_portal_bsp(0), std::span{&camera, 1},
                                 world.cache->load_bsp_atmosphere(0).cluster_at(eye), mask.words);
    view.position = eye;
    view.forward = forward;
    view.up = up;
    view.width = 640;
    view.height = 480;
    const auto &basis = camera.camera_to_world.basis;
    view.orientation = {basis[0].x, basis[0].y, basis[0].z, basis[1].x, basis[1].y,
                        basis[1].z, basis[2].x, basis[2].y, basis[2].z};
    const auto &weapon = world.content.weapons.at(player.weapon);
    for (const auto &row : weapon.firing_effects)
      for (auto effect : row.effects)
        if (effect != null_datum) {
          const auto tag = world.cache->query_tag({.handle = TagHandle{effect}});
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
      draws = graphics.draw_first_person(source, visibility, view, 0, angles.projection_angle,
                                         angles.lighting_distance_scale);
    });
    return draws;
  }
};
} // namespace
int main(int argc, char **argv) {
  if (argc != 4 && argc != 5)
    return 2;
  Report report{argv[3]};
  try {
    need(std::getenv("HALO2_TEST_SCRATCH"), "Use owned test storage");
    if (!std::filesystem::is_regular_file(argv[1]))
      return 77;
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
        std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH")) / "fx-uninterrupted";
    MultiplayerPreparedWorld uninterrupted(options);
    uninterrupted.bind_source_local_views(uninterrupted.local_slots);
    need(uninterrupted.cache->is_mcc(), "Continuation requires prepared MCC");
    std::array<Controls, multiplayer_capacity> controls{};
    for (unsigned i = 0; i < 60; ++i)
      uninterrupted.match.step(controls);
    {
      Frame previous_frame(uninterrupted);
      (void)previous_frame.advance(1.f / 30.f);
    }
    const auto before_fire = uninterrupted.match.state();
    const auto slot = uninterrupted.local_slots.front();
    controls[slot].fire = true;
    uninterrupted.match.step(controls);
    const auto fired = uninterrupted.match.state();
    need(fired.players[slot].loaded < before_fire.players[slot].loaded,
         "Fixture has no actual native SMG ammunition consumption");
    std::set<std::uint32_t> normal_effects;
    for (const auto &row :
         uninterrupted.content.weapons.at(fired.players[slot].weapon).firing_effects)
      normal_effects.insert(row.effects[0]);
    const auto prior_serial =
        before_fire.weapon_effects.empty() ? 0 : before_fire.weapon_effects.back().serial;
    bool authored{};
    for (const auto &event : fired.weapon_effects)
      if (event.serial > prior_serial && event.weapon == fired.players[slot].source_weapon &&
          normal_effects.contains(event.effect) && event.source_scheduled &&
          event.source_instance != null_datum)
        authored = true;
    need(authored, "Fixture has no newly authored native normal firing event");
    report.data["native_shot"] = {{"before_loaded", before_fire.players[slot].loaded},
                                  {"after_loaded", fired.players[slot].loaded},
                                  {"new_authored_firing_event", authored}};
    SourceMatchAuthority source(uninterrupted.match, *uninterrupted.source_objects);
    constexpr SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15}, std::byte{0x4c}};
    const auto incoming = source.checkpoint(binding);
    const auto original = native_state(uninterrupted.source_objects->memory());
    Frame reference(uninterrupted);
    unsigned seen{};
    for (unsigned i = 0; i < 2; ++i)
      seen += reference.advance(1.f / 60.f).size();
    const auto aged = native_state(uninterrupted.source_objects->memory());
    need(original != aged, "Continuation control has no changed native FX bytes");
    need(seen, "Uninterrupted reference produced no native FP particles");
    options.source_object_savegame_directory =
        std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH")) / "fx-adopted";
    MultiplayerPreparedWorld adopted(options);
    adopted.bind_source_local_views(adopted.local_slots);
    SourceMatchAuthority recipient(adopted.match, *adopted.source_objects);
    const bool retain = argc == 4;
    if (!retain)
      need(std::string_view(argv[4]) == "without-retention", "Unknown continuation control");
    {
      auto prepared = source.prepare(incoming, binding);
      bool rejected{};
      try {
        prepared.retain_native_particle_history_from(source);
      } catch (const FormatError &) {
        rejected = true;
      }
      need(rejected, "Particle retention accepted its own staging source");
    }
    need(native_state(uninterrupted.source_objects->memory()) == aged,
         "Rejected staging source changed live native particle state");
    {
      auto prepared = recipient.prepare(incoming, binding);
      prepared.retain_live_multiplayer_render_history_from(source);
      if (retain)
        prepared.retain_native_particle_history_from(source);
      need(recipient.commit(std::move(prepared)), "Native continuation import did not commit");
    }
    const auto restored = native_state(adopted.source_objects->memory());
    Json mismatch = Json::array();
    for (const auto &[name, bytes] : aged)
      if (restored.at(name) != bytes)
        mismatch.push_back(name);
    report.data["initial_mismatched_native_owners"] = mismatch;
    if (!retain) {
      const auto particle = std::to_string(0x52d714u) + "/records";
      need(std::any_of(
               mismatch.begin(), mismatch.end(),
               [&](const auto &name) { return name.template get<std::string>() == particle; }),
           "Stale control did not detect the already advanced native particle records");
      report.data["stale_particle_control_detected"] = true;
      report.data["passed"] = true;
      std::cout << report.data.dump() << '\n';
      return 0;
    }
    need(mismatch.empty(), "Native continuation import lost already advanced FX state");
    Frame candidate(adopted);
    unsigned compared{}, draws{};
    Json samples = Json::array();
    for (unsigned frame = 0; frame < 24; ++frame) {
      const auto a = reference.advance(1.f / 60.f), b = candidate.advance(1.f / 60.f);
      const auto a_state = native_state(uninterrupted.source_objects->memory()),
                 b_state = native_state(adopted.source_objects->memory());
      Json changed = Json::array();
      for (const auto &[name, bytes] : a_state)
        if (b_state.at(name) != bytes)
          changed.push_back(name);
      samples.push_back({{"frame", frame},
                         {"reference_draws", a.size()},
                         {"adopted_draws", b.size()},
                         {"different_native_owners", changed}});
      report.data["frames"] = samples;
      need(changed.empty(), "Adopted native FX frame diverged from uninterrupted native execution");
      need(a.size() == b.size(), "Adopted native FP particle count diverged");
      for (unsigned i = 0; i < a.size(); ++i) {
        need(a[i].effect_owner == b[i].effect_owner &&
                 a[i].effect_definition == b[i].effect_definition,
             "Adopted native particle changed its real effect owner");
        need(!std::memcmp(a[i].vertex.attributes.data(), b[i].vertex.attributes.data(),
                          sizeof(a[i].vertex.attributes)) &&
                 !std::memcmp(a[i].vertex.constants.data(), b[i].vertex.constants.data(),
                              sizeof(a[i].vertex.constants)),
             "Adopted native particle GPU inputs diverged");
      }
      ++compared;
      draws += a.size();
    }
    {
      SourceRuntimeMemory invalid;
      invalid.restore(incoming.world.world.bank);
      const auto pool = word(invalid, 0x52d714u), bad_count = word(invalid, pool + 0x20u) + 1u;
      std::array<std::byte, 4> value{};
      for (unsigned i = 0; i < 4; ++i)
        value[i] = std::byte(bad_count >> (8 * i));
      invalid.write(pool + 0x3cu, value);
      const auto before = native_state(invalid);
      need(before != native_state(uninterrupted.source_objects->memory()),
           "Atomic rejection control has no different native particle history");
      bool rejected{};
      try {
        source_stage_native_particle_history_tu15(uninterrupted.source_objects->memory(), invalid);
      } catch (const FormatError &e) {
        rejected = std::string_view(e.what()) ==
                   "Native particle history exceeds its allocator cursor/count bounds";
      }
      need(rejected, "Particle retention accepted an out-of-range final pool count");
      need(before == native_state(invalid),
           "Rejected final pool count partially wrote native particle history");
      report.data["final_pool_rejected_atomically"] = true;
    }
    controls[slot].fire = false;
    uninterrupted.match.step(controls);
    const auto advanced = native_state(uninterrupted.source_objects->memory());
    {
      auto prepared = recipient.prepare(incoming, binding);
      bool rejected{};
      try {
        prepared.retain_native_particle_history_from(source);
      } catch (const FormatError &e) {
        rejected =
            std::string_view(e.what()) ==
            "Native particle history requires the same multiplayer session and gameplay tick";
      }
      need(rejected, "Particle retention accepted a different gameplay tick");
      need(recipient.commit(std::move(prepared)),
           "Rejected particle retention damaged the prepared authority");
    }
    need(native_state(adopted.source_objects->memory()) == original,
         "Rejected different-tick retention changed the prepared native particle state");
    need(native_state(uninterrupted.source_objects->memory()) == advanced,
         "Rejected different-tick retention changed its native source");
    report.data["different_tick_rejected_atomically"] = true;
    report.data["staging_source_rejected"] = true;
    report.data["passed"] = true;
    report.data["scope"] =
        "Actual prepared-MCC native particle/frame continuation versus uninterrupted execution at "
        "the same complete source gameplay tick; no pixels, PCM, remote-role or full goal claim";
    report.data["frames_compared"] = compared;
    report.data["FP_draws_compared"] = draws;
    report.data["initial_reference_draws"] = seen;
    report.data["retention_selected"] = retain;
    std::cout << report.data.dump() << '\n';
    return 0;
  } catch (const std::exception &e) {
    report.data["error"] = e.what();
    std::cerr << e.what() << '\n';
    return 1;
  }
}
