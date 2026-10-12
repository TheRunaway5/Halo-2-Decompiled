#include "halo2/source_round_end.hpp"
#include "halo2/source_game_end.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<SourceRoundEndParticipant> players(const Json& row) {
  std::vector<SourceRoundEndParticipant> out;
  for(const auto& p:row.at("players"))out.push_back({p.at("datum"),p.at("flags"),p.at("membership"),p.at("unit"),p.at("lives")});
  return out;
}
SourceRoundEndInput input(const Json& row) {
  SourceRoundEndInput out;out.engine_present=row.value("engine_present",true);out.team_mode=row.at("team_mode");
  out.life_limit=row.at("limit");out.retained_team_mask=row.at("team_mask");out.source_engine=row.at("source_engine");
  out.variant_240=row.value("variant_240",0);out.engine_2f0=row.value("engine_2f0",0xffffffffu);
  out.lifecycle=row.value("lifecycle",1);out.status=row.value("status",1);out.raw_mode=row.value("mode",1);return out;
}
SourceGameEndStats stats(const std::string& hex) {
  check(hex.size()==sizeof(SourceGameEndStats)*2,"Original stat bank has wrong width");
  std::array<std::uint8_t,sizeof(SourceGameEndStats)> bytes{};
  for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return std::bit_cast<SourceGameEndStats>(bytes);
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the original round-end fixture");std::ifstream file(argv[1]);Json fixture;file>>fixture;unsigned calls=0;
  for(const auto& [version,data]:fixture.at("versions").items()) {
    const auto& queries=fixture.at("inputs").at("queries");
    for(unsigned n=0;n<queries.size();++n) {
      const auto& expected=data.at("queries")[n];const auto actual=source_round_end_query(input(queries[n]),players(queries[n]),expected.at("initial_winner"));
      check(actual.end==bool(expected.at("end").get<int>()) && actual.force==bool(expected.at("force").get<int>()) &&
          actual.winner==expected.at("winner"),"Whole original participant/life end query differs");calls+=2;
    }
    const auto& ticks=fixture.at("inputs").at("ticks");
    for(unsigned n=0;n<ticks.size();++n) {
      const auto& row=ticks[n];const auto& expected=data.at("ticks")[n];const auto options=input(row);const auto roster=players(row);
      const auto decision=source_round_end_check(options,roster);auto bank=stats(expected.at("before_bank"));
      auto after=expected.at("before");
      if(decision.end) {
        std::vector<SourceGameEndParticipant> award;
        for(const auto& p:roster)award.push_back({p.datum,p.membership,p.flags});
        const auto wins=source_game_end_award(bank,award,options.team_mode,decision.winner);
        const bool finishes=source_round_finishes(row.at("round_kind"),0,decision.winner,wins,decision.force);
        after["flags"]=after.at("flags").get<std::uint32_t>()|0x20;
        if(options.raw_mode!=4) {
          after["status"]=2;
          if(finishes){after["game_over"]=1;after["countdown"]=210;}
        }
      }
      const auto original=stats(expected.at("after_bank"));
      if(std::memcmp(&bank,&original,sizeof(bank)) || after!=expected.at("after")) {
        std::cerr<<version<<" wrapper case "<<n<<": "<<row.dump()<<" actual "<<after.dump()<<" expected "<<expected.at("after").dump()<<'\n';
        throw std::runtime_error("Whole original active wrapper/award/end fields differ");
      }
      ++calls;
    }
  }
  check(calls==fixture.at("whole_calls"),"Original round-end call count changed");
  std::cout<<calls<<" whole original participant/life end queries and active-wrapper comparisons pass\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
