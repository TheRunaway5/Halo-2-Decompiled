#include "halo2/cache.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/player_command.hpp"
#include "halo2/transform.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#include <netpacket/packet.h>
#include <stdexcept>
#include <unistd.h>
using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
struct Directory {
  std::filesystem::path path;
  Directory() {
    auto name =
        (std::filesystem::temp_directory_path() / "halo2-campaign-kill-XXXXXX")
            .string();
    check(::mkdtemp(name.data()), "Native save directory unavailable");
    path = name;
  }
  ~Directory() {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }
};
std::uint32_t word(const SourceRuntimeMemory &bank, std::uint32_t p,
                   unsigned n = 4) {
  const auto bytes = bank.view(p, n);
  std::uint32_t result{};
  for (unsigned i = 0; i < n; ++i)
    result |= std::to_integer<std::uint32_t>(bytes[i]) << (8 * i);
  return result;
}
} // namespace
int main(int argc, char **argv) try {
  std::cout << std::unitbuf;
  check(argc == 3, "Supply actual TU1.5 image and campaign map");
  Cache cache(argv[2]);
  Directory files;
  PhysicsLiveWorldTu15 world(cache, argv[1], files.path);
  world.initialize_map();
  world.configure_campaign_session(1, 1, 30, 0x12345678);
  auto &bank = world.memory();
  const auto pvs = word(bank, 0x5020a8);
  check(pvs && word(bank, pvs + 1, 1) == 1,
        "Complete original campaign AI new-map owner has not published readiness");
  std::cout << "PASS complete original campaign AI cold/new-map readiness\n";
  const auto options = word(bank, 0x4f3218);
  check(word(bank, options + 8) == 1 && word(bank, options + 0x132, 2) == 1,
        "Campaign kind/difficulty delivery differs");
  const auto scenario = word(bank, 0x4ecc20),
             count = word(bank, scenario + 0x230),
             kills = word(bank, scenario + 0x234),
             volumes = word(bank, scenario + 0x10c);
  check(count > 0, "Campaign map has no authored kill volume for this probe");
  std::int16_t trigger = -1;
  std::uint32_t volume{};
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto index = std::bit_cast<std::int16_t>(
        std::uint16_t(word(bank, kills + i * 2, 2)));
    if (index < 0)
      continue;
    const auto row = volumes + std::uint32_t(index) * 0x44;
    if (word(bank, row + 4, 2) == 0xffff) {
      trigger = index;
      volume = row;
      break;
    }
  }
  check(trigger >= 0,
        "No unattached authored kill volume for the declared placement");
  const auto vector = [&](unsigned offset) {
    return Vec3{std::bit_cast<float>(word(bank, volume + offset)),
                std::bit_cast<float>(word(bank, volume + offset + 4)),
                std::bit_cast<float>(word(bank, volume + offset + 8))};
  };
  const auto forward = vector(0xc), up = vector(0x18), origin = vector(0x24),
             extent = vector(0x30);
  const auto right = cross(up, forward);
  const auto position =
      origin + (forward * extent.x + right * extent.y + up * extent.z) * .5f;
  const auto player = std::ranges::find_if(cache.tags(), [](const auto &tag) {
    return tag.group == "bipd" &&
           tag.name == "objects\\characters\\masterchief\\masterchief";
  });
  check(player != cache.tags().end(),
        "Actual campaign player definition unavailable");
  std::cout << "Creating actual campaign player at an authored BSP0 start\n";
  check(!cache.spawns().empty(), "Authored player starts unavailable");
  const auto spawn = std::ranges::find_if(
      cache.spawns(), [](const auto &s) { return s.bsp == 0; });
  check(spawn != cache.spawns().end(), "No authored BSP0 player start");
  const auto object =
      world.create_biped(player->datum, spawn->position, {1, 0, 0});
  check(object != 0xffffffff, "Original biped birth returned NONE");

  SourceMachineIdentity machine{};
  ifaddrs *interfaces{};
  check(!::getifaddrs(&interfaces), "Native interface inventory failed");
  bool found = false;
  for (auto *at = interfaces; at; at = at->ifa_next)
    if (at->ifa_addr && at->ifa_addr->sa_family == AF_PACKET &&
        (at->ifa_flags & IFF_UP) && !(at->ifa_flags & IFF_LOOPBACK)) {
      auto *link = reinterpret_cast<sockaddr_ll *>(at->ifa_addr);
      if (link->sll_halen == machine.size()) {
        std::copy_n(link->sll_addr, machine.size(), machine.begin());
        found = true;
        break;
      }
    }
  ::freeifaddrs(interfaces);
  check(found, "Actual native identity unavailable");
  DatumPool players(bank, world.player_pool());
  const auto identity = players.allocate_at(0);
  check(identity != DatumPool::none, "Actual player allocation failed");
  world.construct_player(identity, 0, source_default_player_configuration(),
                         machine);
  world.bind_player(identity, object);
  world.set_local_player(identity, 0);
  std::array<std::byte, 0x5c> neutral;
  initialize_source_neutral_command(neutral);
  for (unsigned i = 0; i < 10; ++i) {
    world.deliver_command(identity, object, neutral);
    world.step_objects();
  }
  const auto initial_body = world.source_object_body(object);
  std::cout << "After ten actual campaign player ticks component=" << std::hex
            << word(bank, initial_body + 0xb4) << std::dec << '\n';
  // Declared diagnostic position input through the original owner; no body,
  // component, health or death fields are assigned by the test.
  world.set_object_position(object, position);
  std::cout << "Original position owner delivered declared authored-volume "
               "point; component="
            << std::hex << word(bank, initial_body + 0xb4) << std::dec << '\n';
  std::cout << "Actual source biped " << std::hex << object << std::dec
            << "; testing original containment\n";
  // This probe intentionally uses a map with one kill row, so the source clock
  // selects it on every query. Multiple-row scheduling has separate original
  // fixtures.
  check(count == 1,
        "This source-world probe requires the declared single-row Cairo map");
  check(world.object_in_kill_volume(object),
        "Original object pose is outside the declared authored volume");
  world.set_kill_volume_enabled(trigger, false);
  check(!world.object_in_kill_volume(object),
        "Disabled source volume still admits the body");
  auto saved = world.authority_checkpoint();
  world.set_kill_volume_enabled(trigger, true);
  check(world.object_in_kill_volume(object),
        "Re-enabled source volume lost actual containment");
  auto prepared = world.prepare_authority(saved);
  check(world.commit_authority(std::move(prepared)),
        "Actual source authority publication failed");
  check(!world.object_in_kill_volume(object),
        "Checkpoint lost actual kill-volume state");
  world.set_kill_volume_enabled(-1, true);
  world.set_kill_volume_enabled(trigger, true);
  check(world.object_in_kill_volume(object),
        "Restored owner cannot re-enable its actual state");
  std::cout
      << "PASS actual campaign options, biped/geometry containment, "
         "disabled-bit control and complete world checkpoint restoration\n";
  const auto body = world.source_object_body(object);
  check(std::bit_cast<float>(word(bank, body + 0xec)) > 0,
        "Actual source biped begins dead");
  world.deliver_command(identity, object, neutral);
  world.step_objects();
  check(std::bit_cast<float>(word(bank, body + 0xec)) <= 0 ||
            (word(bank, body + 0x10a, 2) & 4),
        "Original biped update did not apply its authored fatal damage");
  std::cout << "PASS ordinary campaign world tick applied original kill-volume "
               "damage without assigned health/death state\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
