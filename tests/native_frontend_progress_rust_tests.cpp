#include "halo2/frontend_matchmaking_policy.hpp"
#include "halo2/original_matchmaking_widgets.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#define halo2 before
#include "reference/frontend_progress_before_rust_20261102/frontend_matchmaking_progress.hpp.reference"
#include "reference/frontend_progress_before_rust_20261102/original_matchmaking_widgets.hpp.reference"
#undef halo2

// Observation-only adapter for the unedited captured MenuState methods.
// Appearances encode the observation source/index; match speaker queries
// return 100+slot so a wrong routing decision is observable.
namespace before {
using FrontendAppearance=unsigned;
enum class OnlineMatchmakingStatus {Unconfigured,Idle,Connecting,Ready,Joining,Searching,CheckingConnections,AwaitingTransport,Loading,Failed};
struct Member {std::string name,handle;unsigned team{};bool self{};int local_controller{-1};std::optional<unsigned> level,appearance;};
struct OnlinePlaylist {std::string id;bool ranked{};std::optional<unsigned> level;};
struct Match {std::vector<Member> players;};
struct Gathering {std::vector<Member> players;unsigned maximum_players{};};
struct OnlineMatchmakingSnapshot {OnlineMatchmakingStatus status{};std::string playlist_id;std::vector<OnlinePlaylist> playlists;std::optional<Match> match;std::optional<Gathering> gathering;};
struct MenuEntry {std::string name;int destination{},value{};};
struct Frame {unsigned screen{};std::vector<MenuEntry> entries;};
struct Party {};
struct Account {std::optional<Party> party;};
struct Voice {bool joined{},microphone_muted{},transmitting{};std::vector<std::string> talking;};
struct Queue {std::optional<unsigned> player_speaker(unsigned i) const{return 100+i;}};
namespace detail {struct Rules{bool team_mode;};Rules frontend_multiplayer_rules(bool teams){return {teams};}}
const OnlinePlaylist *selected(const OnlineMatchmakingSnapshot &s,const std::string &id) {
  const auto i=std::find_if(s.playlists.begin(),s.playlists.end(),[&](const auto &p){return p.id==id;});return i==s.playlists.end()?nullptr:&*i;
}
std::optional<bool> matched_variant(bool teams,const Match &){return teams;}
class MenuState {
public:
  OnlineMatchmakingSnapshot matchmaking_;Account account_;Voice voice_;bool voice_party_{},content_{};
  std::optional<Queue> online_matchmaking_{Queue{}};Frame frame_;unsigned pane_{};int ui_{};
  std::vector<Member> party_,local_;
  unsigned online_matchmaking_progress_pane() const{return pane_;}
  MatchmakingProgressUi online_matchmaking_ui() const{return static_cast<MatchmakingProgressUi>(ui_);}
  const Frame &frame() const{return frame_;}
  std::vector<Member> online_party_members() const{return party_;}
  std::vector<Member> local_players() const{return local_;}
  std::vector<unsigned> online_matchmaking_roster_order() const;
  std::optional<unsigned> online_matchmaking_rank(unsigned) const;
  std::optional<FrontendAppearance> online_matchmaking_player_appearance(unsigned) const;
  std::optional<unsigned> online_matchmaking_speaker(unsigned) const;
  std::pair<std::vector<MenuEntry>,std::vector<MatchmakingProgressRow>> populate_rows() const;
};
#include "frontend_progress_reference.inc"
}

