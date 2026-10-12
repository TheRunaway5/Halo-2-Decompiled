#include "halo2/source_result_statistics.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
using nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<std::uint8_t> hex(const std::string& s) {
  check(s.size()%2==0,"Odd source hex record");std::vector<std::uint8_t> result;
  for(std::size_t i=0;i<s.size();i+=2)result.push_back(std::uint8_t(std::stoul(s.substr(i,2),nullptr,16)));
  return result;
}
SourceResultStatistics seed() {
  SourceResultStatistics bank;
  auto* raw=reinterpret_cast<std::uint8_t*>(&bank);
  for(unsigned i=0;i<0x2020;++i) {
    const auto v=std::uint16_t(i*73+0x3141);raw[i*2]=std::uint8_t(v);raw[i*2+1]=std::uint8_t(v>>8);
  }
  return bank;
}
SourceResultStatDescriptors descriptors(const json& rows) {
  SourceResultStatDescriptors result;
  for(unsigned i=0;i<45;++i) {
    result[i].minimum=rows[i].at("minimum");result[i].maximum=rows[i].at("maximum");
    const auto tail=hex(rows[i].at("remaining_hex"));check(tail.size()==12,"Descriptor tail size differs");
    std::copy(tail.begin(),tail.end(),result[i].remaining.begin());
  }
  return result;
}
void expected(SourceResultStatistics& bank,const json& words) {
  for(const auto& row:words) {
    const unsigned slot=row.at("slot"),stat=row.at("stat");
    (row.at("player").get<bool>() ? bank.players.at(slot).at(stat) : bank.teams.at(slot).at(stat))=row.at("value");
  }
}
SourceGameEndStats match_bank(const json& encoded) {
  const auto bytes=hex(encoded);check(bytes.size()==sizeof(SourceGameEndStats),"Match bank size differs");
  SourceGameEndStats result;std::memcpy(&result,bytes.data(),bytes.size());return result;
}
void same(const SourceResultStatistics& a,const SourceResultStatistics& b) {
  check(std::memcmp(&a,&b,sizeof(a))==0,"Retained bank differs from original writer or unclassified bytes changed");
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original result-stat fixture");
    std::ifstream stream(argv[1]);json fixture;stream>>fixture;
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original oracle patched or bypassed a dependency");
    unsigned cases=0;
    for(const auto& version:fixture.at("versions")) {
      const auto table=descriptors(version.at("descriptors"));
      for(const auto& row:version.at("direct")) {
        auto actual=seed(),wanted=seed();expected(wanted,row.at("after"));
        const SourceResultRecording flags{row.at("flags")[0],row.at("flags")[1]};
        SourceResultPublication p{actual,flags,table};
        if(row.at("op")=="set")source_result_stat_set(p,row.at("stat"),row.at("player"),row.at("team"),row.at("value"));
        else source_result_stat_add(p,row.at("stat"),row.at("player"),row.at("team"),row.at("value"));
        same(actual,wanted);++cases;
      }
      for(const auto& row:version.at("writers")) {
        auto actual=seed(),wanted=seed();expected(wanted,row.at("after"));
        const SourceResultRecording flags{row.at("flags")[0],row.at("flags")[1]};
        SourceResultPublication p{actual,flags,table};auto bank=match_bank(row.at("before_bank_hex"));
        if(row.at("player").get<bool>())source_game_stat_player_add(bank,row.at("slot"),row.at("field"),row.at("delta"),row.at("stat"),&p,row.at("team_destination"));
        else source_game_stat_team_add(bank,row.at("slot"),row.at("field"),row.at("delta"),row.at("stat"),&p);
        const auto after=match_bank(row.at("after_bank_hex"));
        check(std::memcmp(&bank,&after,sizeof(bank))==0,"Whole original player/team match writer differs");
        same(actual,wanted);++cases;
      }
    }
    const auto table=descriptors(fixture.at("versions").at("u5").at("descriptors"));
    for(const auto& row:fixture.at("recorded_awards")) {
      auto actual=seed(),wanted=seed();expected(wanted,row.at("after"));
      const SourceResultRecording flags{1,0};SourceResultPublication p{actual,flags,table};
      auto bank=match_bank(row.at("before_bank_hex"));std::vector<SourceGameEndParticipant> roster;
      for(const auto& r:row.at("roster"))roster.push_back({r.at("datum"),r.at("membership"),r.at("flags")});
      source_game_end_award(bank,roster,row.at("team"),std::uint32_t(std::stoul(row.at("winner").get<std::string>(),nullptr,16)),&p);
      const auto after=match_bank(row.at("after_bank_hex"));
      check(std::memcmp(&bank,&after,sizeof(bank))==0,"Actual recorded round-end award differs");same(actual,wanted);++cases;
    }
    std::cout<<"PASS "<<cases<<" unchanged original result/match writers and recorded win awards, preserving all other bank bytes\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
