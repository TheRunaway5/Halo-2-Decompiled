#include "halo2/frontend.hpp"
#include "ui/frontend_multiplayer_rules.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_player_profile.hpp"
#include <algorithm>

namespace halo2 {
namespace {
bool matchmaking_screen(const MenuFrame &f) {
  return f.screen == 210 || ((f.screen == 219 || f.screen == 237) && f.context == 0);
}
const OnlinePlaylist *selected(const OnlineMatchmakingSnapshot &s, const std::string &id) {
  const auto it = std::find_if(s.playlists.begin(), s.playlists.end(), [&](const auto &p) { return p.id == id; });
  return it == s.playlists.end() ? nullptr : &*it;
}
// mainmenu.map's goof names append the authored U+E008 display glyph.
// Match the catalog name to that row while retaining every source rule.
std::optional<UiVariant> matched_variant(const FrontendContent &content, const std::string &name) {
  const auto variant=std::find_if(content.variants.begin(),content.variants.end(),[&](const auto &v){
    std::string_view value=v.name;
    if(value.ends_with(" \xee\x80\x88"))value.remove_suffix(4);
    return value==name;
  });
  if(variant!=content.variants.end())return *variant;
  const auto extra=std::find_if(content.bundled_variants.begin(),content.bundled_variants.end(),[&](const auto &v){return v.name==name;});
  if(extra==content.bundled_variants.end())return std::nullopt;
  return *extra;
}
std::string matched_map_name(const FrontendContent &content, const std::string &map) {
  for(const auto &m:content.multiplayer_maps) {
    std::string_view path=m.scenario_path;
    const auto slash=path.find_last_of("\\/");
    if(slash!=std::string_view::npos)path.remove_prefix(slash+1);
    if(path==map && !m.name.empty())return m.name;
  }
  return map;
}
std::string replace_token(std::string text, std::string_view token, const std::string &value) {
  if(const auto at=text.find(token);at!=std::string::npos)text.replace(at,token.size(),value);
  return text;
}
} // namespace
void MenuState::set_online_matchmaking(std::shared_ptr<OnlineMatchmaking> client) {
  if (online_matchmaking_) online_matchmaking_->sign_out();
  online_matchmaking_ = std::move(client);
  matchmaking_active_ = quickmatch_pending_ = false;
  matchmaking_ = {}; matchmaking_name_.clear();
  reset_online_matchmaking_progress();
}
void MenuState::reset_online_matchmaking_progress() {
  matchmaking_start_clock_.reset();
  matchmaking_countdown_ = matchmaking_countdown_none;
  matchmaking_have_fun_ = false;
  matchmaking_progress_pane_ = 0;
  matchmaking_pane_clock_ = ui_clock_;
}
void MenuState::refresh_online_matchmaking() {
  if (!online_identity()) return;
  reset_online_matchmaking_progress();
  matchmaking_active_ = true; matchmaking_name_ = profile_name();
  matchmaking_couch_revision_=couch_revision_;
  if (online_matchmaking_) online_matchmaking_->refresh(matchmaking_name_,online_local_players());
  update_online_matchmaking();
}
void MenuState::update_online_matchmaking() {
  const auto previous_pane = online_matchmaking_progress_pane();
  const bool browsing = online_identity() && std::any_of(stack_.begin(), stack_.end(),
      [&](const auto &f){return matchmaking_screen(f) || online_party_screen(f);});
  if (!browsing) {
    if (matchmaking_active_ && online_matchmaking_) online_matchmaking_->sign_out();
    matchmaking_active_ = quickmatch_pending_ = false;
    matchmaking_ = {}; matchmaking_name_.clear();
    reset_online_matchmaking_progress();
    return;
  }
  if (matchmaking_active_ && (matchmaking_name_ != profile_name() || matchmaking_couch_revision_!=couch_revision_)) {
    quickmatch_pending_ = frame().screen == 210;
    refresh_online_matchmaking(); return;
  }
  bool changed = false;
  if (online_matchmaking_) {
    if (auto snapshot = online_matchmaking_->snapshot_after(matchmaking_.revision)) { matchmaking_ = std::move(*snapshot); changed = true; }
  } else if (matchmaking_.message.empty()) {
    matchmaking_.message = "No matchmaking service configured."; changed = true;
  }
  if (quickmatch_pending_ && matchmaking_.status == OnlineMatchmakingStatus::Ready) {
    const auto *playlist = selected(matchmaking_, selected_playlist_);
    if (!playlist || !playlist->eligible) {
      auto it = std::find_if(matchmaking_.playlists.begin(), matchmaking_.playlists.end(), [](const auto &p) { return p.quickmatch && p.eligible; });
      if(it==matchmaking_.playlists.end() && (account_.party || local_players().size()>1))
        it=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),[](const auto &p){return p.eligible && !p.clan_match;});
      if (it != matchmaking_.playlists.end()) playlist = &*it;
    }
    quickmatch_pending_ = false;
    if (playlist && playlist->eligible && online_matchmaking_) {
      selected_playlist_ = playlist->id;
      if(account_.party) {
        if(!online_account_ || !online_account_->party_command({"ready",{},playlist->id,playlist->revision})) {
          matchmaking_.status=OnlineMatchmakingStatus::Failed;
          matchmaking_.message="Wait for the current party operation to finish.";
          for(auto &f:stack_)if(matchmaking_screen(f)){f.entries.clear();populate_online_matchmaking(f);}
          ++revision;return;
        }
        if(account_.party->leader)online_matchmaking_->join(playlist->id);
      } else online_matchmaking_->join(playlist->id);
      matchmaking_ = online_matchmaking_->snapshot();
    } else {
      matchmaking_.status = OnlineMatchmakingStatus::Failed;
      matchmaking_.message = playlist ? "This playlist is unavailable for this party or installed maps." : "Choose an available playlist in Optimatch first.";
    }
    changed = true;
  }
  // c_session_state_start_match::update (0x72700). The relay's loading
  // phase is the replacement service's "every game-session peer is ready"
  // gate (0x6E720/0x6E6B0). B then counts 10000 ms before mode4 starts the
  // game; losing readiness clears the start clock.
  const bool start_ready=matchmaking_.status==OnlineMatchmakingStatus::Loading &&
      matchmaking_.match && online_matchmaking_;
  if(start_ready && !matchmaking_have_fun_) {
    if(!matchmaking_start_clock_)matchmaking_start_clock_=ui_clock_;
    const auto countdown=matchmaking_start_countdown(ui_clock_-*matchmaking_start_clock_);
    if(countdown.seconds!=matchmaking_countdown_){matchmaking_countdown_=countdown.seconds;changed=true;}
    if(countdown.ready) {
      // network_session_set_mode(b,4): match mode2, "Have fun!". A session
      // not yet handed over by the relay is retried on the next update.
      if(!launch_online_match())return;
      if(launch){matchmaking_have_fun_=true;changed=true;}
    }
  } else if(!start_ready && (matchmaking_start_clock_ || matchmaking_have_fun_)) {
    matchmaking_start_clock_.reset();matchmaking_countdown_=matchmaking_countdown_none;
    matchmaking_have_fun_=false;changed=true;
  }
  // c_matchmaking_screen update (0x251B39): a pane change records the pane
  // clock; entering pane3 requests UI sound12.
  if(const auto pane=online_matchmaking_progress_pane();pane!=matchmaking_progress_pane_) {
    matchmaking_progress_pane_=pane;matchmaking_pane_clock_=ui_clock_;changed=true;
    if(pane==3 && frame().screen==210 && pending_ui_sounds_.size()<16)
      pending_ui_sounds_.push_back(FrontendUiSound::MatchmakingAdvance);
  }
  if (!changed) return;
  for (auto &f : stack_) {
    if (!matchmaking_screen(f)) continue;
    if (f.screen == 210 && previous_pane != online_matchmaking_progress_pane()) {
      ++f.pane_generation;
      f.pane_created_clock = ui_clock_;
    }
    std::string focus;
    if (f.screen == 219 && f.selected < f.entries.size()) focus = f.entries[f.selected].label;
    f.entries.clear(); populate_online_matchmaking(f);
    if (!focus.empty()) {
      const auto it = std::find_if(f.entries.begin(), f.entries.end(), [&](const auto &e) { return e.label == focus; });
      if (it != f.entries.end()) f.selected = unsigned(it - f.entries.begin());
    }
    f.selected = std::min(f.selected, unsigned(f.entries.size() - 1));
  }
  ++revision;
}
bool MenuState::launch_online_match() {
  try {
    FrontendLaunch result;
    const auto &matched=*matchmaking_.match;
    const auto variant=matched_variant(content_,matched.variant);
    if(!variant)throw FormatError("Playlist selected an unavailable game variant.");
    result.variant=*variant;
    result.variant.name=matched.variant;
    const auto rules=detail::frontend_multiplayer_rules(result.variant);
    if(rules.custom==CustomSlayer::none && !rules.defusal && (matched.map=="midship" || matched.map=="waterworks"))throw FormatError("Playlist selected a map whose starting weapons are unfinished.");
    if(matched.map.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")!=std::string::npos)throw FormatError("Invalid matchmaking map.");
    result.map=matched.map+".map"; result.multiplayer=true;
    result.match_voice=match_voice_ || (party_voice_wanted_ && voice_party_ && voice_.joined);
    const auto self=std::find_if(matched.players.begin(),matched.players.end(),[](const auto &p){return p.self;});
    if(self==matched.players.end())throw FormatError("Matchmaking omitted the local player.");
    result.online_match=online_matchmaking_->take_launch();
    if(result.online_match) {
      if(const auto teams=result.online_match->team_mode();teams && *teams!=rules.team_mode)throw FormatError("Matchmaking team mode differs from the selected variant");
      const auto players=local_players();const auto slots=result.online_match->local_slots();
      if(players.size()!=slots.size())throw FormatError("Couch profiles changed before match launch.");
      result.local_players=unsigned(players.size());
      for(unsigned i=0;i<players.size();++i) {
        const auto &p=players[i];const auto &admitted=result.online_match->roster().at(slots[i]);
        if(p.name!=admitted.name)throw FormatError("Couch profile differs from admitted player.");
        result.selected_profiles.push_back(native_selected_player_configuration(p.name,
            std::span<const std::uint8_t,9>(reinterpret_cast<const std::uint8_t*>(&p.appearance),9),
            rules.team_mode?admitted.team:p.team));
        result.input_instances.push_back(p.input_instance);
        if(rules.team_mode)result.selected_teams.push_back(std::int8_t(admitted.team));
      }
      if(multiplayer_map_validator_)multiplayer_map_validator_(result);
      launch=std::move(result);
    }
  }catch(const FormatError &e) {
    online_matchmaking_->cancel_queue();reset_online_matchmaking_progress();unavailable(e.what());return false;
  }
  return true;
}
MatchmakingProgressUi MenuState::online_matchmaking_ui() const {
  // 0x199D7C over the replacement service's queue phases.
  using S=OnlineMatchmakingStatus;using U=MatchmakingProgressUi;
  if(matchmaking_have_fun_)return U::have_fun;
  switch(matchmaking_.status) {
  case S::Failed:return U::failed;
  case S::Searching:return matchmaking_.gathering?U::waiting:U::searching;
  case S::CheckingConnections:case S::AwaitingTransport:
    return matchmaking_.match?U::joining:U::searching;
  case S::Loading:return matchmaking_.match?U::start_match:U::searching;
  default:return U::none;
  }
}
unsigned MenuState::online_matchmaking_progress_pane() const {
  return matchmaking_progress_pane(online_matchmaking_ui(),matchmaking_countdown_);
}
bool MenuState::online_matchmaking_team_colors() const {
  // 0x19A127: hopper types3/4/5 are team hoppers. The native playlist names
  // its variant; that variant's own team rule is the equivalent property.
  if(!matchmaking_.match)return false;
  const auto variant=matched_variant(content_,matchmaking_.match->variant);
  return variant && matchmaking_progress_team_colors(matchmaking_countdown_,
      detail::frontend_multiplayer_rules(*variant).team_mode);
}
void MenuState::populate_online_matchmaking(MenuFrame &f) const {
  f.description.clear();
  if (f.screen == 219) {
    f.title = "ONLINE PLAYLISTS";
    for (const auto &p : matchmaking_.playlists) f.entries.push_back({p.name, 237, 0});
    if (f.entries.empty()) f.entries.push_back({"Refresh playlists", -13, 0});
  } else if (f.screen == 237) {
    f.title = "PLAYLIST DETAILS";
    const auto *p = selected(matchmaking_, selected_playlist_);
    f.entries.push_back({p && p->eligible ? "Find Match" : "Playlist unavailable", -14, 0});
    f.entries.push_back({match_voice_ ? "Match Voice: On" : "Match Voice: Off", -15, 0});
  } else {
    f.title = "ONLINE MATCHMAKING";
    if (matchmaking_.match) {
      for (const auto i : online_matchmaking_roster_order()) {
        const auto &p=matchmaking_.match->players[i];
        f.entries.push_back({p.name + (p.self ? " (you)" : ""), -1, int(p.team)});
      }
    } else if (matchmaking_.gathering) {
      for (const auto &p : matchmaking_.gathering->players) f.entries.push_back({p.name + (p.self ? " (you)" : ""), -1, 0});
    } else f.entries.push_back({profile_name(), -1, 0});
  }
}
unsigned MenuState::online_matchmaking_pane() const {
  return matchmaking_progress_pc_pane(online_matchmaking_ui(),matchmaking_countdown_);
}
std::vector<unsigned> MenuState::online_matchmaking_roster_order() const {
  if(!matchmaking_.match)return {};
  std::vector<MatchmakingProgressPlayer> players;
  for(const auto &p:matchmaking_.match->players)players.push_back({int(p.team),p.level?int(*p.level):-1});
  // 0x2514FD sorts game-session B only on pane3; earlier panes keep order.
  if(online_matchmaking_progress_pane()!=3) {
    std::vector<unsigned> order(players.size());
    for(unsigned i=0;i<order.size();++i)order[i]=i;
    return order;
  }
  const auto *playlist=selected(matchmaking_,matchmaking_.playlist_id);
  const auto variant=matched_variant(content_,matchmaking_.match->variant);
  return matchmaking_progress_order(players,playlist && playlist->ranked,
      variant && detail::frontend_multiplayer_rules(*variant).team_mode);
}
std::optional<unsigned> MenuState::online_matchmaking_rank(unsigned row) const {
  if(matchmaking_.status!=OnlineMatchmakingStatus::Joining &&
     matchmaking_.status!=OnlineMatchmakingStatus::Searching &&
     matchmaking_.status!=OnlineMatchmakingStatus::CheckingConnections &&
     matchmaking_.status!=OnlineMatchmakingStatus::AwaitingTransport &&
     matchmaking_.status!=OnlineMatchmakingStatus::Loading)return {};
  const auto *playlist=selected(matchmaking_,matchmaking_.playlist_id);
  if(!playlist || !playlist->ranked || frame().screen!=210)return {};
  std::optional<unsigned> level;
  if(matchmaking_.match) {
    const auto order=online_matchmaking_roster_order();
    if(row<order.size())level=matchmaking_.match->players[order[row]].level;
  } else if(matchmaking_.gathering) {
    if(row<matchmaking_.gathering->players.size())level=matchmaking_.gathering->players[row].level;
  } else if(row==0 && (matchmaking_.status==OnlineMatchmakingStatus::Searching ||
                      matchmaking_.status==OnlineMatchmakingStatus::Joining))level=playlist->level;
  // Service levels are1..50. TU1.5's session byte and wigl ranges are0..49;
  // the last range also covers private raw values49..127.
  return level && *level>=1 && *level<=50 ? std::optional<unsigned>(*level-1) : std::nullopt;
}
std::string MenuState::online_matchmaking_text(const MenuFrame &f, unsigned i) const {
  const auto *p = selected(matchmaking_, f.screen == 219 && f.selected < matchmaking_.playlists.size()
      ? matchmaking_.playlists[f.selected].id : selected_playlist_);
  if (f.screen == 210) {
    if (i == 0 && content_.vista && content_.vista->screens.contains(210)) {
      if (matchmaking_.status == OnlineMatchmakingStatus::Searching || matchmaking_.match) {
        const auto &screen = content_.vista->screen(210);
        const auto localized=[&](std::uint32_t id){return detail::frontend_pc_compiled_localized(*content_.vista,screen,id);};
        const auto ui=online_matchmaking_ui();
        if (online_matchmaking_progress_pane() == 3 && matchmaking_.match) {
          // TU1.5 pane3 (0x251D84) on the PC pane2 frame: text0 "Game starts
          // in" with E478 bound to 0x22D775 and shown only while 0x59590 has a
          // value; text2 the variant (E40A) and text3 "on" the map (E408).
          std::vector<std::string> lines;
          if (matchmaking_countdown_ != matchmaking_countdown_none)
            lines.push_back(replace_token(localized(matchmaking_game_starts_pc),"\xee\x91\xb8",
                matchmaking_countdown_text(matchmaking_countdown_)));
          else if(const auto title=matchmaking_progress_title(ui))lines.push_back(localized(title->pc));
          lines.push_back(replace_token(localized(matchmaking_variant_pc),"\xee\x90\x8a",matchmaking_.match->variant));
          lines.push_back(replace_token(localized(matchmaking_on_map_pc),"\xee\x90\x88",
              matched_map_name(content_,matchmaking_.match->map)));
          std::string text;
          for(const auto &line:lines){if(!text.empty())text+='\n';text+=line;}
          return text;
        }
        // PC64692B replaces the authored pane2 title in its join state; the
        // TU1.5 title (0x251BFF) names the starting states that follow it.
        // Native transport phases are a service boundary, not PC enum values.
        if (online_matchmaking_pane() == 2)
          if(const auto title=matchmaking_progress_title(ui))return localized(title->pc);
        return content_.vista->localized(screen, screen.panes.at(online_matchmaking_pane()).texts.at(0).string);
      }
      return matchmaking_.message.empty() ? "Connecting to matchmaking..." : matchmaking_.message;
    }
    switch (i) {
    case 0: return matchmaking_.status == OnlineMatchmakingStatus::Searching ? "Searching for players..."
        : matchmaking_.match ? "Players found" : "ONLINE matchmaking";
    case 1: return "\xee\x84\x81 CANCEL";
    case 2: return std::to_string(matchmaking_.queued_players) + (matchmaking_.queued_players == 1 ? " player in queue" : " players in queue");
    case 3: return p ? p->name : "Quickmatch";
    case 4: return matchmaking_.match ? "Match details" : "Search status";
    case 5: return (account_.party && !account_.party->leader && matchmaking_.status==OnlineMatchmakingStatus::Ready
        ? account_.party_message.empty()?"Waiting for the party leader.":account_.party_message : matchmaking_.message) +
        (matchmaking_.match ? "\n\nMap: " + matchmaking_.match->map + "\nGame: " + matchmaking_.match->variant : "") + "\n\nVoice: F6 on/off, F7 mic mute. Tap N for team radio.";
    default: return {};
    }
  }
  if (i == 0) return p ? std::string(p->clan_match ? "Clan Ranked" : p->ranked ? "Ranked" : "Casual") + "\n\n" + std::to_string(p->minimum_players) +
      " - " + std::to_string(p->maximum_players) + " players" + (p->level ?
      std::string(p->clan_match ? "\n\nYour clan level: " : "\n\nYour level: ") + std::to_string(*p->level) :
      p->clan_match ? "\n\nClan membership required" : "") : "ONLINE matchmaking";
  if (i == 1) return p ? p->name : "Playlists";
  if (i == 2) return p ? p->description + (p->eligible ? "" : "\n\nUnavailable for this party or installed maps.") : matchmaking_.message;
  return {};
}
} // namespace halo2
