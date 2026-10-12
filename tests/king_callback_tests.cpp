#include "halo2/source_king.hpp"
#include "halo2/source_result_statistics.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T>T decode(const std::string& hex) {
  check(hex.size()==sizeof(T)*2,"Original record width differs");T out;
  auto* bytes=reinterpret_cast<unsigned char*>(&out);
  for(unsigned n=0;n<sizeof(T);++n)bytes[n]=std::uint8_t(std::stoul(hex.substr(n*2,2),nullptr,16));
  return out;
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the original King callback fixture");
  std::ifstream file(argv[1]);Json fixture;file>>fixture;unsigned traits=0,deaths=0;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original callback evidence substituted returns");
  for(const auto& [version,rows]:fixture.at("versions").items()) {
    for(unsigned n=0;n<fixture.at("inputs").at("traits").size();++n) {
      const auto& input=fixture.at("inputs").at("traits")[n];
      const auto value=source_king_trait(input.at("inside_ticks"),input.at("kind"),input.at("hill_flags"),input.at("global_flags"));
      check(value==(rows.at("traits")[n].at("returned_al").get<unsigned>()!=0),"Whole original King trait differs");++traits;
    }
    SourceResultStatDescriptors descriptors{};
    for(const auto& [stat,hex]:rows.at("descriptors").items())descriptors.at(std::stoul(stat))=decode<SourceResultStatDescriptor>(hex);
    for(unsigned n=0;n<fixture.at("inputs").at("death").size();++n) {
      const auto& input=fixture.at("inputs").at("death")[n];const auto& row=rows.at("death")[n];
      auto bank=decode<SourceResultStatistics>(fixture.at("raw_blobs").at(row.at("before_bank").get<std::string>()));
      const auto expected=decode<SourceResultStatistics>(fixture.at("raw_blobs").at(row.at("after_bank").get<std::string>()));
      const SourceResultRecording recording{input.at("recording")[0],input.at("recording")[1]};
      SourceResultPublication publication{bank,recording,descriptors};Json calls=Json::array();
      SourceKingPlayer victim{0x9abc000f,0xffffffffu,input.at("membership"),input.at("victim_inside")};
      SourceKingPlayer attacker{input.at("credit")=="self" ? victim.datum : 0x9abc0003u,
          0xffffffffu,input.at("membership"),input.at("attacker_inside")};
      // Self credit names the actual victim record, not a second timer view.
      if(input.at("credit")=="self")attacker.inside_ticks=victim.inside_ticks;
      source_king_death_statistics(victim,input.at("credit")=="none" ? nullptr : &attacker,
          std::uint8_t(input.at("betrayal").get<unsigned>()),[&](std::uint32_t player,int stat) {
        const int team=input.at("team_mode").get<bool>() ? attacker.membership : -1;
        calls.push_back(Json{{"delta",1},{"player",player&0xffffu},{"stat",stat},{"team",std::uint32_t(team)}});
        source_result_stat_add(publication,stat,int(player&0xffffu),team,1);
      });
      if(calls!=row.at("retained_add_calls") || std::memcmp(&bank,&expected,sizeof(bank))!=0) {
        std::cerr<<version<<" death="<<n<<' '<<input.dump()<<'\n';throw std::runtime_error("Whole original King retained stats differ");
      }
      ++deaths;
    }
  }
  SourceKingPlayer victim{1,2,0,1},attacker{3,4,0,1};bool rejected=false;
  try{source_king_death_statistics(victim,&attacker,0,{});}catch(const std::runtime_error&){rejected=true;}
  check(rejected,"King fabricated an absent retained-stat service");
  source_king_death_statistics(victim,nullptr,0,{});
  source_king_death_statistics(victim,&attacker,1,{});
  std::cout<<"Original King callbacks: "<<traits<<" traits, "<<deaths<<" whole retained banks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
