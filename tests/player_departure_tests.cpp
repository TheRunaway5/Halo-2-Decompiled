#include "halo2/source_player_departure.hpp"
#include "halo2/source_result_retention.hpp"
#include "halo2/full_scoreboard.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const char* message) {
  if(!value)throw std::runtime_error(message);
}
void retained_departure() {
  std::array<SourceDeparturePlayer,4> players;
  for(unsigned n=0;n<players.size();++n) {
    auto& p=players[n];p.salt=0x8000+n;p.flags=0x19;
    for(unsigned i=0;i<p.identity.size();++i)p.identity[i]=n*13+i;
    for(unsigned i=0;i<p.configuration.size();++i)p.configuration[i]=i*17+n;
    p.configuration[0x7c]=n%2;
    p.machine={1,2,3,4,5,6};p.machine_index=2;p.machine_user=1;p.machine_controller=3;
    p.previous_unit=0x80040008;
    p.remaining.fill(0xa7);
  }
  const auto before=players;
  SourceDepartureCensus census;census.count=4;
  SourceDepartureContext context;context.tick=12345;context.engine_present=true;context.engine_team_mode=true;
  std::vector<std::string> calls;
  SourceDepartureServices services;
  services.unlink_current_unit=[&](auto datum){check(datum==0x80010001,"Unlink lost full datum");calls.push_back("unlink");};
  services.refresh_team_masks=[&]{check(players[1].flags==0x1a && census.count==3,"Engine saw pre-departure state");calls.push_back("teams");};
  services.engine_leave=[&](auto datum){check(datum==0x80010001,"Engine leave lost full datum");calls.push_back("engine");};
  services.publish_leave_event=[&](auto datum,auto team){check(datum==0x80010001 && team==1,"Leave event identity/team differs");calls.push_back("event");};
  services.remap_results=[&]{calls.push_back("remap");};
  services.dirty_player=[&](auto slot,auto mask){check(slot==1 && mask==0x200,"Leave dirty fields differ");calls.push_back("dirty");};
  check(source_simulation_player_depart(players,1,before[1].identity,census,context,services),"Original ordinary departure was rejected");
  auto expected=before[1];expected.flags=0x1a;expected.leave_tick=12345;
  expected.machine={};expected.machine_index=expected.machine_user=-1;expected.machine_controller=-1;
  expected.previous_unit=0xffffffffu;
  check(std::equal(reinterpret_cast<const unsigned char*>(&expected),reinterpret_cast<const unsigned char*>(&expected)+sizeof(expected),
      reinterpret_cast<const unsigned char*>(&players[1])),"Departure changed retained player bytes");
  check(census.count==3 && calls==std::vector<std::string>{"unlink","teams","engine","event","remap","dirty"},"Departure service order differs");
  const auto departed=players;const auto called=calls;
  check(!source_simulation_player_depart(players,1,before[1].identity,census,context,services),"Repeated departure accepted");
  check(census.count==3 && calls==called,"Repeated departure changed census/services");
  check(std::equal(reinterpret_cast<const unsigned char*>(departed.data()),reinterpret_cast<const unsigned char*>(departed.data())+sizeof(departed),
      reinterpret_cast<const unsigned char*>(players.data())),"Repeated departure changed retained records");
}
void external_services() {
  SourceDeparturePlayer player;player.salt=0x1234;player.flags=0x8041;
  player.current_unit=0x80040008;player.previous_unit=0x80030008;
  player.local_index=2;player.controller_index=3;
  SourceDepartureCensus census;census.count=1;census.user_count=census.controller_count=1;
  census.users[2]=census.controllers[3]=0x12340000;
  SourceDepartureContext context;context.tick=0xfffffffe;context.raw_mode=4;
  context.world_present=context.engine_present=context.engine_team_mode=true;
  std::vector<std::string> calls;SourceDepartureServices services;
  services.mark_current_unit=[&](auto datum){check(datum==0x80040008,"Unit mark lost owner");calls.push_back("mark");};
  services.mode4_clear_unit_backlinks=[&](auto datum){check(datum==0x12340000,"Mode4 scan lost player");calls.push_back("scan");};
  services.unlink_current_unit=[&](auto datum){check(datum==0x12340000,"Unit unlink lost player");player.current_unit=0xffffffffu;calls.push_back("unlink");};
  services.simulation_leave=[&](auto datum){check(datum==0x12340000 && player.previous_unit==0xffffffffu,"World leave ran before unlink");calls.push_back("world");};
  services.control_set_unit=[&](auto local,auto unit){check(local==2 && unit==0xffffffffu,"Local control departure differs");calls.push_back("control");};
  services.first_person_set_unit=[&](auto unit,auto local,auto character){check(unit==0xffffffffu && local==2 && character==-1,"Local first person departure differs");calls.push_back("view");};
  services.engine_leave=[&](auto){calls.push_back("engine");};
  services.publish_leave_event=[&](auto,auto){calls.push_back("event");};
  services.remap_results=[&]{calls.push_back("remap");};
  services.dirty_player=[&](auto,auto){calls.push_back("dirty");};
  check(source_simulation_player_depart(std::span(&player,1),0,player.identity,census,context,services),"Declared external departure rejected");
  check(player.flags==0x8042 && player.leave_tick==0xfffffffe && player.salt==0x1234 &&
      player.local_index==-1 && player.controller_index==-1 && census.count==0 &&
      census.user_count==0 && census.controller_count==0 && census.users[2]==0xffffffffu && census.controllers[3]==0xffffffffu,
      "Local/controller departure fields differ");
  check(calls==std::vector<std::string>{"mark","scan","unlink","world","control","view","engine","event","remap","dirty"},
      "External departure service order differs");
  // Missing original consumers cannot silently become successful teardown.
  player.flags=1;player.current_unit=0x80040008;bool rejected=false;
  try {source_simulation_player_depart(std::span(&player,1),0,player.identity,census,context,{});}
  catch(const std::runtime_error&){rejected=true;}
  check(rejected && player.flags==1 && player.current_unit==0x80040008,"Missing unit owner accepted");
}
void retained_standings() {
  std::array<SourceDeparturePlayer,4> players;
  for(unsigned i=0;i<players.size();++i){players[i].salt=0x8000+i;players[i].flags=1;}
  SourceDepartureCensus census;census.count=4;
  SourceDepartureServices services;
  // This declared branch has no current unit; original14CAD0 is a no-op.
  services.unlink_current_unit=[](auto){};
  SourceDepartureContext context;context.raw_mode=1;
  std::array<FullScoreboardPlayer,4> rows;
  for(unsigned i=0;i<players.size();++i) {
    rows[i].datum=(std::uint32_t(players[i].salt)<<16)|i;
    rows[i].flags=players[i].flags;rows[i].membership=i%2;
    rows[i].total_score=i==1 ? 100 : 10-int(i);
  }
  FullScoreboardOptions options;options.engine_present=options.game_over=true;
  check(full_scoreboard_ranks(rows,options).players[1]==0,"Initial FFA fixture winner differs");
  check(source_simulation_player_depart(players,1,players[1].identity,census,context,services),"FFA departure rejected");
  rows[1].flags=players[1].flags;
  check(full_scoreboard_ranks(rows,options).players[1]==6 && rows[1].total_score==100,
      "Allocated FFA quitter did not rank below finishers with retained score");
  options.team_mode=true;std::array<FullScoreboardTeam,8> teams{};
  teams[0].retained=teams[1].retained=teams[0].playing=teams[1].playing=true;
  teams[0].total_score=10;teams[1].total_score=100;
  auto ranks=full_scoreboard_ranks(rows,options,teams);
  check(ranks.teams[1]==0 && ranks.players[1]<ranks.players[0],
      "Quitter's still-playing winning team lost original team ordering");
  check(source_simulation_player_depart(players,3,players[3].identity,census,context,services),"Second team departure rejected");
  rows[3].flags=players[3].flags;
  // Both allocated team1 rows are now flagged left; original team-mask owner
  // supplies playing=false while retaining the team/statistical record.
  check((rows[1].flags&2) && (rows[3].flags&2),"Departed team membership lost");
  teams[1].playing=false;ranks=full_scoreboard_ranks(rows,options,teams);
  check(ranks.teams[1]==2 && ranks.players[1]>ranks.players[0] && teams[1].total_score==100,
      "Wholly departed team did not rank below playing team with retained score");
}
std::vector<std::uint8_t> bytes(const nlohmann::json& value) {
  const auto text=value.get<std::string>();check(text.size()%2==0,"Odd original hex record");
  std::vector<std::uint8_t> raw;
  for(unsigned i=0;i<text.size();i+=2)raw.push_back(std::uint8_t(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
template<class T>void read_record(T& value,const nlohmann::json& hex) {
  const auto raw=bytes(hex);check(raw.size()==sizeof(value),"Original record size differs");
  std::memcpy(&value,raw.data(),sizeof(value));
}
template<class T>void compare_record(const T& value,const nlohmann::json& hex,const char* message) {
  const auto raw=bytes(hex);check(raw.size()==sizeof(value),"Compared original record size differs");
  const auto* actual=reinterpret_cast<const std::uint8_t*>(&value);
  for(unsigned i=0;i<raw.size();++i)if(actual[i]!=raw[i])
    throw std::runtime_error(std::string(message)+" at byte "+std::to_string(i)+
        " actual "+std::to_string(actual[i])+" expected "+std::to_string(raw[i]));
}
void original_controls(const char* path) {
  std::ifstream stream(path);nlohmann::json fixture;stream>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Departure oracle supplied a dependency");
  unsigned count=0;
  for(const auto& control:fixture.at("controls")) {
    std::array<SourceDeparturePlayer,4> players;read_record(players,control.at("before_players_hex"));
    SourceDepartureCensus census;read_record(census,control.at("initial_census_hex"));
    SourceRetainedResults results;read_record(results,control.at("before_retained_hex"));
    SourceDepartureContext context;context.tick=control.at("clock_tick");
    context.raw_mode=control.at("raw_game_mode");context.engine_present=control.at("selected_engine").get<int>()!=0;
    context.engine_team_mode=(control.at("raw_options_184").get<unsigned>()&1u)!=0;
    const auto datum=(std::uint32_t(players[1].salt)<<16)|1u;std::vector<std::string> calls;
    SourceDepartureServices services;
    services.unlink_current_unit=[&](auto actual){check(actual==datum && players[1].current_unit==0xffffffffu,"Original no-unit control differed");calls.push_back("unlink");};
    services.refresh_team_masks=[&]{calls.push_back("teams");};
    services.engine_leave=[&](auto actual){check(actual==datum,"Original current salt differs");calls.push_back("engine");};
    services.publish_leave_event=[&](auto actual,auto team){check(actual==datum && team==1,"Original event arguments differ");calls.push_back("event");};
    services.remap_results=[&]{
      SourceResultLivePlayers live{};SourceResultMachineRoster machines{};
      for(unsigned i=0;i<players.size();++i) {
        live[i].salt=players[i].salt;live[i].identity=players[i].identity;
        live[i].machine=players[i].machine_index;live[i].configuration=players[i].configuration;
      }
      //199310 reads the currently selected census+30 roster. The fixture's
      // earlier unselected session backing is not this provider's result.
      read_record(machines,control.at("resolved_source_machine_roster_hex"));
      check(control.at("machine_policy")=="PruneUnreferenced","Original machine policy differs");
      source_result_remap(results,{control.at("recording").at(0),control.at("recording").at(1)},
          live,control.at("high_water"),machines,SourceResultMachinePolicy::PruneUnreferenced);
      calls.push_back("remap");
    };
    services.dirty_player=[&](auto slot,auto mask){check(slot==1 && mask==0x200,"Original dirty fields differ");calls.push_back("dirty");};
    check(source_simulation_player_depart(players,1,players[1].identity,census,context,services)==(control.at("result_al").get<int>()!=0),"Original dispatcher result differs");
    compare_record(players,control.at("after_players_hex"),"Whole original allocated player bank differs");
    compare_record(census,control.at("final_census_hex"),"Whole original census differs");
    compare_record(results,control.at("after_retained_hex"),"Whole original retained result bank differs");
    check(calls==std::vector<std::string>{"unlink","teams","engine","event","remap","dirty"},"Original service order differs");
    const auto before_repeat=calls;
    check(!source_simulation_player_depart(players,1,players[1].identity,census,context,services) && calls==before_repeat,"Original repeat gate differs");
    compare_record(players,control.at("after_players_hex"),"Repeat changed original player bank");
    compare_record(census,control.at("final_census_hex"),"Repeat changed original census");
    for(const auto& gate:control.at("rejected_gate_controls")) {
      auto initial=players;auto initial_census=census;const auto initial_calls=calls;
      auto key=players[0].identity;int slot=0;const auto kind=gate.at("case").get<std::string>();
      if(kind=="wrong-key")key[0]^=1;
      else if(kind=="out-of-highwater")slot=4;
      else if(kind=="none-index")slot=-1;
      else if(kind=="zero-salt"){players[0].salt=0;initial=players;}
      else throw std::runtime_error("Unknown original gate control");
      check(!source_simulation_player_depart(players,slot,key,census,context,services),"Original rejected gate accepted");
      check(std::memcmp(players.data(),initial.data(),sizeof(players))==0 &&
          std::memcmp(&census,&initial_census,sizeof(census))==0 && calls==initial_calls,"Rejected gate mutated state");
      ++count;
    }
    count+=2;
  }
  std::cout<<"PASS "<<count<<" unchanged original departure/gate controls; complete player, census and retained-result bytes\n";
}
}
int main(int argc,char** argv) {
  try {retained_departure();external_services();retained_standings();
    if(argc==2)original_controls(argv[1]);
    else if(argc!=1)throw std::runtime_error("Supply one unchanged original departure fixture");
    std::cout<<"PASS retained ordinary departure, full datum, census and declared external service order\n";}
  catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
