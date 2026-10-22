#include "halo2/source_hud_score.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr SourceHudScoreColor white{0x3f800000,0x3f800000,0x3f800000};
// Literal retail [468734]->440A74 RGB; TU color-provider output agrees.
constexpr SourceHudScoreColor team_fallback{0x3f800000,0x3ed1eb85,0x3f333333};
SourceHudScoreColor color(std::int8_t index, bool teams,
    const SourceHudScoreColors& colors) {
  const auto palette = teams ? colors.teams : colors.appearance;
  if ((teams || colors.scenario_present) && index >= 0 && std::size_t(index) < palette.size())
    return palette[std::size_t(index)];
  return teams ? team_fallback : white;
}
std::u16string text(std::string value) {
  value.resize(std::min<std::size_t>(value.size(),7));
  return {value.begin(),value.end()};
}
std::string time_text(int seconds) {
  const int minutes=seconds/60, remainder=seconds%60;
  return (minutes ? std::to_string(minutes) : " ") + ":" +
      (remainder < 10 ? "0" : "") + std::to_string(remainder);
}
std::u16string score_text(int score, int source_type) {
  return text((source_type>=3 && source_type<=4) || source_type==8
      ? time_text(score) : std::to_string(score));
}
} // namespace
SourceHudScoreDisplay source_hud_score_display(const SourceHudScoreInput& in,
    const SourceHudScoreColors& colors) {
  SourceHudScoreDisplay out;
  if (!in.engine_present || in.local_player==0xffffffffu) return out;
  const unsigned slot=in.local_player&65535u;
  if (slot>=in.players.size()) {out.status=SourceHudScoreStatus::InvalidInput;return out;}
  const auto& owner=in.players[slot];
  if (owner.membership==-1) return out;
  if (in.teams && (owner.membership<0 || owner.membership>=8)) {
    out.status=SourceHudScoreStatus::InvalidInput;return out;
  }
  const bool rounds=in.limit==1;
  if (rounds && (in.round_mode<0 || in.round_mode>=7)) {
    out.status=SourceHudScoreStatus::InvalidInput;return out;
  }
  const auto stat=[&](const SourceHudScorePlayer& p)->std::optional<std::int16_t> {
    return rounds ? p.wins : std::optional{p.score};
  };
  const auto team_stat=[&](unsigned team)->std::optional<std::int16_t> {
    return rounds ? in.team_wins[team] : std::optional{in.team_scores[team]};
  };
  const auto own_score=in.teams ? team_stat(unsigned(owner.membership)) : stat(owner);
  if (!own_score) {out.status=SourceHudScoreStatus::UnavailableRoundStatistics;return out;}
  out.status=SourceHudScoreStatus::Ready;
  out.type=in.source_type==1 ? 2 : in.source_type==9 ? 3 : 1;
  out.teams=in.teams;
  constexpr std::array limits{1,2,4,6,2,3,4};
  out.limit=rounds ? limits[unsigned(in.round_mode)] : in.limit;
  out.name=in.variant_name.substr(0,std::min<std::size_t>(in.variant_name.find(u'\0'),31));
  out.timer=in.time_limit!=0;
  if (out.timer) out.timer_text=text(time_text(std::max(int(in.clock_seconds),0)));
  out.score=*own_score;
  out.score_text=score_text(out.score,in.source_type);
  out.color=color(in.teams ? owner.membership : owner.primary_color,in.teams,colors);
  out.player=in.teams ? 0xffffffffu : in.local_player;
  if (in.teams) out.rival_player=0xffffffffu;
  int best=-32768;
  const auto accept=[&](int candidate, std::uint32_t datum, std::int8_t color_index) {
    if (candidate<=best) return;
    best=candidate;out.rival=true;out.rival_score=candidate;
    if (!in.teams) out.rival_player=datum;
    out.rival_color=color(color_index,in.teams,colors);
  };
  if (in.teams) {
    for (unsigned team=0;team<8;++team)
      if (int(team)!=owner.membership && (in.playing_teams&(1u<<team))) {
        const auto value=team_stat(team);
        if (!value) {out.status=SourceHudScoreStatus::UnavailableRoundStatistics;return out;}
        accept(*value,0xffffffffu,std::int8_t(team));
      }
  } else {
    for (unsigned i=0;i<in.players.size();++i) {
      const auto& p=in.players[i];
      if (!(in.connected_players&(1u<<i)) || (p.flags&2u) || p.datum==in.local_player) continue;
      const auto value=stat(p);
      if (!value) {out.status=SourceHudScoreStatus::UnavailableRoundStatistics;return out;}
      accept(*value,p.datum,p.primary_color);
    }
  }
  out.leading=!out.rival || out.score>best;
  if (out.rival) out.rival_text=score_text(best,in.source_type);
  if (!rounds && !out.limit) out.limit=std::max({out.score,out.rival_score,1});
  out.score_fraction=float(out.score)/float(out.limit);
  if (out.rival) out.rival_fraction=float(out.rival_score)/float(out.limit);
  return out;
}
std::uint16_t source_hud_score_mask(const SourceHudScoreDisplay& score) {
  const bool ready=score.status==SourceHudScoreStatus::Ready;
  return std::uint16_t((ready && score.teams ? 8 : 4) |
      (ready && score.leading ? 0x10 : 0x20) |
      (ready && score.timer ? 0x40 : 0x80) |
      (ready && score.rival ? 0x100 : 0x200));
}
} // namespace halo2
