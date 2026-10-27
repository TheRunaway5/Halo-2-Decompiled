// End-to-end non-host movement prediction over real UDP (NETCODE_ENHANCED_
// GOAL items 1 and 5). A prepared MCC Lockout TU1.5 world hosts through
// LanHost; a LanClient joins through a loopback proxy that delays every
// datagram by a whole number of 30 Hz ticks (100-150 ms round trip) and can
// drop a fixed share. The client predicts its own biped with the runtime's
// NonHostMovementClient; the host consumes inputs in order (with repeats)
// and adopts motion through the native TU1.5 setters. The fixture moves the
// host biped once by the test placement owner (a teleport) and once by a
// native velocity write (an impulse).
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_tu15_object_motion.hpp"
#include "halo2/nonhost_movement_client.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_player_profile.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value)
    throw FormatError(message);
}
struct Files {
  std::filesystem::path path;
  Files() {
    auto name = (std::filesystem::temp_directory_path() / "halo2-nonhost-lan-XXXXXX").string();
    check(::mkdtemp(name.data()) != nullptr, "LAN prediction scratch creation failed");
    path = name;
  }
  ~Files() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};
// Loopback proxy holding each datagram for `delay` pump calls (one per tick)
// and dropping every `drop_every`th client datagram (0: none).
struct DelayProxy {
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in self{}, host{}, client{};
  bool client_known{};
  unsigned delay_up{}, delay_down{}, drop_every{}, drop_down_every{};
  struct Held {
    unsigned due{};
    bool to_host{};
    std::vector<char> bytes;
  };
  std::deque<Held> held;
  unsigned now{}, client_packets{}, dropped_up{}, dropped_down{}, host_packets{};
  std::uint64_t client_bytes{};
  DelayProxy(unsigned host_port, unsigned up, unsigned down) : delay_up(up), delay_down(down) {
    check(fd >= 0, "Create proxy");
    self.sin_family = AF_INET;
    self.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(bind(fd, reinterpret_cast<sockaddr *>(&self), sizeof(self)) == 0, "Bind proxy");
    socklen_t size = sizeof(self);
    check(getsockname(fd, reinterpret_cast<sockaddr *>(&self), &size) == 0, "Read proxy address");
    host = self;
    host.sin_port = htons(host_port);
  }
  ~DelayProxy() { close(fd); }
  DelayProxy(const DelayProxy &) = delete;
  void pump() {
    std::array<char, 1501> bytes{};
    for (;;) {
      sockaddr_in from{};
      socklen_t size = sizeof(from);
      const auto n = recvfrom(fd, bytes.data(), bytes.size(), MSG_DONTWAIT, reinterpret_cast<sockaddr *>(&from), &size);
      if (n < 0)
        break;
      const bool from_host = from.sin_port == host.sin_port;
      if (!from_host) {
        client = from;
        client_known = true;
        ++client_packets;
        client_bytes += std::uint64_t(n);
        if (drop_every && client_packets % drop_every == 0) {
          ++dropped_up;
          continue;
        }
      } else {
        ++host_packets;
        if (drop_down_every && host_packets % drop_down_every == 0) {
          ++dropped_down;
          continue;
        }
      }
      held.push_back({now + (from_host ? delay_down : delay_up), !from_host,
                      std::vector<char>(bytes.begin(), bytes.begin() + n)});
    }
    for (auto at = held.begin(); at != held.end();) {
      if (at->due > now) {
        ++at;
        continue;
      }
      const auto &target = at->to_host ? host : client;
      if (at->to_host || client_known)
        sendto(fd, at->bytes.data(), at->bytes.size(), 0, reinterpret_cast<const sockaddr *>(&target), sizeof(target));
      at = held.erase(at);
    }
  }
  void tick() { ++now; }
};
struct Scenario {
  const char *name;
  unsigned up_ticks{}, down_ticks{}; // whole 30 Hz ticks each way
  unsigned drop_every{};             // client datagrams (0: none)
  unsigned drop_down_every{};        // host datagrams (0: none)
};
struct Outcome {
  unsigned input_tick{}, predicted_move_tick{}, snapshot_move_tick{};
  unsigned skipped_consumption{}, consumed_states{};
  float max_step{}, max_visible_correction{}, final_error{}, teleport_error{}, impulse_error{};
  unsigned max_correction_tick{}, visible_corrections{};
  float max_jump_correction{};
  unsigned max_jump_tick{};
  NonHostMovementClient::Stats stats;
  double upstream_bytes_per_packet{};
  unsigned cuts_before_teleport{}, cuts_after_teleport{};
};

