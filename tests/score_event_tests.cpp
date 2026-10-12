#include "halo2/source_score_event.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string event_hex(const VistaAnnouncerEvent& event) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  const auto append=[&](std::uint32_t value,unsigned count) {
    for(unsigned i=0;i<count;++i){const auto byte=std::uint8_t(value>>(i*8));out+=digits[byte>>4];out+=digits[byte&15];}
  };
  for(const auto value:event.words)append(value,4);
  append(event.data,2);return out;
}
void reload_and_unavailable() {
  SourceScoreEventServices services;SourceScoreEventOptions options{100,0,1};unsigned reloads{},engine{},membership{};
  services.options=[&]()->const auto&{++reloads;return options;};
  services.engine_present=[&]{++engine;return true;};services.membership=[&](std::uint32_t){++membership;return std::int8_t(-1);};
  std::vector<std::uint32_t> events;
  services.network=[&](const auto& event){events.push_back(event.words[1]);options.score_limit+=event.words[1]==47?20:event.words[1]==9?30:0;};
  services.local=[&](const auto& event){events.push_back(event.words[1]);};
  source_score_changed_event(0x9abc0000,90,services);
  check(events==std::vector<std::uint32_t>{47,47,9,9,7,7} && reloads==3 && engine==3 && membership==3,"Source option reload or independent thresholds were folded away");
  options={100,1,4};services.network={};services.local={};engine=membership=0;
  source_score_changed_event(0x9abc0000,90,services);
  check(engine==1 && membership==1,"Playback suppressed original engine/player reads before the mode gate");
  options.raw_mode=1;bool rejected=false;
  try{source_score_changed_event(0x9abc0000,90,services);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"Missing network publisher became success");
  bool network_called=false;services.network=[&](const auto&){network_called=true;};rejected=false;
  try{source_score_changed_event(0x9abc0000,90,services);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected && network_called,"Missing local dispatcher did not preserve prior network call");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply whole original score-event report");std::ifstream input(argv[1]);Json fixture;input>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original score-event proof substitutes returns");
  unsigned calls{},publications{};
  for(const auto& [version,data]:fixture.at("versions").items())for(unsigned n=0;n<fixture.at("inputs").size();++n)try {
    const auto& row=fixture.at("inputs")[n];SourceScoreEventOptions options{row.at("limit"),row.at("team_mode").get<bool>()?1u:0u,row.at("mode")};
    SourceScoreEventServices services;services.options=[&]()->const auto&{return options;};
    services.engine_present=[&]{return row.at("engine_present").get<bool>();};
    services.membership=[&](std::uint32_t player){check(player==row.at("player"),"Score publication changed full raw player identity");return row.at("membership").get<std::int8_t>();};
    Json events=Json::array();services.network=[&](const auto& event){events.push_back(Json{{"kind","network"},{"event34_hex",event_hex(event)}});};
    services.local=[&](const auto& event){events.push_back(Json{{"kind","local"},{"event34_hex",event_hex(event)}});};
    source_score_changed_event(row.at("player"),std::bit_cast<std::int32_t>(row.at("score").get<std::uint32_t>()),services);
    check(events==data.at("results")[n],"Original score event prefix/order/gate differs");publications+=unsigned(events.size());++calls;
  }catch(const std::exception& error){throw std::runtime_error(version+" input "+std::to_string(n)+": "+error.what());}
  reload_and_unavailable();std::cout<<calls<<" whole original score-change calls and "<<publications<<" original network/local prefixes matched\n";
  std::cout<<"LIMIT original ordinary no-local-user dispatch; live native network/caption/audio consumers remain separate\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
