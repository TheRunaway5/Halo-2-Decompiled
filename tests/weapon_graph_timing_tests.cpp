#include "halo2/fp_registration.hpp"
#include "halo2/content.hpp"
#include "halo2/weapon_energy.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5 && argc!=6)throw FormatError("Supply MAP SHARED XBE ORIGINAL_GRAPH_TIMING_FIXTURE");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  const auto original=OriginalData::load_xbe(argv[3]);
  const bool put_away=argc==6 && std::string(argv[5])=="put-away";
  if(argc==6 && !put_away)throw FormatError("Unknown graph timing scope");
  Cache cache(argv[1],argv[2],original);
  const auto content=put_away?std::optional<LevelContent>(cache.load_content(0,true)):std::nullopt;
  std::ifstream input(argv[4]);if(!input)return 77;
  std::map<std::pair<std::uint32_t,unsigned>,FirstPersonBindingDefinition> definitions;
  std::string line;unsigned cases{},nonzero{},no_models{};
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::uint32_t datum,representation,dual,no_model,activity,section,expected;
    if(!(row>>std::hex>>datum>>representation>>dual>>no_model>>activity>>section>>expected))
      throw FormatError("Truncated original graph timing fixture");
    const auto key=std::pair{datum,representation};
    if(!definitions.contains(key))definitions.emplace(key,cache.load_first_person_binding(datum,int(representation)));
    auto definition=definitions.at(key);
    if(no_model){definition.weapon_model=null_datum;++no_models;}
    const auto actual=std::uint16_t(weapon_animation_activity_timing(definition,dual,activity,
        std::int16_t(section),30,std::bit_cast<float>(original->melee_constants->frame_seconds_bits)));
    if(actual!=expected) {
      std::cerr<<std::hex<<"datum="<<datum<<" representation="<<representation<<" dual="<<dual
          <<" no_model="<<no_model<<" activity="<<activity<<" section="<<section
          <<" expected="<<expected<<" actual="<<actual<<'\n';
      throw FormatError("Native authored graph timing differs from unchanged original query");
    }
    if(content && activity==0x08000025u)if(const auto found=content->weapons.find(datum);
        found!=content->weapons.end() && found->second.barrel_timing) {
      const auto &timing=found->second.put_away_timing[representation==3][dual!=0];
      const auto retained=timing.present?std::uint16_t(weapon_animation_timing(timing.frames,timing.event4,timing.event0,
          std::int16_t(section),30,std::bit_cast<float>(original->melee_constants->frame_seconds_bits))):0;
      if(retained!=expected)throw FormatError("Retained unregistered put-away timing differs from original");
    }
    ++cases;nonzero+=actual!=0;
  }
  if(definitions.size()!=66 || cases!=(put_away?29568u:28512u) || no_models!=(put_away?14784u:14256u) || nonzero!=(put_away?5344u:4720u))
    throw FormatError("Incomplete original graph timing coverage");
  std::cout<<"PASS "<<cases<<" actual graph timing queries, "<<definitions.size()
      <<" represented weapons, "<<nonzero<<" nonzero results, including dual and absent render models\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
