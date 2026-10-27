// Oracle for the native TU1.5 vehicle motion owners (non-host vehicle
// adoption, phase 1). A player on prepared MCC Coagulation enters the
// driver seat of each authored vehicle and drives it (throttle, steering by
// aim, braking, coasting, strafing; the Banshee also climbs) through the
// retained TU1.5 world. On every seated tick:
//   - native BC190/BC200 against their translated entries on the vehicle;
//   - native D2B60/D2BD0 rider order against the translated iterator;
//   - native B4C40 against its reference: the translated D2B60/D2BD0
//     iterator plus the B4C40 glue transcribed from the listing (B4C40 has
//     no translated entry). Cases: corrected velocity only (B44C0), position
//     and velocity (B4DA0) and position only;
//   - native B4F00 against the translated helper chain on the vehicle;
//   - native B9800 against its translated entry on the vehicle;
//   - native B44C0 decision 2 (vehicle: B4F00, B4C40, B9800) against the
//     composed reference (verified B4F00, reference B4C40, translated B9800)
//     with reported position, forward, up, linear and angular velocity.
// Every comparison checks every byte either route wrote outside the dead
// call frames and the oracle's own scratch, then rolls both routes back, so
// the world follows the unperturbed TU1.5 trajectory. That trajectory is
// then used to measure the force-free extrapolation stepper of the vehicle
// predictor against the TU1.5 host after 1..6 ticks.
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_tu15_object_motion.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/nonhost_vehicle_prediction.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <xmmintrin.h>

