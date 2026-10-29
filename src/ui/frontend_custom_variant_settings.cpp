#include "ui/frontend_custom_variant_settings.hpp"
#include "halo2/bomb_defusal.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <bit>
#include <iomanip>
#include <set>
#include <sstream>

namespace halo2::detail {
namespace {
bool admitted(const UiVariant &variant,unsigned parameter,int value) {
  auto candidate=variant;
  candidate.values.set(parameter,value);
  try {
    const auto rules=slayer_rules(candidate,true);
    return parameter!=90 || rules.maximum_players==unsigned(value);
  }
  catch(const FormatError &) {return false;}
}
bool meaningful(const UiVariant &variant,unsigned parameter) {
  if(bomb_defusal_variant(variant)) {
    // Round wins, lives, sides, loadouts and map equipment define this mode.
    // Ordinary respawn/kill scoring controls do not drive its round machine.
    switch(parameter) {
    case 10:case 90:case 24:case 104:case 105:return true;
    default:return false;
    }
  }
  const auto mode=custom_slayer(variant);
  if(mode==CustomSlayer::fiesta) {
    if(parameter>=1 && parameter<=9)return true; // Original score aliases.
    switch(parameter) {
    case 10:case 90:case 16:case 17:case 18:case 29:case 30:
    case 88:case 104:case 105:return true;
    default:return false;
    }
  }
  if(mode==CustomSlayer::infection) {
    // Faction loadouts, zombie traits, unlimited lives and survival/conversion
    // endings stay fixed. Global equipment controls would misstate that policy.
    switch(parameter) {
    case 10:case 90:case 17:case 18:case 29:case 30:
    case 104:case 105:return true;
    default:return false;
    }
  }
  return false;
}
std::string seconds_label(int ticks) {
  std::ostringstream text;
  text<<std::fixed<<std::setprecision(3)<<double(ticks)/30.;
  auto result=text.str();
  while(result.ends_with('0'))result.pop_back();
  if(result.ends_with('.'))result.pop_back();
  return result+(ticks==30?" second":" seconds");
}
} // namespace

bool frontend_custom_variant(const UiVariant &variant) {
  return custom_slayer(variant)!=CustomSlayer::none || bomb_defusal_variant(variant);
}
std::vector<unsigned> frontend_custom_variant_parameters(const UiVariant &variant) {
  if(!bomb_defusal_variant(variant))return {};
  return {frontend_defusal_fuse_datum,frontend_defusal_plant_datum,frontend_defusal_disarm_datum};
}
int frontend_variant_parameter_value(const UiVariant &variant,const UiParameter &parameter) {
  const bool native_timer=bomb_defusal_variant(variant) &&
      parameter.parameter>=native_defusal_fuse_parameter && parameter.parameter<=native_defusal_disarm_parameter;
  if(!native_timer) {
    const auto source=source_game_variant(variant);
    // The custom adapter has a single participant limit, not a separate
    // active-player pool. Parameter15 is an additional saved capacity bound.
    if(parameter.parameter==90 && frontend_custom_variant(variant))
      return std::min(source_game_variant_get(source,15),source_game_variant_get(source,90));
    return source_game_variant_get(source,std::bit_cast<std::int32_t>(std::uint32_t(parameter.parameter)));
  }
  auto value=parameter.default_value;
  for(const auto &[id,write]:variant.values)if(id==parameter.parameter)value=write;
  return value;
}
std::optional<UiParameter> frontend_custom_variant_parameter(const UiVariant &variant,unsigned datum) {
  if(!bomb_defusal_variant(variant))return {};
  UiParameter result;
  const BombDefusalRules defaults;
  std::set<int> seconds;
  if(datum==frontend_defusal_fuse_datum) {
    result.parameter=native_defusal_fuse_parameter;result.default_value=int(defaults.fuse_ticks);
    result.title=result.header="Bomb Fuse";
    result.description="Time from planting until the bomb explodes. This must be longer than the defuse time.";
    seconds={10,15,20,25,30,35,40,45,50,55,60,75,90,120,150,180};
  } else if(datum==frontend_defusal_plant_datum) {
    result.parameter=native_defusal_plant_parameter;result.default_value=int(defaults.plant_ticks);
    result.title=result.header="Plant Time";
    result.description="Hold Use, stay still and remain grounded for this long to plant the bomb.";
    seconds={1,2,3,4,5,6,7,8,10,12,15,20,30};
  } else if(datum==frontend_defusal_disarm_datum) {
    result.parameter=native_defusal_disarm_parameter;result.default_value=int(defaults.defuse_ticks);
    result.title=result.header="Defuse Time";
    result.description="Hold Use, stay still and remain grounded for this long to defuse. This must be shorter than the fuse.";
    seconds={1,2,3,4,5,6,7,8,10,12,15,20,30};
  } else return {};
  std::set<int> values;
  for(const auto value:seconds)values.insert(value*30);
  values.insert(frontend_variant_parameter_value(variant,result));
  for(const auto value:values)if(admitted(variant,result.parameter,value))
    result.choices.push_back({seconds_label(value),value});
  if(result.choices.empty())return {};
  return result;
}
std::optional<UiParameter> frontend_custom_variant_parameter(const UiVariant &variant,const UiParameter &parameter) {
  if(!frontend_custom_variant(variant))return parameter;
  if(!meaningful(variant,parameter.parameter))return {};
  auto result=parameter;
  result.choices.clear();
  if(parameter.parameter==10 && bomb_defusal_variant(variant)) {
    result.title=result.header="Plant Window";
    result.description="Time attackers have to plant the bomb. A planted bomb uses its own fuse.";
  } else if(parameter.parameter==10 && custom_slayer(variant)==CustomSlayer::infection) {
    result.title=result.header="Survival Time";
    result.description="Time humans must survive. The timer begins once at least two players have joined.";
  } else if(parameter.parameter==90) {
    result.title=result.header="Player Limit";
    result.description="Maximum number of players in this match.";
  }
  const auto before=source_game_variant(variant);
  const auto current=frontend_variant_parameter_value(variant,parameter);
  bool changes{};
  const auto accepts=[&](int value) {
    if(parameter.parameter==90 &&
        (bomb_defusal_variant(variant) || custom_slayer(variant)==CustomSlayer::infection) && value<2)return false;
    return admitted(variant,parameter.parameter,value);
  };
  for(const auto &choice:parameter.choices) {
    // Both Infection and Defusal need at least one participant on each side.
    if(!accepts(choice.value))continue;
    auto candidate=before;source_game_variant_set(candidate,int(parameter.parameter),choice.value);
    changes|=parameter.parameter==90 ? choice.value!=current : candidate.bytes!=before.bytes;
    result.choices.push_back(choice);
  }
  if(accepts(current) && std::ranges::none_of(result.choices,[&](const auto &choice){return choice.value==current;})) {
    auto label=std::to_string(current);
    if(parameter.parameter==10)label=current?seconds_label(current*30):"No Limit";
    else if(parameter.parameter==17 || parameter.parameter==18)label=seconds_label(current*30);
    else if(parameter.parameter==16 && !current)label="Unlimited";
    else if(parameter.parameter>=1 && parameter.parameter<=9 && !current)label="No Limit";
    result.choices.push_back({std::move(label),current});
  }
  if(!changes || result.choices.empty())return {};
  return result;
}
std::string frontend_custom_variant_description(const UiVariant &variant) {
  if(bomb_defusal_variant(variant))
    return "Two teams, one life per round and first to 13 wins. Battle rifle and magnum starts; map weapons and vehicles stay off. Sides switch after 12 rounds. Adjust the plant window and bomb timings.";
  switch(custom_slayer(variant)) {
  case CustomSlayer::fiesta:
    return "Free-for-all with a random primary weapon on every spawn and no secondary weapon. Map pickups keep their defaults. Adjust the score, time, respawns and player limits.";
  case CustomSlayer::infection:
    return "One initial zombie. Humans keep shotgun and magnum starts; zombies keep swords, no shields and no grenades. Human deaths convert them. Survive the time limit or infect every human.";
  default:return {};
  }
}
} // namespace halo2::detail
