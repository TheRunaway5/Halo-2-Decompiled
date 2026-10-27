// Oracle for the native TU1.5 object motion setters (non-host adoption).
// A player on prepared MCC Lockout runs, idles, jumps and crouches through
// the retained TU1.5 world. On every tick the translated route runs under a
// write journal over the same live bank, is rolled back, and the native
// kernel runs; every byte either wrote outside the dead call frame must
// agree. B9800 and BC190/BC200 have translated entries. B4F00 does not: its
// reference is composed from the translated helpers it calls (BC190, 144BF0,
// 144090, 145560, 144A60, 120150) in its order, with its glue written here
// from the listing. B44C0 decision 2 composes those with translated B9800.
// The world keeps the native result, as production would.
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_tu15_object_motion.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_player_profile.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <array>
#include <bit>
#include <cfenv>
#include <cmath>
#include <iostream>
#include <map>
#include <xmmintrin.h>

using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value)
    throw FormatError(message);
}
struct Files {
  std::filesystem::path path;
  Files() {
    auto name = (std::filesystem::temp_directory_path() / "halo2-object-motion-XXXXXX").string();
    check(::mkdtemp(name.data()) != nullptr, "Object motion scratch creation failed");
    path = name;
  }
  ~Files() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};
constexpr std::uint32_t scratch = 0x60060100u; // the live world's call-input region
std::uint32_t u32(const SourceRuntimeMemory &m, std::uint32_t at) {
  const auto b = m.view(at, 4);
  return std::to_integer<std::uint32_t>(b[0]) | std::to_integer<std::uint32_t>(b[1]) << 8 |
         std::to_integer<std::uint32_t>(b[2]) << 16 | std::to_integer<std::uint32_t>(b[3]) << 24;
}
float f32(const SourceRuntimeMemory &m, std::uint32_t at) { return std::bit_cast<float>(u32(m, at)); }
Vec3 vec(const SourceRuntimeMemory &m, std::uint32_t at) { return {f32(m, at), f32(m, at + 4), f32(m, at + 8)}; }
void put(SourceRuntimeMemory &m, std::uint32_t at, Vec3 v) {
  std::array<std::byte, 12> b{};
  for (unsigned i = 0; i < 3; ++i) {
    const auto x = std::bit_cast<std::uint32_t>(i == 0 ? v.x : i == 1 ? v.y : v.z);
    for (unsigned k = 0; k < 4; ++k)
      b[i * 4 + k] = std::byte(x >> (8 * k));
  }
  m.write(at, b);
}
bool same(Vec3 a, Vec3 b) {
  return std::bit_cast<std::uint32_t>(a.x) == std::bit_cast<std::uint32_t>(b.x) &&
         std::bit_cast<std::uint32_t>(a.y) == std::bit_cast<std::uint32_t>(b.y) &&
         std::bit_cast<std::uint32_t>(a.z) == std::bit_cast<std::uint32_t>(b.z);
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
  ~Journal() { stop(); }
};
struct Counters {
  unsigned calls{}, bytes{}, compared_bytes{};
};
// Run the translated reference, roll it back, run native, compare.
void compare_writes(SourceRuntimeMemory &bank, std::uint32_t frame_low, std::uint32_t frame_high,
                    const std::function<void()> &reference, const std::function<void()> &native, const char *what,
                    Counters &counters) {
  const auto dead = [&](std::uint32_t at) { return at >= frame_low && at < frame_high; };
  std::map<std::uint32_t, std::byte> expected;
  {
    Journal journal(bank);
    reference();
    for (const auto &[at, before] : journal.undo)
      for (std::size_t i = 0; i < before.size(); ++i)
        expected[std::uint32_t(at + i)] = bank.view(std::uint32_t(at + i), 1)[0];
    journal.stop();
    for (auto it = journal.undo.rbegin(); it != journal.undo.rend(); ++it)
      bank.write(it->first, it->second);
  }
  {
    Journal journal(bank);
    native();
    journal.stop();
    for (const auto &[at, before] : journal.undo)
      for (std::size_t i = 0; i < before.size(); ++i)
        expected.try_emplace(std::uint32_t(at + i), before[i]);
  }
  ++counters.calls;
  for (const auto &[at, value] : expected) {
    if (dead(at))
      continue;
    ++counters.compared_bytes;
    if (bank.view(at, 1)[0] != value) {
      std::ostringstream why;
      why << "Native " << what << " differs from the translated route at " << std::hex << at
          << ": native=" << std::to_integer<unsigned>(bank.view(at, 1)[0]) << " translated=" << std::to_integer<unsigned>(value);
      throw FormatError(why.str());
    }
  }
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
} // namespace