// A flat authored BSP floor with every point within 2 wu clear for a
// standing capsule (fixture placement only; all measured motion is TU1.5's).
Vec3 clear_floor(const Cache &cache, const LevelContent &content) {
  const CollisionWorld terrain(content.collision);
  const auto clear_to = [&](Vec3 origin, float radius) {
    for (float x = -radius; x <= radius + 1e-3f; x += .25f)
      for (float y = -radius; y <= radius + 1e-3f; y += .25f) {
        const auto p = origin + Vec3{x, y, 0};
        const auto floor = terrain.raycast(p + Vec3{0, 0, .5f}, {0, 0, -1}, 1);
        if (!floor || floor->normal.z < .98f || std::abs(floor->position.z - origin.z) > .03f ||
            terrain.overlaps(p + Vec3{0, 0, .05f}, content.player.height * 1.3f, content.player.radius))
          return false;
      }
    return true;
  };
  std::optional<Vec3> best;
  float best_radius = 0;
  for (const auto &spawn : cache.spawns())
    for (int ox = -8; ox <= 8; ++ox)
      for (int oy = -8; oy <= 8; ++oy) {
        const auto anchor = spawn.position + Vec3{float(ox) * .5f, float(oy) * .5f, 0};
        const auto origin = terrain.raycast(anchor + Vec3{0, 0, .6f}, {0, 0, -1}, 1.5f);
        if (!origin || origin->normal.z < .98f)
          continue;
        for (const float radius : {2.f, 1.5f, 1.25f, 1.f})
          if (radius > best_radius && clear_to(origin->position, radius)) {
            best = origin->position;
            best_radius = radius;
            break;
          }
      }
  if (!best || best_radius < 1.f)
    throw FormatError("No clear authored floor for the LAN prediction course");
  std::cout << "course floor clear to " << best_radius << " wu\n";
  return *best;
}

