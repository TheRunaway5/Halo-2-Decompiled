#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t>couch_bytes(const std::string&s){rust::Vec<std::uint8_t>out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string couch_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
MenuEntry couch_entry(const bridge::AccountFlowEntry&e){return {couch_text(e.label),e.destination,e.value};}
bridge::CouchFlowFrame couch_frame(const MenuFrame&f){bridge::CouchFlowFrame out{};out.screen=f.screen;out.context=f.context;out.selected=f.selected;out.instance=f.instance;out.description=couch_bytes(f.description);for(const auto&e:f.entries)out.entries.push_back({couch_bytes(e.label),e.destination,e.value});return out;}
}
// Actual shared-pointer lifetimes, provider/profile IO and primitive frame IO.
// Rust owns sign-in/guest/duplicate/join/reset policy and all couch state.
struct FrontendCouchNativeIo final:bridge::NativeFrontendCouchIo{
 MenuState&m;MenuFrame*view;mutable std::shared_ptr<DiscordLogin>held;mutable std::optional<DiscordLoginSnapshot>snapshot;mutable std::optional<MenuFrame>built;mutable std::vector<FrontendLocalPlayer>players;mutable std::vector<OnlineLocalPlayer>result;mutable std::exception_ptr exception;
 FrontendCouchNativeIo(MenuState&state,MenuFrame*frame):m(state),view(frame){}
 template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native couch flow IO failed");}}
 template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
 MenuFrame&target()const{if(!view)throw std::logic_error("Native couch frame owner absent");return *view;}
 std::shared_ptr<DiscordLogin>&login(std::uint32_t kind,std::uint32_t i)const{switch(kind){case 0:return m.discord_login_;case 1:return m.local_discord_logins_.at(i);case 2:return held;default:throw std::logic_error("Invalid native couch login source");}}
 bridge::CouchFlowFacts couch_flow_facts()const override{return capture([&]{bridge::CouchFlowFacts out{};out.status=static_cast<std::int32_t>(m.matchmaking_.status);out.mode=m.mode_;out.primary_login=bool(m.discord_login_);out.account_client=bool(m.online_account_);out.authenticated=m.account_.authenticated;out.factory=bool(m.local_discord_factory_);for(const auto&p:m.local_discord_logins_)out.local_logins.push_back(bool(p));for(const auto&p:m.local_profile_accounts_)out.local_accounts.push_back(bool(p));for(const auto&p:m.local_profiles_)out.profiles.push_back({couch_bytes(p)});for(const auto&i:m.local_inputs_)out.inputs.push_back(i);out.profile=m.profile_;out.signed_in=couch_bytes(m.signed_in_);out.system_link_host=m.system_link_host_.has_value();out.countdown_present=m.session_countdown_.has_value();out.countdown=m.session_countdown_.value_or(0);if(view){out.frame_present=true;out.frame=couch_frame(*view);}else if(!m.stack_.empty()){out.frame_present=true;out.frame=couch_frame(m.frame());}for(const auto&f:m.stack_)out.frames.push_back(couch_frame(f));return out;});}
 bool couch_flow_hold(std::uint32_t i)const override{return capture([&]{held=m.local_discord_logins_.at(i);return bool(held);});}
 void couch_flow_release()const override{capture([&]{held.reset();});}
 bridge::CouchFlowSnapshot couch_flow_snapshot(std::uint32_t kind,std::uint32_t i)const override{return capture([&]{snapshot=login(kind,i)->snapshot();bridge::CouchFlowSnapshot out{};out.status=static_cast<std::int32_t>(snapshot->status);out.identity_present=snapshot->identity.has_value();if(snapshot->identity)out.identity=couch_bytes(snapshot->identity->id);return out;});}
 bool couch_flow_authorized(std::uint32_t kind,std::uint32_t i)const override{return capture([&]{return login(kind,i)->authorization().has_value();});}
 bool couch_flow_steam(std::uint32_t kind,std::uint32_t i)const override{return capture([&]{return login(kind,i)->uses_steam();});}
 void couch_flow_cancel(std::uint32_t kind,std::uint32_t i)const override{capture([&]{login(kind,i)->cancel();});}
 bool couch_flow_identity()const override{return capture([&]{return m.online_identity().has_value();});}
 rust::Vec<std::uint8_t>couch_flow_description()const override{return capture([&]{return couch_bytes(m.build(186,0).description);});}
 bridge::CouchFlowFrame couch_flow_build()const override{return capture([&]{built=m.build(31,0x434f5543);return couch_frame(*built);});}
 rust::Vec<std::uint32_t>couch_flow_players()const override{return capture([&]{players=m.local_players();rust::Vec<std::uint32_t>out;for(const auto&p:players)out.push_back(p.controller);return out;});}
 void couch_flow_effect(const bridge::CouchFlowEffect&e)const override{capture([&]{switch(e.kind){
  case 0:target().title=couch_text(e.text);break;
  case 1:target().description=couch_text(e.text);break;
  case 2:{std::vector<MenuEntry>rows;rows.reserve(e.entries.size());for(const auto&p:e.entries)rows.push_back(couch_entry(p));target().entries=std::move(rows);break;}
  case 3:target().entries.push_back(couch_entry(e.entry));break;
  case 4:m.open(e.a,e.b);break;
  case 5:m.back();break;
  case 6:m.stack_.pop_back();break;
  case 7:++m.revision;break;
  case 8:m.unavailable(couch_text(e.text));break;
  case 9:m.local_profiles_.at(e.index)=couch_text(e.text);break;
  case 10:m.local_inputs_.at(e.index)=e.value;break;
  case 11:m.local_teams_.at(e.index)=0;m.local_team_edits_.at(e.index)={};break;
  case 12:m.profile_=e.index;break;
  case 13:m.local_profile_accounts_.at(e.index)->sign_out();break;
  case 14:m.local_profile_accounts_.at(e.index).reset();break;
  case 15:m.local_profile_accounts_={};break;
  case 16:m.local_discord_logins_.at(e.index).reset();break;
  case 17:m.local_discord_logins_.at(e.index)=m.local_discord_factory_();break;
  case 18:m.local_discord_logins_.at(e.index)->bind_profile(m.preferences_.empty()?std::string{}:std::filesystem::absolute(m.preferences_).lexically_normal().string(),m.profile_login_key(m.local_profiles_.at(e.index)));break;
  case 19:m.local_discord_logins_.at(e.index)->start();break;
  case 20:m.local_profile_accounts_.at(e.index)=m.online_account_->profile_worker(m.local_discord_logins_.at(e.index));break;
  case 21:{const auto tag=m.profile_service_tags_.find(m.local_profiles_.at(e.index));m.local_profile_accounts_.at(e.index)->profile(m.appearance_for_profile(m.local_profiles_.at(e.index)),tag==m.profile_service_tags_.end()?std::string{}:tag->second);break;}
  case 22:m.local_profile_accounts_.at(e.index)->connect(m.local_profiles_.at(e.index));break;
  case 23:m.stack_.at(e.index).description=couch_text(e.text);break;
  case 24:if(!built)throw std::logic_error("Native couch rebuilt frame absent");built->instance=e.instance;built->selected=e.a;m.stack_.back()=std::move(*built);built.reset();break;
  case 25:built.reset();break;
  case 26:{const auto&p=players.at(e.index);result.push_back({p.name,m.local_discord_logins_.at(p.controller),p.appearance});break;}
  case 27:m.local_profiles_.at(e.index)=m.profile_name();break;
  default:throw std::logic_error("Invalid native couch flow effect");
 }});}
};
FrontendCouchResult MenuState::couch_flow_drive(unsigned action,MenuFrame*frame,unsigned a,int signed_value,bool flag,std::string text)const{FrontendCouchNativeIo io(const_cast<MenuState&>(*this),frame);bridge::CouchFlowArgument arg{};arg.a=a;arg.signed_value=signed_value;arg.flag=flag;arg.text=couch_bytes(text);auto value=io.invoke([&]{return party_flow_.couch(action,arg,io);});return {value.handled,value.present?std::optional<unsigned>(value.index):std::nullopt,couch_text(value.text)};}
std::optional<DiscordIdentity>MenuState::couch_identity()const{FrontendCouchNativeIo io(const_cast<MenuState&>(*this),nullptr);bridge::CouchFlowArgument arg{};auto value=io.invoke([&]{return party_flow_.couch(13,arg,io);});if(!value.handled)return {};if(!io.snapshot||!io.snapshot->identity)throw std::logic_error("Native couch identity observation absent");return io.snapshot->identity;}
std::vector<OnlineLocalPlayer>MenuState::couch_players()const{FrontendCouchNativeIo io(const_cast<MenuState&>(*this),nullptr);bridge::CouchFlowArgument arg{};io.invoke([&]{return party_flow_.couch(7,arg,io);});return std::move(io.result);}
} // namespace halo2
