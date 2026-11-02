#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> friends_bytes(const std::string&s){rust::Vec<std::uint8_t>out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string friends_text(rust::Slice<const std::uint8_t>s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
std::string friends_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
bridge::FriendsFlowFriend friends_record(const OnlineFriend&p){return {friends_bytes(p.id),friends_bytes(p.name),friends_bytes(p.tag),friends_bytes(p.state),friends_bytes(p.presence),{p.party_joinable.has_value(),p.party_joinable.value_or(false)},friends_bytes(p.party_context)};}
bridge::FriendsFlowSocial friends_snapshot(const DiscordSocialSnapshot&s){
 bridge::FriendsFlowSocial out{};out.available=s.available;out.enabled=s.enabled;out.connecting=s.connecting;out.ready=s.ready;out.busy=s.busy;out.message=friends_bytes(s.message);out.revision=s.revision;
 for(const auto&p:s.friends)out.friends.push_back({friends_bytes(p.handle),friends_bytes(p.name),static_cast<std::int32_t>(p.kind),static_cast<std::int32_t>(p.presence)});
 for(const auto&p:s.invitations)out.invitations.push_back({friends_bytes(p.handle),friends_bytes(p.name),friends_bytes(p.status)});return out;
}
bridge::AccountFlowFrame friends_frame(const MenuFrame&f){bridge::AccountFlowFrame out{};out.screen=f.screen;out.context=f.context;out.selected=f.selected;out.pcr=bool(f.vista_pcr);for(const auto&e:f.entries)out.entries.push_back({friends_bytes(e.label),e.destination,e.value});return out;}
OnlinePartyCommand friends_command(const bridge::PartyFlowCommand&c){OnlinePartyCommand out;out.action=friends_text(c.action);out.value=friends_text(c.value);out.playlist=friends_text(c.playlist);out.revision=friends_text(c.revision);out.version=c.version;if(c.privacy_present)out.privacy=c.privacy;out.privacy_revision=c.privacy_revision;out.privacy_context=friends_text(c.privacy_context);out.join_context=friends_text(c.join_context);return out;}
MenuEntry friends_entry(const bridge::AccountFlowEntry&e){return {friends_text(e.label),e.destination,e.value};}
}
// Cached facts, actual provider observations, and primitive frame/worker IO.
// All target selection, join gates, commands, focus and text choices are Rust.
struct FrontendFriendsNativeIo final:bridge::NativeFrontendFriendsIo {
 MenuState&m;MenuFrame*view;
 using Screen=std::remove_cvref_t<decltype(std::declval<MenuState&>().content_.screens.at(26))>;
 using Pane=std::remove_cvref_t<decltype(std::declval<Screen>().panes.at(0))>;
 mutable const Screen*screen{};mutable const Pane*pane{};
 mutable std::optional<DiscordSocialSnapshot>next;mutable std::exception_ptr exception;
 FrontendFriendsNativeIo(MenuState&state,MenuFrame*frame):m(state),view(frame){}
 template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native friends flow IO failed");}}
 template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
 MenuFrame&target()const{if(!view)throw std::logic_error("Native friends frame owner absent");return *view;}
 bridge::FriendsFlowFacts friends_flow_facts()const override{return capture([&]{
  bridge::FriendsFlowFacts out{};out.account_client=bool(m.online_account_);out.social_client=bool(m.discord_social_);out.login_client=bool(m.discord_login_);
  const auto&a=m.account_;out.account.authenticated=a.authenticated;out.account.busy=a.busy;out.account.party=a.party.has_value();out.account.leader=a.party&&a.party->leader;out.account.available=a.friends_available;
  out.account.tag=friends_bytes(a.friend_tag);out.account.message=friends_bytes(a.friends_message);for(const auto&p:a.friends)out.account.friends.push_back(friends_record(p));out.account.search_present=a.friend_search.has_value();if(a.friend_search)out.account.search=friends_record(*a.friend_search);
  out.social=friends_snapshot(m.friends_);if(view)out.frame=friends_frame(*view);else if(!m.stack_.empty())out.frame=friends_frame(m.frame());for(const auto&f:m.stack_)out.frames.push_back(friends_frame(f));out.launch=m.launch.has_value();out.status=static_cast<std::uint32_t>(m.matchmaking_.status);return out;
 });}
 bool friends_flow_steam()const override{return capture([&]{return m.discord_login_->uses_steam();});}
 bool friends_flow_identity()const override{return capture([&]{return m.online_identity().has_value();});}
 void friends_flow_pane()const override{capture([&]{screen=&m.content_.screens.at(26);pane=&screen->panes.at(0);});}
 rust::Vec<std::uint8_t> friends_flow_localized(std::uint32_t index)const override{return capture([&]{if(!screen||!pane)throw std::logic_error("Native friends localization owner absent");return friends_bytes(m.content_.localized(*screen,pane->texts.at(index).string));});}
 rust::Vec<std::uint8_t> friends_flow_profile_name()const override{return capture([&]{return friends_bytes(m.profile_name());});}
 bridge::FriendsFlowSocial friends_flow_social_snapshot()const override{return capture([&]{next=m.discord_social_->snapshot();return friends_snapshot(*next);});}
 bool friends_flow_friend_command(rust::Slice<const std::uint8_t>action,rust::Slice<const std::uint8_t>target)const override{return capture([&]{return m.online_account_->friend_command(friends_text(action),friends_text(target));});}
 bool friends_flow_party_command(const bridge::PartyFlowCommand&c)const override{return capture([&]{return m.online_account_->party_command(friends_command(c));});}
 void friends_flow_wrapped_party_command(const bridge::PartyFlowCommand&c)const override{capture([&]{m.party_command(friends_command(c));});}
 std::uint64_t friends_flow_serial()const override{return capture([&]{return m.online_account_->snapshot().party_command_serial;});}
 void friends_flow_update_account()const override{capture([&]{m.update_online_account();});}
 bool friends_flow_social_command(std::uint32_t kind,rust::Slice<const std::uint8_t>target,rust::Slice<const std::uint8_t>action)const override{return capture([&]{
  const auto handle=friends_text(target);switch(kind){case 0:return m.discord_social_->command(handle,friends_text(action));case 1:return m.discord_social_->invite_to_party(handle);case 2:return m.discord_social_->accept_activity(handle);case 3:return m.discord_social_->dismiss_activity(handle);default:throw std::logic_error("Invalid native friends provider operation");}
 });}
 void friends_flow_effect(const bridge::FriendsFlowEffect&e)const override{capture([&]{switch(e.kind){
  case 0:target().title=friends_text(e.text);break;
  case 1:target().description=friends_text(e.text);break;
  case 2:target().entries.push_back(friends_entry(e.entry));break;
  case 3:{std::vector<MenuEntry>rows;rows.reserve(e.entries.size());for(const auto&p:e.entries)rows.push_back(friends_entry(p));target().entries=std::move(rows);break;}
  case 4:m.keyboard_text.clear();break;
  case 5:m.open(e.a,e.b);break;
  case 6:m.back();break;
  case 7:m.stack_.pop_back();break;
  case 8:++m.revision;break;
  case 9:m.unavailable(friends_text(e.text));break;
  case 10:next=DiscordSocialSnapshot{};break;
  case 11:m.friends_=std::move(*next);next.reset();break;
  case 12:m.discord_social_->bind_account(m.online_account_);break;
  case 13:m.discord_social_->enable();break;
  case 14:m.discord_social_->disconnect();break;
  case 15:m.discord_social_->pump();break;
  case 16:m.discord_social_->presence(friends_text(e.text),false);break;
  case 17:m.stack_.at(e.index).entries.clear();break;
  case 18:m.populate_online_friends(m.stack_.at(e.index));break;
  case 19:m.stack_.at(e.index).selected=e.a;break;
  default:throw std::logic_error("Invalid native friends effect");
 }});}
};
FrontendFriendsResult MenuState::friends_flow_drive(unsigned action,MenuFrame*frame,unsigned a,unsigned b)const{
 FrontendFriendsNativeIo io(const_cast<MenuState&>(*this),frame);auto result=io.invoke([&]{return party_flow_.friends(action,a,b,io);});return {result.handled,friends_text(result.text)};
}
} // namespace halo2
