#include "helpers/gameplay_fixture.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/hud.hpp"
#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
#ifndef _WIN32
struct Proxy {
  using Packet=std::vector<std::byte>;
  int fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  sockaddr_in host{},client{};
  explicit Proxy(unsigned port) {
    check(fd>=0,"Create actual UDP proxy");sockaddr_in at{};at.sin_family=AF_INET;at.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    check(bind(fd,reinterpret_cast<sockaddr*>(&at),sizeof(at))==0,"Bind actual UDP proxy");
    check(fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK)==0,"Set proxy nonblocking");
    host.sin_family=AF_INET;host.sin_addr.s_addr=htonl(INADDR_LOOPBACK);host.sin_port=htons(port);
  }
  ~Proxy(){if(fd>=0)close(fd);}
  unsigned port()const {
    sockaddr_in at{};socklen_t n=sizeof(at);check(getsockname(fd,reinterpret_cast<sockaddr*>(&at),&n)==0,"Read proxy port");return ntohs(at.sin_port);
  }
  std::vector<Packet> pump() {
    std::vector<Packet> replies;
    for(unsigned i=0;i<256;++i) {
      Packet packet(1401);sockaddr_in from{};socklen_t n=sizeof(from);
      const auto size=recvfrom(fd,packet.data(),packet.size(),0,reinterpret_cast<sockaddr*>(&from),&n);
      if(size<=0)break;
      packet.resize(std::size_t(size));check(size<=1400,"Snapshot exceeded datagram bound");
      if(from.sin_port==host.sin_port && from.sin_addr.s_addr==host.sin_addr.s_addr)replies.push_back(std::move(packet));
      else {client=from;check(sendto(fd,packet.data(),packet.size(),0,reinterpret_cast<const sockaddr*>(&host),sizeof(host))==size,"Relay real client request");}
    }
    return replies;
  }
  void deliver(const std::vector<Packet>& packets,bool reverse=true,bool duplicate=false,unsigned limit=~0u) {
    for(unsigned i=0;i<std::min<unsigned>(unsigned(packets.size()),limit);++i) {
      const auto& packet=packets[reverse?packets.size()-1-i:i];
      for(unsigned copy=0;copy<(duplicate?2u:1u);++copy)
        check(sendto(fd,packet.data(),packet.size(),0,reinterpret_cast<const sockaddr*>(&client),sizeof(client))==int(packet.size()),"Relay real objective snapshot");
    }
  }
};
std::array<float,3> interior(const SourceKingState& hill) {
  SourceKingPlayer player;player.datum=0;player.current_unit=0;
  for(const float z:{hill.center[2],(hill.minimum_z+hill.maximum_z)*.5f})
    for(int x=-9;x<=9;++x)for(int y=-9;y<=9;++y) {
      const std::array<float,3> point{hill.center[0]+hill.radius*float(x)*.1f,hill.center[1]+hill.radius*float(y)*.1f,z};
      if(source_king_contains(hill,player,[&](std::uint32_t){return point;}))return point;
    }
  throw FormatError("Authored hill has no sampled original-contains interior");
}
void run(const Cache& cache,const LevelContent& content,bool teams,bool moving) {
  auto rules=king_rules({"King objective transport",3,{{91,int(teams)},{40,moving?1:0},{2,moving?0:2},{0x59,5}}},true);
  rules.weapon=cache.slayer_starting_weapon(5,4);rules.time_ticks=0;
  const auto hud=cache.load_hud(content.player.datum,rules.weapon);
  unsigned records{},changes{},notices{};
  MultiplayerKingServices services;
  // These observe existing mandatory boundaries; this suite does not claim
  // enabled recorder/HUD/announcer consumers or normal menu admission.
  services.record=[&](const MultiplayerMatch&,std::uint32_t,std::int32_t,std::uint32_t){++records;};
  services.score_changed=[&](const MultiplayerMatch&,std::uint32_t,std::int32_t){++changes;};
  services.notification=[&](const MultiplayerMatch&,const SourceKingNotice&){++notices;};
  MultiplayerMatch match(content,0,cache.spawns(),rules,services);
  for(unsigned i=0;i<15;++i)match.join("Full King roster "+std::to_string(i)+std::string(60,'x'),teams?0:-1);
  LanHost host(match,0,0x4b494e47);Proxy proxy(host.port());std::array<std::int8_t,1> team{0};
  LanClient client("127.0.0.1:"+std::to_string(proxy.port()),0x4b494e47,1,teams?std::span<const std::int8_t>(team):std::span<const std::int8_t>{});
  std::array<Controls,1> local{};std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned i=0;i<32 && !client.ready();++i) {
    client.send(local);proxy.pump();host.receive(controls);host.broadcast();proxy.deliver(proxy.pump(),true,true);client.receive();
  }
  check(client.ready() && client.error().empty() && client.slots().size()==1,"Actual King client did not join");
  check(client.state().source_engine==4 && client.state().king.has_value(),"King joined without objective state");
  std::vector<SourceKingMarker> markers;
  for(const auto& row:content.netgame_flags) {SourceKingMarker marker;std::memcpy(&marker,row.source_bytes.data(),32);markers.push_back(marker);}
  SourceKingState replica;SourceKingHills hills;source_king_initialize(replica,hills,markers);
  std::vector<Proxy::Packet> stale;unsigned full_masks{},relocations{},fragments{};auto previous_hill=match.source_king_state()->hill;
  const auto compare=[&] {
    const auto actual=match.state();const auto& received=client.state();
    check(received.tick==actual.tick && received.source_engine==actual.source_engine && received.king.has_value(),"Complete objective snapshot tick/engine differs");
    check(received.round_mode==actual.round_mode,"Authoritative round selector changed across UDP");
    check(received.king->hill==actual.king->hill && received.king->occupants==actual.king->occupants,"Authoritative hill/mask changed across UDP");
    check(received.finished==actual.finished && received.results_finalized==actual.results_finalized && received.winner==actual.winner &&
      received.winner_team==actual.winner_team && received.team_scores==actual.team_scores,"Objective match completion/results changed across UDP");
    source_king_build_hill(replica,markers,received.king->hill);replica.hill=received.king->hill;replica.occupants=received.king->occupants;
    const auto& source=*match.source_king_state();
    check(std::memcmp(reinterpret_cast<const std::uint8_t*>(&replica),reinterpret_cast<const std::uint8_t*>(&source),0x180)==0,
      "Client authored hill reconstruction differs from actual native source geometry");
    for(unsigned i=0;i<multiplayer_capacity;++i) {
      check(received.players[i].source_king_inside_ticks==actual.players[i].source_king_inside_ticks && received.players[i].score==actual.players[i].score &&
        received.players[i].source_player==actual.players[i].source_player,"King timer/score/full player identity changed across UDP");
    }
    for(const unsigned slot:{0u,client.slots().front()}) {
      const auto local_hud=multiplayer_hud_score_display(actual,slot,actual.score_limit,actual.time_ticks,hud);
      const auto remote_hud=multiplayer_hud_score_display(received,slot,received.score_limit,received.time_ticks,hud);
      check(local_hud.status==SourceHudScoreStatus::Ready && remote_hud.status==local_hud.status &&
          local_hud.score_text==remote_hud.score_text && local_hud.rival_text==remote_hud.rival_text &&
          local_hud.limit==remote_hud.limit,"Actual King objective scores did not reach the same source HUD over UDP");
      if(local_hud.score>=0 && local_hud.score<60)
        check(local_hud.score_text.size()==4 && local_hud.score_text[1]==u':',"Actual King HUD lost source time formatting");
    }
  };
  for(unsigned tick=0;tick<100;++tick) {
    const auto point=interior(*match.source_king_state());Spawn spawn;spawn.position={point[0],point[1],point[2]};
    // Explicit position service input isolates objective delivery from normal
    // movement/spawn/HUD verification. All actual gameplay ticks still run.
    for(unsigned i=0;i<multiplayer_capacity;++i)test::place_player(match,i,spawn);
    client.send(local);proxy.pump();host.receive(controls);proxy.pump();match.step(controls);host.broadcast();
    auto packets=proxy.pump();fragments=std::max(fragments,unsigned(packets.size()));check(packets.size()>1,"Full King roster was not fragmented");
    if(tick==29) {
      stale=packets;const auto before=client.state().tick;proxy.deliver(packets,false,true,1);client.receive();
      check(client.state().tick==before,"Partial objective snapshot published hill/timers");continue;
    }
    proxy.deliver(packets,tick%2,tick%3==0);client.receive();compare();
    if(tick==31){proxy.deliver(stale,true,true);client.receive();compare();}
    full_masks+=client.state().king->occupants==0xffffu;
    if(match.source_king_state()->hill!=previous_hill){++relocations;previous_hill=match.source_king_state()->hill;}
    if(tick==20) {
      // Reuse an occupied slot while the match admits joins. The stationary
      // case later reaches game over, where join is correctly rejected.
      check(match.player(0).source_king_inside_ticks>0,"Departing King player never acquired a timer");
      const auto old=match.player(0).source_datum;match.leave(0);host.broadcast();proxy.deliver(proxy.pump());client.receive();
      check(!client.state().players[0].connected,"King departed player retained live snapshot presence");
      check(match.join("Replacement King player",teams?0:-1)==0 && match.player(0).source_datum!=old,"Native King slot did not acquire a new generation");
      host.broadcast();proxy.deliver(proxy.pump());client.receive();
      check(client.state().players[0].source_player==match.player(0).source_datum && client.state().players[0].source_king_inside_ticks==0,
        "Reused King slot retained a departed timer or identity");
    }
  }
  check(full_masks>20 && fragments>1 && changes>0,"King occupied roster/scoring did not reach transport");
  if(moving)check(relocations>0 && notices>0 && !match.finished(),"Actual moving King did not relocate across UDP");
  else check(match.finished() && client.state().results_finalized && client.state().game_over_countdown<210,"Actual stationary King did not finish over UDP");
  if(!teams)check(records>0,"FFA recording boundary was not exercised");
  std::cout<<"King UDP teams="<<teams<<" moving="<<moving<<" full_masks="<<full_masks<<" relocations="<<relocations<<" max_fragments="<<fragments<<'\n';
}
#endif
}
int main(int argc,char** argv)try {
#ifdef _WIN32
  return 77;
#else
  check(argc==4,"Supply original MAP SHARED_MAPS UPDATE_XBE");
  // This fixture drops, duplicates and replays fragments of complete
  // snapshots; delta-stream delivery is covered by native_lan_snapshot_stream.
  setenv("H2_NET_PROFILE","complete",1);
  const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));const auto content=cache.load_content(0,true);
  for(const bool teams:{false,true})for(const bool moving:{false,true})run(cache,content,teams,moving);
  std::cout<<"PASS actual Lockout King objective UDP state, original hill reconstruction, 16-player timers, loss/reorder/duplicate/stale fragments, scoring, completion and slot reuse\n";
  std::cout<<"LIMIT explicit debug position inputs and observed recording/presentation callbacks; no normal King admission, Xbox wire interoperability or complete playable-match claim\n";
  return 0;
#endif
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
