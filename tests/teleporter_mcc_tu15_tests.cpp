#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_teleporter.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_codec.hpp"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
float distance_squared(Vec3 a,Vec3 b){const auto x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
nlohmann::json vector(Vec3 v){return {v.x,v.y,v.z};}
struct Files {
  std::filesystem::path path;
  Files(){auto text=(std::filesystem::temp_directory_path()/"halo2-teleporter-live-XXXXXX").string();
    const auto *created=::mkdtemp(text.data());check(created,"Cannot create teleporter test save directory");path=created;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  if(argc!=2)return 2;
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  const Cache cache(argv[1]);check(cache.is_mcc(),"Live teleporter acceptance requires prepared MCC assets");
  const auto content=cache.load_content(0,true);
  SourceTeleporterBank pairs;source_teleporter_initialize(pairs,content.netgame_flags);
  check(pairs.count>0,"MCC test map has no authored teleporter pair");
  const auto pair_index=std::getenv("HALO2_TELEPORTER_PAIR")?std::stoul(std::getenv("HALO2_TELEPORTER_PAIR")):0u;
  check(pair_index<unsigned(pairs.count),"Requested teleporter pair is not authored on this map");
  const auto &pair=pairs.pairs[pair_index];
  const auto &entrance=content.netgame_flags.at(std::size_t(pair.entrance));
  const auto &exit=content.netgame_flags.at(std::size_t(pair.exit));
  const bool approach=std::getenv("HALO2_TELEPORTER_APPROACH")!=nullptr;
  Files files;MultiplayerObjectRuntime world(cache,0,files.path);
  unsigned blocked_player_stores{};
  world.observe_physics_stores([&](auto pc,const auto& store) {
    if(pc==0x1a67fcu) {
      check(store.written.size()==1 && (std::to_integer<unsigned>(store.written[0])&4u),
          "Original occupied-exit player store lost flag4");
      ++blocked_player_stores;
    }
  });
  std::vector<std::string> sound_requests;
  world.observe_sound_submissions([&](bool before,std::uint32_t sound,auto,const auto&,const auto&,auto,auto) {
    if(before)if(const auto *tag=cache.find_tag(TagHandle{sound}))sound_requests.push_back(tag->name);
  });
  const UiVariant variant{"Slayer",0,{{89,5}}};
  auto rules=slayer_rules(variant);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  world.configure_session(source_game_variant(variant),1,30,rules.random_seed);
  check(!cache.spawns().empty(),"MCC map has no actual multiplayer start definition");
  // Explicit initial placement input at the real original birth boundary.
  // The player still runs ordinary Match::step and actual source physics;
  // no expected pose/contact/query outcome is injected after construction.
  std::array<Spawn,1> starts{cache.spawns().front()};
  starts[0].position=entrance.position;starts[0].facing=entrance.facing;
  if(approach) {
    // A pad's exit-facing angle need not face its walkable approach. A
    // declared route can choose another initial facing before source birth.
    if(const auto *yaw=std::getenv("HALO2_TELEPORTER_APPROACH_YAW"))starts[0].facing=std::stof(yaw);
    starts[0].position.x-=1.25f*std::cos(starts[0].facing);
    starts[0].position.y-=1.25f*std::sin(starts[0].facing);
  }
  MultiplayerMatch match(content,0,starts,rules,{}, {},&world);
  const auto player=match.join("TU1.5 teleporter entrance");
  const auto local_view=std::getenv("HALO2_TELEPORTER_LOCAL_VIEW")?std::stoul(std::getenv("HALO2_TELEPORTER_LOCAL_VIEW")):0u;
  check(local_view<4,"Requested teleporter local view exceeds the original four-view bank");
  std::vector<unsigned> locals;
  for(unsigned view=0;view<local_view;++view)locals.push_back(match.join("Additional original local view"));
  locals.push_back(player);
  const auto first_person=cache.load_first_person_binding(rules.weapon,2);
  match.configure_replicated_first_person(first_person);
  match.configure_local_views(locals,first_person);
  SourceMatchAuthority authority(match,world);
  const auto object=match.unit(player).source_datum;
  const auto before=world.biped_motion(object);
  check(distance_squared(before.position,starts[0].position)<.01f,"Declared birth was not admitted by the real source world");
  std::array<Controls,multiplayer_capacity> controls{};
  controls[player].yaw=match.unit(player).yaw;controls[player].pitch=match.unit(player).pitch;
  if(approach) {
    check(distance_squared(before.position,entrance.position)>.25f,"Approach began inside the teleporter");
    controls[player].forward=1;
    for(unsigned n=0;n<120 && !match.player(player).source_teleport_serial;++n)match.step(controls);
    controls[player].forward=0;
  } else match.step(controls);
  const auto after=world.biped_motion(object);
  nlohmann::json result{{"map",cache.name()},{"entrance",vector(entrance.position)},
    {"exit",vector(exit.position)},{"before",vector(before.position)},{"after",vector(after.position)},
    {"source_unit",object},{"tick",match.state().tick}};
  std::cout<<result.dump()<<'\n';
  std::cout<<nlohmann::json{{"original_sound_requests",sound_requests}}.dump()<<'\n';
  check(std::find(sound_requests.begin(),sound_requests.end(),
      "sound\\game_sfx\\multiplayer\\teleporter_activate")!=sound_requests.end(),
      "Original event22 did not select its actual MCC activation sound");
  check(distance_squared(after.position,exit.position)<distance_squared(after.position,entrance.position),
      "RED: normal MCC multiplayer tick did not traverse its authored teleporter");
  check(distance_squared(match.unit(player).body.position,after.position)<.000001f,
      "Multiplayer readback lost the completed source teleporter pose");
  check(match.state().players[player].source_teleport_serial==1,"Completed source teleport was not published");
  const auto activation_sound=match.player(player).source_teleport_sound;
  const auto *activation_tag=cache.find_tag(TagHandle{activation_sound});
  check(activation_tag && activation_tag->name=="sound\\game_sfx\\multiplayer\\teleporter_activate",
      "Native teleporter audio publication lost the actual source selector result");
  const auto heading=std::atan2(after.forward.y,after.forward.x);
  for(unsigned n=0;n<4;++n)match.step(controls);
  check(match.player(player).source_teleport_serial==1,"Original latch/cooldown allowed a repeated teleport");
  check(std::abs(std::remainder(match.state().players[player].yaw-heading,6.28318530718f))<.0001f,
      "Native input restored the pre-teleport heading");
  if(approach || std::getenv("HALO2_TELEPORTER_LOCAL_ONLY")) {
    std::cout<<"PASS MCC traversal, original event22, exit heading and no bounce; pair="<<pair_index
        <<" local_view="<<local_view<<" approach="<<approach<<'\n';
    return 0;
  }
  LanHost host(match,0,0x1568u);LanClient client("127.0.0.1:"+std::to_string(host.port()),0x1568u,1);
  std::array<Controls,1> remote{};
  for(unsigned n=0;n<20 && !client.ready();++n) {
    client.send(remote);host.receive(controls);host.broadcast();client.receive();
  }
  check(client.ready() && client.error().empty(),"Native UDP teleporter client did not join");
  const auto remote_player=client.slots().front();
  const auto remote_object=match.unit(remote_player).source_datum;
  check(distance_squared(world.biped_motion(remote_object).position,entrance.position)<.01f,
      "Remote player's declared entrance birth was not admitted");
  remote[0].yaw=client.state().players[remote_player].yaw;
  remote[0].pitch=client.state().players[remote_player].pitch;
  const auto network_step=[&](bool broadcast) {
    client.send(remote);host.receive(controls);match.step(controls);
    if(broadcast){host.broadcast();client.receive();check(client.error().empty(),"Native teleporter snapshot was rejected");}
  };
  for(unsigned n=0;n<6;++n)network_step(true);
  check(match.state().players[remote_player].source_teleport_serial==0,
      "Original occupied-exit query admitted the blocked remote player");
  check(blocked_player_stores>0,"Occupied exit did not run its original player flag4 store");
  const auto notices=source_hud_message_lines(match.player(remote_player).source_messages,
      match.state().tick,tick_seconds,1000,1,4);
  check(!notices.empty() && !notices.front().text.empty(),"Actual remote blocked event lost its authored message");
  check(client.state().players[remote_player].source_messages==match.player(remote_player).source_messages,
      "UDP recipient lost its actual blocked-message queue");
  check(match.player(player).source_messages==SourceHudMessages{},"Remote blocked notice leaked into the host viewport");
  {
    const SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15},std::byte{0x68}};
    const auto saved=decode_source_authority(encode_source_authority(authority.checkpoint(binding)));
    auto &memory=world.memory();
    const auto engine=Bytes(memory.view(0x4f63b8u,4)).u32(0);
    std::array<std::byte,0x104> before_bank{},after_bank{};
    memory.read(engine+0xafcu,before_bank);
    const auto before_messages=match.player(remote_player).source_messages;
    auto prepared=authority.prepare(saved,binding);
    check(authority.commit(std::move(prepared)),"Coupled teleporter continuation did not commit");
    memory.read(engine+0xafcu,after_bank);
    check(before_bank==after_bank && match.player(player).source_teleport_serial==1 &&
        match.player(player).source_teleport_sound==activation_sound && match.player(remote_player).source_teleport_serial==0 &&
        match.player(remote_player).source_messages==before_messages,
        "Continuation changed original teleporter timers/latches or native completion serials");
  }
  // The local blocker walks away through ordinary controls. No position,
  // contact, latch or query-result store is supplied by the test.
  controls[player].forward=1;
  for(unsigned n=0;n<90 && !match.state().players[remote_player].source_teleport_serial;++n)network_step(false);
  controls[player].forward=0;
  check(match.state().players[remote_player].source_teleport_serial==1,
      "Remote player did not traverse after the blocker walked clear");
  const auto remote_heading=match.state().players[remote_player].yaw;
  for(unsigned n=0;n<4;++n)network_step(false);
  host.broadcast();client.receive();
  check(client.error().empty() && client.state().players[remote_player].source_teleport_serial==1 &&
      client.state().players[remote_player].source_teleport_sound==activation_sound &&
      distance_squared(client.state().players[remote_player].position,match.state().players[remote_player].position)<.000001f,
      "Completed remote traversal did not survive omitted UDP snapshots");
  check(std::abs(std::remainder(match.state().players[remote_player].yaw-remote_heading,6.28318530718f))<.0001f,
      "Repeated native UDP input restored the remote pre-teleport heading");
  std::cout<<"PASS normal MCC local/remote traversal, occupied exit, real movement release, retained heading and UDP snapshot loss\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