Outcome run(const Cache &cache, const LevelContent &content, const Scenario &s) {
  std::cout << s.name << ": starting world\n";
  Files files;
  MultiplayerObjectRuntime world(cache, 0, files.path);
  UiVariant variant{"Slayer", 0, {{89, 5}}};
  auto rules = slayer_rules(variant, false);
  rules.weapon = cache.slayer_starting_weapon(rules.starting_weapon);
  rules.random_seed = 0x2b2b2bu;
  world.configure_session(source_game_variant(variant), 1, 30, rules.random_seed);
  MultiplayerMatch match(content, 0, cache.spawns(), rules, {}, {}, &world);
  check(match.source_engine_owned(), "LAN prediction did not select the original source match engine");
  // An idle local host player keeps the source match running before and
  // while the remote client joins (as the LAN player-collision fixture).
  auto host_configuration = source_default_player_configuration();
  host_configuration[0x44] = 2;
  const std::array<unsigned, 1> host_slots{match.join("LAN host", -1, host_configuration)};
  const std::vector<FirstPersonBindingDefinition> host_views{cache.load_first_person_binding(rules.weapon, 2)};
  match.configure_local_views(host_slots, host_views);
  LanHost host(match, 0, 0x2b2bu);
  DelayProxy proxy(host.port(), s.up_ticks, s.down_ticks);
  proxy.drop_every = s.drop_every;
  proxy.drop_down_every = s.drop_down_every;
  auto configuration = source_default_player_configuration();
  configuration[0x44] = 2;
  const std::array<SourcePlayerConfiguration, 1> profiles{configuration};
  LanClient client(host.join_address("127.0.0.1", ntohs(proxy.self.sin_port)), 0x2b2bu, 1, {}, profiles);
  std::array<Controls, multiplayer_capacity> controls{};
  std::array<Controls, 1> local{};
  const CollisionWorld client_collision(content.collision); // the client's static BSP collision
  const auto deadzone = cache.original_data() ? cache.original_data()->unit_locomotion_deadzone_bits : std::nullopt;
  std::unique_ptr<NonHostMovementClient> movement;
  const auto &biped = multiplayer_biped(content, 2);
  Outcome out;
  std::optional<std::uint32_t> last_consumed, last_tick;
  std::optional<Vec3> previous_shown, previous_offset;
  std::uint64_t previous_cuts = 0;
  const auto host_unit = [&](unsigned slot) { return match.player(slot).source_current_object; };
  unsigned slot = 0;
  bool joined = false;
  Vec3 start_position{};
  // 0..89 join/settle, 90 input starts, scripted course, 420 teleport,
  // 500 impulse, 560.. idle until 700.
  // Two course loops, then idle: the host fixtures teleport (420) and kick
  // (500) a standing biped, and convergence is measured standing.
  constexpr unsigned input_start = 90, idle_from = 390, teleport_tick = 420, impulse_tick = 500, ticks = 640;
  unsigned teleported = 0, impulsed = 0;
  bool placed = false;
  const auto course_floor = clear_floor(cache, content);
  const char *stage = "start";
  unsigned at_tick = 0;
  try {
  for (unsigned t = 0; t < ticks; ++t) {
    at_tick = t;
    stage = "client";
    std::this_thread::sleep_for(std::chrono::duration<float>(tick_seconds));
    proxy.pump();
    client.receive();
    check(client.error().empty(), std::string(s.name) + ": client error " + client.error());
    if (client.ready() && !joined) {
      joined = true;
      std::cout << s.name << ": joined at tick " << t << '\n';
      slot = client.slots().front();
      local[0].yaw = client.state().players[slot].yaw;
      local[0].pitch = client.state().players[slot].pitch;
      movement = std::make_unique<NonHostMovementClient>(client.state().netcode_mode, content.movement, deadzone);
    }
    if (joined) {
      const auto states = client.received_states();
      const auto receipts = client.received_inputs();
      check(states.size() == receipts.size(), "Every received state carries an input receipt");
      for (std::size_t i = 0; i < states.size(); ++i) {
        movement->reconcile(0, states[i].players[slot], receipts[i], client_collision);
        if (receipts[i].known && last_consumed && last_tick && states[i].tick == *last_tick + 1 &&
            t > input_start + 10) {
          ++out.consumed_states;
          if (receipts[i].consumed != *last_consumed + 1 && receipts[i].consumed != *last_consumed)
            out.skipped_consumption += receipts[i].consumed - *last_consumed - 1;
        }
        if (receipts[i].known)
          last_consumed = receipts[i].consumed;
        last_tick = states[i].tick;
      }
      // Scripted input.
      auto &c = local[0];
      c.forward = c.right = 0;
      c.jump = c.crouch = false;
      // A course that stays within about 2 wu of a surveyed clear floor:
      // runs, strafes, diagonals, a turning run, jumps and crouch-walking,
      // each followed by its return leg.
      // A short walk keeps the Havok island awake for the impulse fixture.
      if (t >= impulse_tick - 15 && t < impulse_tick + 2)
        c.forward = .5f;
      if (t >= input_start && t < idle_from) {
        const unsigned k = (t - input_start) % 150;
        if (k < 12)
          c.forward = 1;
        else if (k < 24)
          c.forward = -1;
        else if (k < 36)
          c.right = 1;
        else if (k < 48)
          c.right = -1;
        else if (k < 60)
          c.forward = c.right = .7f;
        else if (k < 72)
          c.forward = c.right = -.7f;
        else if (k < 107) {
          c.forward = 1;
          c.yaw += .08f; // a running circle (about 0.9 wu radius, 140 deg/s)
        } else if (k < 125) {
          c.jump = k == 108 || k == 122;
        } else if (k < 137) {
          c.forward = .8f;
          c.crouch = true;
        } else {
          c.forward = -.8f;
          c.crouch = true;
        }
      }
      if (t == input_start)
        out.input_tick = t;
      const auto &latest = client.state().players[slot];
      float yaw = c.yaw, pitch = c.pitch;
      if (const auto consumed = client.consumed_look(0)) {
        yaw = std::remainder(latest.yaw + std::remainder(c.yaw - (*consumed)[0], 6.283185307f), 6.283185307f);
        pitch = std::clamp(latest.pitch + (c.pitch - (*consumed)[1]), -1.54f, 1.54f);
      }
      client.set_motion(0, movement->predict(0, client.next_input_sequence(), c, yaw, pitch, latest, biped,
                                             client_collision));
      if (const auto shown = movement->presented(0, 1.f)) {
        if (t == input_start - 1)
          start_position = *shown;
        if (!out.predicted_move_tick && t >= input_start &&
            std::hypot(shown->x - start_position.x, shown->y - start_position.y) > 1e-3f)
          out.predicted_move_tick = t;
        // Normal movement only: the host-forced teleport and impulse are
        // excluded for the round trip that delivers them.
        // Host-forced events (a teleport cut, an impulse's flight) and jumps
        // (known airborne/landing mispredictions) are bounded by the blend
        // check separately; the strict bound covers ground movement.
        const bool host_event = (teleported && t >= teleported && t <= teleported + 20) ||
                                (impulsed && t >= impulsed && t <= impulsed + 60);
        // Jumps and crouch transitions (standing up included) are the
        // trajectory oracle's known mispredictions; ground runs, strafes,
        // diagonals and turns carry the strict bound.
        const unsigned course = t >= input_start ? (t - input_start) % 150 : 0;
        const bool jumping = t >= input_start && t < idle_from + 12 &&
                             (course >= 107 || (t >= input_start + 150 && course < 10) || t >= idle_from);
        if (previous_shown && t > input_start && !host_event)
          out.max_step = std::max(out.max_step, length(*shown - *previous_shown));
        previous_shown = shown;
        // Visible correction: the shown offset's change this tick (the part
        // of the shown motion that is not the predicted biped's own move).
        const auto offset = movement->predictor(0)->position_offset();
        const auto cuts = movement->stats(0).cuts;
        if (previous_offset && t > input_start && !host_event && cuts == previous_cuts) {
          const auto shown_correction = length(offset - *previous_offset);
          if (jumping) { // jump or crouch
            if (shown_correction > out.max_jump_correction) {
              out.max_jump_correction = shown_correction;
              out.max_jump_tick = t - input_start;
            }
          }
          else if (shown_correction > out.max_visible_correction) {
            out.max_visible_correction = shown_correction;
            out.max_correction_tick = t - input_start;
          }
          if (shown_correction > .01f)
            ++out.visible_corrections;
        }
        previous_offset = offset;
        previous_cuts = cuts;
      }
      if (!out.snapshot_move_tick && t >= input_start &&
          std::hypot(latest.position.x - start_position.x, latest.position.y - start_position.y) > 1e-3f)
        out.snapshot_move_tick = t;
    }
    stage = "send";
    client.send(local);
    proxy.pump();
    stage = "host receive";
    host.receive(controls);
    stage = "host step";
    match.step(controls);
    stage = "fixture";
    const auto unit = host_unit(slot);
    const bool alive = joined && unit != null_datum && !match.unit(slot).vitality.dead &&
                       world.biped_motion(unit).physical_state == 1;
    if (alive && !placed && t >= 30) {
      // Fixture placement onto the clear floor before the course starts.
      world.tu15_world().set_object_position(unit, course_floor + Vec3{0, 0, .05f});
      placed = true;
    }
    if (alive && t >= teleport_tick && !teleported) {
      out.cuts_before_teleport = unsigned(movement->stats(0).cuts);
      // Test placement through the existing B95D0 fixture owner, onto the
      // authored spawn farthest from the biped (always a real floor).
      const auto here = world.biped_motion(unit).position;
      Vec3 target = here;
      for (const auto &spawn : cache.spawns())
        if (length(spawn.position - here) > length(target - here))
          target = spawn.position;
      world.tu15_world().set_object_position(unit, target + Vec3{0, 0, .05f});
      teleported = t;
    }
    if (alive && t >= impulse_tick && !impulsed &&
        native_tu15_object_velocities_unsupported(world.memory(), unit, nullptr, nullptr).empty()) {
      const Vec3 kick{0, 6.f, 1.5f};
      const auto frame = world.tu15_world().prepare_source_call();
      native_tu15_set_object_velocities(world.memory(), unit, &kick, nullptr, *frame.floating_point);
      impulsed = t;
    }
    stage = "host broadcast";
    host.broadcast();
    proxy.pump();
    proxy.tick();
    stage = "measure";
    if (teleported && t == teleported + 40) {
      out.cuts_after_teleport = unsigned(movement->stats(0).cuts);
      if (const auto shown = movement->presented(0, 1.f))
        out.teleport_error = length(*shown - world.biped_motion(host_unit(slot)).position);
    }
    if (impulsed && t == impulsed + 40)
      if (const auto shown = movement->presented(0, 1.f))
        out.impulse_error = length(*shown - world.biped_motion(host_unit(slot)).position);
  }
  } catch (const std::exception &error) {
    throw FormatError(std::string(s.name) + ": " + stage + " at tick " + std::to_string(at_tick) + ": " + error.what());
  }
  check(joined, std::string(s.name) + ": client never joined");
  check(teleported && impulsed, std::string(s.name) + ": the host fixtures never found a living biped");
  if (const auto shown = movement->presented(0, 1.f))
    out.final_error = length(*shown - world.biped_motion(host_unit(slot)).position);
  out.stats = movement->stats(0);
  out.upstream_bytes_per_packet = proxy.client_packets ? double(proxy.client_bytes) / proxy.client_packets : 0;
  return out;
}
} // namespace

