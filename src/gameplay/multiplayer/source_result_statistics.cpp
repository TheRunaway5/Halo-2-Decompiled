#include "halo2/source_result_statistics.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
void validate(const SourceResultPublication& p,int stat,int player,int team) {
  if(stat<0 || stat>=45 || player < -1 || player>=16 || team < -1 || team>=16)
    throw std::invalid_argument("Source result statistic outside recovered store");
  if(p.descriptors[stat].minimum>p.descriptors[stat].maximum)
    throw std::invalid_argument("Source result statistic bounds inverted");
}
std::uint16_t write(std::uint16_t old,std::int32_t value,const SourceResultStatDescriptor& d) {
  const auto bounded=std::uint16_t(std::clamp(value,std::int32_t(d.minimum),std::int32_t(d.maximum)));
  return std::uint16_t((old&0x8000u)|(bounded&0x7fffu));
}
std::int32_t add(std::uint16_t old,std::int32_t delta) {
  return std::bit_cast<std::int32_t>(std::uint32_t(old&0x7fffu)+std::uint32_t(delta));
}
std::int16_t increment(std::int16_t& v,std::int16_t delta) {
  return v=std::int16_t(std::clamp(int(v)+int(delta),-30000,30000));
}
}
void source_result_stat_set(SourceResultPublication& p,int stat,int player,int team,std::int32_t value) {
  if(!p.recording.enabled || p.recording.frozen)return;
  validate(p,stat,player,team);
  if(player!=-1)p.statistics.players[player][stat]=write(p.statistics.players[player][stat],value,p.descriptors[stat]);
  if(team!=-1)p.statistics.teams[team][stat]=write(p.statistics.teams[team][stat],value,p.descriptors[stat]);
}
void source_result_stat_add(SourceResultPublication& p,int stat,int player,int team,std::int32_t delta) {
  if(!p.recording.enabled || p.recording.frozen)return;
  validate(p,stat,player,team);
  if(player!=-1) {
    auto& word=p.statistics.players[player][stat];word=write(word,add(word,delta),p.descriptors[stat]);
  }
  if(team!=-1) {
    auto& word=p.statistics.teams[team][stat];word=write(word,add(word,delta),p.descriptors[stat]);
  }
}
std::int16_t source_game_stat_team_add(SourceGameEndStats& bank,unsigned team,unsigned field,
    std::int16_t delta,int stat,SourceResultPublication* p) {
  if(team>=8 || field>=9)throw std::invalid_argument("Source team stat outside match bank");
  if(p && stat!=-1 && p->recording.enabled && !p->recording.frozen)validate(*p,stat,-1,int(team));
  const auto value=increment(bank.teams[team].fields[field],delta);
  if(p && stat!=-1)source_result_stat_set(*p,stat,-1,int(team),value);
  return value;
}
std::int16_t source_game_stat_player_add(SourceGameEndStats& bank,unsigned player,unsigned field,
    std::int16_t delta,int stat,SourceResultPublication* p,int team) {
  if(player>=16 || field>=12 || team < -1 || team>=8 || (team!=-1 && field>=9))
    throw std::invalid_argument("Source player stat outside match bank");
  if(p && stat!=-1 && p->recording.enabled && !p->recording.frozen)validate(*p,stat,int(player),team);
  const auto value=increment(bank.players[player].fields[field],delta);
  if(p && stat!=-1)source_result_stat_set(*p,stat,int(player),-1,value);
  if(team!=-1)source_game_stat_team_add(bank,unsigned(team),field,delta,stat,p);
  return value;
}
} // namespace halo2
