#include "halo2/bsp_atmosphere.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_camera.hpp"
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
  if (argc != 4)
    return 2;
  if (!std::filesystem::is_regular_file(argv[1]))
    return 77;
  Report report{argv[3]};
  try {
    need(std::getenv("HALO2_TEST_SCRATCH"), "Use owned test storage");
    MultiplayerOptions options;
    options.map = argv[1];
    options.shared_maps = argv[2];
    options.local_players = 1;
    options.starting_weapon = 6;
    options.source_object_world = true;
    options.host_port = 1;
    options.mute = true;
    options.session_random_seed = 0x105240;
    options.variant = {"Slayer", 0, {{2, 0}, {10, 0}, {89, 6}}};
    options.source_object_savegame_directory =
        std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH")) / "plasma-native-effects-world";
    MultiplayerPreparedWorld world(options);
    world.bind_source_local_views(world.local_slots);
    need(world.cache->is_mcc(), "Requires prepared MCC");
    std::array<Controls, multiplayer_capacity> controls{};
    for (unsigned i = 0; i < 60; ++i)
      world.match.step(controls);
    Frame frame(world);
    (void)frame.advance(1.f / 30.f);
    std::vector<std::uint32_t> pending_sounds;
    struct Observer {
      MultiplayerObjectRuntime &native;
      ~Observer() { native.tu15_world().observe_sound_requests_with_context({}); }
    } observer{*world.source_objects};
    world.source_objects->tu15_world().observe_sound_requests_with_context(
        [&](auto sound, std::span<const std::byte, 160>, auto) {
          pending_sounds.push_back(sound);
        });
    const auto slot = world.local_slots.front();
    std::uint32_t serial{};
    unsigned total_draws{}, events{};
    Json ticks = Json::array();
    const auto &weapon = world.content.weapons.at(world.match.state().players[slot].weapon);
    report.data["weapon"] = weapon.datum;
    const auto effect_name = [&](std::uint32_t id) {
      const auto *t = world.cache->query_tag({.handle = TagHandle{id}});
      return t ? t->name : std::string{};
    };
    Json authored = Json::array();
    for (const auto &row : weapon.firing_effects)
      for (auto effect : row.effects)
        if (effect != null_datum)
          authored.push_back({{"definition", effect}, {"name", effect_name(effect)}});
    for (const auto &barrel : weapon.barrel_views)
      for (const auto &row : barrel->firing_effects)
        for (auto effect : row.effects)
          if (effect != null_datum)
            authored.push_back({{"definition", effect}, {"name", effect_name(effect)}});
    report.data["authored_firing_effects"] = authored;
    Json layouts = Json::array();
    std::set<std::uint32_t> inspected;
    const auto &memory = world.source_objects->memory();
    const auto tags = word(memory, 0x4f0414u);
    for (const auto &entry : authored) {
      const auto id = entry.at("definition").get<std::uint32_t>();
      if (!inspected.insert(id).second)
        continue;
      const auto root = word(memory, tags + (id & 65535u) * 16u + 8u);
      const auto count = word(memory, root + 0x14u), events = word(memory, root + 0x18u);
      need(count <= 64, "Native effect event count escapes probe bounds");
      Json rows = Json::array();
      for (unsigned event = 0; event < count; ++event) {
        const auto e = events + event * 0x38u;
        const auto part_count = word(memory, e + 0x18u), parts = word(memory, e + 0x1cu),
                   system_count = word(memory, e + 0x30u), systems = word(memory, e + 0x34u);
        need(part_count <= 1024 && system_count <= 1024,
             "Native authored effect rows escape probe bounds");
        Json groups = Json::array(), cameras = Json::array();
        for (unsigned part = 0; part < part_count; ++part)
          groups.push_back({{"group", word(memory, parts + part * 0x38u + 8u)},
                            {"definition", word(memory, parts + part * 0x38u + 0x10u)}});
        for (unsigned system = 0; system < system_count; ++system)
          cameras.push_back(Bytes(memory.view(systems + system * 0x38u + 0x12u, 2)).u16(0));
        rows.push_back({{"event", event}, {"parts", groups}, {"particle_system_cameras", cameras}});
      }
      layouts.push_back({{"definition", id}, {"name", effect_name(id)}, {"native_events", rows}});
    }
    report.data["native_authored_layouts"] = layouts;

    const auto normal = weapon.firing_effects.at(0).effects[0],
               charged = weapon.barrel_views.at(1)->firing_effects.at(0).effects[0];
    need(normal != null_datum && charged != null_datum && normal != charged,
         "Fixture lost distinct authored plasma pistol barrels");
    for (const auto &[phase, held, count] :
         std::array<std::tuple<const char *, bool, unsigned>, 4>{{{"tap", true, 1},
                                                                  {"tap-release", false, 60},
                                                                  {"charge", true, 120},
                                                                  {"charge-release", false, 60}}})
      for (unsigned i = 0; i < count; ++i) {
        controls[slot].fire = held;
        world.match.step(controls);
        const auto state = world.match.state();
        const auto &player = state.players[slot];
        const auto draws = frame.advance(1.f / 30.f);
        total_draws += draws.size();
        std::map<std::uint32_t, unsigned> drawn;
        for (const auto &draw : draws) {
          ++drawn[draw.effect_definition];
          if (draw.effect_definition == normal)
            need(draw.effect_owner == player.source_weapon,
                 "Native tap particle lost its actual full weapon owner");
        }
        Json drawn_effects = Json::array();
        for (const auto &[id, count] : drawn)
          drawn_effects.push_back(
              {{"definition", id}, {"name", effect_name(id)}, {"draws", count}});
        Json born = Json::array();
        for (const auto &event : state.weapon_effects)
          if (event.serial > serial) {
            serial = event.serial;
            ++events;
            born.push_back({{"definition", event.effect},
                            {"name", effect_name(event.effect)},
                            {"instance", event.source_instance},
                            {"scheduled", event.source_scheduled}});
          }
        Json sounds = Json::array();
        for (auto sound : pending_sounds)
          sounds.push_back({{"definition", sound}, {"name", effect_name(sound)}});
        pending_sounds.clear();
        Json counts = Json::array();
        for (auto global : {0x4f7208u, 0x4f720cu, 0x51df74u, 0x52d71cu, 0x52d718u, 0x52d714u}) {
          const auto &memory = world.source_objects->memory();
          const auto pool = word(memory, global);
          counts.push_back({{"global", global},
                            {"high_water", word(memory, pool + 0x38u)},
                            {"count", word(memory, pool + 0x3cu)}});
        }
        ticks.push_back({{"phase", phase},
                         {"tick", state.tick},
                         {"loaded", player.loaded},
                         {"trigger_mode", player.weapon_trigger.mode},
                         {"trigger_ticks", player.weapon_trigger.ticks},
                         {"charge", player.weapon_trigger.charge},
                         {"fp_draws", draws.size()},
                         {"draw_effects", drawn_effects},
                         {"sound_requests", sounds},
                         {"born", born},
                         {"pools", counts}});
      }

    unsigned normal_events{}, charged_events{}, normal_draws{}, charged_draws{}, charging_draws{};
    std::map<std::uint32_t, unsigned> tap_sounds, release_sounds;
    Json phases = Json::object();
    for (const auto &tick : ticks) {
      const auto phase = tick.at("phase").get<std::string>();
      auto &summary = phases[phase];
      if (summary.is_null())
        summary = {{"FP_draws", 0}, {"native_events", 0}, {"native_sound_requests", 0}};
      summary["FP_draws"] =
          summary["FP_draws"].get<unsigned>() + tick.at("fp_draws").get<unsigned>();
      summary["native_events"] = summary["native_events"].get<unsigned>() + tick.at("born").size();
      summary["native_sound_requests"] =
          summary["native_sound_requests"].get<unsigned>() + tick.at("sound_requests").size();
      for (const auto &born : tick.at("born")) {
        need(born.at("scheduled").get<bool>() &&
                 born.at("instance").get<std::uint32_t>() != null_datum,
             "Plasma firing event has no original native scheduled instance");
        const auto definition = born.at("definition").get<std::uint32_t>();
        if (definition == normal) {
          need(phase == "tap-release", "Normal plasma firing effect crossed charge/release phases");
          ++normal_events;
        } else if (definition == charged) {
          need(phase == "charge-release",
               "Charged plasma firing effect crossed tap/release phases");
          ++charged_events;
        } else
          throw FormatError("Plasma firing event lost its actual authored barrel effect");
      }
      for (const auto &draw : tick.at("draw_effects")) {
        const auto definition = draw.at("definition").get<std::uint32_t>(),
                   count = draw.at("draws").get<unsigned>();
        if (definition == normal)
          normal_draws += count;
        if (definition == charged)
          charged_draws += count;
        if (phase == "charge")
          charging_draws += count;
      }
      for (const auto &sound : tick.at("sound_requests")) {
        const auto id = sound.at("definition").get<std::uint32_t>();
        if (phase == "tap-release")
          ++tap_sounds[id];
        if (phase == "charge-release")
          ++release_sounds[id];
      }
    }
    need(normal_events == 1 && charged_events == 1,
         "Plasma tap and charged release did not each produce one actual native barrel event");
    need(normal_draws > 0 && charging_draws > 0,
         "Plasma tap or held charge has no actual native FP particle submissions");
    need(charged_draws == 0,
         "Sound-only charged firing effect unexpectedly submitted FP particles");
    unsigned checked_normal_sounds{}, checked_charged_sounds{};
    for (const auto &layout : layouts) {
      const auto id = layout.at("definition").get<std::uint32_t>();
      if (id != normal && id != charged)
        continue;
      for (const auto &event : layout.at("native_events")) {
        if (id == charged)
          need(event.at("particle_system_cameras").empty(),
               "Charged sound-only fixture acquired authored particle systems");
        for (const auto &part : event.at("parts")) {
          need(part.at("group").get<std::uint32_t>() == 0x736e6421u,
               "Plasma firing fixture acquired another native part owner");
          const auto sound = part.at("definition").get<std::uint32_t>();
          if (sound == null_datum)
            continue;
          if (id == normal) {
            need(tap_sounds[sound] == 1,
                 "Native tap omitted or duplicated an authored firing sound request");
            ++checked_normal_sounds;
          } else {
            need(release_sounds[sound] == 1,
                 "Native charged release omitted or duplicated its authored firing sound request");
            ++checked_charged_sounds;
          }
        }
      }
    }
    need(checked_normal_sounds == 2 && checked_charged_sounds == 1,
         "Plasma firing fixture sound graph differs from qualified authored inputs");
    need(!frame.graphics.resource_failure_count(),
         "Native plasma particles quarantined an authored resource");
    report.data["phases"] = phases;
    report.data["normal_native_events"] = normal_events;
    report.data["charged_native_events"] = charged_events;
    report.data["normal_FP_draws"] = normal_draws;
    report.data["charged_FP_draws"] = charged_draws;
    report.data["normal_authored_sound_requests"] = checked_normal_sounds;
    report.data["charged_authored_sound_requests"] = checked_charged_sounds;
    report.data["FP_draws"] = total_draws;
    report.data["events"] = events;
    report.data["resource_failures"] = Json::array();
    for (const auto &f : frame.graphics.resource_failures())
      report.data["resource_failures"].push_back(
          {{"definition", f.definition}, {"reason", f.reason}});
    report.data["scope"] =
        "Actual native prepared-MCC plasma-pistol tap particle delivery and sound-only charged "
        "firing requests; no framebuffer, PCM waveform or full parity claim";
    report.data["passed"] = true;
    std::cout << "plasma census completed " << events << " events " << total_draws << " draws\n";
    return 0;
  } catch (const std::exception &e) {
    report.data["error"] = e.what();
    std::cerr << e.what() << '\n';
    return 1;
  }
}
