#include "halo2/bsp_atmosphere.hpp"
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
#include <string>
#include <unistd.h>
#include <utility>
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
        (std::filesystem::temp_directory_path() / "halo2-campaign-ai-XXXXXX")
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
  check(argc >= 3 && argc <= 5, "Supply actual TU1.5 image, campaign map, "
                                "optional placement mode and tick count");
  const std::string mode = argc >= 4 ? argv[3] : "counted";
  check(mode == "counted" || mode == "authored" || mode == "starting-location",
        "Unknown original placement mode");
  const auto ticks = argc == 5 ? std::stoi(argv[4]) : 1;
  check(ticks >= 1 && ticks <= 3,
        "This bounded fixture admits one to three actual ticks");
  Cache cache(argv[2]);
  Directory files;
  PhysicsLiveWorldTu15 world(cache, argv[1], files.path);
  world.initialize_map();
  world.configure_campaign_session(1, 1, 30, 0x12345678);
  auto &bank = world.memory();
  const auto pvs = word(bank, 0x5020a8);
  check(pvs && word(bank, pvs + 1, 1) == 1,
        "Complete original campaign AI new-map owner has not published "
        "readiness");
  std::cout << "PASS complete original campaign AI cold/new-map readiness\n";
  const auto options = word(bank, 0x4f3218);
  check(word(bank, options + 8) == 1 && word(bank, options + 0x132, 2) == 1,
        "Campaign kind/difficulty delivery differs");
  const auto scenario = word(bank, 0x4ecc20);
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
  std::uint32_t selected = 0xffffffff;
  const auto squad_count = word(bank, scenario + 0x160);
  const auto squads = word(bank, scenario + 0x164);
  const auto atmosphere = cache.load_bsp_atmosphere(0);
  for (std::uint32_t i = 0; i < squad_count; ++i) {
    const auto row = squads + i * 0x74;
    const auto starts = word(bank, row + 0x48), rows = word(bank, row + 0x4c);
    if (!starts || std::int16_t(word(bank, row + 0x2c, 2)) <= 0)
      continue;
    bool admitted = true;
    for (std::uint32_t j = 0; j < starts; ++j) {
      const auto s = rows + j * 0x64;
      const Vec3 position{std::bit_cast<float>(word(bank, s + 4)),
                          std::bit_cast<float>(word(bank, s + 8)),
                          std::bit_cast<float>(word(bank, s + 12))};
      if (word(bank, s + 0x10, 2) != 0xffff ||
          atmosphere.cluster_at(position) < 0)
        admitted = false;
    }
    if (admitted) {
      selected = i;
      break;
    }
  }
  check(selected != 0xffffffff, "No admitted authored ordinary squad in BSP0");
  const auto name = bank.view(squads + selected * 0x74, 32);
  std::cout << "Declared original ai_place mode=" << mode
            << " squad=" << selected
            << " name=" << reinterpret_cast<const char *>(name.data()) << '\n';
  DatumPool actors(bank, word(bank, 0x5020c8));
  const auto before = actors.count();
  const auto pending = word(bank, pvs + 0x364);
  world.place_ai(0xffffffffu, 1);
  world.place_ai(0xffffffffu);
  check(actors.count() == before && word(bank, pvs + 0x364) == pending,
        "NONE placement altered actors or the pending AI reference");
  for (const auto requested : std::array<std::int16_t, 3>{0, -1, -32768}) {
    world.place_ai(selected, requested);
    check(actors.count() == before && word(bank, pvs + 0x364) == selected,
          "Nonpositive placement lost its original pending-reference write");
  }
  world.place_ai(0x40000000u | selected, 1);
  check(
      actors.count() == before &&
          word(bank, pvs + 0x364) == (0x40000000u | selected),
      "Counted packed reference changed actors or lost its pending reference");
  if (mode == "counted")
    world.place_ai(selected, 1);
  else if (mode == "authored")
    world.place_ai(selected);
  else {
    check(selected < 0x4000,
          "Selected squad cannot fit the original starting-location identity");
    world.place_ai(0xc0000000u | (selected << 16));
  }
  check(word(bank, pvs + 0x364) == selected,
        "Actual placement lost its original pending reference");
  check(actors.count() > before,
        "Complete original placement did not create an actor");
  std::cout << "PASS complete original " << mode
            << " placement created actual actor bank count=" << actors.count()
            << "\n";
  for (int i = 0; i < actors.high_water(); ++i) {
    const auto rows = word(bank, word(bank, 0x5020c8) + 0x44);
    const auto record = rows + std::uint32_t(i) * 0x888;
    const auto salt = word(bank, record, 2);
    if (!salt)
      continue;
    const auto actor = (salt << 16) | std::uint32_t(i);
    const auto unit = word(bank, record + 0x18);
    check(unit != 0xffffffff &&
              word(bank, world.source_object_body(unit) + 0x12c) == actor,
          "Actual actor/unit binding differs");
  }
  DatumPool commands(bank, word(bank, 0x50eee0));
  DatumPool threads(bank, word(bank, 0x505e5c));
  check(commands.stride() == 0xd4 && threads.stride() == 0x418,
        "Actual command/thread pool shape differs");
  const std::array counts{actors.count(), commands.count(), threads.count()};
  check(counts[0] > 0 && counts[1] > 0 && counts[2] > 0,
        "Authored actor command or real HS thread missing");
  const auto assert_script_binding = [&] {
    const auto actor_pool = word(bank, 0x5020c8),
               rows = word(bank, actor_pool + 0x44);
    bool linked = false;
    for (int i = 0; i < actors.high_water(); ++i) {
      const auto record = rows + std::uint32_t(i) * 0x888;
      if (!word(bank, record, 2))
        continue;
      const auto command = word(bank, record + 0x858);
      if (command == 0xffffffff)
        continue;
      const auto command_record = std::as_const(commands).lookup(command);
      check(!command_record.empty(), "Actual actor command binding is stale");
      const auto thread = word(bank, word(bank, word(bank, 0x50eee0) + 0x44) +
                                         (command & 65535) * 0xd4 + 0x38);
      check(!std::as_const(threads).lookup(thread).empty(),
            "Actual command HS thread binding is stale");
      linked = true;
    }
    check(linked,
          "No authored starting-command thread attached to actual actor");
  };
  assert_script_binding();
  std::cout
      << "Actual actor/command/thread links validated before checkpoint\n";
  const auto actor_binding = actors.source_binding(),
             command_binding = commands.source_binding(),
             thread_binding = threads.source_binding(),
             player_binding = players.source_binding();
  const auto saved = world.authority_checkpoint();
  auto prepared = world.prepare_authority(saved);
  auto staged_actors =
      actors.prepare_source_view(actor_binding, prepared.replacement_memory());
  auto staged_commands = commands.prepare_source_view(
      command_binding, prepared.replacement_memory());
  auto staged_threads = threads.prepare_source_view(
      thread_binding, prepared.replacement_memory());
  auto staged_players = players.prepare_source_view(
      player_binding, prepared.replacement_memory());
  check(world.commit_authority(std::move(prepared)),
        "Actual populated AI/HS checkpoint publication failed");
  check(
      actors.commit_source_view(std::move(staged_actors)) &&
          commands.commit_source_view(std::move(staged_commands)) &&
          threads.commit_source_view(std::move(staged_threads)) &&
          players.commit_source_view(std::move(staged_players)),
      "Populated checkpoint pool views did not publish with their actual bank");
  check(actors.count() == counts[0] && commands.count() == counts[1] &&
            threads.count() == counts[2],
        "Checkpoint lost actual actor/command/thread pools");
  assert_script_binding();
  std::cout << "PASS actual actor, command and HS thread binding through "
               "complete authority checkpoint restoration\n";
  for (int i = 0; i < ticks; ++i) {
    world.deliver_command(identity, object, neutral);
    world.step_objects();
    std::cout << "PASS ordinary original campaign AI/object tick " << i + 1
              << '\n';
  }
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
