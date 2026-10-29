#include "halo2/frontend.hpp"
#include "ui/frontend_multiplayer_rules.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_matchmaking_readiness.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/original_matchmaking_widgets.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
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
std::optional<UiVariant> matched_variant(const FrontendContent &content, const OnlineMatch &match) {
  const auto &name=match.variant;
  if(match.settings) {
    constexpr std::array<int,7> types{2,3,7,4,1,9,8};
    const auto found=std::find(types.begin(),types.end(),source_game_variant_type(*match.settings));
    if(found==types.end()||!source_game_variant_valid(*match.settings))return std::nullopt;
    UiVariant result{name,unsigned(found-types.begin()),{}};result.source_settings=match.settings;
    return result;
  }
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
  if(online_matchmaking_==client)return;
  if (online_matchmaking_) online_matchmaking_->sign_out();
  online_matchmaking_ = std::move(client);
  matchmaking_active_ = quickmatch_pending_ = false;
  online_lobby_playlist_pending_ = false;
  matchmaking_ = {}; matchmaking_name_.clear();
  reset_online_matchmaking_progress();
}
void MenuState::prepare_online_matchmaking_lobby(bool quickmatch) {
  // OG ONLINE Quickmatch (0x230F92) and Optimatch selection (0x2B2181)
  // set party A's matchmaking mode/hopper, then 0x19B1C9 presents lobby14.
  // Only its START GAME action starts 0x19A78E(10,3), followed by search.
  mode_=3;quickmatch_pending_=false;
  online_lobby_playlist_pending_=quickmatch || !selected_playlist_.empty();
  online_lobby_create_submitted_=false;
  while(stack_.size()>1 && frame().screen!=186 && frame().screen!=14)stack_.pop_back();
  if(frame().screen!=14)open(14);
  enter_online_matchmaking_lobby();
  if(!matchmaking_active_)refresh_online_matchmaking();
  update_online_matchmaking();
}
void MenuState::reset_online_matchmaking_progress() {
  matchmaking_consent_pending_ = false;
  matchmaking_start_clock_.reset();
  matchmaking_countdown_ = matchmaking_countdown_none;
  matchmaking_have_fun_ = false;
  matchmaking_progress_pane_ = 0;
  matchmaking_progress_title_state_ = 0;
  matchmaking_pane_clock_ = ui_clock_;
  matchmaking_search_party_.clear();
}
bool MenuState::update_online_party_search() {
  // A party's search ended without this player's Leave Matchmaking
  // (0x252150 sets g_51ec99 for the canceller only). TU1.5 returns the whole
  // party to pre-game, and the lobby's 0x250804 shows session state4 as 0xAD
  // and a member's departure (states 6/14/19) as 0xAE, once.
  using S = OnlineMatchmakingStatus;
  const bool shown = std::any_of(stack_.begin(), stack_.end(),
      [](const auto &f){return f.screen == 210 || f.screen == 14;});
  const bool searching = matchmaking_.status == S::Searching ||
      matchmaking_.status == S::CheckingConnections || matchmaking_.status == S::AwaitingTransport;
  if (searching && shown && account_.party && account_.party->members.size() > 1) {
    if (!matchmaking_search_party_.empty()) return false;
    for (const auto &m : account_.party->members) matchmaking_search_party_.push_back(m.handle);
    // 0x19B1C9 presents party A's matchmaking modes as screen210 on every
    // member's machine: a member in the lobby follows the leader's search.
    if (account_.party->leader || mode_ != 3 ||
        std::any_of(stack_.begin(), stack_.end(), [](const auto &f){return f.screen == 210;}))
      return false;
    while (stack_.size() > 1 && frame().screen != 14) stack_.pop_back();
    open(210);
    quickmatch_pending_ = false; // The leader owns this search's playlist.
    return true;
  }
  if (matchmaking_search_party_.empty()) return false;
  const auto searched = std::exchange(matchmaking_search_party_, {});
  // Loading, failures and local commands are not a party-wide cancel. A
  // player who left the party itself gets no notice.
  if (matchmaking_.status != S::Ready || !shown || !account_.party) return false;
  const bool departed = std::any_of(searched.begin(), searched.end(), [&](const auto &handle) {
    return std::none_of(account_.party->members.begin(), account_.party->members.end(),
                        [&](const auto &m){return m.handle == handle;});
  });
  // Stop this client's own queue intent: a leader would otherwise enqueue
  // the retained playlist again.
  quickmatch_pending_ = false;
  if (online_matchmaking_) online_matchmaking_->cancel_queue();
  reset_online_matchmaking_progress();
  while (stack_.size() > 1 && frame().screen != 186 && frame().screen != 14) stack_.pop_back();
  if (mode_ == 3) {
    if (frame().screen != 14) open(14);
    enter_online_matchmaking_lobby();
  }
  dialog(departed ? 0xae : 0xad, -5, true);
  ++revision;
  return true;
}
void MenuState::refresh_online_matchmaking() {
  if (!online_identity()) return;
  reset_online_matchmaking_progress();
  matchmaking_active_ = true; matchmaking_name_ = profile_name();
  matchmaking_couch_revision_=couch_revision_;
  if (online_matchmaking_) online_matchmaking_->refresh(matchmaking_name_,online_local_players(),appearance_for_profile(signed_in_));
  update_online_matchmaking();
}
void MenuState::update_online_matchmaking() {
  const auto previous_pane = online_matchmaking_progress_pane();
  // Party A outlives game B and the report. Removing its API session while
  // PCR is visible (or between the fresh return factories) invalidates every
  // member's accepted hopper consent. Retain this idle session until the
  // player actually leaves ONLINE/the party or signs out.
  const bool retained_party=matchmaking_active_ && mode_==3 && account_.party.has_value();
  const bool browsing = online_identity() && (retained_party || std::any_of(stack_.begin(), stack_.end(),
      [&](const auto &f){return matchmaking_screen(f) || online_party_screen(f);}));
  if (!browsing) {
    if (matchmaking_active_ && online_matchmaking_) online_matchmaking_->sign_out();
    matchmaking_active_ = quickmatch_pending_ = false;
    online_lobby_playlist_pending_ = false;
    matchmaking_ = {}; matchmaking_name_.clear();
    reset_online_matchmaking_progress();
    return;
  }
  // The lobby countdown's search start opens screen210, whose open() runs a
  // complete update of its own.
  if (update_online_lobby_countdown()) return;
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
  if(matchmaking_.failure_state && std::any_of(stack_.begin(),stack_.end(),[](const auto &f){return f.screen==210;})) {
    // TU1.5 0x742F0 returns failed searches to party A; 0x256AF7 shows
    // the original AB/AC/AE dialog once in its fresh pregame lobby.
    const auto error=original_matchmaking_error_dialog(*matchmaking_.failure_state);
    matchmaking_.failure_state.reset();quickmatch_pending_=false;
    if(online_matchmaking_)online_matchmaking_->cancel_queue();
    reset_online_matchmaking_progress();
    while(stack_.size()>1 && frame().screen!=186 && frame().screen!=14)stack_.pop_back();
    if(frame().screen!=14)open(14);
    enter_online_matchmaking_lobby();
    if(error)dialog(*error,-5,true);
    ++revision;return;
  }
  if(online_lobby_playlist_pending_ && matchmaking_.status==OnlineMatchmakingStatus::Ready) {
    if(online_account_) {
      const auto party=online_account_->snapshot();
      if(!party.party && !party.busy) {
        // open14 may have encountered a busy profile worker. Defer the
        // real party creation until it is available, without inventing a
        // local accepted party or repeatedly retrying a service rejection.
        if(online_lobby_create_submitted_) {
          online_lobby_playlist_pending_=false;
          matchmaking_.status=OnlineMatchmakingStatus::Failed;
          matchmaking_.message=party.party_message.empty()?"The party could not be created.":party.party_message;
        } else if(online_account_->party_command({"create"}))online_lobby_create_submitted_=true;
      }
    }
    if(online_lobby_playlist_pending_) {
      const auto *playlist=selected(matchmaking_,selected_playlist_);
      if(!playlist || !playlist->eligible) {
        auto it=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),[](const auto &p){return p.quickmatch && p.eligible;});
        if(it==matchmaking_.playlists.end() && (account_.party || local_players().size()>1))
          it=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),[](const auto &p){return p.eligible && !p.clan_match;});
        playlist=it==matchmaking_.playlists.end()?nullptr:&*it;
      }
      if(playlist) {
        selected_playlist_=playlist->id;
        // Preserve the user's consent intent across asynchronous party
        // creation. A busy worker retries here; no search is enqueued.
        if(!online_account_ || (account_.party && online_account_->party_command({"ready",{},playlist->id,playlist->revision})))
          online_lobby_playlist_pending_=false;
      } else online_lobby_playlist_pending_=false;
    }
    for(auto &f:stack_)if(f.screen==14){f.entries.clear();populate_online_party(f);}
    changed=true;
  }
  if (update_online_party_search()) return;
  if (quickmatch_pending_ && matchmaking_.status == OnlineMatchmakingStatus::Ready) {
    const auto *playlist = selected(matchmaking_, selected_playlist_);
    if (!playlist || !playlist->eligible) {
      auto it = std::find_if(matchmaking_.playlists.begin(), matchmaking_.playlists.end(), [](const auto &p) { return p.quickmatch && p.eligible; });
      if(it==matchmaking_.playlists.end() && (account_.party || local_players().size()>1))
        it=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),[](const auto &p){return p.eligible && !p.clan_match;});
      if (it != matchmaking_.playlists.end()) playlist = &*it;
    }
    if (playlist && playlist->eligible && online_matchmaking_) {
      selected_playlist_ = playlist->id;
      if(account_.party) {
        const auto party=online_account_?online_account_->snapshot():OnlineAccountSnapshot{};
        const bool consent=party.party && std::any_of(party.party->members.begin(),party.party->members.end(),
            [&](const auto &member){return member.self && member.ready_playlist==playlist->id;});
        if(!online_account_ || !party.party) {
          quickmatch_pending_=matchmaking_consent_pending_=false;
          matchmaking_.status=OnlineMatchmakingStatus::Failed;
          matchmaking_.message=party.party_message.empty()?"The accepted party is no longer available.":party.party_message;
        } else if(party.busy) {
          // Party and queue workers are independent. Keep the request here
          // until the party worker publishes its actual consent response.
        } else if(consent) {
          quickmatch_pending_=matchmaking_consent_pending_=false;
          if(party.party->leader) {
            online_matchmaking_->join(playlist->id);
            matchmaking_=online_matchmaking_->snapshot();
          }
        } else if(matchmaking_consent_pending_) {
          // A completed command which did not publish consent is a real
          // rejection; do not retry it forever or enqueue anyway.
          quickmatch_pending_=matchmaking_consent_pending_=false;
          matchmaking_.status=OnlineMatchmakingStatus::Failed;
          matchmaking_.message=party.party_message.empty()?"Party playlist selection was not accepted.":party.party_message;
        } else if(online_account_->party_command({"ready",{},playlist->id,playlist->revision})) {
          matchmaking_consent_pending_=true;
        }
      } else {
        quickmatch_pending_=matchmaking_consent_pending_=false;
        online_matchmaking_->join(playlist->id);
        matchmaking_=online_matchmaking_->snapshot();
      }
    } else {
      quickmatch_pending_=matchmaking_consent_pending_=false;
      matchmaking_.status = OnlineMatchmakingStatus::Failed;
      matchmaking_.message = playlist ? "This playlist is unavailable for this party or installed maps." : "Choose an available playlist in Optimatch first.";
    }
    changed = true;
  }
  // c_session_state_start_match::update (0x72700). The relay's loading
  // phase is the replacement service's "every game-session peer is ready"
  // gate (0x6E720/0x6E6B0). B then counts 10000 ms before mode4 starts the
  // game. B's service session publishes the remaining time for every peer;
  // observing Loading late must not start a new countdown on this machine.
  const bool start_ready=matchmaking_.status==OnlineMatchmakingStatus::Loading &&
      matchmaking_.match && online_matchmaking_;
  if(start_ready && !matchmaking_have_fun_) {
    const auto countdown=matchmaking_published_start_countdown(
        matchmaking_.start_countdown_supported?matchmaking_.start_countdown_ms:std::nullopt);
    if(countdown.seconds!=matchmaking_countdown_){matchmaking_countdown_=countdown.seconds;changed=true;}
    if(countdown.ready) {
      // network_session_set_mode(b,4): match mode2, "Have fun!". A session
      // not yet handed over by the relay is retried on the next update.
      if(!launch_online_match())return;
      if(launch){matchmaking_have_fun_=true;changed=true;}
    }
  } else if(!start_ready && (matchmaking_countdown_!=matchmaking_countdown_none || matchmaking_have_fun_)) {
    matchmaking_start_clock_.reset();matchmaking_countdown_=matchmaking_countdown_none;
    matchmaking_have_fun_=false;changed=true;
  }
  // c_matchmaking_screen update (0x251B39): a pane change records the pane
  // clock; entering pane3 requests UI sound12.
  if(const auto pane=online_matchmaking_progress_pane();pane!=matchmaking_progress_pane_) {
    const auto entry_sound=matchmaking_progress_pane_sound(matchmaking_progress_pane_,pane);
    matchmaking_progress_title_state_=int(online_matchmaking_ui());
    matchmaking_progress_pane_=pane;matchmaking_pane_clock_=ui_clock_;changed=true;
    // 0x148523 closes an open online Y menu from pane1 on; dialog callback
    // 0x252166 closes the leave-matchmaking choice once pane2 is reached.
    const auto above_progress=[&]{
      for(auto i=stack_.size();i-->0;)if(stack_[i].screen==210)return i+1<stack_.size();
      return false;
    };
    while(above_progress()) {
      const auto &top=frame();
      const bool leave_dialog=top.screen==7 && !top.entries.empty() && top.entries.front().destination==-16;
      const bool y_menu=top.screen==26 || top.screen==31 || top.screen==216 || top.screen==217 || top.screen==211;
      if(!((leave_dialog && pane>=2) || (y_menu && pane>=1)))break;
      stack_.pop_back();
    }
    if(entry_sound && frame().screen==210 && pending_ui_sounds_.size()<16)
      pending_ui_sounds_.push_back(static_cast<FrontendUiSound>(*entry_sound));
  }
  if (!changed) return;
  for (auto &f : stack_) {
    if (!matchmaking_screen(f)) continue;
    if (f.screen == 210 && previous_pane != online_matchmaking_progress_pane()) {
      ++f.pane_generation;
      f.pane_created_clock = ui_clock_;
    }
    std::string focus;
    if ((f.screen == 219 || f.screen == 237) && f.selected < f.entries.size()) focus = f.entries[f.selected].label;
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
    const auto variant=matched_variant(content_,matched);
    if(!variant)throw FormatError("Playlist selected an unavailable game variant.");
    result.variant=*variant;
    result.variant.name=matched.variant;
    const auto rules=detail::frontend_multiplayer_rules(result.variant);
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
      if(multiplayer_map_validator_) {
        try { multiplayer_map_validator_(result); }
        catch(const FormatError &) {
          // Only the actual cache/native admission validator produces AC.
          // Account, profile and malformed-service errors never become a
          // missing-disc diagnosis. Notify the authenticated B owner first.
          online_matchmaking_->reject_content();return false;
        }
      }
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
  const auto variant=matched_variant(content_,*matchmaking_.match);
  return variant && matchmaking_progress_team_colors(matchmaking_countdown_,
      detail::frontend_multiplayer_rules(*variant).team_mode);
}
void MenuState::populate_online_matchmaking(MenuFrame &f) const {
  f.description.clear();
  if (f.screen == 219 || f.screen == 237) {
    const auto &screen=content_.screen(f.screen);
    f.title=detail::mcc_frontend_content(content_)?detail::og_matchmaking_layout::original_string(f.screen,
        detail::og_matchmaking_layout::data().at("original").at("screens").at(std::to_string(f.screen)).at("header")):
        content_.localized(screen,screen.header);
    for (const auto &p : matchmaking_.playlists) f.entries.push_back({p.name, -14, 0});
    if (f.entries.empty()) f.entries.push_back({"Refresh playlists", -13, 0});
  } else {
    const auto &definition=content_.screen(210);
    f.title=expand(detail::mcc_frontend_content(content_)
        ? detail::og_matchmaking_layout::original_string(210,0x06000233)
        : content_.localized(definition,definition.header));
    // Xbox's authored header includes E43A; the PC header has no token.
    // Bind the same actual selected hopper to that PC title explicitly.
    if(const auto *hopper=selected(matchmaking_,selected_playlist_);hopper && !detail::mcc_frontend_content(content_) &&
        content_.localized(definition,definition.header).find("\xee\x90\xba")==std::string::npos)
      f.title += ": "+hopper->name;
    const auto *playlist=selected(matchmaking_,matchmaking_.playlist_id);
    const auto pane=online_matchmaking_progress_pane();
    const bool party=matchmaking_progress_uses_party(pane,playlist && playlist->ranked);
    std::vector<MenuEntry> players;
    if(party) {
      // Session A includes the accepted party and every couch seat, without
      // identifying a party member by a possibly duplicated display name.
      if(account_.party) {
        for(const auto &p:online_party_members())
          players.push_back({p.name,-1,-1});
      } else for(const auto &p:local_players())
        players.push_back({p.name,-1,-1});
    } else if (matchmaking_.match) {
      for (const auto i : online_matchmaking_roster_order()) {
        const auto &p=matchmaking_.match->players[i];
        players.push_back({p.name, -1, int(p.team)});
      }
    } else if (matchmaking_.gathering) {
      for (const auto &p : matchmaking_.gathering->players) players.push_back({p.name, -1, -1});
    }
    const auto game_players=matchmaking_.match?matchmaking_.match->players.size():
        matchmaking_.gathering?matchmaking_.gathering->players.size():0;
    std::optional<unsigned> maximum;
    if(matchmaking_.gathering && (online_matchmaking_ui()==MatchmakingProgressUi::waiting ||
        online_matchmaking_ui()==MatchmakingProgressUi::starting))maximum=matchmaking_.gathering->maximum_players;
    const auto rows=matchmaking_progress_rows(pane,playlist && playlist->ranked,
        party?unsigned(players.size()):0,unsigned(game_players),maximum);
    std::string matched="Matched Player";
    if(content_.vista && content_.vista->screens.contains(210))
      matched=detail::frontend_pc_compiled_localized(*content_.vista,content_.vista->screen(210),matchmaking_matched_player_pc);
    for(unsigned i=0;i<rows.size();++i) {
      switch(rows[i]) {
      case MatchmakingProgressRow::player:f.entries.push_back(players.at(i));break;
      case MatchmakingProgressRow::matched:f.entries.push_back({matched,-1,matchmaking_matched_player_row});break;
      case MatchmakingProgressRow::empty:f.entries.push_back({{},-1,matchmaking_empty_player_row});break;
      }
    }
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
  const auto variant=matched_variant(content_,*matchmaking_.match);
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
  if(row>=frame().entries.size() || frame().entries[row].value==matchmaking_matched_player_row ||
      frame().entries[row].value==matchmaking_empty_player_row)return {};
  std::optional<unsigned> level;
  if(matchmaking_progress_uses_party(online_matchmaking_progress_pane(),true)) {
    // Only the primary viewer's rank has a party-A identity binding in this
    // service snapshot. Opponents' ranks never identify a same-named friend.
    if(account_.party) {
      const auto party=online_party_members();
      if(row<party.size() && party[row].self && party[row].local_controller==-1)level=playlist->level;
    } else if(row==0)level=playlist->level;
  } else if(matchmaking_.match) {
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
std::optional<std::uint32_t> MenuState::online_matchmaking_map_bitmap() const {
  if(!matchmaking_.match)return {};
  for(const auto &map:content_.multiplayer_maps) {
    std::string_view path=map.scenario_path;const auto slash=path.find_last_of("\\/");
    if(slash!=std::string_view::npos)path.remove_prefix(slash+1);
    if(path==matchmaking_.match->map && map.bitmap!=null_datum &&
       detail::og_matchmaking_layout::data().at("dynamic_map_images").contains(map.scenario_path))return map.bitmap;
  }
  return {};
}
std::string MenuState::online_matchmaking_original_text(unsigned pane,unsigned i) const {
  auto value=detail::og_matchmaking_layout::original_text(210,pane,i);
  const auto count=matchmaking_.match?matchmaking_.match->players.size():
      matchmaking_.gathering?matchmaking_.gathering->players.size():online_party_members().size();
  value=replace_token(std::move(value),"\xee\x90\xa5",std::to_string(count)); // E425
  if(i==0 && pane==3) {
    if(matchmaking_countdown_==matchmaking_countdown_none)return {};
    return replace_token(std::move(value),"\xee\x91\x9b",matchmaking_countdown_text(matchmaking_countdown_));
  }
  if(i==0 && (pane==0 || pane==1)) {
    if(const auto title=original_matchmaking_title(matchmaking_progress_title_state_))
      return expand(detail::og_matchmaking_layout::original_string(210,title));
  }
  if(pane==1 && i==3) {
    const auto minimum=matchmaking_.gathering?matchmaking_.gathering->minimum_players:0;
    const auto needed_state=original_matchmaking_needed(matchmaking_.gathering.has_value(),int(count),int(minimum));
    if(needed_state==OriginalMatchmakingNeeded::hidden)return {};
    if(needed_state==OriginalMatchmakingNeeded::no_players_needed)return detail::og_matchmaking_layout::original_string(210,0x11000762);
    auto needed=detail::og_matchmaking_layout::original_string(210,0x0e000761);
    return replace_token(std::move(needed),"%d",std::to_string(minimum-count));
  }
  if(pane==3 && matchmaking_.match) {
    if(i==2)return replace_token(std::move(value),"\xee\x90\x8a",matchmaking_.match->variant);
    if(i==3)return replace_token(std::move(value),"\xee\x90\x88",matched_map_name(content_,matchmaking_.match->map));
    if(i>=4 && i<=9) {
      const auto variant=matched_variant(content_,*matchmaking_.match);if(!variant)return {};
      std::map<unsigned,int> queries;const auto settings=source_game_variant(*variant);
      for(unsigned id=0;id<112;++id)queries.emplace(id,source_game_variant_get(settings,int(id)));
      const auto options=frontend_matchmaking_quick_options(content_,variant->engine,queries,
          detail::frontend_compiled_localized(content_,content_.screens.at(14),0x1a001820));
      return options[i-4].value_or("");
    }
    //2580B2 calls257FBF only for source hopper-description type5. That
    // helper joins the first member's actual account/clan string at+50 for
    // each represented team, not localized Red/Blue/etc. The replacement
    // match payload does not publish those source account strings.
    if(i==11)return {};
  }
  return expand(value);
}
std::string MenuState::online_matchmaking_text(const MenuFrame &f, unsigned i) const {
  const auto *p = selected(matchmaking_, (f.screen == 219 || f.screen == 237) && f.selected < matchmaking_.playlists.size()
      ? matchmaking_.playlists[f.selected].id : selected_playlist_);
  if(f.screen==219 || f.screen==237) {
    if(i==0)return detail::og_matchmaking_layout::original_text(f.screen,0,0);
    if(i==1)return p?p->name:std::string{};
    if(i==2)return p?p->description+(p->eligible?"":p->clan_match?"\n\nClan membership required; unavailable for this party.":
        "\n\nUnavailable for this party or installed maps."):matchmaking_.message;
    return {};
  }
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
          // The PC main text record holds two lines; the Xbox settings block's
          // variant and "on <map>" rows share the second.
          lines.push_back(replace_token(localized(matchmaking_variant_pc),"\xee\x90\x8a",matchmaking_.match->variant)+" "+
              replace_token(localized(matchmaking_on_map_pc),"\xee\x90\x88",matched_map_name(content_,matchmaking_.match->map)));
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
std::optional<FrontendAppearance> MenuState::online_matchmaking_player_appearance(unsigned row) const {
  if(frame().screen!=210 || row>=frame().entries.size() ||
      frame().entries[row].value==matchmaking_matched_player_row ||
      frame().entries[row].value==matchmaking_empty_player_row)return {};
  const auto *playlist=selected(matchmaking_,matchmaking_.playlist_id);
  if(matchmaking_progress_uses_party(online_matchmaking_progress_pane(),playlist&&playlist->ranked)) {
    if(account_.party){const auto players=online_party_members();if(row<players.size())return players[row].appearance;}
    else {const auto players=local_players();if(row<players.size())return players[row].appearance;}
  } else if(matchmaking_.match) {
    const auto order=online_matchmaking_roster_order();
    if(row<order.size())return matchmaking_.match->players[order[row]].appearance;
  } else if(matchmaking_.gathering && row<matchmaking_.gathering->players.size())
    return matchmaking_.gathering->players[row].appearance;
  return {};
}
std::optional<unsigned> MenuState::online_matchmaking_speaker(unsigned row) const {
  if(frame().screen!=210||row>=frame().entries.size()||
      frame().entries[row].value==matchmaking_matched_player_row||
      frame().entries[row].value==matchmaking_empty_player_row)return {};
  const auto *playlist=selected(matchmaking_,matchmaking_.playlist_id);
  if(matchmaking_progress_uses_party(online_matchmaking_progress_pane(),playlist&&playlist->ranked)) {
    if(!account_.party||!voice_party_||!voice_.joined)return {};
    const auto players=online_party_members();if(row>=players.size())return {};
    const auto &player=players[row];
    if(player.local_controller!=-1)return {}; // No independent microphone observation for a couch guest.
    if(player.self)return voice_.microphone_muted?2u:voice_.transmitting?0u:1u;
    if(std::find(voice_.talking.begin(),voice_.talking.end(),player.handle)!=voice_.talking.end())return 0u;
    return {}; // Joining local party voice is not a remote headset certificate.
  }
  if(matchmaking_.match&&online_matchmaking_){const auto order=online_matchmaking_roster_order();
    if(row<order.size())return online_matchmaking_->player_speaker(order[row]);}
  return {};
}
std::optional<unsigned> MenuState::online_matchmaking_game_icon() const {
  if(!matchmaking_.match)return {};
  const auto variant=matched_variant(content_,*matchmaking_.match);
  if(!variant)return {};
  return original_matchmaking_game_icon(frontend_lobby_variant_type(variant->engine).value_or(-1));
}
} // namespace halo2
