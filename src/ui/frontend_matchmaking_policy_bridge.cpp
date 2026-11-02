#include "halo2/frontend_matchmaking_policy.hpp"
#include "halo2/original_matchmaking_widgets.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
namespace halo2 {
namespace bridge = online_protocol_bridge;
namespace {
std::optional<unsigned> optional(int n) { return n < 0 ? std::nullopt : std::optional<unsigned>(unsigned(n)); }
rust::Vec<bridge::ProgressMember> members(const std::vector<FrontendProgressMember> &input) {
  rust::Vec<bridge::ProgressMember> out;
  for(const auto &m:input) {
    bridge::ProgressMember p{};p.key={m.key.team,m.key.value};p.level_present=m.level.has_value();p.level=m.level.value_or(0);
    p.is_self=m.self;p.local_controller=m.local_controller;p.handle=m.handle;out.push_back(std::move(p));
  }
  return out;
}
bridge::ProgressFacts facts(const FrontendProgressFacts &f) {
  bridge::ProgressFacts out{};
  out.pane=f.pane;out.ui=f.ui;out.status=f.status;out.ranked=f.ranked;out.teams=f.teams;
  out.party_present=f.party_present;out.match_present=f.match_present;out.gathering_present=f.gathering_present;
  out.party=members(f.party);out.local=members(f.local);out.matched=members(f.matched);out.gathering=members(f.gathering);
  out.maximum=f.maximum;out.viewer_level_present=f.viewer_level.has_value();out.viewer_level=f.viewer_level.value_or(0);
  out.party_voice=f.party_voice;out.voice_joined=f.voice_joined;out.microphone_muted=f.microphone_muted;out.transmitting=f.transmitting;
  for(const auto &h:f.talking)out.talking.push_back(h);
  return out;
}
FrontendProgressRow row(const bridge::ProgressRow &r) {
  return {static_cast<MatchmakingProgressRow>(r.kind),static_cast<FrontendProgressSource>(r.source),r.index,optional(r.rank),optional(r.speaker)};
}
} // namespace
MatchmakingStartCountdown matchmaking_start_countdown(std::uint32_t elapsed) {
  auto c=bridge::progress_countdown(elapsed);return {c.seconds,c.ready};
}
MatchmakingStartCountdown matchmaking_published_start_countdown(std::optional<unsigned> remaining) {
  auto c=bridge::progress_published(remaining.has_value(),remaining.value_or(0));return {c.seconds,c.ready};
}
unsigned matchmaking_progress_pane(MatchmakingProgressUi ui,std::int32_t countdown) {return bridge::progress_pane(int(ui),countdown);}
unsigned matchmaking_progress_pc_pane(MatchmakingProgressUi ui,std::int32_t countdown) {return bridge::progress_pc_pane(int(ui),countdown);}
std::optional<unsigned> matchmaking_progress_pane_sound(unsigned previous,unsigned current) {return optional(bridge::progress_sound(previous,current));}
std::optional<MatchmakingProgressTitle> matchmaking_progress_title(MatchmakingProgressUi ui) {
  const auto title=bridge::progress_titles(int(ui));
  if(!title.xbox)return {};return MatchmakingProgressTitle{title.xbox,title.pc};
}
bool matchmaking_progress_cancel_allowed(unsigned pane,std::uint32_t elapsed) {return bridge::progress_cancel(pane,elapsed);}
bool matchmaking_progress_consumes_y(unsigned pane) {return bridge::progress_y(pane);}
unsigned matchmaking_progress_leave_dialog(std::size_t party,std::size_t local) {return bridge::progress_leave(party,local);}
std::vector<unsigned> matchmaking_progress_order(const std::vector<MatchmakingProgressPlayer> &players,bool ranked,bool teams) {
  std::vector<bridge::ProgressKey> input;for(const auto &p:players)input.push_back({p.team,p.value});
  auto result=bridge::progress_order({input.data(),input.size()},ranked,teams);return {result.begin(),result.end()};
}
bool matchmaking_progress_team_colors(std::int32_t countdown,bool teams) {return bridge::progress_team_colors(countdown,teams);}
std::vector<unsigned> matchmaking_progress_pane_order(const std::vector<MatchmakingProgressPlayer> &players,unsigned pane,bool ranked,bool teams) {
  std::vector<bridge::ProgressKey> input;for(const auto &p:players)input.push_back({p.team,p.value});
  auto result=bridge::progress_pane_order({input.data(),input.size()},pane,ranked,teams);return {result.begin(),result.end()};
}
bool matchmaking_progress_uses_party(unsigned pane,bool ranked) {return bridge::progress_uses_party(pane,ranked);}
std::vector<MatchmakingProgressRow> matchmaking_progress_rows(unsigned pane,bool ranked,unsigned party,unsigned game,std::optional<unsigned> maximum) {
  auto result=bridge::progress_rows(pane,ranked,party,game,maximum.has_value(),maximum.value_or(0));
  std::vector<MatchmakingProgressRow> out;for(const auto r:result)out.push_back(static_cast<MatchmakingProgressRow>(r));return out;
}
std::int32_t matchmaking_lobby_countdown_press(std::int32_t current) {return bridge::progress_lobby_press(current);}
bool matchmaking_lobby_countdown_due(std::int32_t current,std::uint32_t elapsed) {return bridge::progress_lobby_due(current,elapsed);}
bool matchmaking_lobby_countdown_beeps(std::int32_t previous,std::int32_t current) {return bridge::progress_lobby_beeps(previous,current);}
std::uint32_t original_matchmaking_title(int ui) {return bridge::progress_title(ui);}
std::optional<unsigned> original_matchmaking_option_category(int kind) {return optional(bridge::progress_category(kind));}
unsigned original_matchmaking_game_icon(int kind) {return bridge::progress_icon(kind);}
OriginalMatchmakingNeeded original_matchmaking_needed(bool available,int count,int required) {
  return static_cast<OriginalMatchmakingNeeded>(bridge::progress_needed(available,count,required));
}
struct OriginalMatchmakingTipClock::Impl {
  rust::Box<bridge::ProgressTipClock> state;
  Impl(std::uint32_t last,bool initialized):state(bridge::new_progress_tip_clock(last,initialized)){}
  explicit Impl(const bridge::ProgressTipClock &other):state(bridge::clone_progress_tip_clock(other)){}
};
OriginalMatchmakingTipClock::OriginalMatchmakingTipClock(std::uint32_t last,bool initialized):impl_(std::make_unique<Impl>(last,initialized)){}
OriginalMatchmakingTipClock::~OriginalMatchmakingTipClock()=default;
OriginalMatchmakingTipClock::OriginalMatchmakingTipClock(const OriginalMatchmakingTipClock &o):impl_(std::make_unique<Impl>(*o.impl_->state)){}
OriginalMatchmakingTipClock &OriginalMatchmakingTipClock::operator=(const OriginalMatchmakingTipClock &o) {
  if(this!=&o){auto p=std::make_unique<Impl>(*o.impl_->state);impl_.swap(p);}return *this;
}
bool OriginalMatchmakingTipClock::update(std::uint32_t now) {return bridge::progress_tip_update(*impl_->state,now);}
std::optional<unsigned> original_matchmaking_tip_index(std::uint32_t &seed,unsigned count) {
  auto result=bridge::progress_tip_index(seed,count);seed=result.seed;return result.present?std::optional<unsigned>(result.index):std::nullopt;
}
std::vector<FrontendProgressRow> frontend_matchmaking_project(const FrontendProgressFacts &f) {
  auto result=bridge::progress_project(facts(f));std::vector<FrontendProgressRow> out;
  out.reserve(result.size());for(const auto &r:result)out.push_back(row(r));return out;
}
std::optional<FrontendProgressRow> frontend_matchmaking_query(const FrontendProgressFacts &f,int screen,const std::vector<int> &entries,unsigned index) {
  auto result=bridge::progress_query(facts(f),screen,{entries.data(),entries.size()},index);
  return result.present?std::optional<FrontendProgressRow>(row(result)):std::nullopt;
}
} // namespace halo2