int main(int argc, char **argv) try {
  std::cout << std::unitbuf;
  if (argc != 2)
    throw FormatError("Supply a prepared MCC Lockout map");
  if (!std::filesystem::is_regular_file(argv[1]))
    return 77;
  const Cache cache(argv[1]);
  check(cache.is_mcc(), "Object motion oracle requires prepared MCC assets");
  const auto content = cache.load_content(0, true);
  Files files;
  MultiplayerObjectRuntime world(cache, 0, files.path);
  UiVariant variant{"Slayer", 0, {{89, 5}}};
  auto rules = slayer_rules(variant, false);
  rules.weapon = cache.slayer_starting_weapon(rules.starting_weapon);
  rules.random_seed = 0x0b9800u;
  world.configure_session(source_game_variant(variant), 1, 30, rules.random_seed);
  MultiplayerMatch match(content, 0, cache.spawns(), rules, {}, {}, &world);
  check(match.source_engine_owned(), "Object motion oracle did not select the original source match engine");
  auto configuration = source_default_player_configuration();
  configuration[0x44] = 2;
  const unsigned slot = match.join("Object motion", -1, configuration);
  std::array<unsigned, 1> slots{slot};
  std::vector<FirstPersonBindingDefinition> views{cache.load_first_person_binding(rules.weapon, 2)};
  match.configure_local_views(slots, views);
  std::array<Controls, multiplayer_capacity> controls{};
  for (unsigned tick = 0; tick < 40 && match.player(slot).source_current_object == null_datum; ++tick)
    match.step(controls);
  const auto unit = match.player(slot).source_current_object;
  check(unit != null_datum, "Object motion player was not born");
  controls[slot].yaw = match.unit(slot).yaw;
  controls[slot].pitch = match.unit(slot).pitch;
  match.step(controls);

  auto &live = world.tu15_world();
  auto &bank = world.memory();
  Counters setter, nudge, transforms, velocities, corrections;
  unsigned refused = 0, wakes = 0, dead_band = 0, held = 0, inactive_islands = 0, airborne = 0, crouched = 0, resting = 0;
  const auto heading = controls[slot].yaw;
  for (unsigned tick = 0; tick < 720; ++tick) {
    auto &c = controls[slot];
    c = {};
    c.yaw = heading + (tick >= 300 && tick < 360 ? float(tick - 300) * .04f : 0.f);
    const unsigned phase = (tick / 60) % 6;
    if (phase == 0 || phase == 3)
      c.forward = 1;
    else if (phase == 1)
      c.right = 1;
    else if (phase == 2) {
      c.forward = .6f;
      c.jump = tick % 30 == 5;
    } else if (phase == 4) {
      c.forward = .7f;
      c.crouch = true;
    } // phase 5: idle, long enough for a resting biped
    match.step(controls);
    if (match.unit(slot).vitality.dead || match.player(slot).source_current_object != unit)
      continue;
    const auto motion = world.biped_motion(unit);
    if (motion.physical_state != 1 || motion.parent != 0xffffffffu)
      continue;
    airborne += (motion.support_flags & 1u) != 0;
    crouched += motion.crouch > .5f;
    resting += phase == 5;

    // BC190 and BC200 against their translated entries.
    {
      auto abi = live.prepare_source_call(std::array{scratch + 0x40u});
      abi.gp[0] = unit;
      live.run_actual(0xbc190u, abi);
      const auto t = native_tu15_object_transform(bank, unit);
      check(same(vec(bank, scratch + 0x44), t.forward) && same(vec(bank, scratch + 0x50), t.left) &&
                same(vec(bank, scratch + 0x5c), t.up) && same(vec(bank, scratch + 0x68), t.position) &&
                f32(bank, scratch + 0x40) == t.scale,
            "Native BC190 transform differs");
      ++transforms.calls;
      auto v = live.prepare_source_call(std::array{scratch + 0x80u});
      v.gp[0] = unit;
      v.gp[1] = scratch + 0x74;
      live.run_actual(0xbc200u, v);
      const auto n = native_tu15_object_velocities(bank, unit);
      check(same(vec(bank, scratch + 0x74), n.linear) && same(vec(bank, scratch + 0x80), n.angular),
            "Native BC200 velocities differ");
      ++velocities.calls;
    }
    const auto current = native_tu15_object_velocities(bank, unit);
    const auto fp = *live.prepare_source_call().floating_point;

    // B4F00 against the translated helper chain plus the listing's glue.
    const auto transform = native_tu15_object_transform(bank, unit);
    const float turn = float(tick % 7) * .03f;
    const Vec3 desired_forward = normalized(Vec3{transform.forward.x * std::cos(turn) - transform.forward.y * std::sin(turn),
                                                 transform.forward.x * std::sin(turn) + transform.forward.y * std::cos(turn),
                                                 transform.forward.z});
    const Vec3 desired_position = transform.position + Vec3{.04f * float(tick % 5), -.03f * float(tick % 3), .01f};
    const Vec3 angular_in{0, 0, .1f * float(tick % 4)};
    {
      put(bank, scratch, desired_position);
      put(bank, scratch + 0x10, desired_forward);
      put(bank, scratch + 0x20, transform.up);
      const std::uint32_t current_t = scratch + 0x100, desired_t = scratch + 0x140, inverse_t = scratch + 0x180,
                          difference_t = scratch + 0x1c0, quat = scratch + 0x200, axis = scratch + 0x210,
                          angle = scratch + 0x21c;
      auto a = live.prepare_source_call(std::array{current_t});
      a.gp[0] = unit;
      live.run_actual(0xbc190u, a);
      auto b = live.prepare_source_call();
      b.gp[0] = scratch + 0x20; b.gp[1] = scratch + 0x10; b.gp[2] = desired_t; b.gp[6] = scratch;
      live.run_actual(0x144bf0u, b);
      auto i = live.prepare_source_call();
      i.gp[1] = current_t; i.gp[0] = inverse_t;
      live.run_actual(0x144090u, i);
      auto p = live.prepare_source_call(std::array{difference_t});
      p.gp[1] = desired_t; p.gp[2] = inverse_t;
      live.run_actual(0x145560u, p);
      auto q = live.prepare_source_call();
      q.gp[1] = difference_t + 4; q.gp[3] = quat;
      live.run_actual(0x144a60u, q);
      auto x = live.prepare_source_call();
      x.gp[0] = quat; x.gp[6] = axis; x.gp[7] = angle;
      live.run_actual(0x120150u, x);
      // B4F00 glue (B4F87..B5134).
      NativeTu15Velocities expected;
      {
        Fpu fpu(fp);
        const auto ss = [](float v) { return _mm_set_ss(v); };
        const auto fs = [](__m128 v) { return _mm_cvtss_f32(v); };
        const float scale = fs(_mm_mul_ss(ss(f32(bank, angle)), ss(5.f)));
        float ax = fs(_mm_mul_ss(ss(f32(bank, axis)), ss(scale))), ay = fs(_mm_mul_ss(ss(f32(bank, axis + 4)), ss(scale))),
              az = fs(_mm_mul_ss(ss(f32(bank, axis + 8)), ss(scale)));
        const float as = fs(_mm_add_ss(_mm_add_ss(_mm_mul_ss(ss(az), ss(az)), _mm_mul_ss(ss(ay), ss(ay))),
                                       _mm_mul_ss(ss(ax), ss(ax))));
        if (_mm_comigt_ss(ss(as), ss(1.f))) {
          volatile long double root = std::sqrt(static_cast<long double>(as));
          volatile long double k = 1.0L / root;
          volatile float rx = static_cast<float>(ax * k), ry = static_cast<float>(ay * k), rz = static_cast<float>(az * k);
          ax = rx; ay = ry; az = rz;
        }
        const float g = std::bit_cast<float>(0x3f666666u);
        float lx = fs(_mm_mul_ss(ss(f32(bank, difference_t + 0x28)), ss(g))),
              ly = fs(_mm_mul_ss(ss(f32(bank, difference_t + 0x2c)), ss(g))),
              lz = fs(_mm_mul_ss(ss(f32(bank, difference_t + 0x30)), ss(g)));
        const float ls = fs(_mm_add_ss(_mm_add_ss(_mm_mul_ss(ss(lz), ss(lz)), _mm_mul_ss(ss(ly), ss(ly))),
                                       _mm_mul_ss(ss(lx), ss(lx))));
        if (_mm_comigt_ss(ss(ls), ss(std::bit_cast<float>(0x3b23d70bu)))) {
          volatile long double root = std::sqrt(static_cast<long double>(ls));
          volatile long double k = static_cast<long double>(std::bit_cast<float>(0x3d4ccccdu)) / root;
          volatile float rx = static_cast<float>(lx * k), ry = static_cast<float>(ly * k), rz = static_cast<float>(lz * k);
          lx = rx; ly = ry; lz = rz;
        }
        expected.linear = {fs(_mm_add_ss(ss(current.linear.x), ss(lx))), fs(_mm_add_ss(ss(current.linear.y), ss(ly))),
                           fs(_mm_add_ss(ss(current.linear.z), ss(lz)))};
        expected.angular = {fs(_mm_add_ss(ss(angular_in.x), ss(ax))), fs(_mm_add_ss(ss(angular_in.y), ss(ay))),
                            fs(_mm_add_ss(ss(angular_in.z), ss(az)))};
      }
      const auto native = native_tu15_nudge_correction(bank, unit, desired_position, desired_forward, transform.up,
                                                       current.linear, angular_in, fp);
      check(same(native.linear, expected.linear) && same(native.angular, expected.angular),
            "Native B4F00 correction differs from the translated helper chain");
      ++corrections.calls;
    }

    // B9800 against its translated entry. Cases: moving velocity, no wake
    // (dead small vectors), linear only, angular only, both.
    {
      const unsigned which = tick % 5;
      const Vec3 linear = which == 1 ? Vec3{1e-3f, 0, 0} : current.linear + Vec3{.2f, -.1f, which == 4 ? .3f : 0.f};
      const Vec3 angular = which == 1 ? Vec3{0, 0, 0} : Vec3{0, 0, .25f};
      const bool with_linear = which != 3, with_angular = which >= 2 || which == 1;
      put(bank, scratch + 0x240, linear);
      put(bank, scratch + 0x250, angular);
      const auto component = u32(bank, u32(bank, u32(bank, u32(bank, 0x4ecbd0u) + 0x44) + (unit & 0xffffu) * 12u + 8) + 0xb4);
      if (component != 0xffffffffu) {
        const auto record = u32(bank, u32(bank, 0x52be5cu) + 0x44) + (component & 0xffffu) * 0xa0u;
        for (std::uint32_t i = 0; std::int32_t(i) < std::int32_t(u32(bank, record + 0x74)); ++i) {
          const auto body = u32(bank, u32(bank, record + 0x70) + i * 0x60 + 0x40);
          const auto island = u32(bank, body + 0x44);
          inactive_islands += island && bank.view(island + 0x29, 1)[0] == std::byte{0};
        }
      }
      const float lsq = linear.x * linear.x + linear.y * linear.y + linear.z * linear.z;
      wakes += (with_linear && lsq > 1e-4f) || (with_angular && angular.z * angular.z > 1e-4f);
      auto abi = live.prepare_source_call(std::array{with_linear ? scratch + 0x240u : 0u});
      abi.gp[6] = unit;
      abi.gp[0] = with_angular ? scratch + 0x250u : 0u;
      const auto low = abi.gp[4] - 0x10000u, high = abi.gp[4] + 8u;
      const auto refusal = native_tu15_object_velocities_unsupported(bank, unit, with_linear ? &linear : nullptr,
                                                                     with_angular ? &angular : nullptr);
      if (!refusal.empty()) {
        // Production skips this write. Show that the translated route would
        // have activated a deactivated island (and its listener), then undo.
        check(refusal.find("listener") != std::string::npos, "Unexpected B9800 refusal: " + refusal);
        Journal journal(bank);
        live.run_actual(0xb9800u, abi);
        journal.stop();
        bool activated = false;
        const auto record = u32(bank, u32(bank, 0x52be5cu) + 0x44) + (component & 0xffffu) * 0xa0u;
        for (std::uint32_t i = 0; std::int32_t(i) < std::int32_t(u32(bank, record + 0x74)); ++i) {
          const auto island = u32(bank, u32(bank, u32(bank, record + 0x70) + i * 0x60 + 0x40) + 0x44);
          for (const auto &[at, before] : journal.undo)
            activated |= at == island + 0x29 && before.size() == 1 && before[0] == std::byte{0} &&
                         bank.view(at, 1)[0] == std::byte{1};
        }
        for (auto it = journal.undo.rbegin(); it != journal.undo.rend(); ++it)
          bank.write(it->first, it->second);
        check(activated, "Refused B9800 did not reach island activation on the translated route");
        ++refused;
      } else
        compare_writes(bank, low, high, [&] { live.run_actual(0xb9800u, abi); },
                       [&] {
                         native_tu15_set_object_velocities(bank, unit, with_linear ? &linear : nullptr,
                                                           with_angular ? &angular : nullptr, fp);
                       },
                       "B9800", setter);
    }

    // B44C0 decision 2 (mode 2) composed of translated owners vs native.
    if (tick % 3 == 0) {
      const auto now = native_tu15_object_transform(bank, unit);
      const auto have = native_tu15_object_velocities(bank, unit);
      const Vec3 reported_position = now.position + Vec3{.05f, .02f, 0};
      const Vec3 reported_linear = tick % 2 ? have.linear + Vec3{.1f, 0, 0} : have.linear + Vec3{.6f, .2f, 0};
      if (native_tu15_motion_held(bank, unit, fp)) {
        ++held;
      } else {
        bool take = false;
        {
          Fpu fpu(fp);
          const Vec3 d = reported_linear - have.linear;
          const float sq = (d.z * d.z + d.x * d.x) + d.y * d.y;
          take = _mm_comigt_ss(_mm_set_ss(sq), _mm_set_ss(std::bit_cast<float>(0x3d75c28fu)));
        }
        dead_band += !take;
        // B4F00 reference outputs via the same verified correction, then
        // translated B9800 with the linear result only (angular absent).
        const auto out = native_tu15_nudge_correction(bank, unit, reported_position, now.forward, now.up,
                                                      take ? reported_linear : have.linear, have.angular, fp);
        if (!native_tu15_object_velocities_unsupported(bank, unit, take ? &out.linear : nullptr, nullptr).empty()) {
          ++refused;
          continue;
        }
        put(bank, scratch + 0x260, out.linear);
        auto abi = live.prepare_source_call(std::array{take ? scratch + 0x260u : 0u});
        abi.gp[6] = unit;
        abi.gp[0] = 0;
        const auto low = abi.gp[4] - 0x10000u, high = abi.gp[4] + 8u;
        compare_writes(bank, low, high, [&] { if (take) live.run_actual(0xb9800u, abi); },
                       [&] {
                         const bool wrote = native_tu15_nudge_object(bank, unit, 2, &reported_position, nullptr, nullptr,
                                                                     &reported_linear, nullptr, fp);
                         check(wrote == take, "Native B44C0 decision 2 disagreed about the velocity dead band");
                       },
                       "B44C0 decision 2", nudge);
      }
    }
  }
  std::cout << "object motion oracle: B9800 calls=" << setter.calls << " compared_bytes=" << setter.compared_bytes
            << " | B44C0-2 calls=" << nudge.calls << " compared_bytes=" << nudge.compared_bytes
            << " dead_band=" << dead_band << " held=" << held << " | B4F00 calls=" << corrections.calls
            << " | BC190 calls=" << transforms.calls << " BC200 calls=" << velocities.calls << " | wake_cases=" << wakes
            << " inactive_island_bodies=" << inactive_islands << " airborne_ticks=" << airborne
            << " crouched_ticks=" << crouched << " resting_ticks=" << resting << " refused_inactive_island=" << refused
            << '\n';
  check(setter.calls > 300 && setter.compared_bytes > 0, "B9800 oracle compared too few calls");
  check(nudge.calls > 50, "B44C0 decision 2 oracle compared too few calls");
  check(airborne > 0 && crouched > 0 && resting > 0, "Oracle movement coverage is incomplete");
  std::cout << "native TU1.5 object motion matches the translated route\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << "native TU1.5 object motion oracle failed: " << error.what() << '\n';
  return 1;
}
