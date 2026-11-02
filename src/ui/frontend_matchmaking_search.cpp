#include "halo2/frontend.hpp"
#include "halo2/native_player_profile.hpp"
#include "ui/frontend_multiplayer_rules.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <vector>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> bytes(std::string_view s){rust::Vec<std::uint8_t> out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string string(rust::Slice<const std::uint8_t> s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
std::string string(const rust::Vec<std::uint8_t>&s){return string({s.data(),s.size()});}
template<class Party>rust::Vec<bridge::SearchMember> members(const Party&p){rust::Vec<bridge::SearchMember> out;for(const auto&m:p.members)out.push_back({m.handle,m.self,m.ready_playlist});return out;}
bridge::SearchSnapshot snapshot(const OnlineMatchmakingSnapshot &s){
 bridge::SearchSnapshot out{};out.status=unsigned(s.status);out.matched=s.match.has_value();out.gathering=s.gathering.has_value();out.revision=s.revision;out.message_empty=s.message.empty();
 out.countdown_supported=s.start_countdown_supported;out.countdown_present=s.start_countdown_ms.has_value();out.countdown_ms=s.start_countdown_ms.value_or(0);out.failure_present=s.failure_state.has_value();out.failure=s.failure_state.value_or(0);
 for(const auto&p:s.playlists)out.playlists.push_back({p.id,p.revision,p.eligible,p.quickmatch,p.clan_match});return out;
}
bool presentation_frame(const MenuFrame&f){return f.screen==210||((f.screen==219||f.screen==237)&&f.context==0);}
}
std::optional<UiVariant> frontend_matchmaking_variant(const FrontendContent&content,const OnlineMatch&match){
 std::vector<bridge::SearchName> content_names,bundled_names;for(const auto&v:content.variants)content_names.push_back({bytes(v.name)});for(const auto&v:content.bundled_variants)bundled_names.push_back({bytes(v.name)});
 rust::Vec<std::uint8_t> raw;if(match.settings)for(auto b:match.settings->bytes)raw.push_back(b);
 const auto name=bytes(match.variant);const auto choice=bridge::frontend_search_variant({name.data(),name.size()},match.settings.has_value(),{raw.data(),raw.size()},{content_names.data(),content_names.size()},{bundled_names.data(),bundled_names.size()});
 switch(choice.kind){case 0:return {};case 1:{UiVariant v{match.variant,choice.index,{}};v.source_settings=match.settings;return v;}case 2:return content.variants.at(choice.index);case 3:return content.bundled_variants.at(choice.index);default:throw std::logic_error("Invalid native frontend variant choice");}
}
struct FrontendSearchNativeIo final:bridge::NativeFrontendSearchIo {
 MenuState &m;mutable std::exception_ptr exception;
 mutable std::optional<FrontendLaunch> candidate;
 mutable std::vector<FrontendLocalPlayer> locals;
 mutable std::vector<unsigned> slots;
 mutable std::vector<const OnlineAdmission*> admitted;
 explicit FrontendSearchNativeIo(MenuState&state):m(state){}
 template<class F>decltype(auto) capture(F&&f)const{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native frontend search IO failed");}}
 template<class F>decltype(auto) invoke(F&&f)const{try{return f();}catch(const std::exception&e){if(exception)std::rethrow_exception(exception);throw FormatError(e.what());}}
 bridge::SearchContext search_context()const override{return capture([&]{bridge::SearchContext c{};c.mode=std::int32_t(m.mode_);c.clock=m.ui_clock_;c.couch_revision=m.couch_revision_;c.queue=bool(m.online_matchmaking_);c.account=bool(m.online_account_);
  for(const auto&f:m.stack_)c.frames.push_back({f.screen,f.context,m.online_party_screen(f),!f.entries.empty(),f.entries.empty()?0:f.entries.front().destination});
  c.party_present=m.account_.party.has_value();if(m.account_.party){c.leader=m.account_.party->leader;c.members=members(*m.account_.party);}return c;});}
 bridge::SearchSnapshot search_view()const override{return capture([&]{return snapshot(m.matchmaking_);});}
 bool search_identity()const override{return capture([&]{return m.online_identity().has_value();});}
 rust::Vec<std::uint8_t> search_profile()const override{return capture([&]{return bytes(m.profile_name());});}
 std::uint32_t search_local_count()const override{return capture([&]{return unsigned(m.local_players().size());});}
 bool search_pull_after(std::uint64_t revision)const override{return capture([&]{if(auto s=m.online_matchmaking_->snapshot_after(revision)){m.matchmaking_=std::move(*s);return true;}return false;});}
 void search_pull()const override{capture([&]{m.matchmaking_=m.online_matchmaking_->snapshot();});}
 bridge::SearchAccount search_account()const override{return capture([&]{const auto s=m.online_account_->snapshot();bridge::SearchAccount out{};out.present=s.party.has_value();out.busy=s.busy;out.message=bytes(s.party_message);if(s.party){out.leader=s.party->leader;out.members=members(*s.party);}return out;});}
 bool search_command(rust::Str a,rust::Str p,rust::Str r)const override{return capture([&]{return m.online_account_->party_command({std::string(a),{},std::string(p),std::string(r)});});}
 void search_sign_out()const override{capture([&]{m.online_matchmaking_->sign_out();});}
 void search_refresh(rust::Slice<const std::uint8_t> name)const override{capture([&]{m.online_matchmaking_->refresh(string(name),m.online_local_players(),m.appearance_for_profile(m.signed_in_));});}
 void search_cancel()const override{capture([&]{m.online_matchmaking_->cancel_queue();});}
 void search_join(rust::Str p)const override{capture([&]{m.online_matchmaking_->join(std::string(p));});}
 bool search_lobby_countdown()const override{return capture([&]{return m.update_online_lobby_countdown();});}
 bool search_launch()const override{return capture([&]{return m.launch_online_match();});}
 bool search_has_launch()const override{return capture([&]{return m.launch.has_value();});}
 void render(unsigned previous)const{
  for(auto&f:m.stack_){if(!presentation_frame(f))continue;
   if(f.screen==210&&previous!=m.online_matchmaking_progress_pane()){++f.pane_generation;f.pane_created_clock=m.ui_clock_;}
   std::string focus;if((f.screen==219||f.screen==237)&&f.selected<f.entries.size())focus=f.entries[f.selected].label;
   f.entries.clear();m.populate_online_matchmaking(f);
   if(!focus.empty()){const auto i=std::find_if(f.entries.begin(),f.entries.end(),[&](const auto&e){return e.label==focus;});if(i!=f.entries.end())f.selected=unsigned(i-f.entries.begin());}
   f.selected=std::min(f.selected,unsigned(f.entries.size()-1));
  }
 }
 void search_ui(const bridge::SearchUi&e)const override{capture([&]{using K=bridge::SearchUiKind;
  switch(e.kind){
  case K::ClearSnapshot:m.matchmaking_={};break;
  case K::NoService:m.matchmaking_.message="No matchmaking service configured.";break;
  case K::Failure:m.matchmaking_.status=OnlineMatchmakingStatus::Failed;m.matchmaking_.message=string(e.text);break;
  case K::ClearFailure:m.matchmaking_.failure_state.reset();break;
  case K::ModeOnline:m.mode_=3;break;
  case K::PopLobbyOrOnline:while(m.stack_.size()>1&&m.frame().screen!=186&&m.frame().screen!=14)m.stack_.pop_back();break;
  case K::PopLobby:while(m.stack_.size()>1&&m.frame().screen!=14)m.stack_.pop_back();break;
  case K::OpenLobby:m.open(14);break;
  case K::EnterLobby:m.enter_online_matchmaking_lobby();break;
  case K::OpenSearch:m.open(210);break;
  case K::Dialog:m.dialog(e.number,-5,true);break;
  case K::Revision:++m.revision;break;
  case K::RepopulateLobby:for(auto&f:m.stack_)if(f.screen==14){f.entries.clear();m.populate_online_party(f);}break;
  case K::PopTop:m.stack_.pop_back();break;
  case K::Sound:if(m.pending_ui_sounds_.size()<16)m.pending_ui_sounds_.push_back(static_cast<FrontendUiSound>(e.number));break;
  case K::Render:render(e.number);break;
  case K::Unavailable:m.unavailable(string(e.text));break;
  case K::PlaylistError:m.pending_ui_sounds_.push_back(FrontendUiSound::Error);m.unavailable("This playlist is unavailable for your ONLINE client, party, or installed maps. Use an updated ONLINE build and check the playlist's party and map requirements.");break;
  default:throw std::logic_error("Invalid native frontend UI effect");
  }
 });}
 bridge::SearchVariant search_variant()const override{return capture([&]{candidate.emplace();const auto&matched=*m.matchmaking_.match;const auto v=frontend_matchmaking_variant(m.content_,matched);if(!v)return bridge::SearchVariant{false,false};candidate->variant=*v;candidate->variant.name=matched.variant;return bridge::SearchVariant{true,detail::frontend_multiplayer_rules(candidate->variant).team_mode};});}
 rust::Vec<std::uint8_t> search_map()const override{return capture([&]{return bytes(m.matchmaking_.match->map);});}
 void search_prepare_map(rust::Slice<const std::uint8_t> map)const override{capture([&]{candidate->map=string(map);candidate->multiplayer=true;candidate->match_voice=m.party_flow_.match_voice(m.match_voice_,m.voice_.joined);});}
 bool search_self_present()const override{return capture([&]{const auto&players=m.matchmaking_.match->players;return std::any_of(players.begin(),players.end(),[](const auto&p){return p.self;});});}
 bridge::SearchTake search_take()const override{return capture([&]{candidate->online_match=m.online_matchmaking_->take_launch();bridge::SearchTake out{};out.present=bool(candidate->online_match);if(out.present){const auto teams=candidate->online_match->team_mode();out.teams_present=teams.has_value();out.teams=teams.value_or(false);}return out;});}
 bridge::SearchCounts search_local_counts()const override{return capture([&]{locals=m.local_players();slots=candidate->online_match->local_slots();admitted.assign(locals.size(),nullptr);return bridge::SearchCounts{unsigned(locals.size()),unsigned(slots.size())};});}
 void search_set_local_count(std::uint32_t n)const override{capture([&]{candidate->local_players=n;});}
 rust::Vec<std::uint8_t> search_admitted_name(std::uint32_t i)const override{return capture([&]{const auto&p=candidate->online_match->roster().at(slots.at(i));admitted.at(i)=&p;return bytes(p.name);});}
 rust::Vec<std::uint8_t> search_profile_name(std::uint32_t i)const override{return capture([&]{return bytes(locals.at(i).name);});}
 void search_append_profile(std::uint32_t i,bool teams)const override{capture([&]{const auto&p=locals.at(i);const auto&a=*admitted.at(i);candidate->selected_profiles.push_back(native_selected_player_configuration(p.name,std::span<const std::uint8_t,9>(reinterpret_cast<const std::uint8_t*>(&p.appearance),9),teams?a.team:p.team));candidate->input_instances.push_back(p.input_instance);if(teams)candidate->selected_teams.push_back(std::int8_t(a.team));});}
 bool search_validate_map()const override{return capture([&]{if(m.multiplayer_map_validator_){try{m.multiplayer_map_validator_(*candidate);}catch(const FormatError&){return false;}}return true;});}
 void search_reject_content()const override{capture([&]{m.online_matchmaking_->reject_content();});}
 void search_publish()const override{capture([&]{m.launch=std::move(*candidate);});}
 void search_discard()const override{capture([&]{admitted.clear();slots.clear();locals.clear();candidate.reset();});}
 bridge::SearchFailure search_failure()const override{return capture([&]{bridge::SearchFailure f{};if(exception){try{std::rethrow_exception(exception);}catch(const FormatError&e){f.format=true;f.text=bytes(e.what());}catch(...){}}return f;});}
};
bool MenuState::search_drive(FrontendSearchAction action,unsigned index){FrontendSearchNativeIo io(*this);return io.invoke([&]{return matchmaking_search_.run(action,io,index);});}
void MenuState::prepare_online_matchmaking_lobby(bool quick){search_drive(quick?FrontendSearchAction::Quickmatch:FrontendSearchAction::Prepare);}
void MenuState::reset_online_matchmaking_progress(){matchmaking_search_.reset(ui_clock_);}
bool MenuState::update_online_party_search(){return search_drive(FrontendSearchAction::PartySearch);}
void MenuState::refresh_online_matchmaking(){search_drive(FrontendSearchAction::Refresh);}
void MenuState::update_online_matchmaking(){search_drive(FrontendSearchAction::Update);}
bool MenuState::launch_online_match(){return search_drive(FrontendSearchAction::Launch);}
MatchmakingProgressUi MenuState::online_matchmaking_ui()const{FrontendSearchNativeIo io(const_cast<MenuState&>(*this));return io.invoke([&]{return static_cast<MatchmakingProgressUi>(matchmaking_search_.ui(io.search_view()));});}
unsigned MenuState::online_matchmaking_progress_pane()const{FrontendSearchNativeIo io(const_cast<MenuState&>(*this));return io.invoke([&]{return matchmaking_search_.pane(io.search_view());});}
}
