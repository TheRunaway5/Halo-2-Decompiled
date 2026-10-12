#include "halo2/source_result_capture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
std::int16_t wrap_increment(std::int16_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(value)+1u));
}
void validate(unsigned player,int team) {
  if(player>=16 || team < -1 || team>=8)throw std::invalid_argument("Result capture outside native stat bank");
}
void weapon(SourceResultCapture* capture,unsigned player,unsigned kind,std::optional<std::uint8_t> cause) {
  if(capture && cause)source_result_weapon_add(*capture,player,kind,capture->constants.cause_weapons[*cause&0x3fu],1);
}
}
void source_result_weapon_add(SourceResultCapture& c,unsigned player,unsigned kind,unsigned weapon,std::int32_t delta) {
  const auto& recording=c.publication.recording;
  if(!recording.enabled || recording.frozen)return;
  if(player>=16 || kind>=7 || weapon>40)throw std::invalid_argument("Source weapon stat outside recovered store");
  const auto& d=c.constants.weapon_descriptors[kind];
  if(d.minimum>d.maximum)throw std::invalid_argument("Source weapon statistic bounds inverted");
  auto& old=c.publication.statistics.players[player][78+weapon*8+kind];
  const auto value=std::bit_cast<std::int32_t>(std::uint32_t(old&0x7fffu)+std::uint32_t(delta));
  const auto bounded=std::uint16_t(std::clamp(value,std::int32_t(d.minimum),std::int32_t(d.maximum)));
  old=std::uint16_t((old&0x8000u)|(bounded&0x7fffu));
}
std::int32_t source_result_multikill_cutoff(std::uint32_t tick,std::int16_t rate,const SourceResultCaptureConstants& c) {
  const auto span=std::nearbyint(float(rate)*c.multikill_seconds);
  if(!std::isfinite(span) || double(span)<-2147483648. || double(span)>2147483647.)
    throw std::invalid_argument("Source multikill span outside recovered integer range");
  return std::bit_cast<std::int32_t>(tick-std::uint32_t(std::int32_t(span)));
}
std::int16_t source_result_credited_kill(SourceGameEndStats& bank,unsigned player,
    std::uint32_t tick,std::int32_t cutoff,std::optional<std::uint8_t> cause,
    SourceResultCapture* capture,int team) {
  validate(player,team);
  auto* p=capture ? &capture->publication : nullptr;
  const auto kills=source_game_stat_player_add(bank,player,2,1,7,p,team);
  weapon(capture,player,0,cause);
  if(cause && (*cause&0xc0u)==0x40u)weapon(capture,player,6,cause);
  auto& row=bank.players[player];row.fields[9]=wrap_increment(row.fields[9]);
  const auto enabled=p && p->recording.enabled;
  const auto best=enabled ? int(p->statistics.players[player][12]&0x7fffu) : -1;
  if(best<row.fields[9] && p)source_result_stat_set(*p,12,int(player),-1,row.fields[9]);
  if(team!=-1 && p) {
    const auto team_best=enabled ? int(p->statistics.teams[team][12]&0x7fffu) : -1;
    if(team_best<row.fields[9])source_result_stat_set(*p,12,-1,team,row.fields[9]);
  }
  row.fields[10]=std::bit_cast<std::int32_t>(row.last_kill_tick)<cutoff ? 1 : wrap_increment(row.fields[10]);
  row.last_kill_tick=tick;return kills;
}
void source_result_victim_death(SourceGameEndStats& bank,unsigned victim,int credited,
    bool incoming_self,bool betrayal,std::optional<std::uint8_t> cause,
    SourceResultCapture* capture,int victim_team,int credited_team) {
  validate(victim,victim_team);
  if(credited < -1 || credited>=16)throw std::invalid_argument("Source death credit outside stat bank");
  if(credited!=-1)validate(unsigned(credited),credited_team);
  auto* p=capture ? &capture->publication : nullptr;
  source_game_stat_player_add(bank,victim,3,1,9,p,victim_team);weapon(capture,victim,1,cause);
  if(incoming_self) {
    source_game_stat_player_add(bank,victim,4,1,11,p,victim_team);weapon(capture,victim,3,cause);
  } else if(betrayal && credited!=-1) {
    source_game_stat_player_add(bank,unsigned(credited),5,1,10,p,credited_team);weapon(capture,unsigned(credited),2,cause);
  }
  auto& row=bank.players[victim];row.fields[9]=row.fields[10]=0;row.last_kill_tick=0xffffffffu;
}
bool source_result_lifetime_tick(SourceResultPublication& p,unsigned player,const SourceResultLifetimeInput& in) {
  if(in.raw_mode==4 || !in.current_unit || !in.engine_present || !in.lifecycle_one || !in.status_one)return false;
  const auto tick=std::bit_cast<std::int32_t>(in.tick);
  if(!in.clock_rate || (tick==(-2147483647-1) && in.clock_rate==-1))
    throw std::invalid_argument("Source lifetime clock would fault its integer divide");
  if(tick%in.clock_rate)return false;
  source_result_stat_add(p,13,int(player),-1,1);return true;
}
} // namespace halo2
