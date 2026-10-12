#include "halo2/source_king.hpp"
#include "halo2/source_game_end.hpp"
#include "halo2/slayer.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<std::uint8_t> bytes(const std::string& hex) {
  std::vector<std::uint8_t> out(hex.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return out;
}
template<class T>T decode(const std::string& hex) {
  const auto raw=bytes(hex);check(raw.size()==sizeof(T),"Original record width differs");
  T out;std::memcpy(&out,raw.data(),sizeof(out));return out;
}
SourceKingState state(const Json& fixture,const Json& key) {
  return decode<SourceKingState>(fixture.at("raw_blobs").at(key.get<std::string>()));
}
SourceKingHills hills(const std::string& hex) {
  const auto raw=bytes(hex);check(raw.size()==18,"Original group bank width differs");SourceKingHills out;
  std::memcpy(&out.count,raw.data(),2);std::memcpy(out.indices.data(),raw.data()+2,16);return out;
}
void compare(const SourceKingState& actual,const SourceKingState& expected) {
  if(std::memcmp(&actual,&expected,sizeof(actual))==0)return;
  for(unsigned offset=0;offset<sizeof(actual);offset+=4) {
    std::uint32_t a,e;std::memcpy(&a,reinterpret_cast<const char*>(&actual)+offset,4);
    std::memcpy(&e,reinterpret_cast<const char*>(&expected)+offset,4);
    if(a!=e)std::cerr<<"State+"<<std::hex<<offset<<" expected="<<e<<" actual="<<a<<std::dec<<'\n';
  }
  throw std::runtime_error("Whole original King state differs");
}
unsigned scoring(const char* path) {
  std::ifstream file(path);Json fixture;file>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Scoring evidence supplied replacement results");
  const bool world=fixture.contains("maps");std::vector<std::vector<SourceKingMarker>> maps;
  if(world)for(const auto& map:fixture.at("maps")) {
    const auto raw=bytes(map.at("rows_hex"));maps.emplace_back(raw.size()/32);
    std::memcpy(maps.back().data(),raw.data(),raw.size());
  }
  unsigned cases=0;
  for(const auto& [version,rows]:fixture.at("versions").items())for(unsigned n=0;n<fixture.at("inputs").size();++n) {
    const auto& input=fixture.at("inputs")[n];const auto& row=rows.at("results")[n];
    const auto& before=row.at("before");const auto& after=row.at("after");
    auto king=decode<SourceKingState>(before.at("king_hex"));auto stats=decode<SourceGameEndStats>(before.at("bank_hex"));
    auto engine_flags=before.at("engine_flags").get<std::uint32_t>();Json events=Json::array();
    SourceKingContext context;context.engine_present=input.value("engine_present",true);context.team_mode=input.at("team_mode");
    context.raw_mode=input.value("mode",1);context.lifecycle=input.value("lifecycle",1);context.status=input.value("status",1);
    context.flags=input.at("flags");context.tick_rate=input.at("rate");
    context.game_over=input.value("game_over",false);context.move_seconds=input.value("move_seconds",0);
    std::vector<SourceKingPlayer> players;std::vector<SlayerRankingPlayer> ranking;
    for(const auto& p:input.at("players")) {
      const auto slot=p.at("slot").get<unsigned>();
      players.push_back({0x9abc0000u+slot,p.at("unit"),p.at("membership"),p.at("inside_ticks")});
      ranking.push_back({players.back().datum,stats.players[slot].fields[0],p.at("flags"),std::uint8_t(players.back().membership)});
    }
    const auto find=[&](std::uint32_t datum)->SourceKingPlayer& {
      for(auto& player:players)if(player.datum==datum)return player;
      throw std::runtime_error("Scorer selected an absent player");
    };
    SourceKingScoringServices services;
    services.score=[&](std::uint32_t datum)->std::int32_t {
      events.push_back(Json{{"kind","getter"},{"player",datum}});
      if(!context.engine_present)return 0;
      if(!context.team_mode)return stats.players[datum&65535].fields[0];
      const auto team=find(datum).membership;return team==-1 ? 0 : stats.teams.at(unsigned(team)).fields[0];
    };
    services.record=[&](std::uint32_t datum,std::int32_t kind,std::uint32_t other) {
      // Observe delivery to the disabled recorder. The unchanged original
      // owner naturally skips its clock/platform/append branch in this corpus.
      events.push_back(Json{{"kind","record"},{"player",datum},{"request",kind},{"other",other}});
    };
    services.award=[&](std::uint32_t datum,std::int32_t delta) {
      events.push_back(Json{{"kind","award"},{"player",datum},{"delta",delta}});
      const auto slot=datum&65535;auto& player=find(datum);
      const auto applied=slayer_score_write(stats.players[slot].fields[0],delta,0,context.engine_present,
          context.lifecycle==1,context.raw_mode==4 || context.status==1);
      if(!applied.applied)return;
      for(auto& rank:ranking)rank.score=stats.players[rank.player&65535].fields[0];
      if(context.team_mode && player.membership!=-1)
        slayer_team_score_write(stats.teams.at(unsigned(player.membership)).fields[0],player.membership,delta,0,
            input.at("team_score_kind"),ranking);
      engine_flags|=0x20;
    };
    services.score_changed=[&](std::uint32_t datum,std::int32_t score) {
      events.push_back(Json{{"kind","changed"},{"player",datum},{"score",score}});
    };
    try {
      if(world) {
        SourceKingWorldServices world_services;world_services.scoring=services;
        world_services.notification=[&](const SourceKingNotice& event) {
          events.push_back(Json{{"kind","notification"},{"type",event.type},{"player",event.player},{"membership",event.membership}});
        };
        auto seed=before.at("seed").get<std::uint32_t>();auto groups=hills(before.at("groups_hex"));
        source_king_world_update(king,groups,context,maps.at(input.at("map")),players,seed,world_services);
        check(seed==after.at("seed") && before.at("groups_hex")==after.at("groups_hex"),"World update RNG/group fields differ");
      } else source_king_score(king,context,players,services);
      compare(king,decode<SourceKingState>(after.at("king_hex")));
      const auto expected=decode<SourceGameEndStats>(after.at("bank_hex"));
      check(std::memcmp(&stats,&expected,sizeof(stats))==0,"Whole original scoring bank differs");
      check(engine_flags==after.at("engine_flags") && before.at("status")==after.at("status"),"Scoring engine fields differ");
      for(const auto& player:players)check(player.inside_ticks==after.at("timers")[player.datum&65535],"Scorer inside timer differs");
      check(events==row.at("events"),"Original getter/record/award/publication order differs");
    }catch(const std::exception&){std::cerr<<version<<(world ? " world=" : " scoring=")<<n<<' '<<input.dump()<<"\nevents="<<events.dump()<<"\nexpected="<<row.at("events").dump()<<'\n';throw;}
    ++cases;
  }
  // Missing providers must fail at their actual source call position, while
  // contested/inactive source gates remain usable without any providers.
  SourceKingState state;SourceKingContext context;context.tick_rate=30;
  SourceKingPlayer one{0x9abc0000,0x8aaa0000,0,30};
  bool rejected=false;try{source_king_score(state,context,std::span(&one,1),{});}catch(const std::runtime_error&){rejected=true;}
  check(rejected,"Scoring substituted an absent score getter");
  SourceKingScoringServices services;unsigned getters=0,records=0,awards=0;
  services.score=[&](std::uint32_t){++getters;return int(awards);};
  rejected=false;try{source_king_score(state,context,std::span(&one,1),services);}catch(const std::runtime_error&){rejected=true;}
  check(rejected && getters==1,"Scoring bypassed absent recorder/order");
  services.record=[&](std::uint32_t,std::int32_t,std::uint32_t){++records;};
  rejected=false;try{source_king_score(state,context,std::span(&one,1),services);}catch(const std::runtime_error&){rejected=true;}
  check(rejected && records==1,"Scoring bypassed absent award owner/order");
  services.award=[&](std::uint32_t,std::int32_t){++awards;};
  rejected=false;try{source_king_score(state,context,std::span(&one,1),services);}catch(const std::runtime_error&){rejected=true;}
  check(rejected && awards==1,"Scoring bypassed absent publication owner/order");
  context.flags=1;std::array<SourceKingPlayer,2> contested{one,{0x9abc0003,0x8aaa0003,7,30}};
  source_king_score(state,context,contested,{});
  context.flags=0;context.lifecycle=0;source_king_score(state,context,std::span(&one,1),{});
  if(world) {
    context={};context.move_seconds=1;SourceKingHills groups;
    source_king_initialize(state,groups,maps.front());source_king_round_start(state,maps.front(),1,30);state.move_ticks=1;
    std::uint32_t seed=0x12345678;SourceKingWorldServices world_services;
    rejected=false;
    try{source_king_world_update(state,groups,context,maps.front(),{},seed,world_services);}catch(const std::runtime_error&){rejected=true;}
    check(rejected && state.hill==0,"Movement bypassed missing event owner or wrote selected hill too early");
    unsigned notifications=0;world_services.notification=[&](const SourceKingNotice&){++notifications;};
    state.move_ticks=1;context.match_replication_entity=0x12340003;
    rejected=false;
    try{source_king_world_update(state,groups,context,maps.front(),{},seed,world_services);}catch(const std::runtime_error&){rejected=true;}
    check(rejected && notifications==1 && state.hill!=0,"Movement bypassed missing replication owner/order");
    context.game_over=true;state.move_ticks=1;
    source_king_world_update(state,groups,context,maps.front(),{},seed,{});
    check(state.move_ticks==1,"Game-over world update did not preserve movement timer");
  }
  return cases;
}
}
int main(int argc,char** argv)try {
  check(argc==4,"Supply whole original King geometry/occupancy, scoring and world fixtures");std::ifstream file(argv[1]);Json fixture;file>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),
      "King original evidence supplied replacement results");
  std::vector<std::vector<SourceKingMarker>> maps;
  for(const auto& map:fixture.at("maps")) {
    const auto raw=bytes(map.at("rows_hex"));check(raw.size()%32==0,"Scenario row stride differs");
    maps.emplace_back(raw.size()/32);std::memcpy(maps.back().data(),raw.data(),raw.size());
  }
  unsigned geometry_cases=0,occupancy_cases=0,selections=0,inside=0;
  for(const auto& [version,rows]:fixture.at("versions").items()) {
    for(unsigned map=0;map<maps.size();++map)for(const auto& row:rows.at("geometry")[map]) {
      auto actual=state(fixture,row.at("before"));auto groups=hills(row.at("before_groups_hex"));
      const std::string operation=row.at("operation");const auto& input=row.at("inputs");
      try {
        if(operation=="initialize")source_king_initialize(actual,groups,maps[map]);
        else if(operation=="start")source_king_round_start(actual,maps[map],input.at("move_seconds"),input.at("rate"));
        else if(operation=="build")source_king_build_hill(actual,maps[map],input.at("hill"));
        else throw std::runtime_error("Unknown original King operation");
        compare(actual,state(fixture,row.at("after")));
        const auto expected=hills(row.at("after_groups_hex"));
        check(groups.count==expected.count && groups.indices==expected.indices,"Original King group inventory differs");
      }catch(const std::exception&){std::cerr<<version<<" map="<<map<<" operation="<<operation<<' '<<input.dump()<<'\n';throw;}
      ++geometry_cases;
    }
    check(rows.at("occupancy").size()==fixture.at("occupancy_inputs").size(),"Occupancy row count differs");
    for(unsigned i=0;i<fixture.at("occupancy_inputs").size();++i) {
      const auto& input=fixture.at("occupancy_inputs")[i];const auto& expected=rows.at("occupancy")[i];
      const auto original=state(fixture,input.at("state"));auto actual=original;
      const auto slot=input.at("slot").get<unsigned>();
      SourceKingPlayer player{0x9abc0000u+slot,input.at("unit_present").get<bool>() ? 0x8aaa0000u+slot : 0xffffffffu,
          -1,input.at("inside_ticks")};
      const auto point=decode<std::array<float,3>>(input.at("point_hex"));unsigned position_calls=0;
      SourceKingPosition position=[&](std::uint32_t unit){++position_calls;check(unit==player.current_unit,"Wrong current unit queried");return point;};
      const bool contains=source_king_contains(actual,player,position);
      source_king_update_player(actual,player,input.at("game_over"),input.at("mode"),position);
      if(contains!=(expected.at("contains").get<int>()!=0) || player.inside_ticks!=expected.at("inside_ticks")) {
        std::cerr<<version<<" occupancy="<<i<<' '<<input.dump()<<" expected="<<expected.dump()
            <<" actual contains="<<contains<<" timer="<<player.inside_ticks<<'\n';
        throw std::runtime_error("Whole original King occupancy differs");
      }
      compare(actual,original);check(position_calls<=2,"Repeated position owner query");
      if(contains)++inside;
      ++occupancy_cases;
    }
    for(unsigned i=0;i<fixture.at("selection_inputs").size();++i) {
      const auto& input=fixture.at("selection_inputs")[i];const auto& expected=rows.at("selection")[i];
      SourceKingHills groups;groups.count=input.at("count");groups.indices=input.at("indices").get<std::array<std::int16_t,8>>();
      auto seed=input.at("seed").get<std::uint32_t>();const auto before=groups;
      const auto selected=source_king_select_hill(groups,input.at("current"),seed);
      check(selected==expected.at("hill") && seed==expected.at("seed"),"Original King selection/RNG differs");
      check(groups.count==before.count && groups.indices==before.indices,"King selection changed authored groups");++selections;
    }
  }
  SourceKingState gate;gate.marker_count=4;SourceKingPlayer player{0x9abc0000,0x8aaa0000,0,123};
  bool rejected=false;try{source_king_contains(gate,player,{});}catch(const std::runtime_error&){rejected=true;}
  check(rejected,"King position owner silently substituted");
  source_king_update_player(gate,player,true,1,{});check(player.inside_ticks==0,"Game-over occupancy did not reset");
  player.inside_ticks=123;source_king_update_player(gate,player,false,4,{});check(player.inside_ticks==0,"Playback occupancy did not reset");
  player.current_unit=0xffffffffu;check(!source_king_contains(gate,player,{}),"NONE current unit bypassed original gate");
  player.current_unit=0x8aaa0000;player.datum=0xffffffffu;check(!source_king_contains(gate,player,{}),"NONE player bypassed original gate");
  const auto scoring_cases=scoring(argv[2]);
  const auto world_cases=scoring(argv[3]);
  std::cout<<geometry_cases<<" whole original King states, "<<occupancy_cases<<" occupancy pairs ("<<inside
      <<" inside), "<<selections<<" selections and "<<scoring_cases
      <<" whole scoring cases and "<<world_cases
      <<" whole world updates match all three images; absent position/scoring/event/replication owners reject\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
