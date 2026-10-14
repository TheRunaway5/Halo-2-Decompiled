#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "helpers/multiplayer_snapshot_checks.hpp"
#include <chrono>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {void check(bool v,const char *s){if(!v)throw FormatError(s);}}
int main(int argc,char **argv)try {
  if(argc!=3)throw FormatError("Supply TU15_XBE TURF_MAP");
  if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
  MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];
  options.shared_maps=std::filesystem::path(argv[2]).parent_path().parent_path().parent_path()/"disc/maps";
  options.local_players=1;options.starting_weapon=5;options.mute=true;
  MultiplayerPreparedWorld world(options);const auto player=world.match.join("Effect authority");
  const auto initial=world.match.state();world.source_objects->set_local_player(initial.players[player].source_player,0);
  world.source_objects->publish_native_observer_position(0,initial.players[player].position+Vec3{0,0,.6f});
  LanHost host(world.match,0,0x17c370);const auto address="127.0.0.1:"+std::to_string(host.port());
  LanClient first(address,0x17c370,1);
  std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,4> neutral{};
  const auto join=[&](LanClient &client,unsigned count) {
    for(unsigned i=0;i<128 && !client.ready();++i) {
      client.send(std::span(neutral).first(count));host.receive(controls);host.broadcast();client.receive();
      if(!client.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    check(client.ready(),"Actual UDP effect peer failed to join");
    test::verify_multiplayer_snapshot(world.match.state(),client.state());
  };
  join(first,1);world.match.step(controls);host.broadcast();first.receive();
  test::verify_multiplayer_snapshot(world.match.state(),first.state());
  std::uint32_t object=null_datum,definition=null_datum;std::size_t retained{};
  for(const auto &root:first.state().source_scene_roots) {
    const auto metadata=world.cache->load_source_tag_metadata(root.definition);
    if(metadata.size()<0x9c)continue;
    const Bytes def(metadata.bytes_at(metadata.source_address(),0x9c));
    if(!def.u32(0x94))continue;
    const Bytes rows(world.cache->read_tag_bytes(def.u32(0x98),std::size_t(def.u32(0x94))*24));
    for(const auto &effect:root.attached_effects) {
      ++retained;
      if(object==null_datum && effect.instance!=null_datum && rows.u32(effect.slot*24+16)) {
        object=root.object;definition=effect.definition;
      }
    }
  }
  check(object!=null_datum && retained,"Actual source effect allocations did not reach UDP");
  const auto packet=[&](const MultiplayerState &state)->const SourceAttachedEffect& {
    for(const auto &root:state.source_scene_roots)if(root.object==object)
      for(const auto &effect:root.attached_effects)if(effect.definition==definition)
        if(effect.instance!=null_datum)return effect;
    throw FormatError("UDP omitted the actual retained source effect allocation");
  };
  check((packet(first.state()).flags&4u) && packet(first.state()).scale_a==0 && packet(first.state()).scale_b==1,
      "Original undamaged Turf function was not paused");
  auto vitality=world.source_objects->object_vitality(object);vitality.feedback.body={.75f,.75f,30};
  world.source_objects->publish_native_observer_position(0,world.source_objects->object_transform(object).position+Vec3{0,0,1});
  world.source_objects->deliver_vitality(object,vitality);world.match.step(controls);
  host.broadcast();first.receive();test::verify_multiplayer_snapshot(world.match.state(),first.state());
  check(!(packet(first.state()).flags&0xcu) && packet(first.state()).scale_a>0,
      "Completed original damage function did not resume over UDP");
  vitality=world.source_objects->object_vitality(object);vitality.feedback.body={};
  world.source_objects->deliver_vitality(object,vitality);world.match.step(controls);
  host.broadcast();first.receive();test::verify_multiplayer_snapshot(world.match.state(),first.state());
  check(packet(first.state()).flags&4u,"Completed original function pause was not replicated");
  LanClient late(address,0x17c370,4);join(late,4);
  check(late.slots().size()==4,"Late effect peer did not retain four local players");
  test::verify_multiplayer_snapshot(world.match.state(),late.state());
  vitality.feedback.body={.5f,.5f,30};world.source_objects->deliver_vitality(object,vitality);
  for(unsigned i=0;i<3;++i)world.match.step(controls);
  host.broadcast();first.receive();late.receive();
  const auto expected=world.match.state();test::verify_multiplayer_snapshot(expected,first.state());
  test::verify_multiplayer_snapshot(expected,late.state());
  check(!(packet(late.state()).flags&4u) && packet(late.state()).scale_a>0,
      "Late peer did not receive original function resume");
  host.broadcast();first.receive();late.receive();
  test::verify_multiplayer_snapshot(expected,first.state());test::verify_multiplayer_snapshot(expected,late.state());
  std::cout<<"PASS real UDP original attached-effect identities/flags/controls/colors/local frames, pause/resume, duplicate delivery and four-player late join; particle history and independent client PVS remain separate\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
