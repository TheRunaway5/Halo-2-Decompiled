// Live TU1.5 trajectory oracle for non-host movement prediction. A local
// player on prepared MCC Lockout is moved by scripted Controls through the
// retained TU1.5 world (the host's own owner chain, 14FEC0 -> 153070 -> DF880
// -> 1ED880 -> Havok 31B1A0). After every tick the native predictor is
// restarted from the host's motion N ticks earlier and replays the same N
// inputs; the distance to the host's present is the `hist` a non-host sees at
// that round trip. Several native control variants are measured side by side.
// The fixture places the player with the existing B95D0 test placement only;
// every measured move is the original world's.
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/nonhost_motion_adoption.hpp"
#include "halo2/nonhost_movement_prediction.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_player_profile.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value)
    throw FormatError(message);
}
struct Files {
  std::filesystem::path path;
  Files() {
    auto name = (std::filesystem::temp_directory_path() / "halo2-nonhost-trajectory-XXXXXX").string();
    check(::mkdtemp(name.data()) != nullptr, "Trajectory scratch creation failed");
    path = name;
  }
  ~Files() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};
std::uint32_t word(const SourceRuntimeMemory &bank, std::uint32_t at, unsigned n = 4) {
  const auto bytes = bank.view(at, n);
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(bytes[i]) << (8 * i);
  return v;
}
struct Sample {
  NonHostAuthority authority;
  NonHostMotionInput input; // the input whose step produced this authority
  std::string category;
};
struct VariantSpec {
  const char *name;
  bool source_control, source_facing, origin;
};
constexpr std::array variants{VariantSpec{"native", false, false, false},
                              VariantSpec{"source_control", true, false, false},
                              VariantSpec{"source_control_facing", true, true, false},
                              VariantSpec{"predictor", true, true, true}};
