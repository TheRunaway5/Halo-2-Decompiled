#include "halo2/multiplayer_network.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool v,const char *s) { if (!v) throw FormatError(s); }
}
int main(int argc,char **argv) {
  try {
    if (argc!=3) throw FormatError("Supply original map and player identity fixture");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    std::ifstream file(argv[2]);std::string first_word,next_word;
    file>>first_word>>next_word;
    check(bool(file),"Missing original player identity outputs");
    const auto first=std::uint32_t(std::stoul(first_word,nullptr,16));
    const auto next=std::uint32_t(std::stoul(next_word,nullptr,16));
    Cache cache(argv[1]);auto content=cache.load_content(0,true);
    content.kill_triggers.clear(); // recorded ownership-release boundary
    auto rules=slayer_rules({"Identity source",0,{{0x59,5}}});
    rules.weapon=cache.slayer_starting_weapon(5);rules.respawn_ticks=30;
    rules.time_ticks=rules.score_limit=0;
    for (bool lan:{false,true}) {
      MultiplayerMatch match(content,0,cache.spawns(),rules);
      std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
      std::array<Controls,16> controls{};
      auto connect=[&] {
        if (lan) {
          client=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(host->port()),0x16d560,1);
          for(unsigned i=0;i<64 && !client->ready();++i) {
            client->send(std::array<Controls,1>{});host->receive(controls);
            host->broadcast();client->receive();
          }
          check(client->ready() && client->slots()[0]==0,"Identity UDP join failed");
        } else check(match.join("Identity")==0,"Identity local slot differs");
      };
      if(lan) host=std::make_unique<LanHost>(match,0,0x16d560);
      connect();
      check(match.player(0).source_datum==first && match.unit(0).source_player==first &&
          match.unit(0).source_previous_player==null_datum &&
          match.player_from_datum(first)==&match.player(0) && !match.player_from_datum(null_datum),
          "Native initial player pool/ownership differs");
      // A dead bit alone retains ownership. Supply that original prefix output
      // at clock100; native's declared cleanup phase then detaches the unit.
      while(match.simulation().ticks()<100) match.step(controls);
      auto &unit=const_cast<Unit &>(match.unit(0));
      unit.vitality.dead=true;unit.last_damager=-2;
      check(unit.source_player==first,"Body depletion prematurely released player");
      match.step(controls);
      check(match.player(0).source_datum==first && match.unit(0).source_player==null_datum &&
          match.unit(0).source_previous_player==first && match.player_from_datum(first),
          "Native cleanup full player ownership differs");
      while(match.simulation().ticks()<160) match.step(controls);
      match.step(controls);
      check(!match.unit(0).vitality.dead && match.player(0).source_datum==first &&
          match.unit(0).source_player==first && match.unit(0).source_previous_player==null_datum,
          "Native respawn changed player generation or retained stale unit owner");
      if(lan) { client.reset();host->receive(controls); } else match.leave(0);
      check(!match.player(0).connected && !match.player_from_datum(first) &&
          match.unit(0).source_player==null_datum,"Native leave retained active player identity");
      connect();
      check(match.player(0).source_datum==next && match.unit(0).source_player==next &&
          match.player_from_datum(next)==&match.player(0) && !match.player_from_datum(first),
          "Native slot reuse accepted departed player salt");
      if(lan) {
        host->broadcast();client->receive();
        check(client->state().players[0].connected && !client->state().players[0].dead,
              "Actual UDP rejoin state differs");
      }
    }
    std::cout<<"Original player salts and ownership matched local/actual-UDP death, respawn, leave and rejoin boundaries\n";
    return 0;
  } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
