#include "halo2/unit_damage.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const char *why) {
  if (!value) throw FormatError(why);
}
std::uint32_t word(std::istringstream &row) {
  std::string value; row >> value;
  if (row.fail()) throw FormatError("Missing original unit damage field");
  return std::stoul(value, nullptr, 16);
}
float real(std::istringstream &row) { return std::bit_cast<float>(word(row)); }
std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply Lockout and original damage-effect fixtures");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    Cache cache(argv[1]); const auto content=cache.load_content(0,true);
    const auto weapon=content.weapons.at(cache.slayer_starting_weapon(5));
    const auto &v=content.player.vitality;
    check(v.shield_damaged_first_person_shader==0xe9f4086fu &&
          v.shield_damaged_shader==0xe9f70872u &&
          v.shield_damaged_effect==null_datum &&
          v.shield_depleted_effect==0xe95507d0u &&
          v.shield_recharging_effect==0xe95a07d5u,
          "Original Lockout shield presentation references differ");
    unsigned count=0, attempts=0;
    std::ifstream file(argv[2]);
    for (std::string line; std::getline(file,line);) {
      if (line.empty() || line[0]=='#') continue;
      std::istringstream row(line); unsigned id; row >> id;
      RechargeState state; state.shield=real(row); state.body=real(row);
      const auto amount=real(row); unsigned radius; row >> radius;
      auto definition=v; definition.shield_damaged_threshold=real(row);
      const auto flags=word(row); state.dead=(flags&4)!=0;
      state.shield_damaged=(flags&2)!=0; state.shield_depleted=(flags&8)!=0;
      auto damage=weapon; damage.damage_flags=word(row); definition.shield=real(row);
      const auto body=word(row), shield=word(row), after_flags=word(row);
      int body_stun, shield_stun; unsigned effects;
      row >> body_stun >> shield_stun >> effects;
      const auto result=apply_unit_damage(state,definition,damage,
          content.damage_groups,content.damage_materials,amount,radius!=0);
      const unsigned actual_flags=(state.dead?4:0)|(state.shield_damaged?2:0)|
          (state.shield_depleted?8:0);
      if (bits(state.body)!=body || bits(state.shield)!=shield ||
          actual_flags!=(after_flags&14) || state.body_stun!=body_stun ||
          state.shield_stun!=shield_stun || result.effect_count!=effects)
        throw FormatError("Unit damage/effect fixture differs: "+std::to_string(id));
      for (const auto &request:result.effects()) {
        char kind; row >> kind; const auto datum=word(row);
        check(request.kind==(kind=='D'?UnitDamageEffectKind::ShieldDepleted:
                                     UnitDamageEffectKind::ShieldDamaged) &&
              request.datum==datum, "Original shield effect order/tag differs");
        ++attempts;
      }
      check(!row.fail(),"Truncated original shield fixture"); std::string extra;
      check(!(row>>extra),"Trailing original shield fixture"); ++count;
    }
    check(count==25,"Incomplete original shield effect fixtures");
    std::cout << count << " original unit damage cases and " << attempts
              << " ordered shield effect attempts matched; presentation refs imported\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