using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value)
    throw FormatError(message);
}
constexpr std::uint32_t scratch = 0x60060100u; // the live world's call-input region
constexpr std::uint32_t scratch_end = scratch + 0x400u;
constexpr std::uint32_t iterator = scratch + 0x300u;
std::uint32_t u32(const SourceRuntimeMemory &m, std::uint32_t at) {
  const auto b = m.view(at, 4);
  return std::to_integer<std::uint32_t>(b[0]) | std::to_integer<std::uint32_t>(b[1]) << 8 |
         std::to_integer<std::uint32_t>(b[2]) << 16 | std::to_integer<std::uint32_t>(b[3]) << 24;
}
float f32(const SourceRuntimeMemory &m, std::uint32_t at) { return std::bit_cast<float>(u32(m, at)); }
Vec3 vec(const SourceRuntimeMemory &m, std::uint32_t at) { return {f32(m, at), f32(m, at + 4), f32(m, at + 8)}; }
void put32(SourceRuntimeMemory &m, std::uint32_t at, std::uint32_t x) {
  const std::array<std::byte, 4> b{std::byte(x), std::byte(x >> 8), std::byte(x >> 16), std::byte(x >> 24)};
  m.write(at, b);
}
void putf(SourceRuntimeMemory &m, std::uint32_t at, float v) { put32(m, at, std::bit_cast<std::uint32_t>(v)); }
void put(SourceRuntimeMemory &m, std::uint32_t at, Vec3 v) {
  putf(m, at, v.x);
  putf(m, at + 4, v.y);
  putf(m, at + 8, v.z);
}
bool same(Vec3 a, Vec3 b) {
  return std::bit_cast<std::uint32_t>(a.x) == std::bit_cast<std::uint32_t>(b.x) &&
         std::bit_cast<std::uint32_t>(a.y) == std::bit_cast<std::uint32_t>(b.y) &&
         std::bit_cast<std::uint32_t>(a.z) == std::bit_cast<std::uint32_t>(b.z);
}
std::uint32_t object_data(const SourceRuntimeMemory &m, std::uint32_t object) {
  return u32(m, u32(m, u32(m, 0x4ecbd0u) + 0x44) + (object & 0xffffu) * 12u + 8u);
}
// Write journal on the live bank (Impl::snapshot_oracle's approach).
struct Journal {
  SourceRuntimeMemory &bank;
  std::vector<std::pair<std::uint32_t, std::vector<std::byte>>> undo;
  bool recording = true;
  explicit Journal(SourceRuntimeMemory &b) : bank(b) {
    source_runtime_register_write_observer(bank, this, [this](std::uint32_t at, std::size_t bytes) {
      const auto before = bank.view(at, bytes);
      undo.emplace_back(at, std::vector<std::byte>(before.begin(), before.end()));
    });
  }
  void stop() {
    if (recording)
      source_runtime_unregister_write_observer(bank, this);
    recording = false;
  }
  void rollback() {
    stop();
    for (auto it = undo.rbegin(); it != undo.rend(); ++it)
      bank.write(it->first, it->second);
    undo.clear();
  }
  ~Journal() { stop(); }
};
struct Counters {
  unsigned calls{}, compared_bytes{};
};
// Run the reference, roll it back, run native, compare every byte either
// wrote outside the dead ranges, then roll native back too.
void compare_writes(SourceRuntimeMemory &bank, std::uint32_t frame_low, std::uint32_t frame_high,
                    const std::function<void()> &reference, const std::function<void()> &native, const char *what,
                    Counters &counters) {
  const auto dead = [&](std::uint32_t at) {
    return (at >= frame_low && at < frame_high) || (at >= scratch && at < scratch_end);
  };
  std::map<std::uint32_t, std::byte> expected;
  {
    Journal journal(bank);
    reference();
    journal.stop();
    for (const auto &[at, before] : journal.undo)
      for (std::size_t i = 0; i < before.size(); ++i)
        expected[std::uint32_t(at + i)] = bank.view(std::uint32_t(at + i), 1)[0];
    journal.rollback();
  }
  Journal journal(bank);
  native();
  journal.stop();
  for (const auto &[at, before] : journal.undo)
    for (std::size_t i = 0; i < before.size(); ++i)
      expected.try_emplace(std::uint32_t(at + i), before[i]);
  ++counters.calls;
  for (const auto &[at, value] : expected) {
    if (dead(at))
      continue;
    ++counters.compared_bytes;
    if (bank.view(at, 1)[0] != value) {
      std::ostringstream why;
      why << "Native " << what << " differs from the reference at " << std::hex << at
          << ": native=" << std::to_integer<unsigned>(bank.view(at, 1)[0])
          << " reference=" << std::to_integer<unsigned>(value);
      journal.rollback();
      throw FormatError(why.str());
    }
  }
  journal.rollback();
}
struct Fpu {
  unsigned mx;
  std::fenv_t env;
  explicit Fpu(SourcePhysicsFactoryFloatingPoint fp) : mx(_mm_getcsr()) {
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
  }
  ~Fpu() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
float ss(__m128 v) { return _mm_cvtss_f32(v); }
__m128 sv(float v) { return _mm_set_ss(v); }

// Translated D2B60 then D2BD0 until it returns false: the rider order.
std::vector<std::uint32_t> translated_riders(PhysicsLiveWorldTu15 &live, SourceRuntimeMemory &bank,
                                             std::uint32_t object) {
  std::vector<std::uint32_t> riders;
  auto init = live.prepare_source_call();
  init.gp[0] = object;
  init.gp[3] = iterator;
  live.run_actual(0xd2b60u, init);
  for (unsigned guard = 0; guard < 64; ++guard) {
    auto next = live.prepare_source_call();
    next.gp[2] = iterator;
    live.run_actual(0xd2bd0u, next);
    if (!(next.gp[0] & 0xffu))
      return riders;
    riders.push_back(u32(bank, iterator + 0x10));
  }
  throw FormatError("Translated D2BD0 did not terminate");
}
// B4C40 reference: the listing's glue (B4C40..B4D9F) over the translated
// iterator. Lane order and operand order follow the listing.
void reference_b4c40(PhysicsLiveWorldTu15 &live, SourceRuntimeMemory &bank, std::uint32_t object,
                     const Vec3 *position, const Vec3 *linear, SourcePhysicsFactoryFloatingPoint fp) {
  const auto headers = u32(bank, u32(bank, 0x4ecbd0u) + 0x44);
  const auto header = headers + (object & 0xffffu) * 12u;
  if (bank.view(header + 3, 1)[0] != std::byte{1}) // B4C5C
    return;
  const auto o = u32(bank, header + 8);
  const float seconds = f32(bank, u32(bank, 0x51df54u) + 4); // B4C71
  float d0{}, d1{}, d2{};
  if (position) { // B4C78..B4CC9
    Fpu fpu(fp);
    const float x0 = ss(_mm_sub_ss(sv(f32(bank, o + 0x64)), sv(position->x)));
    const float x1 = ss(_mm_sub_ss(sv(f32(bank, o + 0x68)), sv(position->y)));
    const float x2 = ss(_mm_sub_ss(sv(f32(bank, o + 0x6c)), sv(position->z)));
    putf(bank, o + 0x270, ss(_mm_add_ss(sv(f32(bank, o + 0x270)), sv(x0))));
    putf(bank, o + 0x274, ss(_mm_add_ss(sv(f32(bank, o + 0x274)), sv(x1))));
    putf(bank, o + 0x278, ss(_mm_add_ss(sv(f32(bank, o + 0x278)), sv(x2))));
  }
  if (linear) { // B4CD5..B4D03
    Fpu fpu(fp);
    d0 = ss(_mm_mul_ss(_mm_sub_ss(sv(linear->x), sv(f32(bank, o + 0x88))), sv(seconds)));
    d1 = ss(_mm_mul_ss(_mm_sub_ss(sv(linear->y), sv(f32(bank, o + 0x8c))), sv(seconds)));
    d2 = ss(_mm_mul_ss(_mm_sub_ss(sv(linear->z), sv(f32(bank, o + 0x90))), sv(seconds)));
  }
  // B4D1B: D2B60, then D2BD0 per rider (B4D24/B4D91).
  auto init = live.prepare_source_call();
  init.gp[0] = object;
  init.gp[3] = iterator;
  live.run_actual(0xd2b60u, init);
  for (unsigned guard = 0; guard < 64; ++guard) {
    auto next = live.prepare_source_call();
    next.gp[2] = iterator;
    live.run_actual(0xd2bd0u, next);
    if (!(next.gp[0] & 0xffu))
      return;
    const auto u = u32(bank, headers + (u32(bank, iterator + 0x10) & 0xffffu) * 12u + 8u); // B4D34..B4D45
    if (linear) {                                                                        // B4D4B..B4D7F
      Fpu fpu(fp);
      putf(bank, u + 0x270, ss(_mm_add_ss(sv(f32(bank, u + 0x270)), sv(d0))));
      putf(bank, u + 0x274, ss(_mm_add_ss(sv(f32(bank, u + 0x274)), sv(d1))));
      putf(bank, u + 0x278, ss(_mm_add_ss(sv(f32(bank, u + 0x278)), sv(d2))));
    }
    put32(bank, u + 0x134, u32(bank, u + 0x134) | 0x10000000u); // B4D87
  }
  throw FormatError("Reference B4C40 iterator did not terminate");
}

struct Entrance {
  Spawn spawn;
  std::size_t placement{};
};
Entrance entrance(const Cache &cache, const LevelContent &content, std::string_view name, unsigned classification) {
  const auto &assets = *content.unit_interactions;
  for (const auto &[row, placement] : assets.netgame_placements) {
    if (classification && content.netgame_equipment.at(row).classification != classification)
      continue;
    const auto &object = content.objects.at(placement);
    const auto tag = std::find_if(cache.tags().begin(), cache.tags().end(),
                                  [&](const auto &t) { return t.datum == object.definition; });
    if (tag == cache.tags().end() || tag->name.find("\\" + std::string(name) + "\\") == std::string::npos)
      continue;
    const auto basis = authored_basis(object.rotation);
    const auto root = source_object_transform(object.position, basis.forward, basis.up, object.scale, false);
    const auto &definition = assets.units.at(object.definition);
    const auto nodes = pose_model_nodes_world(definition.hierarchy, root);
    if (definition.seats.empty())
      continue;
    const auto &seat = definition.seats[0];
    auto marker = definition.markers.find(seat.entry_marker);
    if (marker == definition.markers.end() && (seat.flags & 0x800u))
      marker = definition.markers.find(seat.marker);
    if (marker == definition.markers.end())
      continue;
    const auto entrances = pose_model_markers(marker->second, nodes, definition.permutations, {}, 1);
    if (entrances.empty())
      continue;
    Spawn spawn;
    spawn.position = entrances.front().world.position + entrances.front().world.basis[0] * (seat.radius * .5f);
    CollisionWorld terrain(content.collision);
    const auto floor = terrain.raycast(spawn.position + Vec3{0, 0, 2 * content.player.height}, {0, 0, -1},
                                       4 * content.player.height);
    if (!floor)
      continue;
    spawn.position.z = floor->position.z + content.player.radius;
    const auto look = entrances.front().world.position - (spawn.position + Vec3{0, 0, content.player.camera_height});
    spawn.facing = std::atan2(look.y, look.x);
    return {spawn, placement};
  }
  throw FormatError("Missing authored vehicle driver approach: " + std::string(name));
}

struct Sample {
  bool valid{};
  std::uint32_t vehicle = null_datum;
  NonHostVehicleMotion motion;
};
struct Percentiles {
  float median{}, p90{}, maximum{};
  std::size_t count{};
};
Percentiles percentiles(std::vector<float> values) {
  Percentiles p;
  p.count = values.size();
  if (values.empty())
    return p;
  std::sort(values.begin(), values.end());
  p.median = values[values.size() / 2];
  p.p90 = values[std::min(values.size() - 1, values.size() * 9 / 10)];
  p.maximum = values.back();
  return p;
}
} // namespace

