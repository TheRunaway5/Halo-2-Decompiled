#include "halo2/source_player_lives.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the original life fixture");std::ifstream file(argv[1]);Json fixture;file>>fixture;
  unsigned comparisons=0;
  for(const auto& [version,data]:fixture.at("versions").items()) {
    for(const auto& [operation,rows]:fixture.at("inputs").items())for(unsigned i=0;i<rows.size();++i) {
      const auto& input=rows[i];const auto& result=data.at("results").at(operation)[i];
      const auto limit=input.at("limit").get<std::int32_t>();
      const auto& player=input.at("players")[0];auto remaining=player.at("lives").get<std::int16_t>();
      if(operation=="initialize") {
        source_initialize_player_lives(remaining,limit,input.value("engine_present",true),input.value("mode",1));
        check(remaining==result.at("lives"),"Complete original player initialization life store differs");++comparisons;
      }else if(operation=="death") {
        source_player_life_death(remaining,input.value("game_type",2),input.value("mode",1),
            input.value("engine_present",true),input.value("lifecycle",1),input.value("status",1));
        check(remaining==result.at("lives"),"Complete original death life decrement differs");++comparisons;
      }else if(operation=="queries") {
        const auto unit=player.at("unit").get<std::uint32_t>();
        check(source_player_lives_exhausted(limit,unit,remaining)==bool(result.at("exhausted").get<int>()),
            "Complete original player exhaustion query differs");
        check(source_player_respawn_allowed(limit,player.at("flags").get<std::uint16_t>(),
            player.at("membership").get<std::int8_t>(),unit,remaining)==bool(result.at("allowed").get<int>()),
            "Complete original respawn admission differs");comparisons+=2;
      }else {
        std::vector<SourcePlayerLifeParticipant> players;
        for(const auto& p:input.at("players"))players.push_back({p.at("flags"),p.at("membership"),p.at("unit"),p.at("lives")});
        check(source_team_lives_exhausted(limit,input.at("engine_present"),input.at("team_mode"),input.at("team"),players)
            ==bool(result.at("team_exhausted").get<int>()),"Complete original team exhaustion query differs");++comparisons;
      }
    }
  }
  check(comparisons==9252,"Original life comparison count changed");
  std::cout<<comparisons<<" whole original initialization/death/admission comparisons pass\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
