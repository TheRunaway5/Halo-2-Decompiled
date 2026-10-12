#include "halo2/math.hpp"
#include "halo2/multiplayer_network.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
using namespace halo2;
namespace {
void check(bool b, const std::string &message) {
  if (!b)
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
LevelContent arena() {
  LevelContent c;
  c.player.datum = 1;
  c.player.height = 1;
  c.player.crouch_height = .6f;
  c.player.radius = .2f;
  c.player.camera_height = .8f;
  c.player.crouch_camera_height = .5f;
  c.player.crouch_velocity = 2;
  c.player.jump_velocity = 3;
  c.player.vitality.health = 100;
  c.collision = {{{-100, -100, 0}, {100, -100, 0}, {100, 100, 0}, 0, 0},
                 {{-100, -100, 0}, {100, 100, 0}, {-100, 100, 0}, 0, 0}};
  c.movement = {2, 2, 2, 10, 1, 1, 1, 5, 2, 1};
  WeaponDefinition w;
  w.datum = 3;
  w.instantaneous = true;
  w.capacity = 20;
  w.initial_rounds = 120;
  w.reserve = 100;
  w.reload_rounds = 20;
  w.reload_time = 1;
  w.rounds_per_shot = 1;
  w.minimum_rounds = 1;
  w.rounds_per_second = 30;
  w.burst = 1;
  w.range = 50;
  // Keep this lifecycle fixture lethal through original spawn-time scaling.
  w.damage_min = 4000;
  w.projectiles_per_shot = 1;
  c.weapons.emplace(3, w);
  return c;
}
std::vector<Spawn> starts() {
  Spawn a;
  a.position = {0, 0, 0};
  a.facing = 0;
  a.player_type = 0;
  Spawn b = a;
  b.position = {4, 0, 0};
  b.facing = 3.14159265f;
  return {a, b};
}
SlayerRules rules() {
  auto r = slayer_rules({"Slayer", 0, {}});
  r.weapon = 3;
  return r;
}
void rules_and_match() {
  auto r = rules();
  check(slayer_rules({"Score Alias",0,{{1,7}}}).score_limit==7 &&
            slayer_rules({"Score Alias",0,{{2,7},{9,3}}}).score_limit==3,
        "Source score aliases did not select their shared variant field");
  check(r.score_limit == 25 && r.time_ticks == 14400 &&
            r.respawn_ticks == 150 && r.suicide_ticks == 150 && r.suicide_loss,
        "Original Slayer defaults");
  for(const int lives:{0,1,2,32767})
    check(slayer_rules({"Configured Lives",0,{{16,lives}}}).life_limit==lives,
        "Original signed life setting was rejected or substituted");
  rejects([] {slayer_rules({"Invalid Lives",0,{{16,-1}}});},"Negative source-invalid life count accepted");
  rejects([] {slayer_rules({"Invalid Lives",0,{{16,32768}}});},"Overflowing source-invalid life count accepted");
  rejects([] {slayer_rules({"Later Rounds",0,{{16,2},{0,5}}});},"Unbound multi-round reset admitted");
  rejects([] { slayer_rules({"CTF", 1, {}}); }, "Unsupported engine accepted");
  check(slayer_rules({"Limit one",0,{{2,1}}}).score_limit==1,
          "Recovered win-stat provider did not admit source limit one");
  rejects([] { slayer_rules({"Team Slayer", 0, {{0x5b, 1}}}); },
          "Unsupported teams accepted");
  rejects([] { slayer_rules({"Rockets", 0, {{0x59, 7}}}); },
          "Unsupported weapon option ignored");
  check(slayer_rules({"SMG Slayer", 0, {{0x59, 5}}}).starting_weapon == 5,
        "Original SMG variant option rejected");
  rejects([] { slayer_rules({"Random", 0, {{0x59, 2}}}); },
          "Random weapon option silently substituted");
  check(!slayer_rules({"No Grenades", 0, {{0x58, 0}}}).starting_grenades,
        "Source starting-grenades option was not applied");
  check(slayer_rules({"Nonzero Grenades", 0, {{0x58, 2}}}).starting_grenades,
        "Source bit setter's nonzero boolean was rejected before serialization");
  auto s = starts()[0];
  check(slayer_spawn(s, 0, true),
        "Campaign type zero must not reject multiplayer starts");
  s.game_types = {0, 1, 3, 4};
  check(slayer_spawn(s, 0, true), "Legacy player-start selectors rejected");
  s.game_types = {2, 0, 0, 0};
  s.spawn_types = {2, 0, 0, 0};
  check(slayer_spawn(s, 0, true) && slayer_spawn(s, 0, false),
        "Legacy initial/respawn selector filtered player starts");
  s.bsp = 1;
  check(slayer_spawn(s, 0, true) && slayer_spawn(s, 0, false),
        "Authored BSP metadata filtered a player start");
  auto c = arena();
  auto spawn = starts();
  // All 121 authored Dune starts have zero selectors. This complete
  // initial join/death/respawn flow must work with that source metadata.
  for(auto &start:spawn) {
    start.game_types.fill(0);
    start.spawn_types.fill(0);
  }
  r.respawn_ticks = 30;
  r.time_ticks = 0;
  r.score_limit = 2;
  MultiplayerMatch m(c, 0, spawn, r);
  auto a = m.join("A"), b = m.join("B");
  check(a == 0 && b == 1 &&
            length(m.unit(a).body.position - m.unit(b).body.position) > 3,
        "Distinct player slots/spawns");
  std::array<Controls, 16> inputs{};
  inputs[a].yaw = 0;
  inputs[b].yaw = 3.14159265f;
  inputs[b].fire = true;
  m.step(inputs);
  inputs[b].fire = false;
  check(m.unit(a).vitality.dead && m.player(b).kills == 1 &&
            m.player(b).score == 1 && m.player(a).deaths == 1,
        "Remote-controlled slot must damage primary and score once");
  for (unsigned i = 0; i < 29; ++i)
    m.step(inputs);
  check(m.unit(a).vitality.dead && m.player(a).deaths == 1,
        "Death double-counted or respawn too early");
  m.step(inputs);
  check(m.unit(a).vitality.dead && m.player(a).respawn.countdown == 0,
        "Zero countdown must wait for the original player-slot phase");
  m.step(inputs);
  check(m.unit(a).vitality.dead, "Respawn ran before the early slot-zero phase");
  m.step(inputs);
  check(!m.unit(a).vitality.dead && m.unit(a).weapon.loaded == 20,
        "Respawn must refill authored loadout after countdown");
  inputs[a].fire = true;
  m.step(inputs);
  check(m.unit(b).vitality.dead && m.player(a).score == 1,
        "Primary must damage controlled remote slot");
  inputs[a].fire = false;
  while (m.unit(b).vitality.dead)
    m.step(inputs);
  inputs[a].fire = true;
  m.step(inputs);
  check(m.finished() && m.state().winner == 0 && m.player(a).score == 2,
        "Score limit did not finish match");
  const auto tick = m.state().tick;
  m.step(inputs);
  check(m.state().tick == tick+1 && m.state().game_over_countdown==208 &&
      m.player(a).score==2 && m.state().players[a].total_score==2,
      "Ended world tick froze or changed finalized scores");
  r = rules();
  r.time_ticks = 30;
  MultiplayerMatch timed(c, 0, spawn, r);
  timed.join("A");
  timed.join("B");
  inputs = {};
  for (unsigned i = 0; i < 30; ++i)
    timed.step(inputs);
  check(!timed.finished(), "Time expiry used the advanced world clock");
  timed.step(inputs);
  check(timed.finished() && timed.state().winner == -1, "Tied timed match");
  r = rules();
  r.maximum_players = 2;
  MultiplayerMatch full(c, 0, spawn, r);
  full.join("A");
  full.join("B");
  rejects([&] { full.join("C"); }, "Capacity ignored");
  full.leave(0);
  check(full.join("C") == 0, "Released slot not reusable");
}
void asset_identity() {
  const auto root = std::filesystem::temp_directory_path() /
      ("halo2-lan-data-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
  } cleanup{root};
  const auto a = root / "a", b = root / "b";
  auto write = [](const std::filesystem::path &path, const std::string &value) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << value;
    if (!file) throw std::runtime_error("Could not write disposable asset fixture");
  };
  write(a / "match.map", "local map fixture");
  write(a / "default.xbe", "executable data fixture");
  for (const auto name : {"mainmenu.map", "shared.map", "single_player_shared.map"})
    write(a / "maps" / name, name);
  write(a / "maps/fonts/font_table.txt", "font fixture");
  write(a / "maps/fonts/font", "font pixels fixture");
  std::filesystem::copy(a, b, std::filesystem::copy_options::recursive);
  auto identity = [&](const std::filesystem::path &directory) {
    return multiplayer_fingerprint(directory / "match.map", directory / "maps", 0, 3);
  };
  const auto expected = identity(a);
  check(expected == identity(b), "Moving imported data changed LAN compatibility");
  check(expected != multiplayer_fingerprint(a / "match.map", a / "maps", 1, 3),
        "Different BSP allowed to join same LAN game");
  check(expected != multiplayer_fingerprint(a / "match.map", a / "maps", 0, 4),
        "Different starting weapon allowed to join same LAN game");
  for (const auto relative : {"match.map", "default.xbe", "maps/mainmenu.map",
                              "maps/shared.map", "maps/single_player_shared.map",
                              "maps/fonts/font", "maps/fonts/font_table.txt"}) {
    std::filesystem::copy_file(a / relative, b / relative,
                              std::filesystem::copy_options::overwrite_existing);
    { std::ofstream changed(b / relative, std::ios::binary | std::ios::app); changed << '!'; }
    check(expected != identity(b), "Changed imported data did not change LAN compatibility");
    std::filesystem::copy_file(a / relative, b / relative,
                              std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(b / "maps/shared.map");
  rejects([&] { identity(b); }, "Missing shared game data silently fingerprinted");
}
void network() {
  auto c = arena();
  c.weapons.at(3).firing_effects.push_back({1,1,{null_datum,null_datum,null_datum},{77,null_datum,null_datum}});
  c.multiplayer_starting_equipment_flags = 2;
  auto spawn = starts();
  auto third = spawn[0];
  third.position = {0, 4, 0};
  spawn.push_back(third);
  auto r = rules();
  r.name = std::string(32, 'S');
  r.score_limit = 17;
  r.time_ticks = 34567;
  MultiplayerMatch match(c, 0, spawn, r);
  LanHost host(match, 0, 123);
  const auto address = "127.0.0.1:" + std::to_string(host.port());
  LanClient one(address, 123, 1), two(address, 123, 2), wrong(address, 124, 1);
  std::array<Controls, 16> controls{};
  std::array<Controls, 1> i1{};
  std::array<Controls, 2> i2{};
  for (unsigned i = 0;
       i < 40 && (!one.ready() || !two.ready() || wrong.error().empty()); ++i) {
    one.send(i1);
    two.send(i2);
    wrong.send(i1);
    host.receive(controls);
    host.broadcast();
    one.receive();
    two.receive();
    wrong.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(one.ready() && two.ready() && !wrong.ready() &&
            !wrong.error().empty() && host.peers() == 2,
        "Real UDP handshake or map rejection failed");
  check(one.state().score_limit == 17 && two.state().time_ticks == 34567 &&
            one.state().variant_name == r.name,
        "Host variant rules were not replicated");
  check(one.slots().size() == 1 && two.slots().size() == 2 &&
            one.slots()[0] != two.slots()[0] &&
            two.slots()[0] != two.slots()[1],
        "LAN local players shared a slot");
  const auto shooter = one.slots()[0], target = two.slots()[0];
  const auto delta =
      match.unit(target).body.position - match.unit(shooter).body.position;
  i1[0].yaw = std::atan2(delta.y, delta.x);
  i1[0].fire = true;
  one.send(i1);
  host.receive(controls);
  match.step(controls);
  host.broadcast();
  one.receive();
  two.receive();
  check(match.player(shooter).score == 1 && two.state().players[target].dead &&
            two.state().players[shooter].score == 1,
        "Host combat/score not replicated to actual UDP client");
  check(!one.state().rumble_events.empty() &&
            one.state().rumble_events.back().event.definition==77 &&
            one.state().rumble_events.back().event.owner==int(shooter)-1 &&
            one.state().rumble_events.back().serial==match.state().rumble_events.back().serial,
        "Authored firing rumble did not survive actual UDP snapshot replication");
  for (unsigned shot = 1; shot < 20; ++shot) {
    one.send(i1);
    host.receive(controls);
    match.step(controls);
    host.broadcast();
    one.receive();
    two.receive();
  }
  check(match.unit(shooter).weapon.loaded == 0, "Disposable magazine did not empty");
  i1[0].fire = false;
  i1[0].reload = true;
  one.send(i1);
  host.receive(controls);
  match.step(controls);
  host.broadcast();
  one.receive();
  two.receive();
  check(two.state().players[shooter].reload_empty &&
            two.state().players[shooter].reload_seconds > 0,
        "Original empty-reload action state not replicated to actual UDP client");
  i1[0].reload = false;
  const auto before = match.state().tick;
#ifndef _WIN32
  const int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in at{};
  at.sin_family = AF_INET;
  at.sin_port = htons(host.port());
  inet_pton(AF_INET, "127.0.0.1", &at.sin_addr);
  const char malformed[1500]{};
  sendto(socket, malformed, sizeof(malformed), 0,
         reinterpret_cast<sockaddr *>(&at), sizeof(at));
  close(socket);
  host.receive(controls);
#endif
  check(match.state().tick == before && host.peers() == 2,
        "Malformed packet mutated host state");
  {
    LanClient temporary(address, 123, 1);
    for (unsigned i = 0; i < 20 && !temporary.ready(); ++i) {
      temporary.send(i1);
      host.receive(controls);
      host.broadcast();
      temporary.receive();
    }
    check(temporary.ready() && host.peers() == 3,
          "Temporary peer did not join");
  }
  host.receive(controls);
  check(host.peers() == 2, "Leave packet did not release participant");
  // Fill all slots: largest snapshot includes all 16 participant records.
  for (;;) {
    const auto state = match.state();
    if (std::count_if(state.players.begin(), state.players.end(),
                      [](const auto &p) { return p.connected; }) == 16)
      break;
    match.join("Local");
  }
  i1[0].fire = false;
  i1[0].forward = 1;
  one.send(i1);
  host.receive(controls);
  match.step(controls);
  host.broadcast();
  one.receive();
  check(std::count_if(one.state().players.begin(), one.state().players.end(),
                      [](const auto &p) { return p.connected; }) == 16,
        "Maximum-sized snapshot truncated");
  for (unsigned i = 0; i < 16; ++i)
    check(one.state().players[i].grenades == match.unit(i).grenades &&
              one.state().players[i].grenade_current == match.unit(i).grenade_current,
          "Independent grenade counts did not fit the sixteen-player snapshot");
  const auto replicated_velocity=one.state().players[shooter].velocity;
  const auto authoritative_velocity=match.unit(shooter).body.velocity;
  check(length(authoritative_velocity)>0 && length(replicated_velocity-authoritative_velocity)==0,
        "Root physics velocity for lighting was not replicated in the largest snapshot");
  rejects([&] { LanHost invalid(match, 70000, 1); },
          "Invalid UDP port accepted");
}
void network_startup_delay() {
  const auto content=arena();const auto spawn=starts();
  MultiplayerMatch match(content,0,spawn,rules());
  LanHost host(match,0,0x137fe0);
  LanClient client("127.0.0.1:"+std::to_string(host.port()),0x137fe0,1);
  // Reproduce graphics preparation outliving the unanswered-request deadline.
  // No request has been sent and the live host has no peer to expire yet.
  std::this_thread::sleep_for(std::chrono::milliseconds(10100));
  client.receive();
  check(client.error().empty(),"Asset preparation consumed unanswered-host timeout");
  std::array<Controls,16> controls{};
  for(unsigned i=0;i<64 && !client.ready();++i) {
    client.send(std::array<Controls,1>{});host.receive(controls);
    host.broadcast();client.receive();
  }
  check(client.ready() && client.slots().size()==1 && host.peers()==1,
        "Delayed-start actual UDP client did not join");
}
void network_snapshot_fragments() {
#ifndef _WIN32
  // Disposable native transport fixture: this tests real UDP reassembly and
  // publication, not original gameplay behavior or Xbox protocol parity.
  auto content=arena();auto spawn=starts();auto selected=rules();
  MultiplayerMatch match(content,0,spawn,selected);
  for(unsigned i=0;i<15;++i)match.join("Roster "+std::to_string(i)+std::string(80,'x'));
  LanHost host(match,0,456);
  struct SocketGuard {int fd;~SocketGuard(){close(fd);}} proxy{::socket(AF_INET,SOCK_DGRAM,0)};
  check(proxy.fd>=0,"Create actual UDP proxy");
  sockaddr_in address{};address.sin_family=AF_INET;
  address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  check(bind(proxy.fd,reinterpret_cast<sockaddr *>(&address),sizeof(address))==0,"Bind UDP proxy");
  socklen_t size=sizeof(address);
  check(getsockname(proxy.fd,reinterpret_cast<sockaddr *>(&address),&size)==0,"Read proxy address");
  sockaddr_in host_address=address;host_address.sin_port=htons(host.port());
  LanClient client("127.0.0.1:"+std::to_string(ntohs(address.sin_port)),456,1);
  std::array<Controls,1> local{};std::array<Controls,16> controls{};
  client.send(local);
  sockaddr_in client_address{};size=sizeof(client_address);std::array<char,1501> bytes{};
  const auto hello_size=recvfrom(proxy.fd,bytes.data(),bytes.size(),MSG_DONTWAIT,
      reinterpret_cast<sockaddr *>(&client_address),&size);
  check(hello_size>0,"Actual hello arrives at proxy");
  check(sendto(proxy.fd,bytes.data(),hello_size,0,reinterpret_cast<sockaddr *>(&host_address),sizeof(host_address))==hello_size,
      "Relay actual native hello");
  host.receive(controls);
  const auto receive=[&] {
    std::vector<std::vector<char>> result;
    for(;;) {
      sockaddr_in from{};socklen_t nfrom=sizeof(from);
      const auto n=recvfrom(proxy.fd,bytes.data(),bytes.size(),MSG_DONTWAIT,reinterpret_cast<sockaddr *>(&from),&nfrom);
      if(n<0)break;
      check(n<=1400 && from.sin_port==host_address.sin_port,"Every snapshot datagram stays within1400 bytes");
      result.emplace_back(bytes.begin(),bytes.begin()+n);
    }
    check(result.size()>=2,"Largest roster uses multiple real datagrams");
    return result;
  };
  const auto deliver=[&](const std::vector<char> &packet) {
    check(sendto(proxy.fd,packet.data(),packet.size(),0,reinterpret_cast<sockaddr *>(&client_address),sizeof(client_address))==int(packet.size()),
        "Relay native snapshot fragment");
    client.receive();
  };
  const auto first=receive();
  deliver(first.back());deliver(first.back());
  check(!client.ready(),"Incomplete and duplicate fragments cannot publish a roster");
  for(std::size_t i=first.size()-1;i>0;--i)deliver(first[i-1]);
  check(client.ready(),"Out-of-order complete snapshot joins actual client");
  const auto expected_state=match.state();
  for(unsigned i=0;i<16;++i) {
    const auto &actual=client.state().players[i];const auto &expected=expected_state.players[i];
    check(actual.name==expected.name && actual.source_player==expected.source_player &&
        actual.source_unit==expected.source_unit && actual.source_weapon==expected.source_weapon &&
        actual.source_hud_zoom_mask==expected.source_hud_zoom_mask &&
        actual.motion_sensor_control==expected.motion_sensor_control,
        "Complete names and full generation identities replicated for every roster row");
  }
  controls[0].fire=true;
  match.step(controls);host.broadcast();const auto dropped=receive();
  deliver(dropped.front());
  check(client.state().tick==0,"Missing fragment leaves the complete previous snapshot intact");
  auto malformed=dropped.back();malformed[27]=char(255);deliver(malformed);
  check(client.state().tick==0,"Malformed fragment does not mutate published state");
  match.step(controls);host.broadcast();const auto next=receive();
  for(auto i=next.rbegin();i!=next.rend();++i)deliver(*i);
  check(client.state().tick==2,"Later complete snapshot recovers from packet loss");
  check(client.state().players[0].motion_sensor_control==1,
        "Live firing radar control did not survive fragmented LAN snapshot recovery");
  for(const auto &packet:dropped)deliver(packet);
  for(const auto &packet:first)deliver(packet);
  check(client.state().tick==2,"Stale fragments cannot roll back the roster");
  // A conflicting duplicate invalidates only its unfinished message.
  match.step(controls);host.broadcast();const auto conflicting=receive();
  deliver(conflicting.front());auto corrupted=conflicting.front();corrupted.back()^=1;deliver(corrupted);
  for(std::size_t i=1;i<conflicting.size();++i)deliver(conflicting[i]);
  check(client.state().tick==2,"Conflicting duplicate cannot publish mixed snapshot bytes");
  match.step(controls);host.broadcast();const auto recovered=receive();
  for(const auto &packet:recovered)deliver(packet);
  check(client.state().tick==4 && client.error().empty(),"Client recovers after conflicting fragments");
#endif
}
void retail(const std::filesystem::path &maps) {
  const std::array<const char *, 10> names{
      "lockout",   "ascension", "beavercreek", "burial_mounds", "colossus",
      "cyclotron", "headlong",  "zanzibar",    "coagulation",   "foundation"};
  for (auto name : names) {
    Cache cache(maps / (std::string(name) + ".map"));
    auto content = cache.load_content(0, true);
    auto r = rules();
    r.weapon = cache.slayer_starting_weapon();
    MultiplayerMatch match(content, 0, cache.spawns(), r);
    for (unsigned i = 0; i < 4; ++i)
      match.join("Player");
    std::array<Controls, 16> inputs{};
    for (unsigned i = 0; i < 4; ++i)
      inputs[i].yaw = match.unit(i).yaw;
    for (unsigned i = 0; i < 30; ++i)
      match.step(inputs);
    check(match.state().players[3].connected &&
              content.weapons.at(r.weapon).name.ends_with("smg"),
          "Authored map match/loadout");
    check(match.unit(0).weapon.loaded == 60 &&
              match.unit(0).weapon.reserve == 120,
          "Original SMG initial total was not split between magazine and "
          "inventory");
  }
  Cache plasma(maps / "midship.map");
  auto content = plasma.load_content(0, true);
  auto r = rules();
  r.weapon = plasma.slayer_starting_weapon();
  rejects([&] { MultiplayerMatch match(content, 0, plasma.spawns(), r); },
          "Battery weapon silently treated as infinite magazine");
  auto verify_smg_option = [&](const std::filesystem::path &path) {
    Cache cache(path);
    auto authored = cache.load_content(0, true);
    auto selected = slayer_rules({"SMG Slayer", 0, {{0x59, 5}}});
    selected.weapon = cache.slayer_starting_weapon(selected.starting_weapon);
    check(authored.weapons.at(selected.weapon).name.ends_with("smg"),
          "Original option 5 did not resolve authored SMG reference");
    MultiplayerMatch match(authored, 0, cache.spawns(), selected);
    for (unsigned i = 0; i < 4; ++i)match.join("Player");
    check(match.unit(0).weapon.loaded == 60 &&
              match.unit(0).weapon.reserve == 120,
          "SMG option did not initialize original magazine loadout");
    const auto name = path.stem().string();
    const auto expected = name == "backwash" ? std::array<std::uint8_t, 2>{0, 0}
        : name == "midship" || name == "gemini" ? std::array<std::uint8_t, 2>{0, 2}
        : std::array<std::uint8_t, 2>{2, 0};
    for (unsigned i = 0; i < 4; ++i)
      check(match.unit(i).grenades == expected && match.state().players[i].grenades == expected,
            "Authored starting-equipment grenade counts differ for " + name);
    std::array<Controls, 16> neutral{};
    for (unsigned i = 0; i < 4; ++i) neutral[i].yaw = match.unit(i).yaw;
    match.step(neutral);
    const int selected_type = expected[0] ? 0 : expected[1] ? 1 : -1;
    for (unsigned i = 0; i < 4; ++i)
      check(match.unit(i).grenade_current == selected_type &&
                match.state().players[i].grenade_current == selected_type,
            "Original first-tick grenade selection differs for " + name);
    selected.starting_grenades = false;
    MultiplayerMatch disabled(authored, 0, cache.spawns(), selected);
    disabled.join("No Grenades");
    check(disabled.unit(0).grenades == std::array<std::uint8_t, 2>{0, 0},
          "Starting-grenade option did not override authored map counts");
    rejects([&] { cache.slayer_starting_weapon(2); },
            "Unimplemented random option accepted by cache resolver");
  };
  for (auto name : names)verify_smg_option(maps / (std::string(name) + ".map"));
  verify_smg_option(maps / "midship.map");
  verify_smg_option(maps / "waterworks.map");
  const auto dlc = maps.parent_path().parent_path() / "map-pack/maps";
  for (const auto name : {"backwash", "containment", "deltatap", "dune",
                         "elongation", "gemini", "triplicate", "turf", "warlock"})
    if (std::filesystem::exists(dlc / (std::string(name) + ".map")))
      verify_smg_option(dlc / (std::string(name) + ".map"));
}
} // namespace
int main(int argc, char **argv) {
  try {
    if(argc==2 && std::string(argv[1])=="--startup-delay") {
      network_startup_delay();
      std::cout<<"Actual UDP joined after preparation exceeded the connection deadline\n";
      return 0;
    }
    rules_and_match();
    asset_identity();
    network();
    network_snapshot_fragments();
    if (argc > 1)
      retail(argv[1]);
    std::cout << "Multiplayer rules, combat/respawn, UDP membership/state, and "
                 "map tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