constexpr std::array windows{1u, 3u, 5u};
struct Stats {
  std::vector<float> values;
  nlohmann::json json() {
    std::sort(values.begin(), values.end());
    const auto at = [&](double q) {
      return values.empty() ? 0.f : values[std::min(values.size() - 1, std::size_t(q * double(values.size())))];
    };
    double sum = 0;
    for (const auto v : values)
      sum += v;
    return {{"count", values.size()},
            {"mean", values.empty() ? 0. : sum / double(values.size())},
            {"p50", at(.5)},
            {"p90", at(.9)},
            {"p99", at(.99)},
            {"max", values.empty() ? 0.f : values.back()}};
  }
};
// Host adoption bookkeeping on synthetic host state (no world required).
void adopter_checks() {
  NonHostMotionDefinition bounds;
  bounds.movement = {2.25f, 2.f, 2.f, 12.f, 1.f, .9f, .9f, 8.f, 3.f, 1.f};
  bounds.height = .7f;
  bounds.crouch_height = .5f;
  bounds.jump_velocity = 1.2f;
  struct Write {
    std::uint32_t object{};
    Vec3 position, velocity;
  };
  std::vector<Write> nudges;
  NonHostMotionWriter writer;
  writer.nudge = [&](std::uint32_t object, Vec3 position, Vec3 velocity) {
    nudges.push_back({object, position, velocity});
    return true;
  };
  NonHostMotionAdopter adopter(bounds, .2f, {}, writer);
  std::vector<CollisionTriangle> floor;
  for (const auto &t : {std::array{Vec3{-50, -50, 0}, Vec3{50, -50, 0}, Vec3{50, 50, 0}},
                        std::array{Vec3{-50, -50, 0}, Vec3{50, 50, 0}, Vec3{-50, 50, 0}}}) {
    CollisionTriangle c{};
    c.a = t[0];
    c.b = t[1];
    c.c = t[2];
    c.source_normal = {0, 0, 1};
    floor.push_back(c);
  }
  const CollisionWorld collision(floor);
  constexpr unsigned slot = 3;
  constexpr std::uint32_t unit = 0x12340007u;
  MultiplayerState state;
  state.players[slot].connected = true;
  state.players[slot].source_unit = unit;
  MultiplayerSourceMotion motion{{1, 2, 0}, {}, {1, 0, 0}, 0, 1, null_datum, -1, 0, 0, 0};
  const NonHostMotionAdopter::MotionReader read = [&](std::uint32_t object) -> std::optional<MultiplayerSourceMotion> {
    if (object != unit)
      return std::nullopt;
    return motion;
  };
  const auto observe = [&](std::uint32_t consumed) {
    const std::array views{NonHostMotionAdopter::View{slot, consumed}};
    adopter.observe(state, read, views);
  };
  observe(10);
  check(adopter.last_forced(slot) == NonHostForcedMotion::respawn && adopter.forced_sequence(slot) == 11u,
        "adopter: birth was not a forced move");
  observe(11);
  check(adopter.forced_sequence(slot) == 11u, "adopter: quiet tick forced a move");
  state.players[slot].source_teleport_serial = 1;
  observe(12);
  check(adopter.last_forced(slot) == NonHostForcedMotion::teleport && adopter.forced_sequence(slot) == 13u,
        "adopter: teleporter serial missed");
  UnitDamageEffectEvent recharge;
  recharge.serial = 5;
  recharge.target = unit;
  recharge.effect.kind = UnitDamageEffectKind::ShieldRecharging;
  state.unit_damage_effects = {recharge};
  observe(13);
  check(adopter.forced_sequence(slot) == 13u, "adopter: shield recharge treated as a damage force");
  auto hit = recharge;
  hit.serial = 6;
  hit.effect.kind = UnitDamageEffectKind::ShieldDamaged;
  state.unit_damage_effects.push_back(hit);
  observe(14);
  check(adopter.last_forced(slot) == NonHostForcedMotion::damage && adopter.forced_sequence(slot) == 15u,
        "adopter: damage on the unit missed");
  observe(15);
  check(adopter.forced_sequence(slot) == 15u, "adopter: retained damage event counted twice");
  for (int fall = 1; fall <= 6; ++fall) { // a long fall builds speed gradually
    motion.velocity = {0, 0, -float(fall)};
    observe(16);
  }
  motion.velocity = {0, 0, 0}; // landing removes speed: ordinary contact
  observe(17);
  check(adopter.forced_sequence(slot) == 15u, "adopter: a landing was treated as forced motion");
  motion.velocity = {6, 0, 2};
  observe(18);
  check(adopter.last_forced(slot) == NonHostForcedMotion::unexplained_velocity && adopter.forced_sequence(slot) == 19u,
        "adopter: man-cannon-like velocity missed");
  motion.velocity = {2, 0, 0};
  observe(19);
  NonHostMotionReport report;
  report.sequence = 20;
  report.baseline = 17; // predicted before the forced move at 19
  report.position = motion.position + Vec3{.5f, 0, 0};
  report.velocity = {2, 0, 0};
  auto result = adopter.adopt(read, state, collision, slot, report, 1);
  check(result.decision.rejection == NonHostRejection::stale && nudges.empty(),
        "adopter: pre-forced prediction adopted");
  report.sequence = 21;
  report.baseline = 19;
  report.velocity = {2.5f, 0, 0};
  result = adopter.adopt(read, state, collision, slot, report, 1);
  check(result.decision.kind == NonHostAdoptionKind::teleport && result.placement_unavailable && nudges.size() == 1 &&
            nudges[0].object == unit && nudges[0].velocity.x == 2.5f && nudges[0].position.x == report.position.x,
        "adopter: teleport without native placement did not take the nudge branch");
  report.sequence = 22;
  report.position = motion.position + Vec3{.1f, 0, 0};
  report.velocity = {2.1f, 0, 0}; // inside the 0.06 dead band of the host's 2.0
  result = adopter.adopt(read, state, collision, slot, report, 1);
  check(result.decision.kind == NonHostAdoptionKind::unchanged && nudges.size() == 2 && nudges[1].velocity.x == 2.1f,
        "adopter: decision-2 report did not reach B44C0 unfiltered");
  report.sequence = 23;
  report.velocity = {2.4f, 0, 0}; // outside the dead band, inside the speed bound
  result = adopter.adopt(read, state, collision, slot, report, 1);
  check(result.decision.kind == NonHostAdoptionKind::nudge && nudges.size() == 3 && nudges[2].velocity.x == 2.4f,
        "adopter: nudge did not pass B44C0's taken velocity");
  state.players[slot].dead = true;
  report.sequence = 24;
  check(adopter.adopt(read, state, collision, slot, report, 1).decision.rejection == NonHostRejection::invalid,
        "adopter: dead unit adopted motion");
  std::cout << "adopter checks passed\n";
}
} // namespace