int main(int argc, char **argv) try {
  std::cout << std::unitbuf;
  if (argc != 2)
    throw FormatError("Supply a prepared MCC Lockout map");
  if (!std::filesystem::is_regular_file(argv[1]))
    return 77;
  const Cache cache(argv[1]);
  check(cache.is_mcc(), "LAN prediction requires prepared MCC assets");
  const auto content = cache.load_content(0, true);
  // Round trips of 3 and 4 ticks (100 and 133 ms), the second with 10% of
  // the datagrams lost in each direction.
  const std::array scenarios{Scenario{"rtt_100ms", 1, 2, 0, 0}, Scenario{"rtt_133ms_loss10", 2, 2, 10, 10}};
  for (const auto &s : scenarios) {
    const auto o = run(cache, content, s);
    std::cout << s.name << ": input_tick=" << o.input_tick << " predicted_move=+" << (o.predicted_move_tick - o.input_tick)
              << " snapshot_move=+" << (o.snapshot_move_tick - o.input_tick) << " predicted=" << o.stats.predicted
              << " reconciled=" << o.stats.reconciled << " trusted=" << o.stats.trusted << " replayed=" << o.stats.replayed
              << " forced=" << o.stats.forced << " cuts=" << o.stats.cuts << " (teleport " << o.cuts_before_teleport << "->"
              << o.cuts_after_teleport << ")" << " max_error=" << o.stats.max_error << " max_step=" << o.max_step
              << " max_visible_correction=" << o.max_visible_correction << " (course tick " << o.max_correction_tick
              << ", " << o.visible_corrections << " ticks over 0.01) jump_crouch_correction=" << o.max_jump_correction << " (course tick " << o.max_jump_tick << ")"
              << " teleport_err=" << o.teleport_error << " impulse_err=" << o.impulse_error
              << " final_err=" << o.final_error << " skipped_inputs=" << o.skipped_consumption << "/"
              << o.consumed_states << " upstream_bytes/packet=" << o.upstream_bytes_per_packet << '\n';
    const std::string name = s.name;
    check(o.predicted_move_tick == o.input_tick, name + ": own position did not respond on the input tick");
    check(o.snapshot_move_tick >= o.input_tick + 3, name + ": snapshot path unexpectedly fast");
    check(o.stats.reconciled > 300, name + ": too few reconciliations");
    check(o.cuts_after_teleport > o.cuts_before_teleport, name + ": teleport was not cut");
    check(o.teleport_error < .25f, name + ": did not converge after the teleport");
    check(o.stats.forced > 0, name + ": impulse was not reported as forced motion");
    check(o.impulse_error < .25f, name + ": did not converge after the impulse");
    check(o.final_error < .2f, name + ": did not converge to the host");
    // No snaps in normal movement: at most 3 cm of correction shown per tick.
    // Lossless: at most 0.02 wu shown per tick. With 10% loss each way a
    // lost snapshot lengthens the replay window (timing-dependent; up to
    // 0.039 wu was seen on a loaded machine), so that case allows 0.05.
    check(o.max_visible_correction < (s.drop_every || s.drop_down_every ? .05f : .02f),
          name + ": shown position snapped in normal ground movement");
    check(o.max_jump_correction < .1f, name + ": a jump or crouch correction snapped");
    check(o.skipped_consumption == 0, name + ": the host skipped an input (redundancy lost one)");
  }
  std::cout << "nonhost LAN prediction: all scenarios passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << "nonhost LAN prediction failed: " << error.what() << '\n';
  return 1;
}