int main(int argc, char **argv) try {
  std::cout << std::unitbuf;
  if (argc != 2)
    throw FormatError("Supply a prepared MCC Coagulation map");
  if (!std::filesystem::is_regular_file(argv[1]))
    return 77;
  struct Case {
    const char *label, *name;
    unsigned parameter, choice, classification;
    bool climbs{};
  };
  // Original multiplayer driver choices (the vehicle camera fixture's rows).
  // TU1.5 has no Mongoose: its vehicle table slot 6 is NONE.
  const std::array cases{Case{"warthog", "warthog", 76, 1, 1},  Case{"ghost", "ghost", 76, 3, 1},
                         Case{"banshee", "banshee", 76, 1, 0, true}, Case{"scorpion", "scorpion", 78, 1, 3},
                         Case{"wraith", "wraith", 78, 2, 3},     Case{"spectre", "spectre", 76, 5, 1}};
  Counters riders_checked, b4c40, setter, nudge;
  unsigned transforms = 0, corrections = 0, refused = 0, dead_band_linear = 0, dead_band_angular = 0;
  unsigned rider_units = 0, seated_ticks = 0, cases_run = 0;
  std::map<std::string, std::vector<Sample>> trajectories;
  for (const auto &selected : cases) {
    if (const auto filter = std::getenv("HALO2_VEHICLE_MOTION_CASE"); filter && std::string_view(filter) != selected.label)
      continue;
    MultiplayerOptions options;
    options.map = argv[1];
    options.shared_maps = options.map.parent_path();
    options.local_players = 1;
    options.source_object_world = true;
    options.mute = true;
    options.starting_weapon = 5;
    options.variant.values.set(selected.parameter, selected.choice);
    options.session_random_seed = 0x0b4c40u;
    Entrance approach;
    {
      const Cache cache(options.map, options.shared_maps);
      check(cache.is_mcc(), "Vehicle motion oracle requires prepared MCC assets");
      const auto content = cache.load_content(0, true, source_game_variant(options.variant));
      approach = entrance(cache, content, selected.name, selected.classification);
    }
    options.native_spawn_inputs = {approach.spawn};
    // No view-definition rebind: this oracle draws nothing and needs no
    // first-person camera owner.
    MultiplayerPreparedWorld world(options);
    auto &objects = *world.source_objects;
    auto &live = objects.tu15_world();
    auto &bank = objects.memory();
    std::array<Controls, multiplayer_capacity> controls{};
    const auto slot = world.local_slots[0];
    controls[slot].yaw = approach.spawn.facing;
    for (unsigned tick = 0; tick < 60; ++tick)
      world.match.step(controls);
    bool entered = false;
    for (unsigned tick = 0; tick < 150 && !entered; ++tick) {
      controls[slot].pickup = true;
      controls[slot].pickup_milliseconds = 900;
      world.match.step(controls);
      entered = world.match.state().players[slot].source_parent != null_datum;
    }
    check(entered, std::string("Vehicle motion case failed original seat entry: ") + selected.label);
    controls[slot].pickup = false;
    controls[slot].pickup_milliseconds = 0;
    for (unsigned tick = 0; tick < 40; ++tick)
      world.match.step(controls);
    const auto vehicle = world.match.state().players[slot].source_parent;
    check(world.match.state().players[slot].source_seat == 0, "Vehicle motion case is not in the driver seat");
    check(bank.view(u32(bank, u32(bank, 0x4ecbd0u) + 0x44) + (vehicle & 0xffffu) * 12u + 3, 1)[0] == std::byte{1},
          "Driven parent is not a vehicle object");
    {
      const auto forward = native_tu15_object_transform(bank, vehicle).forward;
      controls[slot].yaw = std::atan2(forward.y, forward.x);
      controls[slot].pitch = 0;
    }
    const float heading = controls[slot].yaw;
    auto &trajectory = trajectories[selected.label];
    ++cases_run;
    for (unsigned tick = 0; tick < 420; ++tick) {
      auto &c = controls[slot];
      const unsigned phase = tick / 60;
      c.forward = c.right = 0;
      c.pitch = 0;
      c.yaw = heading;
      if (phase == 0)
        c.forward = 1;
      else if (phase == 1) {
        c.forward = 1;
        c.yaw = heading + .6f * std::sin(float(tick - 60) * .105f);
        if (selected.climbs)
          c.pitch = .35f;
      } else if (phase == 2)
        c.forward = -1;
      else if (phase == 3) {
        // coast
      } else if (phase == 4) {
        c.forward = .7f;
        c.right = 1;
        c.yaw = heading + float(tick - 240) * .02f;
      } else if (phase == 5) {
        c.forward = 1;
        c.yaw = heading + 1.2f - float(tick - 300) * .03f;
      } else {
        c.forward = tick % 20 < 10 ? 1.f : 0.f;
        c.yaw = heading + .3f * std::sin(float(tick) * .2f);
      }
      world.match.step(controls);
      const auto state = world.match.state();
      const auto &p = state.players[slot];
      Sample sample;
      if (p.dead || p.source_parent != vehicle || p.source_seat != 0) {
        trajectory.push_back(sample);
        continue;
      }
      ++seated_ticks;
      const auto fp = *live.prepare_source_call().floating_point;

      // BC190/BC200 on the vehicle against their translated entries.
      {
        auto abi = live.prepare_source_call(std::array{scratch + 0x40u});
        abi.gp[0] = vehicle;
        live.run_actual(0xbc190u, abi);
        const auto t = native_tu15_object_transform(bank, vehicle);
        check(same(vec(bank, scratch + 0x44), t.forward) && same(vec(bank, scratch + 0x50), t.left) &&
                  same(vec(bank, scratch + 0x5c), t.up) && same(vec(bank, scratch + 0x68), t.position) &&
                  f32(bank, scratch + 0x40) == t.scale,
              "Native BC190 vehicle transform differs");
        auto v = live.prepare_source_call(std::array{scratch + 0x80u});
        v.gp[0] = vehicle;
        v.gp[1] = scratch + 0x74;
        live.run_actual(0xbc200u, v);
        const auto n = native_tu15_object_velocities(bank, vehicle);
        check(same(vec(bank, scratch + 0x74), n.linear) && same(vec(bank, scratch + 0x80), n.angular),
              "Native BC200 vehicle velocities differ");
        ++transforms;
        sample = {true, vehicle, {t.position, t.forward, t.up, n.linear, n.angular}};
      }
      trajectory.push_back(sample);
      const auto now = native_tu15_object_transform(bank, vehicle);
      const auto have = native_tu15_object_velocities(bank, vehicle);

      // D2B60/D2BD0 rider order.
      {
        Journal journal(bank);
        const auto reference = translated_riders(live, bank, vehicle);
        journal.rollback();
        const auto native = native_tu15_vehicle_riders(bank, vehicle);
        check(reference == native, "Native D2B60/D2BD0 rider order differs from the translated iterator");
        check(std::find(native.begin(), native.end(), p.source_unit) != native.end(),
              "The driver is not among the vehicle's riders");
        ++riders_checked.calls;
        rider_units += unsigned(native.size());
      }

      // B4C40 cases: B44C0's corrected velocity, B4DA0's pose and velocity,
      // and position only.
      {
        const unsigned which = tick % 3;
        const Vec3 position = now.position + Vec3{.07f * float(tick % 5), -.04f, .02f * float(tick % 3)};
        const Vec3 linear = have.linear + Vec3{.3f, -.2f * float(tick % 4), .05f};
        const Vec3 *pp = which == 0 ? nullptr : &position;
        const Vec3 *lp = which == 2 ? nullptr : &linear;
        const auto frame = live.prepare_source_call(std::array{0u});
        compare_writes(bank, frame.gp[4] - 0x10000u, frame.gp[4] + 8u,
                       [&] { reference_b4c40(live, bank, vehicle, pp, lp, fp); },
                       [&] { native_tu15_vehicle_child_update(bank, vehicle, pp, lp, fp); }, "B4C40", b4c40);
      }

      // B4F00 on the vehicle against the translated helper chain.
      const float turn = float(tick % 7) * .03f - .09f;
      const Vec3 desired_forward = normalized(Vec3{now.forward.x * std::cos(turn) - now.forward.y * std::sin(turn),
                                                   now.forward.x * std::sin(turn) + now.forward.y * std::cos(turn),
                                                   now.forward.z});
      const Vec3 desired_up = normalized(now.up + Vec3{.02f * float(tick % 3), 0, 0});
      const Vec3 desired_position = now.position + Vec3{.04f * float(tick % 5), -.03f * float(tick % 3), .01f};
      const Vec3 angular_in = have.angular + Vec3{0, 0, .1f * float(tick % 4)};
      {
        put(bank, scratch, desired_position);
        put(bank, scratch + 0x10, desired_forward);
        put(bank, scratch + 0x20, desired_up);
        const std::uint32_t current_t = scratch + 0x100, desired_t = scratch + 0x140, inverse_t = scratch + 0x180,
                            difference_t = scratch + 0x1c0, quat = scratch + 0x200, axis = scratch + 0x210,
                            angle = scratch + 0x21c;
        Journal journal(bank);
        auto a = live.prepare_source_call(std::array{current_t});
        a.gp[0] = vehicle;
        live.run_actual(0xbc190u, a);
        auto b = live.prepare_source_call();
        b.gp[0] = scratch + 0x20;
        b.gp[1] = scratch + 0x10;
        b.gp[2] = desired_t;
        b.gp[6] = scratch;
        live.run_actual(0x144bf0u, b);
        auto i = live.prepare_source_call();
        i.gp[1] = current_t;
        i.gp[0] = inverse_t;
        live.run_actual(0x144090u, i);
        auto pr = live.prepare_source_call(std::array{difference_t});
        pr.gp[1] = desired_t;
        pr.gp[2] = inverse_t;
        live.run_actual(0x145560u, pr);
        auto q = live.prepare_source_call();
        q.gp[1] = difference_t + 4;
        q.gp[3] = quat;
        live.run_actual(0x144a60u, q);
        auto x = live.prepare_source_call();
        x.gp[0] = quat;
        x.gp[6] = axis;
        x.gp[7] = angle;
        live.run_actual(0x120150u, x);
        NativeTu15Velocities expected;
        {
          Fpu fpu(fp); // B4F00 glue (B4F87..B5134)
          const float scale = ss(_mm_mul_ss(sv(f32(bank, angle)), sv(5.f)));
          float ax = ss(_mm_mul_ss(sv(f32(bank, axis)), sv(scale))), ay = ss(_mm_mul_ss(sv(f32(bank, axis + 4)), sv(scale))),
                az = ss(_mm_mul_ss(sv(f32(bank, axis + 8)), sv(scale)));
          const float as = ss(_mm_add_ss(_mm_add_ss(_mm_mul_ss(sv(az), sv(az)), _mm_mul_ss(sv(ay), sv(ay))),
                                         _mm_mul_ss(sv(ax), sv(ax))));
          if (_mm_comigt_ss(sv(as), sv(1.f))) {
            volatile long double root = std::sqrt(static_cast<long double>(as));
            volatile long double k = 1.0L / root;
            volatile float rx = static_cast<float>(ax * k), ry = static_cast<float>(ay * k), rz = static_cast<float>(az * k);
            ax = rx;
            ay = ry;
            az = rz;
          }
          const float g = std::bit_cast<float>(0x3f666666u);
          float lx = ss(_mm_mul_ss(sv(f32(bank, difference_t + 0x28)), sv(g))),
                ly = ss(_mm_mul_ss(sv(f32(bank, difference_t + 0x2c)), sv(g))),
                lz = ss(_mm_mul_ss(sv(f32(bank, difference_t + 0x30)), sv(g)));
          const float ls = ss(_mm_add_ss(_mm_add_ss(_mm_mul_ss(sv(lz), sv(lz)), _mm_mul_ss(sv(ly), sv(ly))),
                                         _mm_mul_ss(sv(lx), sv(lx))));
          if (_mm_comigt_ss(sv(ls), sv(std::bit_cast<float>(0x3b23d70bu)))) {
            volatile long double root = std::sqrt(static_cast<long double>(ls));
            volatile long double k = static_cast<long double>(std::bit_cast<float>(0x3d4ccccdu)) / root;
            volatile float rx = static_cast<float>(lx * k), ry = static_cast<float>(ly * k), rz = static_cast<float>(lz * k);
            lx = rx;
            ly = ry;
            lz = rz;
          }
          expected.linear = {ss(_mm_add_ss(sv(have.linear.x), sv(lx))), ss(_mm_add_ss(sv(have.linear.y), sv(ly))),
                             ss(_mm_add_ss(sv(have.linear.z), sv(lz)))};
          expected.angular = {ss(_mm_add_ss(sv(angular_in.x), sv(ax))), ss(_mm_add_ss(sv(angular_in.y), sv(ay))),
                              ss(_mm_add_ss(sv(angular_in.z), sv(az)))};
        }
        journal.rollback();
        const auto native = native_tu15_nudge_correction(bank, vehicle, desired_position, desired_forward, desired_up,
                                                         have.linear, angular_in, fp);
        check(same(native.linear, expected.linear) && same(native.angular, expected.angular),
              "Native B4F00 vehicle correction differs from the translated helper chain");
        ++corrections;
      }

      // B9800 on the vehicle: linear only, angular only, both, tiny (no wake).
      {
        const unsigned which = tick % 4;
        const Vec3 linear = which == 3 ? Vec3{1e-3f, 0, 0} : have.linear + Vec3{.2f, -.1f, .05f};
        const Vec3 angular = which == 3 ? Vec3{} : have.angular + Vec3{0, 0, .25f};
        const bool with_linear = which != 1, with_angular = which != 0;
        put(bank, scratch + 0x240, linear);
        put(bank, scratch + 0x250, angular);
        const auto refusal = native_tu15_object_velocities_unsupported(bank, vehicle, with_linear ? &linear : nullptr,
                                                                       with_angular ? &angular : nullptr);
        if (!refusal.empty()) {
          check(refusal.find("listener") != std::string::npos, "Unexpected vehicle B9800 refusal: " + refusal);
          ++refused;
        } else {
          auto abi = live.prepare_source_call(std::array{with_linear ? scratch + 0x240u : 0u});
          abi.gp[6] = vehicle;
          abi.gp[0] = with_angular ? scratch + 0x250u : 0u;
          compare_writes(bank, abi.gp[4] - 0x10000u, abi.gp[4] + 8u, [&] { live.run_actual(0xb9800u, abi); },
                         [&] {
                           native_tu15_set_object_velocities(bank, vehicle, with_linear ? &linear : nullptr,
                                                             with_angular ? &angular : nullptr, fp);
                         },
                         "vehicle B9800", setter);
        }
      }

      // B44C0 decision 2 on the vehicle, every operand reported (865B0).
      {
        const unsigned which = tick % 6;
        const Vec3 reported_position = now.position + Vec3{.05f, .02f * float(tick % 3), 0};
        const Vec3 reported_linear = which == 1 ? have.linear + Vec3{.1f, 0, 0} : have.linear + Vec3{.6f, .2f, 0};
        const Vec3 reported_angular = which == 2 ? have.angular + Vec3{0, 0, .05f} : have.angular + Vec3{0, .05f, .3f};
        const Vec3 *linear = which == 3 ? nullptr : &reported_linear;
        const Vec3 *angular = which == 4 ? nullptr : &reported_angular;
        check(!native_tu15_motion_held(bank, vehicle, fp), "B4BD0 held a vehicle");
        bool take_linear = linear != nullptr, take_angular = angular != nullptr;
        {
          Fpu fpu(fp); // B45A0..B45ED and B460F..B465D
          if (linear) {
            const float dx = ss(_mm_sub_ss(sv(linear->x), sv(have.linear.x))),
                        dy = ss(_mm_sub_ss(sv(linear->y), sv(have.linear.y))),
                        dz = ss(_mm_sub_ss(sv(linear->z), sv(have.linear.z)));
            const float sq = ss(_mm_add_ss(_mm_add_ss(_mm_mul_ss(sv(dz), sv(dz)), _mm_mul_ss(sv(dx), sv(dx))),
                                           _mm_mul_ss(sv(dy), sv(dy))));
            take_linear = _mm_comigt_ss(sv(sq), sv(std::bit_cast<float>(0x3d75c28fu)));
          }
          if (angular) {
            const float dx = ss(_mm_sub_ss(sv(angular->x), sv(have.angular.x))),
                        dy = ss(_mm_sub_ss(sv(angular->y), sv(have.angular.y))),
                        dz = ss(_mm_sub_ss(sv(angular->z), sv(have.angular.z)));
            const float sq = ss(_mm_add_ss(_mm_add_ss(_mm_mul_ss(sv(dz), sv(dz)), _mm_mul_ss(sv(dy), sv(dy))),
                                           _mm_mul_ss(sv(dx), sv(dx))));
            take_angular = _mm_comigt_ss(sv(sq), sv(std::bit_cast<float>(0x3c23d70au)));
          }
        }
        dead_band_linear += linear && !take_linear;
        dead_band_angular += angular && !take_angular;
        const auto out = native_tu15_nudge_correction(bank, vehicle, reported_position, desired_forward, desired_up,
                                                      take_linear ? *linear : have.linear,
                                                      take_angular ? *angular : have.angular, fp);
        const Vec3 *linear_out = take_linear ? &out.linear : nullptr, *angular_out = take_angular ? &out.angular : nullptr;
        if ((take_linear || take_angular) &&
            !native_tu15_object_velocities_unsupported(bank, vehicle, linear_out, angular_out).empty()) {
          ++refused;
        } else {
          put(bank, scratch + 0x260, out.linear);
          put(bank, scratch + 0x270, out.angular);
          // Each prepared call reuses the retained stack top, so B9800's
          // frame is prepared only after the reference B4C40 calls ran.
          const auto frame = live.prepare_source_call(std::array{0u});
          compare_writes(bank, frame.gp[4] - 0x10000u, frame.gp[4] + 8u,
                         [&] {
                           if (take_linear) // B46BD
                             reference_b4c40(live, bank, vehicle, nullptr, linear_out, fp);
                           if (take_linear || take_angular) {
                             auto abi = live.prepare_source_call(std::array{take_linear ? scratch + 0x260u : 0u});
                             abi.gp[6] = vehicle;
                             abi.gp[0] = take_angular ? scratch + 0x270u : 0u;
                             check(abi.gp[4] == frame.gp[4], "Prepared B9800 frame moved");
                             live.run_actual(0xb9800u, abi);
                           }
                         },
                         [&] {
                           const bool wrote = native_tu15_nudge_object(bank, vehicle, 2, &reported_position,
                                                                       &desired_forward, &desired_up, linear,
                                                                       angular, fp);
                           check(wrote == (take_linear || take_angular),
                                 "Native vehicle B44C0 decision 2 disagreed about the dead bands");
                         },
                         "vehicle B44C0 decision 2", nudge);
        }
      }
    }
    std::cout << selected.label << ": seated ticks so far=" << seated_ticks << '\n';
  }
  if (!cases_run)
    return 77;
  std::cout << "vehicle motion oracle: cases=" << cases_run << " seated_ticks=" << seated_ticks
            << " | BC190/BC200=" << transforms << " | D2B60/D2BD0 calls=" << riders_checked.calls
            << " riders=" << rider_units << " | B4C40 calls=" << b4c40.calls << " compared_bytes=" << b4c40.compared_bytes
            << " | B4F00 calls=" << corrections << " | B9800 calls=" << setter.calls
            << " compared_bytes=" << setter.compared_bytes << " | B44C0-2 calls=" << nudge.calls
            << " compared_bytes=" << nudge.compared_bytes << " dead_band_linear=" << dead_band_linear
            << " dead_band_angular=" << dead_band_angular << " refused_inactive_island=" << refused << '\n';
  check(b4c40.calls > 100 * cases_run && b4c40.compared_bytes > 0, "B4C40 oracle compared too few calls");
  check(nudge.calls > 50 * cases_run && nudge.compared_bytes > 0, "Vehicle B44C0 oracle compared too few calls");
  check(setter.calls > 50 * cases_run, "Vehicle B9800 oracle compared too few calls");

  // Trajectory measurement: force-free extrapolation from the TU1.5 motion
  // of tick i, against the TU1.5 motion of tick i+k (what a predictor with
  // k inputs in flight would show), next to the stale host pose (no
  // prediction).
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "extrapolation vs TU1.5 host (position wu / orientation rad: median p90 max)\n";
  for (const auto &[label, trajectory] : trajectories) {
    for (const unsigned k : {1u, 3u, 5u}) {
      std::vector<float> stale, extrapolated, stale_angle, extrapolated_angle;
      for (std::size_t i = 0; i + k < trajectory.size(); ++i) {
        bool ok = trajectory[i].valid;
        for (unsigned j = 1; j <= k && ok; ++j)
          ok = trajectory[i + j].valid && trajectory[i + j].vehicle == trajectory[i].vehicle;
        if (!ok)
          continue;
        const auto &from = trajectory[i].motion, &to = trajectory[i + k].motion;
        auto m = from;
        for (unsigned j = 0; j < k; ++j)
          step_nonhost_vehicle_extrapolation(m);
        const auto target = nonhost_vehicle_orientation(to.forward, to.up);
        stale.push_back(length(to.position - from.position));
        extrapolated.push_back(length(to.position - m.position));
        stale_angle.push_back(nonhost_vehicle_angle(nonhost_vehicle_orientation(from.forward, from.up), target));
        extrapolated_angle.push_back(nonhost_vehicle_angle(nonhost_vehicle_orientation(m.forward, m.up), target));
      }
      const auto s = percentiles(stale), e = percentiles(extrapolated), sa = percentiles(stale_angle),
                 ea = percentiles(extrapolated_angle);
      check(e.count > 0, "Trajectory measurement has no samples for " + label);
      std::cout << label << " k=" << k << " n=" << e.count << " stale " << s.median << ' ' << s.p90 << ' ' << s.maximum
                << " / " << sa.median << ' ' << sa.p90 << ' ' << sa.maximum << " | extrapolated " << e.median << ' '
                << e.p90 << ' ' << e.maximum << " / " << ea.median << ' ' << ea.p90 << ' ' << ea.maximum << '\n';
    }
  }
  std::cout << "native TU1.5 vehicle motion matches the translated route\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << "native TU1.5 vehicle motion oracle failed: " << error.what() << '\n';
  return 1;
}
