#include "halo2/audio.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/simulation.hpp"
#include <bit>
#include <cmath>
#include <filesystem>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <class F> void rejects(F f, const char *message) {
  bool rejected = false;
  try {
    f();
  } catch (const FormatError &) {
    rejected = true;
  }
  check(rejected, message);
}
std::vector<CollisionTriangle> floor() {
  return {{{-100, -100, 0}, {100, -100, 0}, {100, 100, 0}, 0, 0},
          {{-100, -100, 0}, {100, 100, 0}, {-100, 100, 0}, 0, 0}};
}
void wall(std::vector<CollisionTriangle> &triangles, float x) {
  triangles.push_back({{x, -100, 0}, {x, 100, 0}, {x, 100, 10}, 0, 0});
  triangles.push_back({{x, -100, 0}, {x, 100, 10}, {x, -100, 10}, 0, 0});
}
LevelContent arena(bool occluded = false) {
  LevelContent c;
  c.collision = floor();
  if (occluded)
    wall(c.collision, 1);
  c.player.datum = 1;
  c.player.height = 1;
  c.player.crouch_height = .6f;
  c.player.radius = .2f;
  c.player.camera_height = .8f;
  c.player.crouch_camera_height = .5f;
  c.player.crouch_velocity = 2;
  c.player.jump_velocity = 3;
  c.player.team = 1;
  c.player.vitality = {100, 100, 2, .1f, 0};
  c.movement = {2, 2, 2, 10, 1, 1, 1, 5, 2, 1};
  WeaponDefinition w;
  w.datum = 3;
  w.name = "test weapon";
  w.instantaneous = true;
  w.capacity = 20;
  w.reserve = 100;
  w.reload_rounds = 20;
  w.reload_time = 1;
  w.rounds_per_shot = 1;
  w.minimum_rounds = 1;
  w.rounds_per_second = 30;
  w.burst = 1;
  w.range = 50;
  w.damage_min = 25;
  w.projectiles_per_shot = 1;
  c.weapons.emplace(3, w);
  c.starting_weapon = 3;
  CharacterDefinition character;
  character.datum = 2;
  character.biped = c.player;
  character.biped.datum = 4;
  character.biped.team = 3;
  character.vision_distance = 30;
  character.vision_angle = 1.5f;
  character.acknowledgement = .1f;
  c.characters.emplace(2, character);
  SquadDefinition squad;
  squad.name = "enemy";
  squad.count = 1;
  squad.bsp = 0;
  squad.starts.push_back({{2, 0, 0}, 3.14159265f, 0, 2, 3, {}});
  c.squads.push_back(squad);
  return c;
}
struct ProgramBuilder {
  ScriptProgram p;
  std::uint32_t add(ScriptNode n) {
    const auto index = std::uint32_t(p.nodes.size());
    n.salt = std::uint16_t(0xe000 + index);
    p.nodes.push_back(n);
    return (std::uint32_t(n.salt) << 16) | index;
  }
  std::uint32_t leaf(int number, std::uint16_t type = 8) {
    return add({0, type, type, 9, null_datum, 0, std::uint32_t(number)});
  }
  std::uint32_t call(std::string name, std::vector<std::uint32_t> children,
                     std::uint16_t type = 4) {
    const auto at = std::uint32_t(p.strings.size());
    p.strings += name;
    p.strings.push_back('\0');
    for (std::size_t i = 0; i < children.size(); ++i)
      p.nodes[children[i] & 0xffff].next =
          i + 1 < children.size() ? children[i + 1] : null_datum;
    const auto fn =
        add({0, 0, 2, 9, children.empty() ? null_datum : children[0], at, 0});
    return add({0, 0, type, 8, null_datum, at, fn});
  }
  void script(std::string name, std::uint32_t root) {
    p.scripts.push_back({name, 0, 4, root});
  }
};
void scene_tests() {
  auto c = arena();
  c.object_names = {
      {"automatic", 6, 0}, {"scripted", 6, 1}, {"other_bsp", 6, 2}};
  ScenarioObject automatic;
  automatic.type = 6;
  automatic.placement = automatic.name = 0;
  automatic.definition = 42;
  automatic.position = {1, 2, 3};
  automatic.bounding_offset = {1, 0, 0};
  automatic.rotation = {1.570796327f, 0, 0};
  c.objects.push_back(automatic);
  auto scripted = automatic;
  scripted.placement = scripted.name = 1;
  scripted.flags = 1;
  c.objects.push_back(scripted);
  auto other = automatic;
  other.placement = other.name = 2;
  other.bsp_policy = 2;
  other.manual_bsps = 2;
  c.objects.push_back(other);
  SceneObjects objects(c, 0);
  check(objects.resolve(0) != null_datum && objects.resolve(1) == null_datum &&
            objects.resolve(2) != null_datum && !objects.get(objects.resolve(2)).visible,
        "scene initial persistent objects exist and hide outside manual BSP mask");
  const auto center = objects.center(objects.resolve(0));
  check(length(center - Vec3{1, 3, 3}) < 1e-6f,
        "authored bounding offset rotates with scene object");
  const auto first = objects.create(1, false, 42),
             second = objects.create(1, true, 99);
  check(first != second && objects.create(1, false, 100) == second &&
            objects.get(second).created_tick == 99,
        "create-anew invalidates previous generation");
  rejects([&] { objects.get(first); }, "stale scene generation rejected");
  objects.destroy(second);
  check(objects.resolve(1) == null_datum && objects.resolve(-1) == null_datum,
        "destroy clears name mapping");
  rejects([&] { objects.create(3); }, "invalid scenario name rejected");
  TriggerVolume volume{"box",     -1,           0,        {0, 1, 0},
                       {0, 0, 1}, {10, 20, 30}, {2, 3, 4}};
  check(objects.contains(volume, {9, 21, 31}) &&
            !objects.contains(volume, {10, 20, 30}) &&
            !objects.contains(volume, {9, 22, 31}) &&
            !objects.contains(volume, {11, 21, 31}),
        "trigger uses oriented corner, strict bounds and positive extents");
  volume.object_name = 0;
  rejects([&] { objects.contains(volume, {}); },
          "attached trigger fails without its authored node transform");
  ProgramBuilder p;
  const auto name = p.leaf(1, 56);
  const auto live = p.add({0, 56, 55, 9, null_datum, 0, 1});
  p.script("create_hide",
           p.call("begin", {p.call("object_create_anew", {name}),
                            p.call("object_hide", {live, p.leaf(1, 5)})}));
  c.scripts = p.p;
  Simulation simulation(c, 0, {{0, 0, 0}, 0, 0});
  simulation.run_script("create_hide");
  simulation.step({});
  check(
      simulation.scripts().diagnostics().empty() &&
          !simulation.scene().get(simulation.scene().resolve(1)).visible,
      "compiled object-name literal resolves current live handle and hides it");
  simulation.run_script("create_hide");
  simulation.step({});
  check(simulation.scripts().diagnostics().empty() &&
            simulation.scene().get(simulation.scene().resolve(1)).created_tick == 1,
        "object recreation retains dispatch tick before the render boundary");
  c.cutscene_flags.push_back({"anchor", {10, 20, 30}, 1.570796327f, 0});
  auto supplied_data = std::make_shared<OriginalData>();
  // A caller-supplied synthetic curve checks presentation/data ownership.
  // Original curve values are tested by the separate instruction differential.
  for (unsigned i = 0; i < supplied_data->fade_curve.size(); ++i)
    supplied_data->fade_curve[i] = std::uint8_t(i * 255 / 1023);
  c.original_data = supplied_data;
  AnimationClip clip;
  clip.frame_count = 2;
  clip.nodes = {1};
  clip.parents = {-1};
  clip.components = {3};
  clip.channels.resize(1);
  clip.channels[0][0].times = {0};
  clip.channels[0][0].keys = {{0, 0, 0, 1}};
  clip.channels[0][1].times = {0, 1};
  clip.channels[0][1].keys = {{1, 0, 0, 0}, {2, 0, 0, 0}};
  Presentation view(c, [&](auto, auto) { return clip; });
  view.camera_animation(1, 1, 0);
  view.scripted = true;
  view.field_of_view(60, 0);
  check(std::abs(camera_vertical_field_of_view(view.camera({}), 1.6f) -
                 .692605f) < 1e-5f,
        "scripted horizontal field of view converts for viewport aspect");
  check(view.camera_time() == 1 &&
            length(view.camera({}).position - Vec3{10, 21, 30}) < 1e-5f &&
            length(view.camera({}).forward - Vec3{0, 1, 0}) < 1e-5f,
        "animated camera composes anchor transform without axis swap");
  view.tick();
  view.tick();
  check(view.camera_time() == 0 &&
            length(view.camera({}).position - Vec3{10, 22, 30}) < 1e-5f,
        "camera duration is frame count minus one and clamps final frame");
  rejects([&] { view.camera_animation(1, 1, 5); },
          "invalid camera anchor rejected");
  view.fade({1, 1, 1}, true, 30);
  for (int i = 0; i < 15; ++i)
    view.tick();
  check(view.effects().fade == fade_curve_fraction(.5f, supplied_data->fade_curve),
        "fade uses supplied curve at half duration");
  for (int i = 0; i < 15; ++i)
    view.tick();
  check(view.effects().fade == 1, "fade out covers at duration");
  view.fade({1, 1, 1}, false, 0);
  check(view.effects().fade == 0, "zero duration fade in clears immediately");
  view.letterbox(true, false);
  for (int i = 0; i < 15; ++i)
    view.tick();
  check(view.effects().letterbox == .5f, "letterbox takes one second at 30Hz");
}
void synthetic_tests() {
  {
    auto content = arena();
    ProgramBuilder program;
    program.p.globals.push_back(
        {"selected_difficulty", 44, program.leaf(0, 44)});
    const auto target =
        program.add({0, 44, 44, 13, null_datum, 0, 0xabcd0000u});
    program.script(
        "difficulty",
        program.call(
            "set", {target, program.call("game_difficulty_get", {}, 44)}, 44));
    content.scripts = program.p;
    Simulation simulation(content, 0, {{0, 0, 0}, 0, 0});
    simulation.set_difficulty(3);
    simulation.run_script("difficulty");
    simulation.step({});
    check(simulation.scripts().diagnostics().empty() &&
              simulation.scripts().globals().at(0).bits == 3,
          "selected campaign difficulty reaches compiled scripts");
    rejects([&] { simulation.set_difficulty(4); },
            "invalid campaign difficulty rejected");
  }

  const MovementDefinition tuning{8, 4, 6, 100, 2, 1, 1.5f, 50, 30, 3};
  const auto diagonal = movement_intent(tuning, {1, 1, 0}, 0);
  check(std::abs(diagonal.velocity.x - 5.656854f) < 1e-6f &&
            std::abs(diagonal.velocity.y - 4.2426405f) < 1e-6f,
        "control axes normalize before authored directional speeds");
  const auto blended = movement_intent(tuning, {1, 0, 0}, .25f);
  check(blended.velocity.x == 6.5f && blended.acceleration == 87.5f,
        "partial crouch continuously blends speed and acceleration");
  check(movement_intent(tuning, {-1, 1, 0}, 0, true).velocity.x == 0,
        "limited movement prevents reverse and sideways input");
  WalkingInput walking;
  walking.input = {2, 0, 0};
  walking.normal = normalized({-.5f, 0, 1});
  walking.acceleration = 100;
  walking.dt = 1;
  walking.suppress_ground_push = true;
  const auto uphill = walking_velocity(walking);
  check(uphill.velocity.z > 0 &&
            std::abs(dot(uphill.velocity, walking.normal)) < 1e-6f,
        "ground velocity follows the supplied slope plane");
  walking.input = {10, 0, 0};
  walking.current = {0, 0, 2};
  walking.flags = 4 | 8;
  walking.air_acceleration = 3;
  walking.gravity = retail_gravity;
  walking.dt = tick_seconds;
  const auto airborne = walking_velocity(walking);
  check(airborne.velocity.x == 3.f * tick_seconds &&
            airborne.velocity.z == 2.f - retail_gravity * tick_seconds,
        "air acceleration caps only horizontal velocity");
  JumpState jump;
  check(jump_gate(jump, true, true, false, false, true, tick_seconds, 30),
        "first supported jump accepted");
  for (int i = 0; i < 30; ++i)
    check(!jump_gate(jump, true, true, false, false, true, tick_seconds, 30),
          "held jump does not repeat after landing");
  jump_gate(jump, false, true, false, false, true, tick_seconds, 30);
  check(jump_gate(jump, true, true, false, false, true, tick_seconds, 30),
        "release re-arms jump");
  const auto kick = jump_impulse({1, 2, 5}, {0, 0, 1}, {0, 0, 3}, 4);
  check(kick.z == 7 && jump_impulse({1, 2, 9}, {0, 0, 1}, {}, 4).z == 9,
        "jump is support relative and never lowers upward velocity");
  const auto flight = projectile_flight({}, {3, 4, 5}, 4, .5f, 0, 0);
  check(flight.velocity.z == 3 && flight.displacement.z == 2,
        "ballistic displacement uses half-step gravity");
  const auto clipped = projectile_flight({}, {3, 4, 5}, 4, .5f, .5f, 1);
  check(clipped.range_exhausted &&
            std::abs(length(clipped.displacement) -
                     (.5f / std::sqrt(50.f)) * std::sqrt(41.f)) < 1e-6f,
        "projectile range clips the current step");
  RechargeState permanent;
  permanent.body = permanent.shield = .25f;
  permanent.body_stun = permanent.shield_stun = 32767;
  recharge(permanent, tick_seconds, 1, 0);
  check(permanent.body_stun == 32766 && permanent.shield_stun == 32767,
        "body stun has no permanent sentinel; shield stun does");
  permanent.body_stun = permanent.shield_stun = 0;
  permanent.shield_depleted = true;
  recharge(permanent, tick_seconds, 0, 0);
  check(permanent.shield == .25f && permanent.shield_recharging &&
            !permanent.shield_depleted,
        "zero-rate shield still starts retail recharge bookkeeping");
  check(crouch_delta(0, 1.f / 30, 2, true, false) > 0, "crouch begins");
  check(std::bit_cast<std::uint32_t>(crouch_delta(0, 0, 0, false, false)) == 0,
        "retail signed zero");
  RechargeState r;
  r.shield = .25f;
  r.shield_stun = 2;
  recharge(r, 1, .1f, .1f);
  check(r.shield == .25f && r.shield_stun == 1, "shield stun first tick");
  recharge(r, 1, .1f, .1f);
  check(r.shield == .25f && r.shield_stun == 0,
        "shield stun reaches zero without recharge");
  recharge(r, 1, .1f, .1f);
  check(std::abs(r.shield - .35f) < 1e-6f, "shield recharges following tick");
  const std::vector<DamageGroup> armor_table{{1, {{1, .5f}, {2, 2.f}}},
                                             {2, {{1, .25f}, {2, 4.f}}}};
  check(damage_multiplier(armor_table, 1, 2, 1, 2) == 1.f &&
            damage_multiplier(armor_table, 1, 1, 1, 1) == .0625f &&
            damage_multiplier(armor_table, 3, 4, 1, 2) == 1.f,
        "all four armor combinations, repeated keys and missing groups");
  ShieldDamageState shield{.5f, 0, 0};
  ShieldDamageInput impact{75, 100, 2, 0, 2, .25f};
  const auto depleted = shield_damage(shield, impact);
  check(shield.shield == 0 && depleted.body_damage == 50 &&
            depleted.shield_damage == 1.5f && shield.stun == 60 &&
            shield.flags == 10 && depleted.flags == 8 &&
            depleted.depleted_effect && depleted.damaged_effect,
        "shield armor scales absorption and divides overflow back to body");
  shield = {1, 0, 0};
  impact.multiplier = 0;
  const auto absorbed = shield_damage(shield, impact);
  check(shield.shield == 1 && absorbed.body_damage == 0 && shield.stun == 0,
        "zero shield armor multiplier absorbs a direct impact without stun");
  impact.multiplier = 1;
  impact.damage = 10;
  impact.minimum_stun_damage = 10;
  shield = {1, 0, 0};
  shield_damage(shield, impact);
  check(shield.stun == 0, "shield stun threshold comparison is strict");
  shield = {1, 32767, 0};
  impact.minimum_stun_damage = 0;
  shield_damage(shield, impact);
  check(shield.stun == 32767, "damage preserves permanent shield stun");
  BodyDamageState body_vitality{1, 32767, 0};
  BodyDamageInput body_impact;
  body_impact.damage = 75;
  body_impact.maximum_body = body_impact.death_maximum_body = 100;
  body_impact.multiplier = 2;
  body_impact.stun_seconds = .15f;
  const auto killed = body_damage(body_vitality, body_impact);
  check(body_vitality.body == -.5f && body_vitality.stun == 4 &&
            killed.normalized_damage == 1.5f && killed.remaining == -75 &&
            killed.depleted && (body_vitality.flags & 4),
        "body armor, unbounded lethal fraction and nearest-even stun replace sentinel");
  body_vitality = {1, 0, 0};
  body_impact.info_flags = 128;
  const auto protected_body = body_damage(body_vitality, body_impact);
  check(body_vitality.body == .0001f && !protected_body.depleted &&
            protected_body.normalized_damage == 1.5f,
        "cannot-die floor preserves normalized requested body damage");
  body_vitality = {1, 0, 0};
  body_impact.info_flags = 0;
  body_impact.region_valid = true;
  body_impact.region_flags = 1;
  body_impact.region_vitality = .5f;
  const auto section = body_damage(body_vitality, body_impact);
  check(body_vitality.body == 1 && body_vitality.stun == 0 &&
            section.region_update && section.region_damage == 3 &&
            section.normalized_damage == 0 && section.remaining == 75,
        "section damage precedes absorption and absorbed damage does not stun body");
  body_vitality = {1, 0, 0};
  body_impact.region_flags = 128;
  body_impact.hit_region = body_impact.headshot_allowed = true;
  body_impact.definition_flags = 2;
  body_impact.damage = 1;
  body_impact.multiplier = 1;
  const auto headshot = body_damage(body_vitality, body_impact);
  check(headshot.depleted && (headshot.flags & 0x1c0) == 0x1c0 &&
            headshot.reporting_type == 0x40,
        "supplied authoritative head region performs retail instant kill");
  auto geometry = floor();
  wall(geometry, 1);
  CollisionWorld world(geometry);
  const auto ray = world.raycast({0, 0, 1}, {1, 0, 0}, 10);
  check(ray && std::abs(ray->distance - 1) < 1e-5f, "wall ray");
  Body body{{0, 0, 2}, {0, 0, -10}, 1, .2f, false};
  world.move(body, .5f);
  check(body.grounded && std::abs(body.position.z) < .001f,
        "capsule lands without tunneling");
  body.velocity = {100, 0, 0};
  world.move(body, .05f);
  check(body.position.x < .801f, "fast capsule blocked by wall");
  check(world.overlaps({1, 0, 0}, 1, .2f), "capsule wall overlap");
  std::vector<NavigationSector> sectors(3);
  sectors[0] = {
      1, {{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}, {{1, {2, 1, 0}, 32}}};
  sectors[1] = {1,
                {{2, 0, 0}, {4, 0, 0}, {4, 2, 0}, {2, 2, 0}},
                {{0, {2, 1, 0}, 32}, {2, {3, 2, 0}, 32}}};
  sectors[2] = {
      1, {{2, 2, 0}, {4, 2, 0}, {4, 4, 0}, {2, 4, 0}}, {{1, {3, 2, 0}, 32}}};
  const NavigationWorld navigation(sectors);
  const auto route = navigation.route({1, 1, 0}, {3, 3, 0});
  check(route.size() == 3 && route[0].x == 2 && route[1].y == 2 &&
            route.back().y == 3,
        "navigation routes through authored connected portals");
  sectors[1].portals.pop_back();
  check(navigation.route({1, 1, 0}, {3, 3, 0}).empty(),
        "disconnected navigation has no fabricated route");
  Body excessive{{0, 0, 0}, {1e20f, 0, 0}, 1, .2f, false};
  rejects([&] { world.move(excessive, 1); },
          "excessive subdivisions rejected before integer conversion");
  const auto clear = arena();
  Simulation sim(clear, 0, {{0, 0, 0}, 0, 0});
  sim.step({});
  check(sim.player().body.grounded, "simulation support");
  sim.step({0, 0, 0, 0, true});
  check(sim.player().body.position.z > 0 && sim.player().body.velocity.z > 0,
        "jump rises");
  for (int i = 0; i < 120; ++i)
    sim.step({});
  check(sim.player().body.grounded, "jump returns to floor");
  sim.step({0, 0, 0, 0, false, true});
  check(sim.player().body.height < 1, "crouch changes capsule");
  Simulation combat(clear, 0, {{0, 0, 0}, 0, 0});
  check(combat.place_squad("enemy") == 1, "authored squad placement");
  check(combat.actors()[0].team == 3, "default team from unit definition");
  for (int i = 0; i < 8; ++i)
    combat.step({0, 0, 0, 0, false, false, true});
  check(combat.actors()[0].vitality.dead,
        "shield then body damage kills target");
  check(combat.hits() >= 8, "combat hit count");
  auto pellets = arena();
  pellets.weapons.at(3).projectiles_per_shot = 3;
  Simulation multishot(pellets, 0, {{0, 0, 0}, 0, 0});
  multishot.place_squad("enemy");
  multishot.step({0, 0, 0, 0, false, false, true});
  check(multishot.player().weapon.loaded == 19 &&
            multishot.actors()[0].vitality.shield == .25f,
        "one cartridge produces all authored projectiles");
  auto armored = arena();
  armored.damage_groups = {{1, {{2, 2.f}}}};
  armored.damage_materials = {{2, 0}};
  armored.weapons.at(3).damage_group = 1;
  armored.characters.at(2).biped.vitality.shield_material = 0;
  armored.characters.at(2).biped.vitality.body_delay = 2;
  Simulation armored_combat(armored, 0, {{0, 0, 0}, 0, 0});
  armored_combat.place_squad("enemy");
  armored_combat.step({0, 0, 0, 0, false, false, true});
  check(armored_combat.actors()[0].vitality.shield == .5f &&
            armored_combat.actors()[0].vitality.body == 1 &&
            armored_combat.actors()[0].vitality.body_stun == 0,
        "shield armor absorbs impact without incorrectly stunning body");
  auto body_arena = armored;
  body_arena.characters.at(2).biped.vitality.shield = 0;
  body_arena.characters.at(2).biped.vitality.body_material = 0;
  Simulation body_combat(body_arena, 0, {{0, 0, 0}, 0, 0});
  body_combat.place_squad("enemy");
  check(body_combat.actors()[0].vitality.shield == 0,
        "zero maximum shield initializes zero shield fraction");
  body_combat.step({0, 0, 0, 0, false, false, true});
  check(body_combat.actors()[0].vitality.body == .5f,
        "body impact uses its own authored material armor");
  body_arena.characters.at(2).biped.vitality.damage_flags = 128;
  body_arena.weapons.at(3).damage_min = 1000;
  Simulation protected_combat(body_arena, 0, {{0, 0, 0}, 0, 0});
  protected_combat.place_squad("enemy");
  protected_combat.step({0, 0, 0, 0, false, false, true});
  check(protected_combat.actors()[0].vitality.body == .0001f &&
            !protected_combat.actors()[0].vitality.dead,
        "authored cannot-die actor survives a lethal impact");
  Simulation ai(clear, 0, {{0, 0, 0}, 0, 0});
  ai.place_squad("enemy");
  for (int i = 0; i < 8; ++i)
    ai.step({});
  check(ai.shots() > 0 && ai.player().vitality.shield < 1,
        "AI sees and attacks");
  const auto hidden = arena(true);
  Simulation blocked(hidden, 0, {{0, 0, 0}, 0, 0});
  blocked.place_squad("enemy");
  for (int i = 0; i < 20; ++i)
    blocked.step({});
  check(blocked.shots() == 0 && blocked.actors()[0].state == Unit::Idle,
        "wall occludes AI sight");
  ProgramBuilder b;
  const auto one = b.call("mark", {b.leaf(1)});
  const auto wait = b.call("sleep", {b.leaf(3)});
  const auto two = b.call("mark", {b.leaf(2)});
  b.script("test", b.call("begin", {one, wait, two}));
  std::vector<int> marks;
  ScriptVm vm(b.p, [&](const auto &name, const auto &args) {
    check(name == "mark", "script host call");
    marks.push_back(int(args[0].number()));
    return ScriptValue{};
  });
  vm.start("test");
  vm.tick(10);
  check(marks == std::vector<int>{1}, "sleep preserves continuation");
  vm.tick(12);
  check(marks.size() == 1, "sleep keeps thread suspended");
  vm.tick(13);
  check(marks == std::vector<int>({1, 2}) && !vm.running(),
        "sleep resumes at next expression");
  ProgramBuilder polling;
  const auto condition = polling.call("ready", {}, 5);
  const auto until =
      polling.call("sleep_until", {condition, polling.leaf(2, 7)});
  polling.script("poll",
                 polling.call("begin", {until, polling.call("done", {})}));
  int evaluations = 0, finished = 0;
  ScriptVm poll(polling.p, [&](const auto &name, const auto &) -> ScriptValue {
    if (name == "ready")
      return ScriptValue::numeric(++evaluations == 3, 5);
    ++finished;
    return {};
  });
  poll.start("poll");
  poll.tick(0);
  poll.tick(1);
  check(evaluations == 0 && finished == 0,
        "sleep_until defers first predicate evaluation until interval");
  poll.tick(2);
  poll.tick(4);
  check(evaluations == 2 && finished == 0,
        "sleep_until retains condition frame across polls");
  poll.tick(6);
  check(finished == 1 && !poll.running(),
        "sleep_until resumes retail continuation after successful poll");
  ProgramBuilder sleeping;
  const auto before = sleeping.call("mark", {sleeping.leaf(1)});
  const auto forever = sleeping.call("sleep_forever", {});
  const auto after = sleeping.call("mark", {sleeping.leaf(2)});
  sleeping.script("dormant", sleeping.call("begin", {before, forever, after}));
  sleeping.p.scripts[0].type = 1;
  sleeping.script("startup", sleeping.call("wake", {sleeping.leaf(0, 10)}));
  std::vector<int> awakenings;
  ScriptVm sleeping_vm(sleeping.p, [&](const auto &, const auto &args) {
    awakenings.push_back(int(args[0].number()));
    return ScriptValue{};
  });
  sleeping_vm.start_level();
  sleeping_vm.tick(0);
  check(awakenings.empty(),
        "dormant script waits for wake during level startup");
  sleeping_vm.tick(1);
  check(awakenings == std::vector<int>{1} && !sleeping_vm.running(),
        "sleep_forever suspends without destroying continuation");
  sleeping_vm.start("dormant");
  sleeping_vm.tick(2);
  check(awakenings == std::vector<int>({1, 2}),
        "wake resumes expression after sleep_forever");
  ProgramBuilder input_program;
  input_program.script(
      "disable",
      input_program.call("player_enable_input", {input_program.leaf(0, 5)}));
  input_program.script(
      "enable",
      input_program.call("player_enable_input", {input_program.leaf(1, 5)}));
  auto input_content = arena();
  input_content.scripts = input_program.p;
  Simulation input_sim(input_content, 0, {{0, 0, 0}, 0, 0});
  input_sim.run_script("disable");
  input_sim.step({1, 0, 0, 0, false, false, true});
  check(input_sim.player().body.position.x == 0 && input_sim.shots() == 0,
        "script input disable blocks movement and firing");
  input_sim.run_script("enable");
  input_sim.step({1, 0, 0, 0, false, false, true});
  check(input_sim.player().body.position.x > 0 && input_sim.shots() == 1,
        "script input enable restores controls");
  ProgramBuilder branch;
  const auto skipped = branch.call("unsupported", {});
  branch.script("branch", branch.call("if", {branch.leaf(0, 5), skipped,
                                             branch.leaf(7)}));
  ScriptVm branch_vm(branch.p, [](const auto &, const auto &) -> ScriptValue {
    throw FormatError("unsupported host");
  });
  branch_vm.start("branch");
  branch_vm.tick(0);
  check(branch_vm.diagnostics().empty(), "if skips unchosen branch");
  ProgramBuilder unsupported;
  unsupported.script("fault", unsupported.call("missing", {}));
  ScriptVm failed(unsupported.p, [](const auto &, const auto &) -> ScriptValue {
    throw FormatError("unsupported builtin");
  });
  failed.start("fault");
  failed.tick(0);
  check(failed.diagnostics().size() == 1 && !failed.running(),
        "unsupported scripts stop explicitly");
  ProgramBuilder global;
  global.p.globals.push_back({"counter", 8, global.leaf(1)});
  const auto target = global.add({0, 8, 8, 13, null_datum, 0, 0xabcd0000u});
  const auto assignment = global.call("set", {target, global.leaf(2)}, 8);
  const auto reference = global.add({0, 8, 8, 13, null_datum, 0, 0xabcd0000u});
  global.script(
      "global",
      global.call("begin", {assignment, global.call("mark", {reference})}));
  int observed = 0;
  ScriptVm global_vm(global.p, [&](const auto &, const auto &values) {
    observed = int(values.at(0).number());
    return ScriptValue{};
  });
  global_vm.start("global");
  global_vm.tick(0);
  check(observed == 2 && global_vm.globals()[0].number() == 2 &&
            global_vm.diagnostics().empty(),
        "script global indices use low word and assignment persists");
  auto bad = b.p;
  bad.nodes[0].salt ^= 1;
  rejects([&] { ScriptVm invalid(bad, {}); }, "stale syntax salt rejected");
  EncodedSound sound{"silence", 48000, 2, 1, 64, std::vector<std::byte>(72)};
  const auto pcm = decode_sound(sound);
  check(pcm.frames() == 64, "Xbox ADPCM produces 64 frames, not WAV IMA's 65");
  for (float sample : pcm.stereo)
    check(sample == 0, "zero ADPCM block");
  sound.data.pop_back();
  rejects([&] { decode_sound(sound); }, "truncated ADPCM rejected");
  SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
  AudioEngine audio;
  auto tone = std::make_shared<PcmSound>();
  tone->name = "mixer test";
  tone->stereo.resize(4800 * 2, .25f);
  audio.play(tone, 1, 0, true);
  // Wait for the asynchronous callback, including under a loaded build host.
  for (int i = 0; i < 100 && audio.peak() <= .24f; ++i)
    SDL_Delay(10);
  check(audio.mixed_frames() > 0 && audio.peak() > .24f,
        "SDL callback mixes decoded sample format");
  audio.stop();
  auto short_tone = std::make_shared<PcmSound>();
  short_tone->name = "loop";
  short_tone->stereo.assign(32, .125f);
  const auto voice = audio.play_sequence(short_tone, short_tone, short_tone);
  SDL_Delay(35);
  check(audio.active_voices() == 1,
        "short loop wraps across mixer callback boundaries");
  audio.parameters(voice, .5f, -1);
  audio.next_loop(voice, short_tone);
  audio.stop(voice);
  for (int i = 0; i < 100 && audio.active_voices() != 0; ++i)
    SDL_Delay(10);
  check(audio.active_voices() == 0,
        "loop stop plays outro once then retires voice");
  rejects([&] { audio.play(std::make_shared<PcmSound>(), 1, 0, true); },
          "empty loop rejected without callback spin");
}
void retail_tests(const std::filesystem::path &directory) {
  const Cache lockout(directory / "lockout.map");
  const auto lc = lockout.load_content(0);
  check(lc.collision.size() == 12882, "Lockout authored collision count");
  check(lc.player.radius > .17f && lc.player.radius < .18f,
        "MP player representation resolves");
  check(lc.player.maximum_slope_cosine == .642787635f &&
            lc.player.slope.downhill_scale == 1.5f &&
            lc.player.slope.uphill_scale == .5f &&
            lc.player.slope.downhill_full == -.707106769f &&
            lc.player.slope.uphill_full == .766044438f,
        "original cached player slope constants are preserved");
  Simulation movement(lc, 0, lockout.spawns().front());
  for (int i = 0; i < 120; ++i)
    movement.step({});
  check(movement.player().body.grounded, "retail Lockout spawn grounded");
  const float z = movement.player().body.position.z;
  movement.step({0, 0, 0, 0, true});
  check(movement.player().body.position.z > z, "retail jump velocity used");
  for (int i = 0; i < 120; ++i)
    movement.step({});
  check(movement.player().body.grounded &&
            std::abs(movement.player().body.position.z - z) < .01f,
        "actual Lockout jump falls back to ground with retail global gravity");
  Cache cairo(directory / "01b_spacestation.map");
  auto c = cairo.load_content(0);
  check(c.navigation.size() == 12631, "Cairo authored navigation count");
  const NavigationWorld nav(c.navigation);
  check(nav.locate(cairo.spawns().front().position).has_value(),
        "Cairo spawn locates on authored navigation");
  check(c.object_names.size() == 481 && c.objects.size() == 1239 &&
            c.cutscene_flags.size() == 34 && c.camera_points.empty() &&
            c.trigger_volumes.size() == 52,
        "Cairo authored scene block counts");
  SceneObjects cairo_scene(c, 0);
  const auto brute = std::find_if(
      cairo.tags().begin(), cairo.tags().end(), [](const auto &tag) {
        return tag.group == "bipd" &&
               tag.name == "objects\\characters\\brute\\brute";
      });
  check(brute != cairo.tags().end(), "retail Brute biped tag exists");
  const auto brute_model = cairo.load_model(brute->datum, false);
  check(!brute_model.mesh.vertices.empty() &&
            pose_model(brute_model).size() == brute_model.mesh.vertices.size(),
        "Brute transparent-only skinned sections use transparent node count");
  check(cairo_scene.resolve(337) == null_datum,
        "Cairo cinematic Chief is not automatically placed");
  const auto chief = cairo_scene.create(337);
  const auto &chief_placement = c.objects[cairo_scene.get(chief).placement];
  check(chief_placement.definition == 0xfb66192e &&
            chief_placement.scale == 1 &&
            std::abs(chief_placement.position.x - 61.164745f) < 1e-5f,
        "Cairo named Chief placement, palette and zero-scale normalization");
  const auto camera_clip = cairo.load_named_animation(
      0xf3f411bc, cairo.animation_name(0xf3f411bc, "x02_01"));
  check(camera_clip.frame_count == 516 && camera_clip.nodes.size() == 1 &&
            length(sample_animation(camera_clip, 0, 0).translation -
                   Vec3{6.24428415f, -11.76257896f, .48928231f}) < 1e-6f,
        "original Cairo dense camera animation decodes");
  Presentation camera(c, [&](auto graph, auto name) {
    return cairo.load_named_animation(graph, name);
  });
  camera.camera_animation(0xf3f411bc, 0x06001c72, 17);
  camera.scripted = true;
  check(camera.camera_time() == 515 &&
            length(camera.camera({}).position -
                   Vec3{60.39861917f, -11.76257896f, 7.48928040f}) < 1e-5f,
        "original Cairo camera frame0 world anchor fixture");
  for (int i = 0; i < 600; ++i)
    camera.tick();
  check(camera.camera_time() == 0 &&
            length(camera.camera({}).position -
                   Vec3{60.93177223f, -9.42803478f, 7.48600077f}) < 1e-5f,
        "original camera stops at authored last frame");
  Simulation cinematic(c, 0, cairo.spawns().front());
  cinematic.run_script("cinematic_snap_to_white");
  cinematic.step({});
  check(cinematic.scripts().diagnostics().empty() &&
            cinematic.presentation().cinematic &&
            cinematic.presentation().scripted &&
            cinematic.presentation().effects().fade == 1 &&
            cinematic.presentation().effects().letterbox == 1 &&
            !cinematic.presentation().player_camera_enabled,
        "authored Cairo snap-to-white script executes actual presentation "
        "services");
  cinematic.run_script("cinematic_fade_from_white");
  for (int i = 0; i < 35; ++i)
    cinematic.step({});
  check(cinematic.scripts().diagnostics().empty() &&
            !cinematic.presentation().cinematic &&
            !cinematic.presentation().scripted &&
            cinematic.presentation().effects().fade == 0 &&
            cinematic.presentation().effects().letterbox == 0 &&
            cinematic.presentation().player_camera_enabled,
        "authored Cairo fade-from-white script restores player camera/input");
  const WeaponDefinition *rifle = nullptr;
  for (const auto &[id, w] : c.weapons)
    if (w.name == "objects\\weapons\\rifle\\battle_rifle\\battle_rifle")
      rifle = &w;
  check(rifle && rifle->instantaneous && rifle->capacity == 36 &&
            rifle->burst == 3 && rifle->rounds_per_second == 15 &&
            rifle->damage_min == 6,
        "Cairo battle rifle authored parameters");
  check(c.damage_groups.size() == 21 && c.damage_materials.size() == 182 &&
            rifle->damage_group == 184551379 && rifle->damage_group_b == 0 &&
            c.player.vitality.shield_material == 147,
        "Cairo authored damage groups and player shield material");
  const auto armor = c.damage_materials.at(147);
  check(armor.armor_a == 301991883 && armor.armor_b == 0 &&
            damage_multiplier(c.damage_groups, rifle->damage_group,
                              rifle->damage_group_b, armor.armor_a,
                              armor.armor_b) == 1.f,
        "original Cairo rifle versus Chief shield armor multiplier");
  check(c.player.vitality.damage_flags == 3 &&
            c.player.vitality.body_material == 56 &&
            c.player.vitality.default_section == 0 &&
            c.player.vitality.minimum_body_stun_damage == 0 &&
            c.player.vitality.body_delay == 10 &&
            c.player.vitality.sections.size() == 2 &&
            c.player.vitality.sections[0].vitality == 0 &&
            c.player.vitality.sections[1].flags == 128,
        "independent Cairo Chief body material and damage section fixture");
  const auto body_armor = c.damage_materials.at(56);
  check(body_armor.armor_a == 251658404 && body_armor.armor_b == 0 &&
            damage_multiplier(c.damage_groups, rifle->damage_group,
                              rifle->damage_group_b, body_armor.armor_a,
                              body_armor.armor_b) == 1.f,
        "original Cairo rifle versus Chief body armor multiplier");
  Simulation session(c, 0, cairo.spawns().front());
  session.equip(rifle->datum);
  check(session.place_squad("1st_hum_floor_mar") == 3,
        "Cairo authored marine squad");
  const auto &marine_vitality = session.actors()[0].definition.vitality;
  check(marine_vitality.body_material == 9 &&
            marine_vitality.damage_flags == 0 &&
            marine_vitality.default_section == 0 &&
            marine_vitality.sections.size() == 3 &&
            marine_vitality.sections[1].flags == 128 &&
            marine_vitality.sections[1].vitality == .1f &&
            c.damage_materials.at(9).armor_a == 201328575,
        "independent Cairo marine body armor and head section fixture");
  const auto marine =
      cairo.load_model(session.actors()[0].definition.datum, false);
  const auto bind = pose_model(marine);
  check(bind.size() == marine.mesh.vertices.size() && !bind.empty(),
        "Marine render geometry decodes");
  for (std::size_t i = 0; i < bind.size(); ++i)
    check(length(bind[i].position - marine.mesh.vertices[i].position) < 1e-5f,
          "Marine inverse bind reproduces model positions");
  const auto idle =
      cairo.load_action(marine.animation, "combat", "missile", "any", "idle");
  check(idle.frame_count == 90 &&
            std::abs(idle.frames[0].translation.z - .34165248f) < 1e-6f &&
            std::abs(idle.frames[45 * idle.nodes.size()].translation.z -
                     .34285441f) < 1e-6f,
        "retail Marine sparse codec6 pelvis keys");
  const auto animated = pose_model(marine, &idle, 1.5f);
  check(length(animated[0].position - bind[0].position) > .001f,
        "Marine authored idle changes skin pose");
  const auto fp = cairo.load_model(rifle->first_person_model, false);
  const auto arms = cairo.load_model(c.first_person_hands, false);
  const auto fp_idle = cairo.load_action(rifle->first_person_animation, "any",
                                         "any", "any", "idle");
  const auto fp_pose = pose_model(fp, &fp_idle, 0, &arms);
  check(!fp_pose.empty(), "authored rifle idle poses against arm graph");
  for (const auto &[id, w] : c.weapons)
    if (w.name == "objects\\weapons\\rifle\\smg\\smg") {
      const auto smg = cairo.load_model(w.first_person_model, false);
      const auto clip = cairo.load_action(w.first_person_animation, "any",
                                          "any", "any", "idle");
      check(smg.mesh.vertices.size() / 3 == 1788 && clip.frame_count == 89,
            "retail SMG highest LOD and idle decode");
      check(std::abs(clip.frames[0].translation.z + .08f) < 1e-6f,
            "retail SMG codec3 base translation");
      check(pose_model(smg, &clip, 0, &arms).size() == 5364,
            "SMG graph matches gun bones by StringID");
    }
  for (int i = 0; i < 5; ++i)
    session.step({0, 0, cairo.spawns().front().facing, 0, false, false, true});
  check(session.shots() == 3 && session.player().weapon.loaded == 33,
        "battle rifle burst at 30Hz");
  session.step({0, 0, 0, 0, false, false, false, true});
  for (int i = 0; i < 100; ++i)
    session.step({});
  check(session.player().weapon.loaded == 36, "retail magazine reload");
  session.run_script("difficulty_normal");
  session.run_script("player_count");
  session.run_script("player0");
  session.scripts().audit();
  session.step({});
  check(session.scripts().diagnostics().empty(),
        "real Cairo static scripts execute supported host operations");
  const auto encoded = cairo.load_sound(rifle->sound);
  check(encoded.codec == 1 && encoded.channels == 2 &&
            encoded.data.size() == 142200,
        "real firing sound resource chunks");
  const auto pcm = decode_sound(encoded);
  check(pcm.frames() == 137579, "real Xbox ADPCM decoded/resampled length");
  float peak = 0;
  for (float f : pcm.stereo)
    peak = std::max(peak, std::abs(f));
  check(peak > .1f, "real decoded sound signal");
  std::cout << "Retail: " << session.status()
            << " decoded_audio_frames=" << pcm.frames() << " peak=" << peak
            << '\n';
}
} // namespace
int main(int argc, char **argv) {
  try {
    synthetic_tests();
    scene_tests();
    if (argc > 1)
      retail_tests(argv[1]);
    SDL_Quit();
    std::cout << "Physics, AI, combat, script, audio checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
