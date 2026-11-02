#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t>clans_bytes(const std::string&s){rust::Vec<std::uint8_t>out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string clans_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
bridge::ClansFlowCommand clans_encoded(const OnlineClanCommand&c){return {clans_bytes(c.action),clans_bytes(c.value),c.role,c.confirmed};}
OnlineClanCommand clans_command(const bridge::ClansFlowCommand&c){return {clans_text(c.action),clans_text(c.value),c.role,c.confirmed};}
OnlinePartyCommand clans_party_command(const bridge::PartyFlowCommand&c){OnlinePartyCommand out;out.action=clans_text(c.action);out.value=clans_text(c.value);out.playlist=clans_text(c.playlist);out.revision=clans_text(c.revision);out.version=c.version;if(c.privacy_present)out.privacy=c.privacy;out.privacy_revision=c.privacy_revision;out.privacy_context=clans_text(c.privacy_context);out.join_context=clans_text(c.join_context);return out;}
MenuEntry clans_entry(const bridge::AccountFlowEntry&e){return {clans_text(e.label),e.destination,e.value};}
bridge::ClansFlowFrame clans_frame(const MenuFrame&f){bridge::ClansFlowFrame out{};out.screen=f.screen;out.context=f.context;out.selected=f.selected;out.description=clans_bytes(f.description);for(const auto&e:f.entries)out.entries.push_back({clans_bytes(e.label),e.destination,e.value});return out;}
}
// Native lifetimes, cached observations and primitive frame/provider IO only.
// Rust owns clan actions, confirmations, targets, pages and binding order.
struct FrontendClansNativeIo final:bridge::NativeFrontendClansIo{
 MenuState&m;MenuFrame*view;mutable std::shared_ptr<OnlineAccount>candidate_account;mutable std::shared_ptr<OnlineVoice>candidate_voice;
 using Screen=std::remove_cvref_t<decltype(std::declval<MenuState&>().content_.screens.at(26))>;
 using Pane=std::remove_cvref_t<decltype(std::declval<Screen>().panes.at(0))>;
 mutable const Screen*screen{};mutable const Pane*pane{};mutable std::exception_ptr exception;
 FrontendClansNativeIo(MenuState&state,MenuFrame*frame):m(state),view(frame){}
 template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native clans flow IO failed");}}
 template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
 MenuFrame&target(bool top)const{if(top){if(m.stack_.empty())throw std::logic_error("Native clan stack owner absent");return m.stack_.back();}if(!view)throw std::logic_error("Native clan frame owner absent");return *view;}
 bridge::ClansFlowFacts clans_flow_facts()const override{return capture([&]{
  bridge::ClansFlowFacts out{};out.account_client=bool(m.online_account_);out.voice_client=bool(m.online_voice_);out.social_client=bool(m.discord_social_);out.same_account=m.online_account_==candidate_account;out.candidate_account=bool(candidate_account);for(const auto&a:m.local_profile_accounts_)out.local_accounts.push_back(bool(a));
  const auto&a=m.account_;out.account.message=clans_bytes(a.message);out.account.clan_present=a.clan.has_value();if(a.clan){const auto&c=*a.clan;auto&v=out.account.clan;v.name=clans_bytes(c.name);v.role=c.role;for(const auto&p:c.members)v.members.push_back({clans_bytes(p.id),clans_bytes(p.name),p.role,p.self,static_cast<std::int32_t>(p.presence)});for(const auto&p:c.outgoing)v.outgoing.push_back({clans_bytes(p.id),clans_bytes(p.name)});for(const auto&p:c.messages)v.messages.push_back({clans_bytes(p.name),clans_bytes(p.body)});}for(const auto&p:a.invitations)out.account.invitations.push_back({clans_bytes(p.id),clans_bytes(p.name)});out.account.party_present=a.party.has_value();out.account.leader=a.party&&a.party->leader;
  const auto&v=m.voice_;out.voice.joined=v.joined;out.voice.connecting=v.connecting;out.voice.microphone_muted=v.microphone_muted;out.voice.transmitting=v.transmitting;out.voice.message=clans_bytes(v.message);out.voice.revision=v.revision;for(const auto&id:v.muted)out.voice.muted.push_back({clans_bytes(id)});for(const auto&id:v.talking)out.voice.talking.push_back({clans_bytes(id)});
  if(view){out.frame_present=true;out.frame=clans_frame(*view);}else if(!m.stack_.empty()){out.frame_present=true;out.frame=clans_frame(m.frame());}for(const auto&f:m.stack_)out.frames.push_back(clans_frame(f));return out;
 });}
 bool clans_flow_command(const bridge::ClansFlowCommand&c)const override{return capture([&]{return m.online_account_->command(clans_command(c));});}
 void clans_flow_party_command(const bridge::PartyFlowCommand&c)const override{capture([&]{m.party_command(clans_party_command(c));});}
 void clans_flow_update_account()const override{capture([&]{m.update_online_account();});}
 void clans_flow_select(std::uint32_t i)const override{capture([&]{m.select(i);});}
 void clans_flow_prepare_pane(std::uint32_t s,std::uint32_t p)const override{capture([&]{screen=&m.content_.screens.at(s);pane=&screen->panes.at(p);});}
 rust::Vec<std::uint8_t>clans_flow_localized(std::uint32_t i)const override{return capture([&]{if(!screen||!pane)throw std::logic_error("Native clan localization owner absent");return clans_bytes(m.content_.localized(*screen,pane->texts.at(i).string));});}
 rust::Vec<std::uint8_t>clans_flow_localized_direct(std::uint32_t s,std::uint32_t p,std::uint32_t i)const override{return capture([&]{return clans_bytes(m.content_.localized(m.content_.screens.at(s),m.content_.screens.at(s).panes.at(p).texts.at(i).string));});}
 void clans_flow_effect(const bridge::ClansFlowEffect&e)const override{capture([&]{switch(e.kind){
  case 0:target(e.top).title=clans_text(e.text);break;
  case 1:target(e.top).description=clans_text(e.text);break;
  case 2:target(e.top).entries.push_back(clans_entry(e.entry));break;
  case 3:{std::vector<MenuEntry>rows;rows.reserve(e.entries.size());for(const auto&p:e.entries)rows.push_back(clans_entry(p));target(e.top).entries=std::move(rows);break;}
  case 4:target(e.top).selected=e.a;break;
  case 5:m.open(e.a,e.b);break;
  case 6:m.stack_.pop_back();break;
  case 7:m.back();break;
  case 8:m.keyboard_text.clear();break;
  case 9:++m.revision;break;
  case 10:m.unavailable(clans_text(e.text));break;
  case 11:m.local_profile_accounts_.at(e.index)->sign_out();break;
  case 12:m.local_profile_accounts_={};break;
  case 13:m.online_voice_->leave();break;
  case 14:m.online_account_->sign_out();break;
  case 15:m.online_account_=std::move(candidate_account);break;
  case 16:m.account_={};break;
  case 17:m.pending_ui_sounds_.clear();break;
  case 18:m.discord_social_->bind_account(m.online_account_);break;
  case 19:m.online_voice_=std::make_shared<OnlineVoice>(e.flag?candidate_account:m.online_account_);break;
  case 20:m.online_voice_.reset();break;
  case 21:m.voice_={};m.voice_.revision=e.index;break;
  case 22:m.online_voice_=std::move(candidate_voice);break;
  case 23:m.online_voice_->push_to_talk(e.flag);break;
  case 24:m.online_voice_->join();break;
  case 25:m.online_voice_->mute_microphone(e.flag);break;
  case 26:m.online_voice_->mute(clans_text(e.text),e.flag);break;
  default:throw std::logic_error("Invalid native clans effect");
 }});}
};
FrontendClansResult MenuState::clans_flow_drive(unsigned action,MenuFrame*frame,unsigned a,unsigned b,bool flag,OnlineClanCommand command,std::string text)const{
 FrontendClansNativeIo io(const_cast<MenuState&>(*this),frame);bridge::ClansFlowArgument arg{};arg.a=a;arg.b=b;arg.flag=flag;arg.command=clans_encoded(command);arg.text=clans_bytes(text);auto result=io.invoke([&]{return party_flow_.clans(action,arg,io);});return {result.handled,clans_text(result.text)};
}
void MenuState::clans_bind_account(std::shared_ptr<OnlineAccount>client){FrontendClansNativeIo io(*this,nullptr);io.candidate_account=std::move(client);bridge::ClansFlowArgument arg{};io.invoke([&]{return party_flow_.clans(0,arg,io);});}
void MenuState::clans_bind_voice(std::shared_ptr<OnlineVoice>client){FrontendClansNativeIo io(*this,nullptr);io.candidate_voice=std::move(client);bridge::ClansFlowArgument arg{};io.invoke([&]{return party_flow_.clans(1,arg,io);});}
} // namespace halo2
