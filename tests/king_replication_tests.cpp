#include "halo2/source_king_replication.hpp"
#include "halo2/source_game_over.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
std::vector<std::uint8_t> bytes(const std::string& hex) {
  check(hex.size()%2==0,"Odd original byte string");std::vector<std::uint8_t> out(hex.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return out;
}
std::vector<std::uint8_t> blob(const Json& fixture,const Json& key) {
  return bytes(fixture.at("raw_blobs").at(key.get<std::string>()));
}
template<class T>T decode(std::span<const std::uint8_t> raw) {
  check(raw.size()==sizeof(T),"Original object width differs");T value;
  std::memcpy(&value,raw.data(),sizeof(T));return value;
}
template<class T>void equal(const T& value,std::span<const std::uint8_t> raw,const char* message) {
  check(raw.size()==sizeof(T) && std::memcmp(&value,raw.data(),sizeof(T))==0,message);
}
std::uint16_t word(std::span<const std::uint8_t> raw,unsigned at){return std::uint16_t(raw[at]|unsigned(raw[at+1])<<8);}
void word(std::span<std::uint8_t> raw,unsigned at,std::uint16_t value){raw[at]=std::uint8_t(value);raw[at+1]=std::uint8_t(value>>8);}
SourceEngineReplicationState shared(std::span<const std::uint8_t> raw) {
  SourceEngineReplicationState state;
  for(unsigned i=0;i<14;++i)state.words_06_21[i]=word(raw,6+i*2);
  state.phase=std::bit_cast<std::int16_t>(word(raw,0x6c));state.round=word(raw,0x6e);
  state.clock_seconds=word(raw,0xe0);state.game_over=raw[0xc08];return state;
}
void encode(std::span<std::uint8_t> raw,const SourceEngineReplicationState& state) {
  for(unsigned i=0;i<14;++i)word(raw,6+i*2,state.words_06_21[i]);
  word(raw,0x6c,std::uint16_t(state.phase));word(raw,0x6e,state.round);
  word(raw,0xe0,state.clock_seconds);raw[0xc08]=state.game_over;
}
Json describe(const SourceEngineReplicationState& state) {
  return Json{{"words",state.words_06_21},{"phase",state.phase},{"round",state.round},
    {"clock_seconds",state.clock_seconds},{"game_over",state.game_over}};
}
std::array<std::uint8_t,8> encode(const SourceGameOverState& state,const std::array<std::uint8_t,8>& original) {
  auto raw=original;raw[0]=std::uint8_t(state.active);raw[1]=std::uint8_t(state.fade_started);
  std::memcpy(raw.data()+4,&state.countdown,4);return raw;
}
struct Event {
  std::string kind;Json shared;SourceKingState king;
  std::array<std::uint8_t,8> game_over;unsigned phase{};
};
void missing_services() {
  SourceEngineReplicationState state;state.phase=-1;
  SourceKingState king;king.hill=5;king.occupants=12;
  SourceKingReplicationBank bank;bank.shared.words.fill(0x1234);bank.shared.phase=2;
  bank.shared.round=3;bank.shared.clock_seconds=4;bank.hill=-1;bank.occupants=7;
  SourceEngineReplicationServices services;unsigned players=0,recording=0,phases=0;
  const auto rejected=[&](std::uint32_t mask) {
    try{source_king_replication_apply(state,king,{},mask,bank,services);return false;}
    catch(const std::runtime_error&){return true;}
  };
  check(rejected(0x7f) && state.words_06_21==bank.shared.words && state.phase==-1 && king.hill==5,
    "Missing player refresh silently succeeded or stores ran out of order");
  services.refresh_player_properties=[&]{++players;};
  check(rejected(0x7f) && players==1 && state.phase==-1,"Missing recording refresh was skipped");
  services.refresh_recording=[&]{++recording;};
  check(rejected(0x7f) && players==2 && recording==1 && state.phase==-1,"Missing phase owner was skipped");
  services.phase_changed=[&](std::uint8_t value){check(state.phase==-1 && value==2,"Phase store preceded callback");++phases;};
  check(rejected(0x7f) && phases==1 && state.phase==2 && state.round==0 && king.hill==5,
    "False bank game-over flag skipped required owner or later stores ran early");
  // An unchanged phase requires no callback. No requested service bits means
  // no world callbacks, even when the input bank contains a game-over flag.
  source_king_replication_apply(state,king,{},0x80000002,bank,{});
  check(source_king_replication_apply(state,king,{},0x80000060,bank,{}) && king.hill==-1 && king.occupants==7,
    "Service-free application unexpectedly required a provider");
  // The source stores a changed hill before build. A rejected authored input
  // must preserve that ordering and leave the later occupant store untouched.
  bank.hill=0;bank.occupants=19;king.occupants=7;
  std::vector<SourceKingMarker> unavailable(32768);
  bool failed=false;
  try{source_king_replication_apply(state,king,unavailable,0x60,bank,{});}catch(const std::runtime_error&){failed=true;}
  check(failed && king.hill==0 && king.occupants==7,"Hill build failure changed source store order");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply unchanged-original King replication fixture");std::ifstream input(argv[1]);Json fixture;input>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original replication proof supplies replacement results");
  check(std::fesetround(FE_TONEAREST)==0,"Unable to select original nearest rounding");
  std::vector<std::vector<SourceKingMarker>> maps;
  for(const auto& row:fixture.at("maps")) {
    const auto raw=bytes(row.at("rows_hex"));check(raw.size()%32==0,"Scenario row width differs");
    maps.emplace_back(raw.size()/32);if(!raw.empty())std::memcpy(maps.back().data(),raw.data(),raw.size());
  }
  unsigned initializations=0,differences=0,applications=0,callbacks=0,builds=0;
  for(const auto& [version,data]:fixture.at("versions").items()) {
    SourceGameOverConstants game_constants;
    game_constants.seconds=decode<float>(bytes(data.at("game_over_seconds_hex")));
    for(unsigned n=0;n<fixture.at("inputs").size();++n) {
      const auto& row=fixture.at("inputs")[n];const auto& expected=data.at("results")[n];
      auto bank=decode<SourceKingReplicationBank>(blob(fixture,row.at("bank")));
      const auto operation=row.at("operation").get<std::string>();
      if(operation=="initialize") {source_king_replication_initialize(bank);++initializations;}
      else {
        const auto raw=blob(fixture,row.at("globals"));auto state=shared(raw);
        auto king=decode<SourceKingState>(std::span(raw).subspan(0xfc,sizeof(SourceKingState)));
        std::uint32_t mask=row.value("initial_changed",row.at("requested").get<std::uint32_t>());
        if(operation=="difference")source_king_replication_difference(state,king,mask,bank);
        else source_engine_replication_difference(state,row.at("requested"),mask,bank.shared);
        check(mask==expected.at("changed"),"Original changed mask differs");
        equal(bank,blob(fixture,expected.at("bank")),"Original difference bank differs");++differences;
        if(row.value("repeat",false)) {
          mask=row.at("requested");source_king_replication_difference(state,king,mask,bank);
          check(mask==expected.at("repeated_changed"),"Original repeated changed mask differs");
          equal(bank,blob(fixture,expected.at("repeated_bank")),"Original repeated difference bank differs");++differences;
        }
        continue;
      }
      equal(bank,blob(fixture,expected.at("bank")),"Original initialized bank differs");
    }
    for(unsigned n=0;n<data.at("apply_inputs").size();++n)try {
      const auto& row=data.at("apply_inputs")[n];const auto& expected=data.at("apply_results")[n];
      auto raw=blob(fixture,row.at("globals"));auto state=shared(raw);
      auto king=decode<SourceKingState>(std::span(raw).subspan(0xfc,sizeof(SourceKingState)));
      const auto initial_king=king;const auto bank=decode<SourceKingReplicationBank>(blob(fixture,row.at("bank")));
      const auto game_raw=decode<std::array<std::uint8_t,8>>(bytes(row.at("game_over_hex")));
      SourceGameOverState game;game.active=game_raw[0]!=0;game.fade_started=game_raw[1]!=0;
      std::memcpy(&game.countdown,game_raw.data()+4,4);std::vector<Event> events;
      const auto observe=[&](const char* kind,unsigned phase=0) {
        events.push_back({kind,describe(state),king,encode(game,game_raw),phase});
      };
      SourceEngineReplicationServices services;
      // These callbacks represent the original empty-roster/disabled-recorder
      // gates, which the fixture executes fully. They do not admit a live world.
      services.refresh_player_properties=[&]{observe("refresh_player_properties");};
      services.refresh_recording=[&]{observe("refresh_recording");};
      services.phase_changed=[&](std::uint8_t value) {
        observe("phase_changed",value);
        check(value!=1 && (value!=3 || row.at("raw_mode")==4),"Fixture requires an unbound world-phase owner");
      };
      services.begin_game_over=[&] {
        observe("begin_game_over");source_game_over_begin(game,row.at("rate"),game_constants);
      };
      const bool result=source_king_replication_apply(state,king,maps.at(row.at("map")),row.at("mask"),bank,services);
      encode(raw,state);std::memcpy(raw.data()+0xfc,&king,sizeof(king));
      check(raw==blob(fixture,expected.at("globals")),"Whole original applied MP globals differ");
      equal(encode(game,game_raw),bytes(expected.at("game_over_hex")),"Original game-over service effects differ");
      check(unsigned(result)==expected.at("returned_al"),"Original apply result differs");
      unsigned index=0,original_builds=0;
      for(const auto& event:expected.at("events")) {
        if(event.at("kind")=="build") {
          check(index==events.size(),"Hill build preceded a shared service");auto before_build=initial_king;before_build.hill=bank.hill;
          equal(before_build,blob(fixture,event.at("king")),"Original hill was not stored before rebuild");++original_builds;continue;
        }
        check(index<events.size(),"Native skipped an original shared service");const auto& actual=events[index++];
        check(actual.kind==event.at("kind").get<std::string>() && actual.shared==event.at("shared"),"Original shared callback order/state differs");
        equal(actual.king,blob(fixture,event.at("king")),"King state changed before a shared callback");
        equal(actual.game_over,bytes(event.at("game_over_hex")),"Shared service game-over pre-state differs");
        if(event.contains("phase"))check(actual.phase==event.at("phase"),"Original phase callback argument differs");
      }
      check(index==events.size(),"Native added an original shared service");
      check(original_builds==unsigned((row.at("mask").get<std::uint32_t>()&0x20) && initial_king.hill!=bank.hill),
        "Original hill rebuild gate differs");
      callbacks+=unsigned(events.size());builds+=original_builds;++applications;
    }catch(const std::exception&){std::cerr<<version<<" apply="<<n<<'\n';throw;}
  }
  missing_services();
  std::cout<<"Original King replication: "<<initializations<<" initializations, "<<differences<<" differences, "
    <<applications<<" applications, "<<callbacks<<" shared callbacks, "<<builds<<" changed-hill builds; required-service boundaries pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
