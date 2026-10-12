#include "halo2/unit_damage.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
std::uint32_t word(std::istringstream &row) {
  std::string value;row>>value;
  if(row.fail())throw FormatError("Missing original recharge field");
  return std::stoul(value,nullptr,16);
}
float real(std::istringstream &row){return std::bit_cast<float>(word(row));}
std::uint16_t flags(const RechargeState &s) {
  return (s.dead?4:0)|(s.shield_damaged?2:0)|(s.shield_depleted?8:0)|
      (s.body_recharging?0x100:0)|(s.shield_recharging?0x200:0);
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw FormatError("Supply unchanged original recharge-transition fixture");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream file(argv[1]);unsigned cases=0,completed=0,requests=0;
    for(std::string line;std::getline(file,line);) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);unsigned id,species;row>>id>>species;
      RechargeState state;state.body=real(row);state.shield=real(row);
      row>>state.body_stun>>state.shield_stun;const auto before_flags=word(row);
      state.dead=before_flags&4;state.shield_damaged=before_flags&2;
      state.shield_depleted=before_flags&8;state.body_recharging=before_flags&0x100;
      state.shield_recharging=before_flags&0x200;
      VitalityDefinition definition;definition.health=real(row);definition.shield=real(row);
      definition.shield_rate=real(row);definition.body_rate=real(row);definition.body_max=real(row);
      const float dt=real(row),team_scale=real(row);
      definition.shield_recharging_effect=species==0?0xe95a07d5u:0xea8c0907u;
      unsigned requested,returned;row>>requested;const auto effect=word(row);row>>returned;
      const auto body=word(row),shield=word(row);int body_stun,shield_stun;
      row>>body_stun>>shield_stun;const auto after_flags=word(row);
      std::string changed;row>>changed;
      const auto result=update_unit_vitality(state,definition,dt,team_scale);
      if(bool(result.effect)!=bool(requested) || (result.effect &&
          (result.effect->kind!=UnitDamageEffectKind::ShieldRecharging || result.effect->datum!=effect)))
        throw FormatError("Original recharge effect differs in case "+std::to_string(id));
      // Stopped originals establish only the pre-effect branch. Their later
      // shield update/allocation results are deliberately not asserted.
      if(returned) {
        if(std::bit_cast<std::uint32_t>(state.body)!=body ||
            std::bit_cast<std::uint32_t>(state.shield)!=shield ||
            state.body_stun!=body_stun || state.shield_stun!=shield_stun ||
            flags(state)!=(after_flags&0x30eu) || result.changed!=(changed=="1"))
          throw FormatError("Original recharge state differs in case "+std::to_string(id));
        ++completed;
      }
      if(row.fail())throw FormatError("Truncated original recharge fixture");
      std::string extra;if(row>>extra)throw FormatError("Trailing original recharge field");
      requests+=requested;++cases;
    }
    if(cases!=42 || completed!=34 || requests!=8)throw FormatError("Incomplete original recharge fixtures");
    std::cout<<cases<<" original transition cases, "<<completed<<" complete states and "
        <<requests<<" pre-effect boundaries matched\n";
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
