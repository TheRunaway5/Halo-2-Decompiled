#include "halo2/frontend.hpp"
#include "halo2/frontend_matchmaking_presentation.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/matchmaking_variant.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
namespace halo2 {
namespace {
std::string replace_token(std::string text,std::string_view token,const std::string &value) {
  if(const auto at=text.find(token);at!=std::string::npos)text.replace(at,token.size(),value);
  return text;
}
std::string map_name(const FrontendContent &content,const std::string &map) {
  for(const auto &m:content.multiplayer_maps) {
    std::string_view path=m.scenario_path;const auto slash=path.find_last_of("\\/");
    if(slash!=std::string_view::npos)path.remove_prefix(slash+1);
    if(path==map&&!m.name.empty())return m.name;
  }
  return map;
}
} // namespace
std::string MenuState::render_matchmaking_text(bool original,const MenuFrame *frame,unsigned pane,unsigned index) const {
  FrontendTextFacts f;
  if(frame){f.screen=frame->screen;f.selected_row=frame->selected;}
  f.selected_id=matchmaking_search_.selected();
  for(const auto &p:matchmaking_.playlists)f.playlists.push_back({p.id,p.name,p.description,p.eligible,p.ranked,p.clan_match,p.minimum_players,p.maximum_players,p.level});
  f.status=unsigned(matchmaking_.status);f.ui=int(online_matchmaking_ui());f.pane=online_matchmaking_progress_pane();
  f.title=int(matchmaking_search_.title());f.countdown=matchmaking_search_.countdown();
  f.pc_progress=content_.vista&&content_.vista->screens.contains(210);
  if(matchmaking_.match){f.matched=matchmaking_.match->players.size();f.variant=matchmaking_.match->variant;f.map=matchmaking_.match->map;}
  if(matchmaking_.gathering){f.gathering=matchmaking_.gathering->players.size();f.minimum=matchmaking_.gathering->minimum_players;}
  f.queued=matchmaking_.queued_players;f.message=matchmaking_.message;f.party=account_.party.has_value();
  if(account_.party)f.leader=account_.party->leader;
  f.party_message=account_.party_message;
  FrontendTextIo io;
  io.original_text=[](unsigned screen,unsigned pane,unsigned index){return detail::og_matchmaking_layout::original_text(screen,pane,index);};
  io.original_string=[](unsigned screen,std::uint32_t id){return detail::og_matchmaking_layout::original_string(screen,id);};
  io.pc_string=[&](std::uint32_t id){return detail::frontend_pc_compiled_localized(*content_.vista,content_.vista->screen(210),id);};
  io.pc_authored=[&](unsigned p){const auto &screen=content_.vista->screen(210);return content_.vista->localized(screen,screen.panes.at(p).texts.at(0).string);};
  io.map_name=[&]{return map_name(content_,matchmaking_.match->map);};
  io.pc_match_line=[&]{
    const auto &screen=content_.vista->screen(210);
    const auto localized=[&](std::uint32_t id){return detail::frontend_pc_compiled_localized(*content_.vista,screen,id);};
    // Rendering-only expression retained verbatim from the native caller.
    return replace_token(localized(matchmaking_variant_pc),"\xee\x90\x8a",matchmaking_.match->variant)+" "+
        replace_token(localized(matchmaking_on_map_pc),"\xee\x90\x88",map_name(content_,matchmaking_.match->map));
  };
  io.quick_option=[&](unsigned i){
    const auto variant=frontend_matchmaking_variant(content_,*matchmaking_.match);if(!variant)return std::string{};
    std::map<unsigned,int> queries;const auto settings=source_game_variant(*variant);
    for(unsigned id=0;id<112;++id)queries.emplace(id,source_game_variant_get(settings,int(id)));
    const auto options=frontend_matchmaking_quick_options(content_,variant->engine,queries,
        detail::frontend_compiled_localized(content_,content_.screens.at(14),0x1a001820));
    return options.at(i).value_or("");
  };
  io.party_count=[&]{return std::uint64_t(online_party_members().size());};
  io.expand=[&](const std::string &value){return expand(value);};
  return frontend_matchmaking_text_rust(f,original,pane,index,io);
}
} // namespace halo2
