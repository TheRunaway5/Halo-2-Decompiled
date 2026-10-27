#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "helpers/multiplayer_snapshot_checks.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
static void check(bool v,const char *message){if(!v)throw FormatError(message);}
int main(int argc,char **argv)try{
  check(argc==3,"Supply prepared MCC map and bounded report path");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Round UDP test needs owned scratch");
  MultiplayerOptions options;options.map=argv[1];options.source_object_world=true;options.local_players=1;
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"round-network";
  std::filesystem::create_directories(options.source_object_savegame_directory);
  options.variant={"Slayer",0,{}};options.variant.values.set(0,1);options.variant.values.set(10,1);
  options.variant.values.set(2,0); // Actual original two-round, timed rule setters.
  MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;
  check(match.source_engine_owned(),"Round UDP test selected a bounded fallback engine");
  LanHost host(match,0,0x7475313500111ull,options.variant);
  LanClient peer(host.join_address(),0x7475313500111ull,1);
  std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,1> remote{};
  const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(!peer.ready() && std::chrono::steady_clock::now()<until){
    peer.send(remote);host.receive(controls);host.broadcast();peer.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(peer.ready() && host.peers()==1,"Real UDP round participant did not join");
  bool intermission{},preparation{},second_round{},finished{};unsigned frames{},received{};
  nlohmann::json phases=nlohmann::json::array();std::int16_t previous=-1;
  for(;frames<800;++frames){
    //15ED59 observes this entry tick/phase before the pending round transition.
    const auto fade_tick=match.state().tick;
    const auto fade_phase=match.source_round_lifecycle().phase;
    controls[prepared.local_slots.front()].yaw=match.unit(prepared.local_slots.front()).yaw;
    peer.send(remote);host.receive(controls);match.step(controls);host.broadcast();peer.receive();
    const auto actual=match.state();const auto &source=match.source_round_lifecycle();
    check(actual.source_round_lifecycle_available && actual.round_phase==source.phase &&
      actual.round_index==source.index && actual.round_countdown==source.countdown,
      "Authority published guessed round lifecycle values");
    check(actual.source_gameplay_fade && actual.source_gameplay_fade->newest && !actual.source_gameplay_fade->slots.empty(),
      "Actual source15ED59 did not publish its pointer-free fade inputs");
    const auto &fade=*actual.source_gameplay_fade;
    validate_source_gameplay_fade_journal_tu15(fade);
    const auto &latest=fade.slots[(fade.newest-1)%source_gameplay_fade_history_tu15];
    check(latest.tick==fade_tick && latest.phase==fade_phase && latest.rate==30 && latest.engine_present,
      "Native viewport fade observation differs from its actual source lifecycle/rate");
    for(unsigned slot=0;slot<actual.players.size();++slot)if(actual.players[slot].connected) {
      check(fade.current[slot].player==actual.players[slot].source_player && fade.current[slot].activation>0,
        "Actual local or remote player init/unit-bind omitted its original fade lifecycle owner");
      check(latest.identities[slot].player==actual.players[slot].source_player && latest.players[slot].present,
        "Original pre-fade tick lost an admitted full salted player");
    }
    if(actual.round_phase!=previous){phases.push_back({{"tick",actual.tick},{"phase",actual.round_phase},
      {"index",actual.round_index},{"countdown",actual.round_countdown},{"finished",actual.finished}});previous=actual.round_phase;}
    intermission|=!actual.finished && actual.round_phase==2;
    preparation|=!actual.finished && actual.round_phase==3;
    second_round|=!actual.finished && actual.round_phase==1 && actual.round_index==1;
    if(peer.state().tick==actual.tick){test::verify_multiplayer_snapshot(actual,peer.state());
      check(peer.state().source_gameplay_fade==actual.source_gameplay_fade,"Actual UDP snapshot changed original viewport fade inputs");++received;}
    if(actual.game_over_complete()){finished=true;break;}
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(intermission && preparation && second_round && finished && received>10,
    "Actual timed source rounds/UDP lifecycle did not complete intermission, preparation, next round and final game over");
  const auto final=match.state();const auto received_until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(peer.state().tick!=final.tick && std::chrono::steady_clock::now()<received_until){
    peer.send(remote);host.receive(controls);host.broadcast();peer.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  test::verify_multiplayer_snapshot(final,peer.state());
  check(peer.state().source_gameplay_fade==final.source_gameplay_fade,"Final UDP receipt lost exact retained original fade history");
  check(peer.state().game_over_complete(),"UDP peer did not receive final original game-over completion");
  nlohmann::json report={{"passed",true},{"protocol",native_multiplayer_protocol},{"ordinary_frames",frames},
    {"matching_udp_publications",received},{"phases",phases},{"source_fade_frames",final.source_gameplay_fade->newest},
    {"source_fade_local_remote_lifecycle",true},{"source_fade_udp_exact",true},
    {"scope","Actual prepared MCC TU1.5 source world, original timed two-round rule setters and real UDP peer. Every published lifecycle field compared to actual engine after each frame; no winner/phase/countdown supplied. Rendered overlay/PCR and WAN acceptance separate."}};
  std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';check(bool(out),"Could not retain round UDP report");
  std::cout<<"PASS actual TU1.5 two-round world -> UDP intermission/preparation/next round -> final game over; protocol "<<native_multiplayer_protocol<<'\n';
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