int main(int argc, char **argv) try {
  std::cout << std::unitbuf;
  if (argc < 2 || argc > 3)
    throw FormatError("Supply a prepared MCC Lockout map and optionally a JSON report path");
  adopter_checks();
  if (!std::filesystem::is_regular_file(argv[1]))
    return 77;
  const Cache cache(argv[1]);
  check(cache.is_mcc(), "Trajectory oracle requires prepared MCC assets");
  const auto content = cache.load_content(0, true);
  const auto original = cache.original_data();
  check(original && original->unit_locomotion_deadzone_bits, "Trajectory oracle requires the TU1.5 dead zone operand");
  UiVariant variant{"Slayer", 0, {{89, 5}}};
  auto rules = slayer_rules(variant, false);
  rules.weapon = cache.slayer_starting_weapon(rules.starting_weapon);
  rules.random_seed = 0x7a1ec7u;
  // A fresh retained world per run: a run that reaches an unported TU1.5
  // owner is recorded and skipped without contaminating the next one.
  struct Session {
    Files files;
    std::unique_ptr<MultiplayerObjectRuntime> world;
    std::unique_ptr<MultiplayerMatch> match;
    unsigned slot{};
    std::uint32_t unit = null_datum;
    std::array<Controls, multiplayer_capacity> controls{};
  };
  const auto open = [&] {
    auto session = std::make_unique<Session>();
    session->world = std::make_unique<MultiplayerObjectRuntime>(cache, 0, session->files.path);
    session->world->configure_session(source_game_variant(variant), 1, 30, rules.random_seed);
    session->match = std::make_unique<MultiplayerMatch>(content, 0, cache.spawns(), rules,
        MultiplayerKingServices{}, std::function<std::uint32_t()>{}, session->world.get());
    auto &match = *session->match;
    check(match.source_engine_owned(), "Trajectory oracle did not select the original source match engine");
    auto configuration = source_default_player_configuration();
    configuration[0x44] = 2;
    session->slot = match.join("Trajectory", -1, configuration);
    const std::array<unsigned, 1> slots{session->slot};
    const std::vector<FirstPersonBindingDefinition> views{cache.load_first_person_binding(rules.weapon, 2)};
    match.configure_local_views(slots, views);
    for (unsigned tick = 0; tick < 40 && match.player(session->slot).source_current_object == null_datum; ++tick)
      match.step(session->controls);
    session->unit = match.player(session->slot).source_current_object;
    check(session->unit != null_datum, "Trajectory player was not born");
    session->controls[session->slot].yaw = match.unit(session->slot).yaw;
    session->controls[session->slot].pitch = match.unit(session->slot).pitch;
    match.step(session->controls);
    return session;
  };
  const auto &biped = multiplayer_biped(content, 2);
  std::array<NonHostMotionDefinition, variants.size()> definitions;
  for (std::size_t v = 0; v < variants.size(); ++v) {
    definitions[v] = native_motion_definition(content.movement, biped, original->unit_locomotion_deadzone_bits);
    definitions[v].source_control = variants[v].source_control;
    definitions[v].source_facing = variants[v].source_facing;
    if (variants[v].origin)
      definitions[v] = nonhost_predictor_definition(content.movement, biped, original->unit_locomotion_deadzone_bits);
  }
  const auto read = [&](Session &session) {
    auto &world = *session.world;
    const auto motion = world.biped_motion(session.unit);
    const auto &bank = world.memory();
    const auto body = world.source_object_body(session.unit);
    NonHostAuthority a;
    a.position = motion.position;
    a.velocity = motion.velocity;
    a.forward = motion.forward;
    a.crouch = motion.crouch;
    a.grounded = (motion.support_flags & 1u) == 0;
    a.landing.kind = std::bit_cast<std::int16_t>(std::uint16_t(word(bank, body + 0x34cu, 2)));
    a.landing.elapsed = std::bit_cast<std::int8_t>(std::uint8_t(word(bank, body + 0x34eu, 1)));
    a.landing.duration = std::bit_cast<std::int8_t>(std::uint8_t(word(bank, body + 0x34fu, 1)));
    // The host-processed look (18A300) this tick's command consumed.
    a.yaw = session.match->unit(session.slot).yaw;
    a.pitch = session.match->unit(session.slot).pitch;
    return std::pair{a, motion.physical_state};
  };
  // Static BSP collision for fixture surveys; prediction uses each match's.
  const CollisionWorld survey(content.collision);
  std::vector<std::string> failures;

  std::map<std::string, std::map<std::string, std::array<Stats, windows.size()>>> stats, planar_stats,
      vertical_stats;
  std::map<std::string, unsigned> categories;
  unsigned runs = 0, measured = 0, dead_zone_ticks = 0;
  // A run: place, settle, then a scripted input sequence. Each tick records
  // the host's motion after the input and measures every window/variant.
  const auto run = [&](Vec3 start, float heading, const char *label,
                       const std::function<Controls(unsigned, float)> &script, unsigned duration) {
    ++runs;
    std::unique_ptr<Session> owned;
    try {
      owned = open();
    } catch (const std::exception &error) {
      failures.push_back(std::string(label) + " open: " + error.what());
      return;
    }
    auto &session = *owned;
    auto &match = *session.match;
    auto &controls = session.controls;
    const auto slot = session.slot;
    try {
    session.world->tu15_world().set_object_position(session.unit, start + Vec3{0, 0, .05f});
    Controls neutral;
    neutral.yaw = heading;
    controls[slot] = neutral;
    for (unsigned t = 0; t < 30; ++t)
      match.step(controls);
    std::deque<Sample> history;
    for (unsigned t = 0; t < duration; ++t) {
      const auto [before, before_state] = read(session);
      controls[slot] = script(t, heading);
      match.step(controls);
      const auto [after, state] = read(session);
      // Recomposition after a step may replace the composed collision world;
      // borrow it afresh every tick.
      const auto &collision = match.simulation().collision();
      if (state != 1 || before_state != 1 || match.unit(slot).vitality.dead) {
        history.clear(); // only the on-foot walking/airborne owner (state 1) is predicted
        continue;
      }
      Sample sample;
      sample.authority = after;
      sample.input = {controls[slot].forward, controls[slot].right, after.yaw, after.pitch, controls[slot].jump,
                      controls[slot].crouch};
      const auto rise = after.position.z - before.position.z;
      const auto planar = std::hypot(after.position.x - before.position.x, after.position.y - before.position.y);
      const auto expected = std::hypot(sample.input.forward, sample.input.right) * content.movement.forward / 30.f;
      const auto ground = collision.raycast(after.position + Vec3{0, 0, .1f}, {0, 0, -1}, .3f);
      if (!before.grounded || !after.grounded)
        sample.category = "airborne";
      else if (std::abs(rise) > .02f && planar > .01f)
        sample.category = "step_or_ledge";
      else if (ground && ground->normal.z < .985f)
        sample.category = "slope";
      else if (expected > .02f && planar < expected * .5f)
        sample.category = "blocked";
      else if (sample.input.crouch || after.crouch > .1f)
        sample.category = "crouch";
      else if (expected > 0 && expected < .3f * content.movement.forward / 30.f)
        sample.category = "small_stick";
      else if (expected == 0)
        sample.category = planar > .001f ? "stopping" : "idle";
      else
        sample.category = "flat_run";
      if (history.empty())
        history.push_back({before, sample.input, "start"});
      history.push_back(sample);
      while (history.size() > windows.back() + 1)
        history.pop_front();
      ++categories[sample.category];
      ++measured;
      for (std::size_t w = 0; w < windows.size(); ++w) {
        const auto n = windows[w];
        if (history.size() < n + 1)
          continue;
        const auto &base = history[history.size() - 1 - n];
        for (std::size_t v = 0; v < variants.size(); ++v) {
          auto motion = nonhost_motion_from_authority(base.authority, definitions[v], biped.radius,
                                                      biped.maximum_slope_cosine, collision);
          for (std::size_t i = history.size() - n; i < history.size(); ++i) {
            if (v == 1 && n == 1 &&
                source_motion_control(history[i].input, motion.facing, original->unit_locomotion_deadzone_bits).dead &&
                (history[i].input.forward != 0 || history[i].input.right != 0))
              ++dead_zone_ticks;
            step_native_player_motion(motion, definitions[v], collision, history[i].input);
          }
          const auto delta = nonhost_authority_position(definitions[v], motion.body.position) - after.position;
          const auto error = length(delta);
          stats[variants[v].name][sample.category][w].values.push_back(error);
          stats[variants[v].name]["all"][w].values.push_back(error);
          // Planar error and signed vertical error (native minus host).
          planar_stats[variants[v].name][sample.category][w].values.push_back(std::hypot(delta.x, delta.y));
          vertical_stats[variants[v].name][sample.category][w].values.push_back(delta.z);
        }
      }
    }
    } catch (const std::exception &error) {
      failures.push_back(std::string(label) + " run " + std::to_string(runs) + ": " + error.what());
      std::cout << "run " << runs << " " << label << " stopped: " << error.what() << '\n';
      return;
    }
    std::cout << "run " << runs << " " << label << " samples=" << measured << '\n';
  };
  // Scripted segments: straight runs, strafes, diagonals, backward, jumps,
  // crouch, turning and small analog deflections, then stopping.
  const auto course = [&](unsigned t, float heading) {
    Controls c;
    c.yaw = heading;
    if (t < 40)
      c.forward = 1;
    else if (t < 70)
      c.right = 1;
    else if (t < 100)
      c.forward = c.right = -.7f;
    else if (t < 130)
      c.forward = -1;
    else if (t < 175) {
      c.forward = 1;
      c.jump = t == 140 || t == 160;
    } else if (t < 215) {
      c.forward = .8f;
      c.crouch = true;
    } else if (t < 275) {
      c.forward = 1;
      c.yaw = heading + float(t - 215) * .05f;
    } else if (t < 300) {
      c.forward = .08f; // inside the DF880 dead zone (|throttle|^2 < 0.01)
      c.right = .05f;
    } else if (t < 330) {
      c.forward = .35f;
      c.right = .2f;
    }
    return c;
  };
  const auto &spawns = cache.spawns();
  check(!spawns.empty(), "Lockout has no spawns");
  for (std::size_t i = 0; i < spawns.size() && i < 8; ++i)
    for (const float turn : {0.f, 2.1f})
      run(spawns[i].position, spawns[i].facing + turn, "spawn-course", course, 350);
  // Ramps: walkable sloped BSP triangles, run up and down the fall line.
  std::vector<std::pair<Vec3, Vec3>> ramps;
  for (const auto &t : content.collision) {
    const auto n = cross(t.b - t.a, t.c - t.a);
    const auto area = length(n) * .5f;
    const auto normal = normalized(n);
    if (area < 1.f || normal.z < biped.maximum_slope_cosine + .02f || normal.z > .97f)
      continue;
    const auto center = (t.a + t.b + t.c) / 3.f;
    if (survey.overlaps(center + Vec3{0, 0, .06f}, biped.height, biped.radius))
      continue;
    const auto uphill = normalized(Vec3{-normal.x, -normal.y, 0});
    if (std::none_of(ramps.begin(), ramps.end(), [&](const auto &r) { return length(r.first - center) < 3.f; }))
      ramps.push_back({center, uphill});
    if (ramps.size() >= 8)
      break;
  }
  const auto ramp = [&](unsigned t, float heading) {
    Controls c;
    c.yaw = heading;
    if (t < 30)
      c.forward = 1;
    else if (t < 60)
      c.forward = -1;
    else if (t < 90)
      c.right = t < 75 ? 1.f : -1.f;
    return c;
  };
  for (const auto &[center, uphill] : ramps)
    for (const float sign : {1.f, -1.f})
      run(center - uphill * (sign * 1.5f), std::atan2(uphill.y * sign, uphill.x * sign), "ramp", ramp, 100);

  // Live host adoption through the native TU1.5 writer (B44C0 decision 2):
  // a running player reports itself 0.1 wu ahead and 0.4 wu/s faster.
  nlohmann::json adoption;
  {
    auto owned = open();
    auto &session = *owned;
    auto &match = *session.match;
    const auto definition = nonhost_predictor_definition(content.movement, biped, original->unit_locomotion_deadzone_bits);
    NonHostMotionAdopter adopter(definition, biped.radius, {}, native_tu15_motion_writer(*session.world));
    std::map<std::string, unsigned> kinds;
    unsigned written = 0, placement_unavailable = 0;
    for (std::uint32_t t = 1; t <= 90; ++t) {
      session.controls[session.slot].forward = 1;
      match.step(session.controls);
      const auto state = match.state();
      const std::array views{NonHostMotionAdopter::View{session.slot, t}};
      adopter.observe(state, *session.world, views);
      const auto motion = session.world->biped_motion(session.unit);
      NonHostMotionReport report;
      report.sequence = report.baseline = t;
      report.position = motion.position + Vec3{.1f, 0, 0};
      report.velocity = motion.velocity + Vec3{.4f, 0, 0};
      const auto result = adopter.adopt(*session.world, state, match.simulation().collision(), session.slot, report, 1);
      static constexpr std::array names{"reject", "teleport", "nudge", "unchanged"};
      ++kinds[names[std::size_t(result.decision.kind)]];
      written += result.written;
      placement_unavailable += result.placement_unavailable;
    }
    adoption = {{"ticks", 90}, {"decisions", kinds}, {"written", written},
                {"placement_unavailable", placement_unavailable},
                {"first_forced", int(adopter.last_forced(session.slot))}};
    std::cout << "live adoption: " << adoption.dump() << '\n';
    check(written > 0, "Live adoption wrote no native nudge");
  }

  nlohmann::json report;
  report["adoption"] = adoption;
  report["map"] = std::filesystem::path(argv[1]).filename().string();
  report["runs"] = runs;
  report["samples"] = measured;
  report["ramps"] = ramps.size();
  report["dead_zone_ticks"] = dead_zone_ticks;
  report["categories"] = categories;
  report["failed_runs"] = failures;
  for (auto &[variant_name, by_category] : stats)
    for (auto &[category, by_window] : by_category)
      for (std::size_t w = 0; w < windows.size(); ++w)
        report["hist"][variant_name][category]["ticks_" + std::to_string(windows[w])] = by_window[w].json();
  for (auto &[variant_name, by_category] : planar_stats)
    for (auto &[category, by_window] : by_category)
      for (std::size_t w = 0; w < windows.size(); ++w)
        report["planar"][variant_name][category]["ticks_" + std::to_string(windows[w])] = by_window[w].json();
  for (auto &[variant_name, by_category] : vertical_stats)
    for (auto &[category, by_window] : by_category)
      for (std::size_t w = 0; w < windows.size(); ++w)
        report["vertical"][variant_name][category]["ticks_" + std::to_string(windows[w])] = by_window[w].json();
  std::cout << report.dump(1) << '\n';
  if (argc == 3)
    std::ofstream(argv[2]) << report.dump(1) << '\n';
  check(measured > 1000, "Trajectory oracle measured too few on-foot ticks");
  check(categories["flat_run"] > 100, "Trajectory oracle lacks flat running coverage");
  check(failures.empty(), "Trajectory oracle runs stopped: " + (failures.empty() ? std::string() : failures.front()));
  // Regression bounds for the predictor operands (measured 2026-10-25 on
  // Lockout: all/1 p50 0.0003, flat/5 p90 0.0154, idle exact, small stick
  // exact), with headroom; and never worse than the plain native mover.
  const auto &h = report["hist"];
  const auto value = [&](const char *variant, const char *category, const char *window, const char *stat) {
    return h[variant][category][window][stat].get<double>();
  };
  check(value("predictor", "all", "ticks_1", "p50") < .002 &&
            value("predictor", "all", "ticks_1", "p50") < value("native", "all", "ticks_1", "p50"),
        "Predictor one-tick median error regressed");
  check(value("predictor", "flat_run", "ticks_5", "p90") < .03, "Predictor flat-run 150 ms error regressed");
  check(value("predictor", "idle", "ticks_5", "p50") < .001, "Predictor idle error regressed (origin height)");
  check(value("predictor", "small_stick", "ticks_5", "p50") < .005, "Predictor dead-zone error regressed");
  return 0;
} catch (const std::exception &error) {
  std::cerr << "nonhost trajectory oracle failed: " << error.what() << '\n';
  return 1;
}
