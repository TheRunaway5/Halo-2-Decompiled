#include "halo2/source_round_lifecycle.hpp"
#include "halo2/source_game_over.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Raw=std::vector<std::uint8_t>;
Raw raw(const Json& fixture,const Json& ref) {
  const auto hex=fixture.at("raw_blobs").at(ref.at("raw_blob_sha256").get<std::string>()).get<std::string>();
  Raw out(hex.size()/2);
  for(unsigned n=0;n<out.size();++n)out[n]=std::uint8_t(std::stoul(hex.substr(n*2,2),nullptr,16));
  return out;
}
template<class T>T load(const Raw& bytes,unsigned offset=0) {
  check(offset+sizeof(T)<=bytes.size(),"Original raw record is too short");
  T value;std::memcpy(&value,bytes.data()+offset,sizeof(T));return value;
}
template<class T>void store(Raw& bytes,unsigned offset,const T& value) {
  check(offset+sizeof(T)<=bytes.size(),"Original raw record is too short");
  std::memcpy(bytes.data()+offset,&value,sizeof(T));
}
void compare(const Json& fixture,const Json& expected,const char* key,const Raw& actual) {
  if(actual!=raw(fixture,expected.at(key)))throw std::runtime_error(std::string("Whole original raw record differs: ")+key);
}
void run(const Json& fixture,const Json& owners,const Json& input,const Json& record,bool post) {
  const auto& before=record.at("before");const auto& expected=record.at("after");
  SourceRoundLifecycle state;
  state.flags=before.at("flags");state.status=before.at("status");state.phase=before.at("lifecycle");
  state.index=before.at("index");state.start_tick=before.at("start_tick");
  state.clock_seconds=before.at("clock_seconds");state.countdown=before.at("round_countdown");
  auto ranking=raw(fixture,before.at("ranking_hex"));check(ranking.size()==sizeof(state.ranking),"Ranking bank width differs");
  std::memcpy(state.ranking.data(),ranking.data(),ranking.size());
  auto bank=raw(fixture,before.at("bank_hex"));check(bank.size()==sizeof(SourceGameEndStats),"Stat bank width differs");
  auto stats=load<SourceGameEndStats>(bank);
  SourceResultRecording recording{before.at("recording")[0],before.at("recording")[1]};
  SourceRoundContext context;
  context.engine_present=input.value("engine_present",true);context.game_over=before.at("game_over").get<int>()!=0;
  context.raw_mode=input.value("mode",1);context.tick_rate=input.value("rate",30);
  context.tick=input.value("tick",12345);context.variant_flags=unsigned(input.value("team_mode",false));
  context.time_limit_seconds=input.value("time_limit",120);context.life_limit=input.value("limit",1);
  auto players_raw=raw(fixture,before.at("players_hex"));auto respawn_raw=raw(fixture,before.at("player_respawn_rows_hex"));
  auto rows_raw=raw(fixture,before.at("netgame_rows_hex"));
  std::vector<NetgameWeaponState> rows(rows_raw.size()/sizeof(NetgameWeaponState));
  std::memcpy(rows.data(),rows_raw.data(),rows_raw.size());
  std::vector<SourceRoundPlayer> players;
  const auto roster=input.value("players",Json::array({Json{{"slot",0}},Json{{"slot",3}}}));
  for(const auto& entry:roster) {
    const auto slot=entry.at("slot").get<unsigned>();const auto at=slot*0x21c;
    SourceRoundPlayer player;player.datum=std::uint32_t(load<std::uint16_t>(players_raw,at))<<16|slot;
    player.flags=load<std::uint16_t>(players_raw,at+2);player.lives=load<std::int16_t>(players_raw,at+0x1ac);
    player.respawn_countdown=load<std::int32_t>(players_raw,at+0x164);
    player.player_1a7=players_raw.at(at+0x1a7);player.player_1a8=players_raw.at(at+0x1a8);
    player.engine_respawn_flag=respawn_raw.at(slot*24);players.push_back(player);
  }
  SourceGameOverState game_over;game_over.active=context.game_over;game_over.countdown=before.at("game_over_countdown");
  unsigned resets=0,initializations=0,phase_callbacks=0,begins=0;
  SourceRoundServices services;
  // These fixtures explicitly declare an empty object pool, NONE current
  // units/replication/local users, and no full-map-reset bit. Empty-world
  // callbacks here are not supplied to the live native match.
  services.reset_objects=[&]{++resets;};
  services.initialize_engine=[&]{++initializations;}; // registered Slayer leaf has no stores
  services.phase_changed=[&](std::int32_t status){check(status==state.status,"Callback pending status differs");++phase_callbacks;};
  services.begin_game_over=[&]{++begins;source_game_over_begin(game_over,context.tick_rate,{});};
  const bool changed=post ? source_round_post_world(state,context,stats,recording,players,rows,services) :
      source_round_apply_phase(state,context,stats,recording,players,rows,services);
  check(!post || changed==(begins!=0),"Post-world cap result differs");
  for(const auto& player:players) {
    const auto slot=player.datum&65535u,at=slot*0x21c;
    store(players_raw,at+2,player.flags);store(players_raw,at+0x1ac,player.lives);
    store(players_raw,at+0x164,player.respawn_countdown);
    players_raw.at(at+0x1a7)=player.player_1a7;players_raw.at(at+0x1a8)=player.player_1a8;
    respawn_raw.at(slot*24)=player.engine_respawn_flag;
  }
  std::memcpy(bank.data(),&stats,sizeof(stats));std::memcpy(ranking.data(),state.ranking.data(),ranking.size());
  std::memcpy(rows_raw.data(),rows.data(),rows_raw.size());
  compare(fixture,expected,"bank_hex",bank);compare(fixture,expected,"ranking_hex",ranking);
  compare(fixture,expected,"players_hex",players_raw);compare(fixture,expected,"player_respawn_rows_hex",respawn_raw);
  compare(fixture,expected,"netgame_rows_hex",rows_raw);
  check(state.flags==expected.at("flags") && state.status==expected.at("status") && state.phase==expected.at("lifecycle") &&
      state.index==expected.at("index") && state.start_tick==expected.at("start_tick") &&
      state.clock_seconds==expected.at("clock_seconds") && state.countdown==expected.at("round_countdown"),
      "Original phase/clock/control fields differ");
  check(game_over.active==(expected.at("game_over").get<int>()!=0) && game_over.countdown==expected.at("game_over_countdown"),
      "Original game-over cap/begin fields differ");
  check(recording.enabled==expected.at("recording")[0] && recording.frozen==expected.at("recording")[1],
      "Original recording bytes differ");
  // Check external-owner delivery counts as well as complete resulting bytes.
  const auto& calls=record.at("calls");
  const bool applied=(!post || (context.engine_present && context.raw_mode!=4 && !context.game_over)) &&
      before.at("lifecycle")!=expected.at("status");
  const auto call_count=[&](const char* key) {
    std::ostringstream address;address<<std::hex<<std::setw(8)<<std::setfill('0')<<owners.at(key).get<std::uint32_t>();
    return unsigned(std::count(calls.begin(),calls.end(),Json(address.str())));
  };
  check(phase_callbacks==unsigned(applied),"Phase callback count differs");
  check(initializations==call_count("initialize_round") && resets==call_count("reset_objects"),
      "Original initializer/reset delivery count differs");
  // Original preparation wrapper returns without a replication call for
  // these NONE match entities; no substitute notification is installed.
  check(begins<=1 && !calls.empty(),
      "External owner delivery count differs");
}
void preparation(const Json& fixture,const Json& record,bool absent_owner=false) {
  const auto before=raw(fixture,record.at("before").at("engine_hex"));
  const auto expected=raw(fixture,record.at("after").at("engine_hex"));
  const auto clock=raw(fixture,record.at("before").at("clock_hex"));
  SourceRoundLifecycle state;
  state.flags=load<std::uint32_t>(before);state.phase=load<std::int16_t>(before,0x6c);
  state.index=load<std::int16_t>(before,0x6e);state.start_tick=load<std::uint32_t>(before,0x70);
  std::memcpy(state.ranking.data(),before.data()+0x74,sizeof(state.ranking));
  state.clock_seconds=load<std::int16_t>(before,0xe0);
  state.countdown=load<std::int32_t>(before,0xc00);state.status=load<std::int32_t>(before,0xc04);
  auto stats=load<SourceGameEndStats>(before,0x304);SourceResultRecording recording;
  SourceRoundContext context;context.tick_rate=load<std::int16_t>(clock,2);
  context.match_replication_entity=load<std::uint32_t>(before,0x24);
  SourceRoundServices services;unsigned callbacks=0,dirty=0;bool arguments_match=false;
  services.phase_changed=[&](std::int32_t){++callbacks;}; // actual Slayer RET4
  if(!absent_owner && record.at("stop").is_object())
    services.dirty_entity=[&](std::uint32_t entity,std::uint32_t flags){
      ++dirty;const auto& arguments=record.at("stop").at("arguments");
      arguments_match=entity==arguments[0] && flags==arguments[1];
      throw std::runtime_error("Unprovided original dirty-entity owner");
    };
  bool stopped=false;
  try{source_round_apply_phase(state,context,stats,recording,{}, {},services);}
  catch(const std::runtime_error&){stopped=true;}
  check(stopped==record.at("stop").is_object(),"Original preparation completion/stop differs");
  check(callbacks==unsigned(!stopped),"Preparation phase callback ordering differs");
  check(dirty==unsigned(stopped && !absent_owner),"Preparation replication gate differs");
  check(!dirty || arguments_match,"Original preparation replication arguments differ");
  auto actual=before;
  store(actual,0,state.flags);store(actual,0x6c,state.phase);store(actual,0x6e,state.index);
  store(actual,0x70,state.start_tick);store(actual,0xe0,state.clock_seconds);
  std::memcpy(actual.data()+0x74,state.ranking.data(),sizeof(state.ranking));
  std::memcpy(actual.data()+0x304,&stats,sizeof(stats));
  store(actual,0xc00,state.countdown);store(actual,0xc04,state.status);
  check(actual==expected,"Complete original preparation engine record differs");
}
template<class F>void rejects(F&& action,const char* message) {
  bool failed=false;try{action();}catch(const std::runtime_error&){failed=true;}check(failed,message);
}
void missing_owners() {
  SourceGameEndStats stats;SourceResultRecording recording;SourceRoundContext context;
  SourceRoundLifecycle state;SourceRoundServices services;
  services.initialize_engine=[]{};services.phase_changed=[](std::int32_t){};
  state.index=1;
  rejects([&]{source_round_apply_phase(state,context,stats,recording,{}, {},services);},"Later rounds bypassed missing full object/map reset");
  state={};state.status=3;
  source_round_apply_phase(state,context,stats,recording,{}, {},services);
  check(state.phase==3 && state.countdown==30,"NONE match entity incorrectly required a preparation provider");
  state={};state.status=3;context.match_replication_entity=0x12340003;
  rejects([&]{source_round_apply_phase(state,context,stats,recording,{}, {},services);},"Preparation bypassed missing replication owner");
  context.match_replication_entity=0xffffffffu;
  state={};state.status=state.phase=2;state.index=31;state.countdown=1;
  rejects([&]{source_round_post_world(state,context,stats,recording,{}, {},services);},"Round cap bypassed missing game-over owner");
  state={};SourceRoundPlayer player;player.replication_entity=0x12340003;
  rejects([&]{source_round_apply_phase(state,context,stats,recording,std::span(&player,1),{},services);},
      "Respawn reset substituted generic replication dirtiness for original session/tracking branch");
}
}
int main(int argc,char** argv)try {
  check(argc==3 || argc==4,"Supply lifecycle, cap-order and optional preparation fixtures");unsigned cases=0;
  for(int file_index=1;file_index<3;++file_index) {
    std::ifstream file(argv[file_index]);Json fixture;file>>fixture;
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original evidence contains substituted results");
    for(const auto& [version,data]:fixture.at("versions").items())for(bool post:{false,true}) {
      const auto key=post ? "tick_rows" : "phase_rows";if(!data.contains(key))continue;
      const auto& inputs=fixture.at(post ? "tick_inputs" : "inputs");
      check(inputs.size()==data.at(key).size(),"Original input/record count differs");
      for(unsigned n=0;n<inputs.size();++n) {
        try{run(fixture,data.at("owners"),inputs[n],data.at(key)[n],post);}
        catch(const std::exception& error){std::cerr<<version<<' '<<key<<' '<<n<<' '<<inputs[n].dump()<<'\n';throw;}
        ++cases;
      }
    }
  }
  unsigned preparation_cases=0;
  if(argc==4) {
    std::ifstream file(argv[3]);Json fixture;file>>fixture;
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Preparation evidence contains substituted results");
    for(const auto& [version,data]:fixture.at("versions").items())for(const auto& record:data.at("phases")) {
      preparation(fixture,record);++preparation_cases;
      if(record.at("stop").is_object())preparation(fixture,record,true);
    }
  }
  missing_owners();
  std::cout<<cases<<" whole original phase/post-world records and "<<preparation_cases
      <<" preparation records pass; missing live owners stay unavailable\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
