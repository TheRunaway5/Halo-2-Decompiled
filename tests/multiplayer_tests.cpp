#include "halo2/math.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/lan_packet_auth.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <deque>
#include <bit>
#include <map>
#include <optional>
#include <random>
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
#include "lan_admission_fixture.hpp"
void heavy_vehicle_replica_admission() {
  const auto content=arena();const auto spawn=starts();
  for(int mode:{1,2,4}) {
    auto selected=rules();selected.primary_heavy_vehicle=mode;
    MultiplayerMatch replica(content,0,spawn,selected);
    const auto before=replica.state().tick;
    rejects([&]{replica.step({});},"Heavy vehicle display replica advanced without a source authority");
    check(replica.state().tick==before,"Rejected heavy vehicle authority changed the world clock");
  }
}
void session_configuration() {
  // Transport fixture: ordered configuration inputs, not supplied physics.
  const auto content=arena();const auto spawn=starts();
  MultiplayerMatch match(content,0,spawn,rules());
  UiVariant variant{"Authoritative source settings",0,{}};
  for(unsigned n=0;n<600;++n) {
    variant.values.append(76,n%2?5:1);
    variant.values.append(89,5);
  }
  LanHost host(match,0,0x5155455259ull,variant);
  const auto before=match.state().tick;
  std::array<Controls,multiplayer_capacity> controls{};
#ifndef _WIN32
  struct SocketGuard {int fd;~SocketGuard(){close(fd);}} proxy{::socket(AF_INET,SOCK_DGRAM,0)};
  check(proxy.fd>=0,"Session query proxy socket");
  sockaddr_in proxy_address{};proxy_address.sin_family=AF_INET;
  proxy_address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  check(!bind(proxy.fd,reinterpret_cast<sockaddr*>(&proxy_address),sizeof(proxy_address)),"Bind session query proxy");
  socklen_t address_bytes=sizeof(proxy_address);
  check(!getsockname(proxy.fd,reinterpret_cast<sockaddr*>(&proxy_address),&address_bytes),"Read session query proxy port");
  sockaddr_in authority=proxy_address;authority.sin_port=htons(host.port());
  sockaddr_in client{};bool have_client{},dropped{},reordered{};unsigned duplicates{};
  std::vector<std::vector<std::byte>> pending;
  auto result=std::async(std::launch::async,[&] {
    return query_lan_session("127.0.0.1:"+std::to_string(ntohs(proxy_address.sin_port)),std::chrono::seconds(3));
  });
  while(result.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready) {
    std::array<std::byte,1500> packet{};sockaddr_in from{};socklen_t length=sizeof(from);
    for(int n;(n=recvfrom(proxy.fd,packet.data(),packet.size(),MSG_DONTWAIT,
        reinterpret_cast<sockaddr*>(&from),&length))>0;length=sizeof(from)) {
      if(from.sin_port!=authority.sin_port) {
        client=from;have_client=true;
        sendto(proxy.fd,packet.data(),n,0,reinterpret_cast<sockaddr*>(&authority),sizeof(authority));
      }else {
        check(have_client && n>=31,"Unexpected session description fragment");
        if(packet[6]==std::byte{8}){
          sendto(proxy.fd,packet.data(),n,0,reinterpret_cast<sockaddr*>(&client),sizeof(client));continue;
        }
        check(packet[6]==std::byte{7},"Unexpected session description packet type");
        const auto index=std::to_integer<unsigned>(packet[23])|(std::to_integer<unsigned>(packet[24])<<8);
        const auto count=std::to_integer<unsigned>(packet[25])|(std::to_integer<unsigned>(packet[26])<<8);
        check(count>1,"Session description did not exercise fragmentation");
        if(!dropped && index==0){dropped=true;continue;}
        pending.emplace_back(packet.begin(),packet.begin()+n);
        if(index+1==count) {
          for(auto it=pending.rbegin();it!=pending.rend();++it) {
            sendto(proxy.fd,it->data(),it->size(),0,reinterpret_cast<sockaddr*>(&client),sizeof(client));
            if(!duplicates) {
              sendto(proxy.fd,it->data(),it->size(),0,reinterpret_cast<sockaddr*>(&client),sizeof(client));++duplicates;
            }
          }
          pending.clear();reordered=true;
        }
      }
    }
    host.receive(controls);std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  const auto description=result.get();
  check(dropped && reordered && duplicates,"Session query loss/reorder/duplicate routes were not exercised");
#else
  auto result=std::async(std::launch::async,[&] {
    return query_lan_session("127.0.0.1:"+std::to_string(host.port()),std::chrono::seconds(3));
  });
  while(result.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready) {
    host.receive(controls);std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  const auto description=result.get();
#endif
  check(description.fingerprint==0x5155455259ull && description.variant.name==variant.name &&
      description.variant.engine==variant.engine && description.variant.values==variant.values,
      "Complete ordered host configuration did not survive actual UDP");
  const auto after=match.state();
  check(host.peers()==0 && after.tick==before &&
      std::none_of(after.players.begin(),after.players.end(),[](const auto &p){return p.connected;}),
      "Configuration query admitted a player or changed the world");
  auto handwritten=rules();handwritten.session_configuration.reset();
  MultiplayerMatch undeclared(content,0,spawn,handwritten);
  LanHost legacy(undeclared,0,0x5155455259ull);
  auto missing=std::async(std::launch::async,[&] {
    return query_lan_session("127.0.0.1:"+std::to_string(legacy.port()),std::chrono::seconds(3));
  });
  while(missing.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready) {
    legacy.receive(controls);std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  rejects([&]{(void)missing.get();},"Host without declared session settings returned invented configuration");
  struct Bus {
    std::mutex mutex;
    std::array<std::deque<std::pair<unsigned,std::vector<std::byte>>>,2> packets;
  };
  struct Endpoint final : MultiplayerDatagrams {
    std::shared_ptr<Bus> bus;unsigned self;
    Endpoint(std::shared_ptr<Bus> b,unsigned id):bus(std::move(b)),self(id){}
    void send(std::span<const std::byte> bytes,unsigned peer) override {
      std::lock_guard lock(bus->mutex);
      bus->packets.at(peer).emplace_back(self,std::vector<std::byte>(bytes.begin(),bytes.end()));
    }
    bool receive(std::vector<std::byte> &bytes,unsigned &peer) override {
      std::lock_guard lock(bus->mutex);auto &queue=bus->packets[self];
      if(queue.empty())return false;
      peer=queue.front().first;bytes=std::move(queue.front().second);queue.pop_front();return true;
    }
  };
  const auto bus=std::make_shared<Bus>();
  auto host_endpoint=std::make_shared<Endpoint>(bus,0),client_endpoint=std::make_shared<Endpoint>(bus,1);
  LanHost relay(match,host_endpoint,0x5155455259ull,{},variant);
  auto relayed=std::async(std::launch::async,[&] {
    return query_lan_session(client_endpoint,0,std::chrono::seconds(3));
  });
  while(relayed.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready) {
    relay.receive(controls);std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  const auto online=relayed.get();
  check(online.fingerprint==description.fingerprint && online.variant.values==variant.values &&
      relay.peers()==0 && match.state().tick==before,"Session transport endpoint failed complete configuration query");
}
void session_configuration_while_loading() {
  const auto content=arena();const auto spawn=starts();
  const auto selected=rules();MultiplayerMatch match(content,0,spawn,selected);
  LanHost host(match,0,0x4c4f4144494e47ull);
  // The rendering/loading thread is not pumping gameplay receive yet.
  // Discovery must still answer without creating a player or world tick.
  const auto before=match.state();
  const auto description=query_lan_session("127.0.0.1:"+std::to_string(host.port()),
      std::chrono::milliseconds(150));
  check(description.fingerprint==0x4c4f4144494e47ull &&
      description.variant.values==selected.session_configuration->values &&
      host.peers()==0 && match.state().tick==before.tick,
      "Loading host did not answer its actual session configuration independently");
  LanClient client("127.0.0.1:"+std::to_string(host.port()),description.fingerprint,1);
  const std::array<Controls,1> neutral{};client.send(neutral);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  check(host.peers()==0 && match.state().tick==before.tick,
      "Loading transport service executed gameplay admission");
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned n=0;n<64 && !client.ready();++n) {
    host.receive(controls);host.broadcast();client.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(client.ready() && host.peers()==1 && match.state().tick==before.tick,
      "Owner thread did not consume the queued real join request");
}
void sneaking_presentation() {
  for(bool ceiling:{false,true}) {
    auto content=arena();
    if(ceiling) {
      content.collision.push_back({{1,-10,.8f},{8,10,.8f},{8,-10,.8f},0,0});
      content.collision.push_back({{1,-10,.8f},{1,10,.8f},{8,10,.8f},0,0});
    }
    auto selected=rules();selected.time_ticks=selected.score_limit=0;
    MultiplayerMatch match(content,0,starts(),selected);match.join("Sneaking");
    std::array<Controls,multiplayer_capacity> controls{};
    controls[0].crouch=true;
    for(unsigned tick=0;tick<30;++tick)match.step(controls);
    check(match.state().players[0].native_crouching && match.unit(0).crouch==1,
        "Held sneak did not publish its third-person stance");
    if(ceiling) {
      controls[0].forward=1;
      for(unsigned tick=0;tick<90;++tick)match.step(controls);
      check(match.unit(0).body.position.x>1.5f && match.unit(0).body.position.x<7,
          "Sneaking fixture never entered its low ceiling");
      controls[0].forward=0;
    }
    controls[0].crouch=false;match.step(controls);
    check(match.unit(0).crouch>0 && match.state().players[0].native_crouching,
        "Releasing sneak made the third-person stance stand before its capsule");
    for(unsigned tick=0;tick<30;++tick)match.step(controls);
    if(ceiling)
      check(match.unit(0).crouch>0 && match.state().players[0].native_crouching,
          "A blocked stand-up lost the third-person sneaking stance");
    else
      check(match.unit(0).crouch==0 && !match.state().players[0].native_crouching,
          "Completed stand-up retained the third-person sneaking stance");
  }
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
  for(int choice=3;choice<=20;++choice)
    check(slayer_rules({"Specific weapon",0,{{0x59,choice}}}).starting_weapon==unsigned(choice),
        "Supported specific weapon option changed during admission");
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
void mcc_asset_identity() {
  const auto root = std::filesystem::temp_directory_path() /
      ("halo2-mcc-lan-data-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
  } cleanup{root};
  auto write = [](const std::filesystem::path &path, const std::string &value) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary); file << value;
    if (!file) throw std::runtime_error("Could not write disposable MCC identity fixture");
  };
  // Format fixtures exercise identity only, without constructing a game world.
  const std::string header("daeh\x0d\0\0\0", 8);
  const auto a=root/"a/halo2", b=root/"relocated/halo2";
  const auto maps=std::filesystem::path("h2_maps_win64_dx11");
  for (const auto name : {"lockout.map", "mainmenu.map", "shared.map", "single_player_shared.map"})
    write(a/maps/name, header+name);
  for (const auto name : {"textures.dat", "sounds_en.dat", "sounds_neutral.dat", "sounds_remastered.dat"})
    write(a/maps/name, name);
  write(a/"h2_fonts/font_table.txt", "MCC font selector fixture");
  write(a/"h2_fonts/font", "MCC font pixels fixture");
  std::filesystem::create_directories(b.parent_path());
  std::filesystem::copy(a,b,std::filesystem::copy_options::recursive);
  const auto identity=[&](const auto &directory) {
    return multiplayer_fingerprint(directory/maps/"lockout.map",directory/maps,0,3);
  };
  const auto expected=identity(a);
  check(expected==identity(b),"Moving MCC data changed LAN identity");
  check(expected!=multiplayer_fingerprint(a/maps/"lockout.map",a/maps,1,3),"MCC BSP excluded from LAN identity");
  check(expected!=multiplayer_fingerprint(a/maps/"lockout.map",a/maps,0,4),"MCC starting weapon excluded from LAN identity");
  for (const auto relative : {"h2_maps_win64_dx11/lockout.map", "h2_maps_win64_dx11/mainmenu.map",
      "h2_maps_win64_dx11/shared.map", "h2_maps_win64_dx11/single_player_shared.map",
      "h2_maps_win64_dx11/textures.dat", "h2_maps_win64_dx11/sounds_en.dat",
      "h2_maps_win64_dx11/sounds_neutral.dat", "h2_maps_win64_dx11/sounds_remastered.dat",
      "h2_fonts/font_table.txt", "h2_fonts/font"}) {
    { std::ofstream file(b/relative,std::ios::binary|std::ios::app); file << '!'; }
    check(expected!=identity(b),"Changed MCC data allowed matching LAN identity");
    std::filesystem::copy_file(a/relative,b/relative,std::filesystem::copy_options::overwrite_existing);
  }
  write(b/"default.xbe","unrelated offline reference must not be read");
  check(expected==identity(b),"MCC identity consumed an Xbox executable");
  rejects([&]{multiplayer_fingerprint(a/maps/"lockout.map",a/maps,0,3,b/"default.xbe");},
      "MCC LAN identity accepted injected original executable data");
  write(b/maps/"shared.map",std::string("daeh\x08\0\0\0",8));
  rejects([&]{identity(b);},"MCC LAN identity accepted mixed cache versions");
  std::filesystem::copy_file(a/maps/"shared.map",b/maps/"shared.map",std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(b/maps/"textures.dat");
  rejects([&]{identity(b);},"MCC LAN identity omitted required texture data");
}
void native_request_ingress() {
  using Packet=std::vector<std::byte>;
  struct Bus {std::array<std::deque<std::pair<unsigned,Packet>>,5> packets;};
  struct Endpoint final:MultiplayerDatagrams {
    std::shared_ptr<Bus> bus;unsigned self;
    Endpoint(std::shared_ptr<Bus> b,unsigned s):bus(std::move(b)),self(s){}
    void send(std::span<const std::byte> p,unsigned to)override {
      bus->packets.at(to).emplace_back(self,Packet(p.begin(),p.end()));
    }
    bool receive(Packet &p,unsigned &from)override {
      auto &q=bus->packets[self];if(q.empty())return false;
      from=q.front().first;p=std::move(q.front().second);q.pop_front();return true;
    }
  };
  // The in-memory endpoints model the native consumer's retained matched-peer
  // slots. This tests actual LanHost/LanClient behavior, not DTLS authentication.
  const auto bus=std::make_shared<Bus>();
  auto host_endpoint=std::make_shared<Endpoint>(bus,4),client_endpoint=std::make_shared<Endpoint>(bus,0);
  const auto content=arena();MultiplayerMatch match(content,0,starts(),rules());
  std::vector<OnlineAdmission> admitted;
  for(unsigned i=0;i<4;++i){OnlineAdmission p;p.slot=i;p.peer=0;p.name="Seat "+std::to_string(i);p.team=-1;admitted.push_back(p);}
  LanHost host(match,host_endpoint,789,admitted);
  const std::array<std::int8_t,4> teams{-1,-1,-1,-1};
  LanClient client(client_endpoint,4,789,teams);
  std::array<Controls,4> local{};std::array<Controls,multiplayer_capacity> delivered{};
  const auto before=match.state().tick;
  const auto take=[&]{auto &q=bus->packets[4];check(q.size()==1,"Expected one real native request");auto p=std::move(q.front().second);q.clear();return p;};
  const auto inject=[&](const Packet &p){bus->packets[4].emplace_back(0,p);host.receive(delivered);};
  client.send(local);const auto hello=take();
  for(unsigned n=0;n<4;++n){
    auto p=hello;
    if(n==0)p[23]=std::byte{0};
    if(n==1)p[23]=std::byte{5};
    if(n==2)p[28]=std::byte{2};
    if(n==3)p.push_back(std::byte{});
    inject(p);
    const auto state=match.state();
    check(host.peers()==0&&state.tick==before&&
        std::none_of(state.players.begin(),state.players.end(),[](const auto &v){return v.connected;}),
        "Malformed Hello allocated native players or advanced the world");
  }
  inject(hello);host.broadcast();client.receive();
  check(client.ready()&&client.slots().size()==4&&host.peers()==1,"Healthy four-seat join failed after malformed requests");
  for(auto &c:local)c.forward=.25f;
  client.send(local);const auto baseline=take();inject(baseline);
  const auto retained=[&](float value){
    check(host.peers()==1&&match.state().tick==before,"Rejected input retired its healthy peer or advanced world");
    for(auto slot:client.slots())check(delivered[slot].forward==value,"Rejected input partially changed an admitted couch seat");
  };
  retained(.25f);
  for(auto &c:local)c.forward=-.75f;
  client.send(local);const auto valid=take();
  for(unsigned n=0;n<5;++n){
    auto p=valid;
    if(n==0){const auto bits=0x7fc00000u;for(unsigned j=0;j<4;++j)p[28+j]=std::byte(bits>>(8*j));}
    if(n==1)p.back()=std::byte{2}; // Invalid final-seat intent, after all earlier controls.
    if(n==2)p[27]=std::byte{3};
    if(n==3)p[15]^=std::byte{1}; // Valid grammar, wrong live cookie.
    if(n==4)std::copy(baseline.begin()+23,baseline.begin()+27,p.begin()+23); // Replay sequence, changed controls.
    inject(p);retained(.25f);
  }
  inject(valid);retained(-.75f);
  auto leave=Packet(valid.begin(),valid.begin()+23);leave[6]=std::byte{4};
  auto foreign=leave;foreign[15]^=std::byte{1};inject(foreign);retained(-.75f);
  auto trailing=leave;trailing.push_back(std::byte{});inject(trailing);retained(-.75f);
  inject(leave);
  const auto state=match.state();
  check(host.peers()==0&&state.tick==before&&std::none_of(state.players.begin(),state.players.end(),[](const auto &p){return p.connected;}),
      "Valid leave did not remove its four seats atomically");
}
void network_vehicle_intent() {
  const auto content=arena();const auto spawn=starts();
  MultiplayerMatch match(content,0,spawn,rules());LanHost host(match,0,789);
  LanClient client("127.0.0.1:"+std::to_string(host.port()),789,1);
  std::array<Controls,1> local{};std::array<Controls,16> delivered{};
  for(unsigned i=0;i<64 && !client.ready();++i) {
    client.send(local);host.receive(delivered);host.broadcast();client.receive();
  }
  check(client.ready() && client.slots().size()==1,"Vehicle intent UDP admission");
  const auto slot=client.slots()[0];const auto before=match.state().tick;
  const std::array<std::optional<SourcePlayerActionTarget>,4> targets{
    SourcePlayerActionTarget{5,0,0x80010002u},
    SourcePlayerActionTarget{6,4,0x80010002u},
    SourcePlayerActionTarget{},std::nullopt};
  for(const auto &target:targets) {
    local[0].vehicle_interaction_target=target;local[0].pickup=bool(target && target->action);
    bool received=false;
    for(unsigned i=0;i<64 && !received;++i) {
      client.send(local);host.receive(delivered);
      const auto &actual=delivered[slot].vehicle_interaction_target;
      received=actual.has_value()==target.has_value() &&
        (!target || (actual->action==target->action && actual->index==target->index && actual->object==target->object));
      if(!received)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    check(received,"Vehicle action/object/seat intent changed in actual UDP transport");
    if(target && target->action)
      check(delivered[slot].pickup,"Vehicle intent lost its held pickup input");
    // Original input delivery also emits the completed pickup pulse on
    // release, so a neutral target need not clear the button in this tick.
  }
  const SourcePlayerActionTarget release_target{5,0,0x80010002u};
  const SourcePlayerActionTarget subsequent_target{5,1,0x80020003u};
  local[0].pickup=true;local[0].pickup_milliseconds=600;
  local[0].vehicle_interaction_target=release_target;client.send(local);host.receive(delivered);
  local[0].pickup=false;local[0].pickup_milliseconds=0;
  client.send(local); // Release and its target belong to this same input frame.
  local[0].vehicle_interaction_target=subsequent_target;client.send(local);
  host.receive(delivered); // Coalesce both actual UDP packets before gameplay.
  check(delivered[slot].pickup && delivered[slot].pickup_milliseconds==600 &&
      delivered[slot].vehicle_interaction_target==release_target,
      "Completed pickup hold was retargeted by a later neutral UDP packet");
  host.receive(delivered);
  check(!delivered[slot].pickup && delivered[slot].vehicle_interaction_target==subsequent_target,
      "Completed vehicle intent repeated or replaced subsequent continuous input");
  check(match.state().tick==before,"Vehicle intent transport advanced physics");
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
  const auto deliver_current=[&] {
    const auto tick=match.state().tick;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while((one.state().tick!=tick || two.state().tick!=tick) && std::chrono::steady_clock::now()<deadline) {
      // Delivery follows the acknowledged stream's real budget. Service
      // transport/acks without advancing the authoritative combat clock.
      one.send(i1);two.send(i2);host.receive(controls);host.broadcast();one.receive();two.receive();
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    check(match.state().tick==tick && one.state().tick==tick && two.state().tick==tick,
        "Bounded UDP delivery did not retain the actual completed simulation tick");
  };
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
  check(match.player(shooter).score==1,"Host combat did not award its actual fixture score");
  deliver_current();
  check(two.state().players[target].dead &&
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
  deliver_current();
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
  deliver_current();
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
#ifndef _WIN32
// Scoped H2_NET_PROFILE for snapshot streams created inside one fixture.
struct NetProfile {
  std::optional<std::string> previous;
  explicit NetProfile(const char *value) {
    if(const char *old=std::getenv("H2_NET_PROFILE"))previous=old;
    setenv("H2_NET_PROFILE",value,1);
  }
  ~NetProfile(){if(previous)setenv("H2_NET_PROFILE",previous->c_str(),1);else unsetenv("H2_NET_PROFILE");}
};
// Loopback proxy: forwards client datagrams to the host, holds host datagrams
// for the fixture to deliver, drop, reorder or tamper, and counts UDP bytes.
struct UdpProxy {
  int fd=::socket(AF_INET,SOCK_DGRAM,0);
  sockaddr_in self{},host{},client{};
  bool client_known{};
  std::uint64_t delivered_bytes{};
  explicit UdpProxy(unsigned host_port) {
    check(fd>=0,"Create stream proxy");
    self.sin_family=AF_INET;self.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    check(bind(fd,reinterpret_cast<sockaddr*>(&self),sizeof(self))==0,"Bind stream proxy");
    socklen_t size=sizeof(self);
    check(getsockname(fd,reinterpret_cast<sockaddr*>(&self),&size)==0,"Read stream proxy address");
    host=self;host.sin_port=htons(host_port);
  }
  ~UdpProxy(){close(fd);}
  UdpProxy(const UdpProxy &)=delete;
  std::string address()const{return "127.0.0.1:"+std::to_string(ntohs(self.sin_port));}
  std::vector<std::vector<char>> pump() {
    std::vector<std::vector<char>> held;std::array<char,1501> bytes{};
    for(;;) {
      sockaddr_in from{};socklen_t size=sizeof(from);
      const auto n=recvfrom(fd,bytes.data(),bytes.size(),MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&from),&size);
      if(n<0)break;
      check(n<=1400,"Every native datagram stays within 1400 bytes");
      if(from.sin_port==host.sin_port){held.emplace_back(bytes.begin(),bytes.begin()+n);continue;}
      client=from;client_known=true;
      sendto(fd,bytes.data(),n,0,reinterpret_cast<const sockaddr*>(&host),sizeof(host));
    }
    return held;
  }
  void deliver(const std::vector<char> &packet) {
    check(client_known,"Stream proxy has no client endpoint");
    check(sendto(fd,packet.data(),packet.size(),0,reinterpret_cast<const sockaddr*>(&client),sizeof(client))==
        int(packet.size()),"Deliver host datagram");
    delivered_bytes+=packet.size()+28;
  }
};
bool same_positions(const MultiplayerState &a,const MultiplayerState &b) {
  for(unsigned i=0;i<a.players.size();++i) {
    const auto &x=a.players[i],&y=b.players[i];
    if(x.connected!=y.connected)return false;
    if(x.connected&&(std::bit_cast<std::uint32_t>(x.position.x)!=std::bit_cast<std::uint32_t>(y.position.x)||
        std::bit_cast<std::uint32_t>(x.position.y)!=std::bit_cast<std::uint32_t>(y.position.y)||
        std::bit_cast<std::uint32_t>(x.yaw)!=std::bit_cast<std::uint32_t>(y.yaw)||x.name!=y.name))return false;
  }
  return true;
}
#endif
// Acknowledged delta stream over real UDP: a multi-datagram keyframe joins
// out of order, a lost message is recovered from the acknowledged baseline,
// tampered and stale datagrams change nothing, the consumed raw look is named
// for client look prediction, and moving players receive every tick at a
// small fraction of complete-snapshot traffic.
void network_snapshot_stream() {
#ifndef _WIN32
  const auto content=arena();const auto spawn=starts();const auto selected=rules();
  {
    NetProfile profile("unlimited");
    MultiplayerMatch match(content,0,spawn,selected);
    std::mt19937 random(105);
    for(unsigned i=0;i<15;++i) {
      std::string name="Stream "+std::to_string(i);
      while(name.size()<5000)name.push_back(char('a'+random()%26));
      match.join(name);
    }
    LanHost host(match,0,654);
    UdpProxy proxy(host.port());
    LanClient client(proxy.address(),654,1);
    std::array<Controls,1> local{};std::array<Controls,16> controls{};
    client.send(local);proxy.pump();host.receive(controls);
    const auto challenge=proxy.pump();
    check(challenge.size()==1,"Raw join is challenged before its keyframe");
    proxy.deliver(challenge.front());client.receive();
    proxy.pump();host.receive(controls);
    const auto keyframe=proxy.pump();
    check(keyframe.size()>=2,"Incompressible roster keyframe uses several datagrams");
    proxy.deliver(keyframe.back());proxy.deliver(keyframe.back());client.receive();
    check(!client.ready(),"Incomplete and duplicate stream fragments cannot publish a roster");
    for(std::size_t i=keyframe.size()-1;i>0;--i)proxy.deliver(keyframe[i-1]);
    client.receive();
    check(client.ready()&&same_positions(client.state(),match.state()),"Out-of-order stream keyframe joins exactly");
    local[0].forward=1;
    const auto tick=[&](bool deliver) {
      client.send(local);proxy.pump();host.receive(controls);match.step(controls);host.broadcast();
      auto packets=proxy.pump();
      if(deliver){for(const auto &packet:packets)proxy.deliver(packet);client.receive();}
      return packets;
    };
    local[0].yaw=0.25f;local[0].pitch=0.125f;
    tick(true);
    check(client.state().tick==match.state().tick&&same_positions(client.state(),match.state()),
        "Delta follows the acknowledged keyframe exactly");
    const auto consumed=client.consumed_look(0);
    check(consumed&&std::bit_cast<std::uint32_t>((*consumed)[0])==std::bit_cast<std::uint32_t>(local[0].yaw)&&
        std::bit_cast<std::uint32_t>((*consumed)[1])==std::bit_cast<std::uint32_t>(local[0].pitch)&&!client.consumed_look(1),
        "Snapshot does not name the raw look its processed angles consumed");
    const auto before=client.state().tick;
    const auto lost=tick(false);
    check(!lost.empty()&&client.state().tick==before,"Lost delta leaves the accepted state intact");
    tick(true);
    check(client.state().tick==match.state().tick&&same_positions(client.state(),match.state()),
        "Delta after a lost message recovers from the acknowledged baseline");
    const auto genuine=tick(false);
    for(auto packet:genuine){packet[packet.size()/2]^=1;proxy.deliver(packet);}
    client.receive();
    check(client.state().tick!=match.state().tick,"Tampered stream datagrams cannot publish");
    for(const auto &packet:genuine)proxy.deliver(packet);
    client.receive();
    check(client.state().tick==match.state().tick&&same_positions(client.state(),match.state()),
        "Genuine datagrams survive tampered copies");
    for(const auto &packet:keyframe)proxy.deliver(packet);
    for(const auto &packet:lost)proxy.deliver(packet);
    client.receive();
    check(client.state().tick==match.state().tick&&client.error().empty(),"Stale stream datagrams cannot roll back");
  }
  struct Measure {std::uint64_t bytes{};double seconds{};unsigned states{},ticks{};bool exact{};};
  const auto measure=[&](const char *name,unsigned ticks) {
    NetProfile profile(name);
    MultiplayerMatch match(content,0,spawn,selected);
    std::vector<unsigned> bots;
    for(unsigned i=0;i<7;++i)bots.push_back(match.join("Bot "+std::to_string(i)));
    LanHost host(match,0,987);
    UdpProxy proxy(host.port());
    LanClient client(proxy.address(),987,1);
    std::array<Controls,1> local{};std::array<Controls,16> controls{};
    for(unsigned i=0;i<200&&!client.ready();++i) {
      client.send(local);proxy.pump();host.receive(controls);host.broadcast();
      for(const auto &packet:proxy.pump())proxy.deliver(packet);
      client.receive();std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    check(client.ready(),"Budget fixture client joins");
    std::map<std::uint32_t,MultiplayerState> history;
    Measure result;result.ticks=ticks;
    const auto start=std::chrono::steady_clock::now();const auto counted=proxy.delivered_bytes;
    for(unsigned n=0;n<ticks;++n) {
      std::this_thread::sleep_until(start+std::chrono::microseconds(33333)*(n+1));
      local[0].forward=1;local[0].yaw=0.05f*float(n);
      client.send(local);proxy.pump();host.receive(controls);
      for(unsigned b=0;b<bots.size();++b){controls[bots[b]].forward=1;controls[bots[b]].yaw=0.07f*float(n)+float(b);}
      match.step(controls);history.emplace(match.state().tick,match.state());host.broadcast();
      for(const auto &packet:proxy.pump())proxy.deliver(packet);
      client.receive();result.states+=unsigned(client.received_states().size());
    }
    result.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    result.bytes=proxy.delivered_bytes-counted;
    const auto found=history.find(client.state().tick);
    result.exact=found!=history.end()&&same_positions(client.state(),found->second)&&client.error().empty();
    return result;
  };
  const auto budgeted=measure("original",150),complete=measure("complete",150);
  const auto rate=[](const Measure &m){return double(m.bytes)*8/m.seconds/1000;};
  std::cout<<"Snapshot stream: original budget "<<rate(budgeted)<<" kbit/s, "<<budgeted.states<<"/"<<budgeted.ticks
      <<" states; complete snapshots "<<rate(complete)<<" kbit/s, "<<complete.states<<"/"<<complete.ticks<<" states\n";
  check(budgeted.exact&&complete.exact,"Budgeted and complete streams publish exact host states");
  // Game state reaches the client every tick; the budget bounds poses only.
  check(budgeted.states==budgeted.ticks,"Budgeted stream skipped a game-state tick");
  check(budgeted.bytes*20<complete.bytes,"Delta stream did not substantially reduce snapshot traffic");
#endif
}
void network_snapshot_fragments() {
#ifndef _WIN32
  // Disposable native transport fixture: this tests real UDP reassembly and
  // publication, not original gameplay behavior or Xbox protocol parity.
  // Complete snapshots remain the migration-candidate and fallback format.
  NetProfile complete_snapshots("complete");
  auto content=arena();auto spawn=starts();auto selected=rules();
  MultiplayerMatch match(content,0,spawn,selected);
  // Explicit transport stress names make this complete snapshot exceed the
  // previous65535-byte total without fabricating a solved source pose bank.
  for(unsigned i=0;i<15;++i)match.join("Roster "+std::to_string(i)+std::string(5000,'x'));
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
  // The raw join now requires the endpoint to echo a stateless challenge.
  sockaddr_in challenge_from{};socklen_t challenge_from_size=sizeof(challenge_from);
  const auto challenge_size=recvfrom(proxy.fd,bytes.data(),bytes.size(),MSG_DONTWAIT,
      reinterpret_cast<sockaddr*>(&challenge_from),&challenge_from_size);
  check(challenge_size==31&&std::uint8_t(bytes[6])==8,"Raw join did not challenge before its snapshot");
  sendto(proxy.fd,bytes.data(),challenge_size,0,reinterpret_cast<sockaddr*>(&client_address),sizeof(client_address));
  client.receive();size=sizeof(client_address);
  const auto proof_size=recvfrom(proxy.fd,bytes.data(),bytes.size(),MSG_DONTWAIT,
      reinterpret_cast<sockaddr*>(&client_address),&size);
  check(proof_size>0,"Actual client did not echo its admission challenge");
  sendto(proxy.fd,bytes.data(),proof_size,0,reinterpret_cast<sockaddr*>(&host_address),sizeof(host_address));
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
  // A forged duplicate fails authentication without poisoning genuine parts.
  match.step(controls);host.broadcast();const auto conflicting=receive();
  deliver(conflicting.front());auto corrupted=conflicting.front();corrupted.back()^=1;deliver(corrupted);
  for(std::size_t i=1;i<conflicting.size();++i)deliver(conflicting[i]);
  check(client.state().tick==3,"Unauthenticated duplicate poisoned a genuine complete snapshot");
  match.step(controls);host.broadcast();const auto recovered=receive();
  for(const auto &packet:recovered)deliver(packet);
  check(client.state().tick==4 && client.error().empty(),"Client recovers after conflicting fragments");
  // Forged tiny tails cannot allocate the declared multi-megabyte body.
  constexpr unsigned envelope=45,chunk=1400-35-LanPacketAuth::overhead,total=(8*1024*1024/chunk)*chunk+1;
  constexpr unsigned parts=(total+chunk-1)/chunk;
  for(unsigned sequence=1000;sequence<1500;++sequence){
    auto tail=first.front();tail.resize(envelope+36+32);
    const auto field=[&](unsigned at,unsigned value,unsigned size){for(unsigned i=0;i<size;++i)tail[envelope+at+i]=char(value>>(8*i));};
    field(23,sequence,4);field(27,parts-1,2);field(29,parts,2);field(31,total,4);
    deliver(tail);
    check(client.snapshot_buffered_bytes()<=4,"Tiny snapshot tails allocated a declared full body");
  }
  check(client.state().tick==4,"Incomplete hostile snapshot replaced the accepted world");
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
    if(argc==2 && std::string(argv[1])=="--admission-security") {
      raw_lan_admission_security();network();network_snapshot_fragments();
      std::cout<<"Raw UDP endpoint proof, authenticated controls/leave and reordered/tampered snapshot recovery passed\n";return 0;
    }
    if(argc==2 && std::string(argv[1])=="--asset-identity") {
      asset_identity(); mcc_asset_identity();
      std::cout<<"LAN identity authenticates relocated MCC caches, resources and fonts without an XBE\n";
      return 0;
    }
    if(argc==2 && std::string(argv[1])=="--vehicle-intent") {
      network_vehicle_intent();
      std::cout<<"Vehicle action and seat intent survives actual UDP without advancing physics\n";return 0;
    }
    if(argc==2 && std::string(argv[1])=="--request-ingress") {
      native_request_ingress();
      std::cout<<"Actual native host rejects malformed joins/inputs/leaves, wrong cookies and replay without partial four-seat mutation; healthy traffic survives\n";
      return 0;
    }
    if(argc==2 && std::string(argv[1])=="--session-config") {
      session_configuration();
      std::cout<<"Complete ordered host configuration survives UDP loss/reorder/duplicates without player admission\n";
      return 0;
    }
    if(argc==2 && std::string(argv[1])=="--session-config-loading") {
      session_configuration_while_loading();
      std::cout<<"Loading host configuration discovery leaves gameplay on its owner thread\n";return 0;
    }
    if(argc==2 && std::string(argv[1])=="--heavy-replica") {
      heavy_vehicle_replica_admission();
      std::cout<<"Heavy vehicle replicas construct; source-free authority stepping rejects before mutation\n";
      return 0;
    }
    if(argc==2 && std::string(argv[1])=="--crouch") {
      sneaking_presentation();
      std::cout<<"Sneaking stance follows actual capsule release and blocked stand-up\n";
      return 0;
    }
    if(argc==2 && std::string(argv[1])=="--snapshot-stream") {
      network_snapshot_stream();
      std::cout<<"Acknowledged delta snapshots join, recover and stay within the retail budget over actual UDP\n";
      return 0;
    }
    if(argc==2 && std::string(argv[1])=="--startup-delay") {
      network_startup_delay();
      std::cout<<"Actual UDP joined after preparation exceeded the connection deadline\n";
      return 0;
    }
    heavy_vehicle_replica_admission();
    session_configuration();
    sneaking_presentation();
    rules_and_match();
    asset_identity();
    mcc_asset_identity();
    network_vehicle_intent();
    native_request_ingress();
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