using Json=nlohmann::json;
using namespace halo2;
Json optional(std::optional<unsigned> v){return v?Json(*v):Json(nullptr);}
FrontendProgressMember member(const Json &j) {
  FrontendProgressMember m;m.key={j.value("team",-1),j.value("value",-1)};
  if(j.contains("level")&&!j["level"].is_null())m.level=j["level"].get<unsigned>();
  m.self=j.value("self",false);m.local_controller=j.value("controller",-1);m.handle=j.value("handle",std::string{});return m;
}
FrontendProgressFacts facts(const Json &j) {
  FrontendProgressFacts f;f.pane=j.value("pane",0u);f.ui=j.value("ui",0);f.status=j.value("status",5u);
  f.ranked=j.value("ranked",false);f.teams=j.value("teams",false);f.party_present=j.value("party_present",false);
  f.match_present=j.value("match_present",false);f.gathering_present=j.value("gathering_present",false);f.maximum=j.value("maximum",0u);
  if(j.contains("viewer_level")&&!j["viewer_level"].is_null())f.viewer_level=j["viewer_level"].get<unsigned>();
  f.party_voice=j.value("party_voice",false);f.voice_joined=j.value("voice_joined",false);
  f.microphone_muted=j.value("muted",false);f.transmitting=j.value("transmitting",false);f.talking=j.value("talking",std::vector<std::string>{});
  for(auto [name,out]:{std::pair{"party",&f.party},{"local",&f.local},{"matched",&f.matched},{"gathering",&f.gathering}})
    if(j.contains(name))for(const auto &p:j[name])out->push_back(member(p));
  return f;
}
std::vector<before::Member> before_members(const std::vector<FrontendProgressMember> &input,unsigned source) {
  std::vector<before::Member> out;unsigned i=0;
  for(const auto &m:input){before::Member p;p.name=std::to_string(source)+":"+std::to_string(i);p.handle=m.handle;p.team=unsigned(m.key.team);
    p.self=m.self;p.local_controller=m.local_controller;p.level=m.level;p.appearance=source*1000+i++;out.push_back(std::move(p));}
  return out;
}
before::MenuState before_state(const FrontendProgressFacts &f,int screen,const std::vector<int> &entries) {
  before::MenuState s;s.pane_=f.pane;s.ui_=f.ui;s.content_=f.teams;s.matchmaking_.status=static_cast<before::OnlineMatchmakingStatus>(f.status);
  s.matchmaking_.playlist_id="selected";s.matchmaking_.playlists.push_back({"selected",f.ranked,f.viewer_level});
  if(f.party_present)s.account_.party.emplace();
  s.party_=before_members(f.party,1);s.local_=before_members(f.local,2);
  if(f.match_present)s.matchmaking_.match=before::Match{before_members(f.matched,3)};
  if(f.gathering_present)s.matchmaking_.gathering=before::Gathering{before_members(f.gathering,4),f.maximum};
  s.voice_party_=f.party_voice;s.voice_={f.voice_joined,f.microphone_muted,f.transmitting,f.talking};s.frame_.screen=unsigned(screen);
  for(const auto v:entries)s.frame_.entries.push_back({{},0,v});return s;
}
Json project(const Json &j) {
  const auto f=facts(j);const auto screen=j.value("screen",210);const auto entries=j.value("entries",std::vector<int>{});
  const auto old=before_state(f,screen,entries);const auto [old_players,old_kinds]=old.populate_rows();
  Json old_rows=Json::array(),new_rows=Json::array(),old_queries=Json::array(),new_queries=Json::array(),reordered_queries=Json::array();
  for(unsigned i=0;i<old_kinds.size();++i)old_rows.push_back({{"kind",int(old_kinds[i])},{"name",old_kinds[i]==before::MatchmakingProgressRow::player?old_players.at(i).name:""}});
  const auto rows=frontend_matchmaking_project(f);
  const auto old_order=old.online_matchmaking_roster_order();
  for(const auto &r:rows)new_rows.push_back({{"kind",int(r.kind)},{"name",r.kind==MatchmakingProgressRow::player?std::to_string(int(r.source))+":"+std::to_string(r.index):""}});
  for(unsigned i=0;i<entries.size()+2;++i) {
    old_queries.push_back({{"rank",optional(old.online_matchmaking_rank(i))},{"appearance",optional(old.online_matchmaking_player_appearance(i))},{"speaker",optional(old.online_matchmaking_speaker(i))}});
    auto adjusted=old;unsigned query_index=i;
    if(f.pane==3&&i<rows.size()&&i<entries.size()&&rows[i].source==FrontendProgressSource::match) {
      query_index=unsigned(std::find(old_order.begin(),old_order.end(),rows[i].index)-old_order.begin());
      adjusted.frame_.entries.resize(std::max<std::size_t>(entries.size(),query_index+1));
      adjusted.frame_.entries[query_index].value=entries[i];
    }
    reordered_queries.push_back({{"rank",optional(adjusted.online_matchmaking_rank(query_index))},{"appearance",optional(adjusted.online_matchmaking_player_appearance(query_index))},{"speaker",optional(adjusted.online_matchmaking_speaker(query_index))}});
    const auto r=frontend_matchmaking_query(f,screen,entries,i);
    std::optional<unsigned> appearance,speaker,rank;
    if(r){rank=r->rank;speaker=r->speaker;if(r->source!=FrontendProgressSource::none)appearance=unsigned(r->source)*1000+r->index;
      if(r->source==FrontendProgressSource::match)speaker=100+r->index;}
    new_queries.push_back({{"rank",optional(rank)},{"appearance",optional(appearance)},{"speaker",optional(speaker)}});
  }
  return {{"before_rows",old_rows},{"rust_rows",new_rows},{"before_queries",old_queries},{"rust_queries",new_queries},{"before_reordered_queries",reordered_queries},
    {"before_order",old.online_matchmaking_roster_order()},
    {"rust_order",matchmaking_progress_pane_order([&]{std::vector<MatchmakingProgressPlayer> p;for(const auto &m:f.matched)p.push_back(m.key);return p;}(),f.pane,f.ranked,f.teams)}};
}
Json primitives(const Json &j) {
  const int ui=j.at("ui"),seconds=j.at("seconds"),kind=j.at("variant_type");
  const unsigned pane=j.at("pane"),elapsed=j.at("elapsed");const bool ranked=j.at("ranked"),teams=j.at("teams");
  const auto a=matchmaking_start_countdown(elapsed);const auto b=before::matchmaking_start_countdown(elapsed);
  const auto title=matchmaking_progress_title(static_cast<MatchmakingProgressUi>(ui));const auto old_title=before::matchmaking_progress_title(static_cast<before::MatchmakingProgressUi>(ui));
  std::uint32_t seed=j.at("seed"),old_seed=seed;unsigned count=j.at("count");
  const auto tip=original_matchmaking_tip_index(seed,count),old_tip=before::original_matchmaking_tip_index(old_seed,count);
  auto bundle=[&](bool old){return Json::array({old?before::matchmaking_progress_pane(static_cast<before::MatchmakingProgressUi>(ui),seconds):matchmaking_progress_pane(static_cast<MatchmakingProgressUi>(ui),seconds),
    old?before::matchmaking_progress_pc_pane(static_cast<before::MatchmakingProgressUi>(ui),seconds):matchmaking_progress_pc_pane(static_cast<MatchmakingProgressUi>(ui),seconds),
    old?before::matchmaking_progress_cancel_allowed(pane,elapsed):matchmaking_progress_cancel_allowed(pane,elapsed),
    old?before::matchmaking_progress_consumes_y(pane):matchmaking_progress_consumes_y(pane),
    old?before::matchmaking_progress_uses_party(pane,ranked):matchmaking_progress_uses_party(pane,ranked),
    old?before::matchmaking_progress_team_colors(seconds,teams):matchmaking_progress_team_colors(seconds,teams),
    old?before::original_matchmaking_game_icon(kind):original_matchmaking_game_icon(kind),
    optional(old?before::original_matchmaking_option_category(kind):original_matchmaking_option_category(kind)),
    old?before::original_matchmaking_title(ui):original_matchmaking_title(ui),
    old?b.seconds:a.seconds,old?b.ready:a.ready,old?(old_title?old_title->pc:0):(title?title->pc:0),old?old_seed:seed,
    optional(old?old_tip:tip),old?before::matchmaking_lobby_countdown_due(seconds,elapsed):matchmaking_lobby_countdown_due(seconds,elapsed),
    old?before::matchmaking_lobby_countdown_beeps(ui,seconds):matchmaking_lobby_countdown_beeps(ui,seconds)});};
  return {{"before",bundle(true)},{"rust",bundle(false)}};
}
void clock_controls() {
  OriginalMatchmakingTipClock clock{100,true};auto clone=clock;
  if(clock.update(15100)||!clock.update(15101)||clone.update(100))throw std::runtime_error("tip clock clone shared state");
  clone=clock;if(clone.update(15101)||!clone.update(30102)||clock.update(15101))throw std::runtime_error("tip clock assignment shared state");
}
int main(int argc,char **argv) try {
  clock_controls();
  if(argc==2&&std::string_view(argv[1])=="--widgets") {
    int value;unsigned seed,count;while(std::cin>>value>>seed>>count){auto tip=original_matchmaking_tip_index(seed,count);OriginalMatchmakingTipClock c{0,true};
      std::cout<<original_matchmaking_title(value)<<" "<<original_matchmaking_option_category(value).value_or(0xffffffffu)<<" "<<original_matchmaking_game_icon(value)<<" "<<seed<<" "<<tip.value()<<" "<<c.update(std::bit_cast<unsigned>(value))<<"\n";}return 0;
  }
  if(argc==2&&std::string_view(argv[1])=="--sort") {
    unsigned n,ranked,teams;while(std::cin>>n>>ranked>>teams){std::vector<MatchmakingProgressPlayer> p(n);for(auto &m:p)std::cin>>m.team>>m.value;
      for(const auto i:matchmaking_progress_order(p,ranked,teams))std::cout<<i<<" ";std::cout<<"\n";}return 0;
  }
  if(argc==2&&std::string_view(argv[1])=="--rows") {
    unsigned pane,ranked,party,game;int maximum;while(std::cin>>pane>>ranked>>party>>game>>maximum) {
      const auto r=matchmaking_progress_rows(pane,ranked,party,game,maximum<0?std::nullopt:std::optional<unsigned>(unsigned(maximum)));
      std::cout<<matchmaking_progress_uses_party(pane,ranked);for(const auto row:r)std::cout<<" "<<int(row);std::cout<<"\n";
    }return 0;
  }
  if(argc==2&&std::string_view(argv[1])=="--input") {
    char kind;unsigned a,b;while(std::cin>>kind>>a>>b)
      std::cout<<(kind=='C'?unsigned(matchmaking_progress_cancel_allowed(a,b)):matchmaking_progress_leave_dialog(a,b))<<"\n";
    return 0;
  }
  std::string line;while(std::getline(std::cin,line)){const auto j=Json::parse(line);std::cout<<(j.value("op",std::string{})=="primitives"?primitives(j):project(j)).dump()<<"\n";}
} catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}
