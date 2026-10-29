// Exercise the actual private snapshot codec and acknowledged transport.
#include "../src/network/multiplayer_network.cpp"
#include <iostream>
#include <limits>
using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
Packet encode(const MultiplayerState& state,std::span<const unsigned> ids,unsigned version=native_multiplayer_protocol,
    SnapshotBodyLayout* layout=nullptr) {
  Packet bytes;encode_state(bytes,state,ids,layout,version);return bytes;
}
MultiplayerState decode(const Packet& bytes,std::span<const unsigned> ids,unsigned version=native_multiplayer_protocol) {
  Reader reader{bytes};auto value=decode_state(reader,ids,version);reader.end();return value;
}
auto camera_words(const NativeDeathCameraView& camera) {
  const auto& v=camera;std::array<std::uint32_t,12> words{};unsigned n{};
  for(float value:{v.position.x,v.position.y,v.position.z,v.forward.x,v.forward.y,v.forward.z,
      v.up.x,v.up.y,v.up.z,v.horizontal_fov,v.aim_displacement})words[n++]=std::bit_cast<std::uint32_t>(value);
  words[n]=v.camera_mode;return words;
}
}
int main()try {
  MultiplayerState state;state.tick=8;state.devices.tick=state.tick;state.source_game_type=2;state.source_engine=2;
  state.source_round_lifecycle_available=true;state.round_phase=1;state.round_index=0;state.round_countdown=0;
  for(unsigned n=0;n<2;++n) {
    auto& player=state.players[n];player.connected=true;player.source_player=0x10000u+n;
    player.source_unit=0x20000u+n;player.source_membership=std::int16_t(n);player.source_ffa_identity=std::uint8_t(n);
    player.source_player_camera=NativeDeathCameraView{{float(n)+.125f,2.5f,3.75f},{1,0,0},{0,0,1},1.2f,-0.f,0};
  }
  const std::array ids0{0u},ids1{1u};
  const auto bytes=encode(state,ids0);const auto received=decode(bytes,ids0);
  need(received.players[0].source_player_camera.has_value(),
      "Native snapshot dropped the completed ordinary player eye/head correction");
  need(!received.players[1].source_player_camera,"Ordinary player camera escaped its assigned recipient");
  need(camera_words(*received.players[0].source_player_camera)==camera_words(*state.players[0].source_player_camera),
      "Ordinary player camera lost source float bits, basis, FOV or mode");
  need(encode(received,ids0)==bytes,"Ordinary camera snapshot did not roundtrip exactly");
  need(native_multiplayer_protocol>=120,"Ordinary player camera requires the explicit protocol120 extension");
  unsigned rejected{};
  const auto reject=[&](auto&& call){bool failed{};try{call();}catch(const FormatError&){failed=true;}
    need(failed,"Invalid ordinary camera packet was admitted");++rejected;};
  reject([&]{(void)decode(bytes,ids1);});
  auto absent=state;for(auto& player:absent.players)player.source_player_camera.reset();
  for(unsigned version=110;version<120;++version) {
    need(encode(state,ids0,version)==encode(absent,ids0,version),"Ordinary eye extension changed a legacy snapshot extent");
    need(!decode(encode(state,ids0,version),ids0,version).players[0].source_player_camera,
        "A legacy snapshot fabricated an ordinary native camera");
  }
  reject([&]{(void)decode(encode(state,ids0,119),ids0,120);});
  reject([&]{(void)decode(bytes,ids0,119);});
  const auto no_camera=encode(absent,ids0);std::size_t camera_presence{};
  while(camera_presence<no_camera.size() && no_camera[camera_presence]==bytes[camera_presence])++camera_presence;
  need(camera_presence<no_camera.size() && no_camera[camera_presence]==std::byte{} &&
      bytes[camera_presence]==std::byte{1},"Ordinary camera has no exact presence boundary");
  auto malformed=bytes;malformed[camera_presence]=std::byte{2};reject([&]{(void)decode(malformed,ids0);});
  for(unsigned condition=0;condition<12;++condition) {
    auto bad=state;auto& player=bad.players[0];auto& camera=*player.source_player_camera;
    switch(condition) {
      case 0:camera.position.x=std::numeric_limits<float>::infinity();break;
      case 1:camera.aim_displacement=std::numeric_limits<float>::quiet_NaN();break;
      case 2:camera.forward={};break;
      case 3:camera.up=camera.forward;break;
      case 4:camera.horizontal_fov=0;break;
      case 5:camera.camera_mode=1;break;
      case 6:player.dead=true;break;
      case 7:player.source_unit=null_datum;break;
      case 8:player.source_player=null_datum;break;
      case 9:player.source_parent=0x30001u;break;
      case 10:player.source_seat=0;break;
      case 11:player.source_vehicle_camera=camera;break;
    }
    reject([&]{(void)encode(bad,ids0);});
  }
  auto packet=header(input);packet[4]=std::byte(119);packet[5]=std::byte{};
  reject([&]{Reader reader{packet};(void)reader.type();});
  SnapshotBodyLayout layout;const auto stream_body=encode(state,ids0,native_multiplayer_protocol,&layout);
  SnapshotStreamSender sender(1292,80);SnapshotStreamReceiver receiver(1292);
  const SnapshotStreamSender::Frame frame{stream_body,layout.floats,layout.anchor_offsets,layout.anchor_keys,
      layout.poses,1,1,1000,{},{},{}};
  const auto emission=sender.encode(frame);need(emission && emission->emitted,"Ordinary camera stream emitted no frame");
  bool delivered{};
  for(const auto& packet:emission->packets) {
    bool failed{};auto value=receiver.push(packet,1000,failed);need(!failed,"Ordinary camera stream rejected its actual frame");
    if(!value)continue;
    const auto decoded=decode(value->body,ids0);
    need(decoded.players[0].source_player_camera.has_value() && !decoded.players[1].source_player_camera,
        "Acknowledged snapshot lost the recipient's completed player camera");
    need(length(decoded.players[0].source_player_camera->position-state.players[0].source_player_camera->position)<.003f,
        "Acknowledged ordinary eye exceeded native stream coordinate precision");
    need(receiver.commit(value->sequence),"Ordinary camera receipt did not commit");delivered=true;
  }
  need(delivered,"Ordinary camera stream fragments never completed");
  std::cout<<"PASS ordinary player eye: exact production codec, recipient isolation, legacy110..119, "
      <<rejected<<" invalid controls and acknowledged native transport\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
