#include "halo2/frontend.hpp"
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
} // namespace
void MenuState::set_online_matchmaking(std::shared_ptr<OnlineMatchmaking> client) {
  if (online_matchmaking_) online_matchmaking_->sign_out();
  online_matchmaking_ = std::move(client);
  matchmaking_active_ = quickmatch_pending_ = false;
  matchmaking_ = {}; matchmaking_name_.clear();
}
void MenuState::refresh_online_matchmaking() {
  if (!online_identity()) return;
  matchmaking_active_ = true; matchmaking_name_ = profile_name();
  matchmaking_couch_revision_=couch_revision_;
  if (online_matchmaking_) online_matchmaking_->refresh(matchmaking_name_,online_local_players());
  update_online_matchmaking();
}
void MenuState::update_online_matchmaking() {
  const auto previous_pane = online_matchmaking_pane();
  const bool browsing = online_identity() && std::any_of(stack_.begin(), stack_.end(),
      [&](const auto &f){return matchmaking_screen(f) || online_party_screen(f);});
  if (!browsing) {
    if (matchmaking_active_ && online_matchmaking_) online_matchmaking_->sign_out();
    matchmaking_active_ = quickmatch_pending_ = false;
    matchmaking_ = {}; matchmaking_name_.clear();
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
  if (!changed) return;
  if(matchmaking_.status==OnlineMatchmakingStatus::Loading && matchmaking_.match && online_matchmaking_) {
    try {
      FrontendLaunch result;
      const auto &matched=*matchmaking_.match;
      // mainmenu.map's goof names append the authored U+E008 display glyph.
      // Match the catalog name to that row while retaining every source rule.
      const auto variant=std::find_if(content_.variants.begin(),content_.variants.end(),[&](const auto &v){
        std::string_view name=v.name;
        if(name.ends_with(" \xee\x80\x88"))name.remove_suffix(4);
        return name==matched.variant;
      });
      if(variant!=content_.variants.end())result.variant=*variant;
      else {
        const auto extra=std::find_if(content_.bundled_variants.begin(),content_.bundled_variants.end(),[&](const auto &v){return v.name==matched.variant;});
        if(extra==content_.bundled_variants.end())throw FormatError("Playlist selected an unavailable game variant.");
        result.variant=*extra;
      }
      result.variant.name=matched.variant;
      const auto rules=slayer_rules(result.variant,true);
      if(rules.custom==CustomSlayer::none && (matched.map=="midship" || matched.map=="waterworks"))throw FormatError("Playlist selected a map whose starting weapons are unfinished.");
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
    }catch(const FormatError &e) { online_matchmaking_->cancel_queue(); unavailable(e.what()); return; }
  }
  for (auto &f : stack_) {
    if (!matchmaking_screen(f)) continue;
    if (f.screen == 210 && previous_pane != online_matchmaking_pane()) {
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
      for (const auto &p : matchmaking_.match->players) f.entries.push_back({p.name + (p.self ? " (you)" : ""), -1, int(p.team)});
    } else if (matchmaking_.gathering) {
      for (const auto &p : matchmaking_.gathering->players) f.entries.push_back({p.name + (p.self ? " (you)" : ""), -1, 0});
    } else f.entries.push_back({profile_name(), -1, 0});
  }
}
unsigned MenuState::online_matchmaking_pane() const {
  if (matchmaking_.match && (matchmaking_.status == OnlineMatchmakingStatus::CheckingConnections ||
      matchmaking_.status == OnlineMatchmakingStatus::AwaitingTransport ||
      matchmaking_.status == OnlineMatchmakingStatus::Loading)) return 2;
  return matchmaking_.status == OnlineMatchmakingStatus::Searching && matchmaking_.gathering ? 1 : 0;
}
std::string MenuState::online_matchmaking_text(const MenuFrame &f, unsigned i) const {
  const auto *p = selected(matchmaking_, f.screen == 219 && f.selected < matchmaking_.playlists.size()
      ? matchmaking_.playlists[f.selected].id : selected_playlist_);
  if (f.screen == 210) {
    if (i == 0 && content_.vista && content_.vista->screens.contains(210)) {
      if (matchmaking_.status == OnlineMatchmakingStatus::Searching || matchmaking_.match) {
        const auto &screen = content_.vista->screen(210);
        // PC64692B replaces the authored pane2 title in its join state.
        // Native transport phases are a service boundary, not PC enum values.
        if (online_matchmaking_pane() == 2)
          return matchmaking_.status == OnlineMatchmakingStatus::Loading ? matchmaking_.message
              : content_.vista->localized(screen, 0x0c00043d);
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
