#include "halo2/frontend.hpp"
#include "ui/frontend_multiplayer_rules.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_matchmaking_readiness.hpp"
#include "halo2/matchmaking_variant.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/original_matchmaking_widgets.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
#include <algorithm>

namespace halo2 {
namespace {

const OnlinePlaylist *selected(const OnlineMatchmakingSnapshot &s, const std::string &id) {
  const auto it = std::find_if(s.playlists.begin(), s.playlists.end(), [&](const auto &p) { return p.id == id; });
  return it == s.playlists.end() ? nullptr : &*it;
}
// mainmenu.map's goof names append the authored U+E008 display glyph.
// Match the catalog name to that row while retaining every source rule.
std::optional<UiVariant> matched_variant(const FrontendContent &content, const OnlineMatch &match) {
  return frontend_matchmaking_variant(content,match);
}
} // namespace
void MenuState::set_online_matchmaking(std::shared_ptr<OnlineMatchmaking> client) {
  if(online_matchmaking_==client)return;
  if(online_matchmaking_)online_matchmaking_->sign_out();
  online_matchmaking_=std::move(client);
  search_drive(FrontendSearchAction::ResetClient);
}








FrontendProgressFacts MenuState::online_matchmaking_progress_facts() const {
  FrontendProgressFacts f;
  f.pane=online_matchmaking_progress_pane();f.ui=int(online_matchmaking_ui());f.status=unsigned(matchmaking_.status);
  const auto *playlist=selected(matchmaking_,matchmaking_.playlist_id);
  if(playlist){f.ranked=playlist->ranked;f.viewer_level=playlist->level;}
  f.party_present=account_.party.has_value();f.match_present=matchmaking_.match.has_value();f.gathering_present=matchmaking_.gathering.has_value();
  if(account_.party)for(const auto &p:online_party_members())
    f.party.push_back({{},std::nullopt,p.self,p.local_controller,p.handle});
  for(const auto &p:local_players())f.local.push_back({{},std::nullopt,true,int(p.controller),{}});
  if(matchmaking_.match) {
    const auto variant=matched_variant(content_,*matchmaking_.match);
    if(variant)f.teams=detail::frontend_multiplayer_rules(*variant).team_mode;
    for(const auto &p:matchmaking_.match->players)
      f.matched.push_back({{int(p.team),p.level?int(*p.level):-1},p.level,p.self,-1,{}});
  }
  if(matchmaking_.gathering) {
    f.maximum=matchmaking_.gathering->maximum_players;
    for(const auto &p:matchmaking_.gathering->players)
      f.gathering.push_back({{},p.level,p.self,-1,{}});
  }
  f.party_voice=party_flow_.voice_party();f.voice_joined=voice_.joined;f.microphone_muted=voice_.microphone_muted;
  f.transmitting=voice_.transmitting;f.talking=voice_.talking;
  return f;
}
std::optional<FrontendProgressRow> MenuState::online_matchmaking_row(unsigned row) const {
  std::vector<int> entries;for(const auto &e:frame().entries)entries.push_back(e.value);
  return frontend_matchmaking_query(online_matchmaking_progress_facts(),int(frame().screen),entries,row);
}
bool MenuState::online_matchmaking_team_colors() const {
  // 0x19A127: hopper types3/4/5 are team hoppers. The native playlist names
  // its variant; that variant's own team rule is the equivalent property.
  if(!matchmaking_.match)return false;
  const auto variant=matched_variant(content_,*matchmaking_.match);
  return variant && matchmaking_progress_team_colors(matchmaking_search_.countdown(),
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
    if(const auto *hopper=selected(matchmaking_,matchmaking_search_.selected());hopper && !detail::mcc_frontend_content(content_) &&
        content_.localized(definition,definition.header).find("\xee\x90\xba")==std::string::npos)
      f.title += ": "+hopper->name;
    const auto facts=online_matchmaking_progress_facts();
    const auto rows=frontend_matchmaking_project(facts);
    const auto party=online_party_members();
    const auto local=local_players();
    std::string matched="Matched Player";
    if(content_.vista && content_.vista->screens.contains(210))
      matched=detail::frontend_pc_compiled_localized(*content_.vista,content_.vista->screen(210),matchmaking_matched_player_pc);
    for(const auto &row:rows) {
      if(row.kind==MatchmakingProgressRow::matched) {
        f.entries.push_back({matched,-1,matchmaking_matched_player_row});continue;
      }
      if(row.kind==MatchmakingProgressRow::empty) {
        f.entries.push_back({{},-1,matchmaking_empty_player_row});continue;
      }
      switch(row.source) {
      case FrontendProgressSource::party:f.entries.push_back({party.at(row.index).name,-1,-1});break;
      case FrontendProgressSource::local:f.entries.push_back({local.at(row.index).name,-1,-1});break;
      case FrontendProgressSource::match:{
        const auto &p=matchmaking_.match->players.at(row.index);f.entries.push_back({p.name,-1,int(p.team)});break;}
      case FrontendProgressSource::gathering:f.entries.push_back({matchmaking_.gathering->players.at(row.index).name,-1,-1});break;
      case FrontendProgressSource::none:break;
      }
    }
  }
}
unsigned MenuState::online_matchmaking_pane() const {
  return matchmaking_progress_pc_pane(online_matchmaking_ui(),matchmaking_search_.countdown());
}
std::vector<unsigned> MenuState::online_matchmaking_roster_order() const {
  const auto f=online_matchmaking_progress_facts();
  std::vector<MatchmakingProgressPlayer> players;for(const auto &m:f.matched)players.push_back(m.key);
  return matchmaking_progress_pane_order(players,f.pane,f.ranked,f.teams);
}
std::optional<unsigned> MenuState::online_matchmaking_rank(unsigned index) const {
  const auto row=online_matchmaking_row(index);return row?row->rank:std::nullopt;
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
  return render_matchmaking_text(true,nullptr,pane,i);
}
std::string MenuState::online_matchmaking_text(const MenuFrame &f,unsigned i) const {
  return render_matchmaking_text(false,&f,0,i);
}
std::optional<FrontendAppearance> MenuState::online_matchmaking_player_appearance(unsigned index) const {
  const auto row=online_matchmaking_row(index);if(!row)return {};
  switch(row->source) {
  case FrontendProgressSource::party:return online_party_members().at(row->index).appearance;
  case FrontendProgressSource::local:return local_players().at(row->index).appearance;
  case FrontendProgressSource::match:return matchmaking_.match->players.at(row->index).appearance;
  case FrontendProgressSource::gathering:return matchmaking_.gathering->players.at(row->index).appearance;
  case FrontendProgressSource::none:return {};
  }
  return {};
}
std::optional<unsigned> MenuState::online_matchmaking_speaker(unsigned index) const {
  const auto row=online_matchmaking_row(index);if(!row)return {};
  if(row->source==FrontendProgressSource::match && online_matchmaking_)
    return online_matchmaking_->player_speaker(row->index);
  return row->speaker;
}
std::optional<unsigned> MenuState::online_matchmaking_game_icon() const {
  if(!matchmaking_.match)return {};
  const auto variant=matched_variant(content_,*matchmaking_.match);
  if(!variant)return {};
  return original_matchmaking_game_icon(frontend_lobby_variant_type(variant->engine).value_or(-1));
}
} // namespace halo2
