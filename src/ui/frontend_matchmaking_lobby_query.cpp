#include "halo2/frontend.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/bomb_defusal.hpp"
#include "ui/frontend_custom_variant_settings.hpp"
#include "ui/mcc_frontend_string_ids.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> query_bytes(const std::string&s){rust::Vec<std::uint8_t>out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string query_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
bridge::LobbyQueryMember query_member(const OnlinePartyMember&p){return {query_bytes(p.handle),query_bytes(p.name),p.self,p.leader,p.appearance.has_value()};}
bridge::LobbyQueryEdit query_edit(const FrontendLobbyTeamEdit&e){return {e.valid,e.team};}
FrontendLobbyTeamEdit query_edit(const bridge::LobbyQueryEdit&e){return {e.valid,e.team};}
bridge::LobbyQueryMap query_map(const UiLobbyMap&m){bridge::LobbyQueryMap out{};out.scenario=query_bytes(m.scenario_path);out.campaign_id=m.campaign_id;for(auto n:m.team_counts)out.counts.push_back(n);return out;}
FrontendLobbyInput query_input(const bridge::LobbyViewInput&i){FrontendLobbyInput out;
  out.session_enum=i.session;out.previous_pane=i.previous;out.host=i.host;out.team_mode=i.team_mode;out.local_team_selection=i.local_team_selection;
  out.countdown=i.countdown;out.start_permission=i.start_permission;out.update_guard=i.update_guard;out.status=i.status;out.delayed_player=i.delayed_player;
  out.countdown_cancelled=i.countdown_cancelled;out.loading_map=i.loading_map;out.editing_options=i.editing_options;out.team_count=i.team_count;
  out.player_count=i.player_count;out.max_players=i.max_players;out.overflow_status_string=i.overflow_status;out.party_privacy=i.privacy;out.prior_stats_count=i.prior_stats;return out;
}
}
// Rust selects every query and output binding. Native values whose layouts
// belong to rendering/gameplay are retained and copied by Rust-selected indices.
struct FrontendLobbyQueryNativeIo final:bridge::NativeFrontendLobbyQueryIo {
  const MenuState&m;mutable std::exception_ptr exception;
  mutable std::vector<OnlinePartyMember> projected;
  mutable std::optional<FrontendAppearance> appearance;
  mutable std::optional<UiVariant> variant;
  mutable std::optional<SourceGameVariant> settings;
  mutable std::array<std::optional<std::string>,5> quick;
  explicit FrontendLobbyQueryNativeIo(const MenuState&value):m(value){}
  template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native lobby query IO failed");}}
  template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
  bridge::LobbyQueryFacts lobby_query_facts()const override{return capture([&]{
    bridge::LobbyQueryFacts out{};out.mode=m.mode_;out.party=m.account_.party.has_value();
    if(m.account_.party){const auto&p=*m.account_.party;out.leader=p.leader;for(const auto&row:p.members)out.members.push_back(query_member(row));
      if(p.countdown)out.requester=query_bytes(p.countdown->requester);if(p.privacy)out.privacy={true,p.privacy->mode};}
    out.voice_party=m.party_flow_.voice_party();out.joined=m.voice_.joined;out.transmitting=m.voice_.transmitting;out.microphone_muted=m.voice_.microphone_muted;
    for(const auto&name:m.voice_.talking)out.talking.push_back({query_bytes(name)});for(const auto&name:m.voice_.muted)out.muted.push_back({query_bytes(name)});
    out.engine=m.engine_;out.difficulty=m.difficulty_;out.level=m.level_;out.map_index=m.map_;out.protocol=m.lobby_protocol_;
    out.profile=!m.signed_in_.empty();out.team=m.lobby_team_;out.edit=query_edit(m.lobby_team_edit_);
    out.edits={query_edit(m.local_team_edits_[0]),query_edit(m.local_team_edits_[1]),query_edit(m.local_team_edits_[2]),query_edit(m.local_team_edits_[3])};
    out.local_indices={m.system_link_local_slots_[0],m.system_link_local_slots_[1],m.system_link_local_slots_[2],m.system_link_local_slots_[3]};
    out.countdown={m.session_countdown_.has_value(),m.session_countdown_.value_or(0)};out.system_host={m.system_link_host_.has_value(),m.system_link_host_.value_or(false)};
    out.system_machines=m.system_link_machines_;for(const auto&p:m.system_link_players_)out.system_players.push_back({query_bytes(p.name),p.team,p.session_index});
    for(const auto&map:m.content_.campaign_maps)out.campaign_maps.push_back(query_map(map));for(const auto&map:m.content_.multiplayer_maps)out.multiplayer_maps.push_back(query_map(map));return out;
  });}
  rust::Vec<std::uint8_t> lobby_query_start_error()const override{return capture([&]{return query_bytes(m.online_lobby_start_error());});}
  std::int32_t lobby_query_seconds()const override{return capture([&]{return m.online_lobby_.seconds();});}
  rust::Vec<bridge::LobbyQueryMember> lobby_query_members()const override{return capture([&]{projected=m.online_party_members();rust::Vec<bridge::LobbyQueryMember>out;for(const auto&p:projected)out.push_back(query_member(p));return out;});}
  rust::Vec<std::int32_t> lobby_query_locals()const override{return capture([&]{rust::Vec<std::int32_t>out;for(const auto&p:m.local_players())out.push_back(p.team);return out;});}
  bool lobby_query_has_stats()const override{return capture([&]{return m.has_online_prior_stats();});}
  std::int32_t lobby_query_prior_count()const override{return capture([&]{return int(m.online_prior_report_->results->player_count);});}
  rust::Vec<std::uint8_t> lobby_query_campaign_file(std::uint32_t level)const override{return capture([&]{return query_bytes(m.lobby_query_campaign_file(level));});}
  std::int32_t lobby_query_profile_option(std::uint32_t screen,std::uint32_t parameter)const override{return capture([&]{return m.profile_option(screen,parameter);});}
  void lobby_query_variant()const override{capture([&]{variant=m.current_variant();settings=source_game_variant(*variant);});}
  bool lobby_query_defusal()const override{return capture([&]{return bomb_defusal_variant(*variant);});}
  std::int32_t lobby_query_value(std::uint32_t id)const override{return capture([&]{return source_game_variant_get(*settings,int(id));});}
  void lobby_query_appearance()const override{capture([&]{appearance=m.appearance_for_profile(m.signed_in_);});}
  bool lobby_query_custom()const override{return capture([&]{return detail::frontend_custom_variant(*variant);});}
  std::int32_t lobby_query_capacity(std::uint32_t id)const override{return capture([&]{UiParameter p;p.parameter=id;return detail::frontend_variant_parameter_value(*variant,p);});}
  void lobby_query_quick(std::uint32_t engine,rust::Slice<const bridge::LobbyQueryValue>values,std::uint32_t screen,std::uint32_t separator)const override{capture([&]{
    std::map<unsigned,int> queries;for(const auto&v:values)queries.emplace(v.parameter,v.value);
    quick=frontend_lobby_quick_options(m.content_,engine,queries,detail::frontend_compiled_localized(m.content_,m.content_.screens.at(screen),separator));
  });}
  std::optional<FrontendLobbyView> materialize(const bridge::LobbyQueryView&v)const {
    if(!v.present)return {};FrontendLobbyView out;out.input=query_input(v.input);out.protocol=v.protocol;out.machines=v.machines;out.engine=v.engine;out.difficulty=v.difficulty;
    switch(v.map_kind){case 0:break;case 1:out.map=&m.content_.campaign_maps.at(v.map_index);break;case 2:out.map=&m.content_.multiplayer_maps.at(v.map_index);break;default:throw std::logic_error("Invalid native lobby map binding");}
    if(v.profile)out.player_appearance=appearance;out.player_team=v.team;out.team_edit=query_edit(v.edit);
    out.local_team_edits={query_edit(v.edits.v0),query_edit(v.edits.v1),query_edit(v.edits.v2),query_edit(v.edits.v3)};
    out.local_team_session_indices={v.local_indices.v0,v.local_indices.v1,v.local_indices.v2,v.local_indices.v3};
    out.team_count=v.team_count;out.allow_no_team=v.allow_none;if(v.quick)out.quick_options=quick;
    if(v.names_present){out.party_names.emplace();for(const auto&name:v.names)out.party_names->push_back(query_text(name.value));}
    for(auto n:v.speakers)out.party_speakers.push_back(n);
    for(const auto&row:v.rows){
      if(row.system)out.party_players.push_back(m.system_link_players_.at(row.source));
      else {FrontendLobbyRosterPlayer p;p.session_index=row.session_index;p.name=query_text(row.name);p.appearance_available=row.appearance;
        if(row.appearance)p.appearance=*projected.at(row.source).appearance;out.party_players.push_back(std::move(p));}
    }
    return out;
  }
};
std::optional<FrontendLobbyView> MenuState::lobby_view()const {
  FrontendLobbyQueryNativeIo io(*this);return io.invoke([&]{return io.materialize(bridge::frontend_lobby_query(io));});
}
}
