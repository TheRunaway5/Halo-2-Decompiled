#include "halo2/source_player_status.hpp"
#include "halo2/vista_lobby_metadata.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
std::int32_t source_player_status(const SourcePlayerStatusInput& in) {
  auto status=in.engine_status;
  if(!in.engine_override && status!=-1)return status;
  if(in.special_mode)return in.leading?8+int(in.tied):10;
  if(in.current_unit==0xffffffffu) {
    if(in.membership==-1)return 1;
    if(in.exhausted)return 4;
    if(in.player_flags&0x4000u)return 3;
    if(in.respawn_ticks>=0)return 2;
    return status;
  }
  const auto three=std::int32_t(in.rate)*3;
  if(in.alive_ticks<three && in.life_limit!=0)
    return in.lives_remaining==1?24:22+int(in.lives_remaining==2);
  const auto elapsed=std::bit_cast<std::int32_t>(std::uint32_t(in.tick)-std::uint32_t(in.round_start_tick));
  if(std::max(elapsed,0)<three)return 18;
  if(in.round_countdown!=-1 && in.round_countdown!=0 && in.round_countdown<5)return 17;
  if(status==-1)return (in.score_limit==0?20:0)+(in.leading?5+int(in.tied):7);
  return status;
}
std::optional<std::u16string> source_player_status_text(const FullScoreboardAssets& assets,
    const SourcePlayerStatusTextInput& in,std::size_t capacity) {
  if(!in.engine_present || !capacity)return {};
  const std::string* text{};
  if(in.lifecycle==1) {
    if(in.status==-1 || (in.suppress_playing &&
        ((in.status>=5 && in.status<=7)||(in.status>=25 && in.status<=27))))return {};
    const auto row=std::find_if(assets.player_status.begin(),assets.player_status.end(),
        [&](const auto& r){return r.status==in.status;});
    if(row==assets.player_status.end())return {};
    text=in.team_mode?&row->team_text:&row->text;
  } else if(in.special_mode)text=&assets.game_over_label;
  else if(in.lifecycle==2)text=&assets.round_over_label;
  else if(in.lifecycle==3)text=&assets.waiting_for_next_round_label;
  else return {};
  if(text->empty())return {};
  const auto source=vista_lobby_metadata_utf16(*text);
  std::u16string out;
  for(std::size_t at=0;at<source.size();) {
    if(source[at]!=u'#') {out+=source[at++];continue;}
    auto end=at+1;
    while(end<source.size() && ((source[end]>=u'a' && source[end]<=u'z')||source[end]==u'_'))++end;
    const auto token=std::u16string_view(source).substr(at,end-at);
    const auto number=[](std::int32_t value){const auto s=std::to_string(value);return std::u16string(s.begin(),s.end());};
    if(token==u"#local_spawn_time")out+=number(in.respawn_seconds);
    else if(token==u"#local_lives_remaining")out+=number(in.lives_remaining);
    else if(token==u"#local_player_score")out+=in.player_score;
    else if(token==u"#local_team_score")out+=in.team_score;
    else if(token==u"#score_to_win")out+=in.score_to_win;
    else return {};
    at=end;
  }
  out.resize(std::min(out.size(),capacity-1));
  return out;
}
}
