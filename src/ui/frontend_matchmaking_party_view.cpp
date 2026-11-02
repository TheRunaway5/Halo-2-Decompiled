#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include "ui/mcc_frontend_string_ids.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> view_bytes(const std::string &s){rust::Vec<std::uint8_t> out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string view_text(rust::Slice<const std::uint8_t>s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
std::string view_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
bridge::PartyViewMember view_member(const OnlinePartyMember&p){return {view_bytes(p.handle),view_bytes(p.name),p.self,p.leader,p.local_controller};}
bridge::LobbyViewInput view_input(const FrontendLobbyInput &i){
  bridge::LobbyViewInput out{};out.session=i.session_enum;out.previous=i.previous_pane;out.host=i.host;out.team_mode=i.team_mode;
  out.local_team_selection=i.local_team_selection;out.countdown=i.countdown;out.start_permission=i.start_permission;out.update_guard=i.update_guard;
  out.status=i.status;out.delayed_player=i.delayed_player;out.countdown_cancelled=i.countdown_cancelled;out.loading_map=i.loading_map;
  out.editing_options=i.editing_options;out.team_count=i.team_count;out.player_count=i.player_count;out.max_players=i.max_players;
  out.overflow_status=i.overflow_status_string;out.privacy=i.party_privacy;out.prior_stats=i.prior_stats_count;return out;
}
}
// Cached snapshots and actual source/layout/worker queries, plus primitive frame
// operations. Every menu row, branch, binding and lookup key is selected by Rust.
struct FrontendPartyViewNativeIo final:bridge::NativeFrontendPartyViewIo {
  const MenuState &m;const MenuFrame &frame;MenuFrame *write_frame{};
  mutable const UiScreen *screen{};mutable const UiPane *pane{};mutable std::exception_ptr exception;
  FrontendPartyViewNativeIo(const MenuState &value,MenuFrame &f):m(value),frame(f),write_frame(&f){}
  FrontendPartyViewNativeIo(const MenuState &value,const MenuFrame &f):m(value),frame(f){}
  template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native party view IO failed");}}
  template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
  bridge::PartyViewFacts party_view_facts()const override{return capture([&]{
    static_assert(unsigned(OnlineClanPresence::Online)==2 && unsigned(OnlineClanPresence::Playing)==3);
    bridge::PartyViewFacts out{};out.screen=frame.screen;out.context=frame.context;out.description=view_bytes(frame.description);
    out.party=m.account_.party.has_value();if(m.account_.party){const auto &p=*m.account_.party;out.leader=p.leader;out.countdown_supported=p.countdown_supported;
      for(const auto &row:p.members)out.members.push_back(view_member(row));
      for(const auto &row:p.outgoing)out.outgoing.push_back({view_bytes(row.id),view_bytes(row.name)});}
    for(const auto &row:m.account_.party_invitations)out.invitations.push_back({view_bytes(row.id),view_bytes(row.name)});
    for(const auto &row:m.account_.friends)out.friends.push_back({view_bytes(row.name),view_bytes(row.state),view_bytes(row.presence)});
    out.clan=m.account_.clan.has_value();if(m.account_.clan){out.clan_role=m.account_.clan->role;
      for(const auto &row:m.account_.clan->members)out.clan_members.push_back({view_bytes(row.name),row.self,unsigned(row.presence)});}
    out.friends_message=view_bytes(m.account_.friends_message);out.party_message=view_bytes(m.account_.party_message);out.account_message=view_bytes(m.account_.message);
    out.voice_client=bool(m.online_voice_);out.voice_party=m.party_flow_.voice_party();out.voice_joined=m.voice_.joined;out.voice_connecting=m.voice_.connecting;
    out.microphone_muted=m.voice_.microphone_muted;for(const auto &id:m.voice_.muted)out.muted.push_back({view_bytes(id)});return out;
  });}
  bridge::PartyViewLayout party_view_layout(std::uint32_t id,std::uint32_t index,bool direct,bool theme)const override{return capture([&]{
    screen=direct?&m.content_.screens.at(id):&m.content_.screen(id);pane=&screen->panes.at(index);
    return bridge::PartyViewLayout{theme?detail::mcc_frontend_content(m.content_):false,pane->buttons.size()};
  });}
  rust::Vec<std::uint8_t> party_view_read(bool original,std::uint32_t kind,std::uint32_t index)const override{return capture([&]{
    switch(kind){
    case 0:return view_bytes(original?detail::og_matchmaking_layout::original_string(14,
      detail::og_matchmaking_layout::data().at("original").at("screens").at("14").at("header")):m.content_.localized(*screen,screen->header));
    case 1:return view_bytes(original?detail::og_matchmaking_layout::original_string(14,
      detail::og_matchmaking_layout::data().at("original").at("screens").at("14").at("panes").at(2).at("buttons").at(index).at("text").at("string")):
      m.content_.localized(*screen,pane->buttons[index].text.string));
    case 2:return view_bytes(original?detail::og_matchmaking_layout::original_text(14,2,index):m.content_.localized(*screen,pane->texts.at(index).string));
    case 3:return view_bytes(original?detail::og_matchmaking_layout::original_string(14,index):detail::frontend_compiled_localized(m.content_,*screen,index));
    default:throw std::logic_error("Invalid native party view text read");
    }
  });}
  rust::Vec<bridge::PartyViewMember> party_view_members()const override{return capture([&]{rust::Vec<bridge::PartyViewMember> out;for(const auto &p:m.online_party_members())out.push_back(view_member(p));return out;});}
  std::int32_t party_view_seconds()const override{return capture([&]{return m.online_lobby_.seconds();});}
  bool party_view_matchmaking()const override{return capture([&]{return m.online_lobby_.matchmaking();});}
  rust::Vec<std::uint8_t> party_view_selected()const override{return capture([&]{return view_bytes(m.matchmaking_search_.selected());});}
  bridge::PartyViewSnapshot party_view_snapshot()const override{return capture([&]{bridge::PartyViewSnapshot out{};const auto view=m.lobby_view();out.present=view.has_value();
    if(view){out.input=view_input(view->input);out.protocol=view->protocol;out.machines=view->machines;}return out;});}
  bool party_view_has_stats()const override{return capture([&]{return m.has_online_prior_stats();});}
  rust::Vec<std::uint8_t> party_view_start_error()const override{return capture([&]{return view_bytes(m.online_lobby_start_error());});}
  rust::Vec<std::uint8_t> party_view_expand(rust::Slice<const std::uint8_t>value)const override{return capture([&]{return view_bytes(m.expand(view_text(value)));});}
  bool party_view_entries_empty()const override{return capture([&]{return frame.entries.empty();});}
  void party_view_effect(const bridge::PartyViewEffect&e)const override{capture([&]{
    if(!write_frame)throw std::logic_error("Native party text attempted a frame mutation");
    auto &f=*write_frame;
    switch(e.kind){
    case 0:f.description.clear();break;
    case 1:f.title=view_text(e.text);break;
    case 2:f.description=view_text(e.text);break;
    case 3:f.entries.push_back({view_text(e.entry.label),e.entry.destination,e.entry.value});break;
    case 4:{std::vector<MenuEntry> rows;rows.reserve(e.entries.size());for(const auto &entry:e.entries)rows.push_back({view_text(entry.label),entry.destination,entry.value});f.entries=std::move(rows);break;}
    case 5:throw FormatError(view_text(e.text));
    default:throw std::logic_error("Invalid native party view frame effect");
    }
  });}
};
void MenuState::party_view_populate(MenuFrame &frame)const{FrontendPartyViewNativeIo io(*this,frame);io.invoke([&]{party_flow_.populate(io);});}
std::string MenuState::party_view_text(const MenuFrame &frame,unsigned index)const{FrontendPartyViewNativeIo io(*this,frame);return io.invoke([&]{return party_flow_.text(index,io);});}
}
