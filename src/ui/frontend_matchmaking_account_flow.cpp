#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> account_bytes(const std::string&s){rust::Vec<std::uint8_t>out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string account_text(rust::Slice<const std::uint8_t>s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
std::string account_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
bridge::AccountFlowAccount account_facts(const OnlineAccountSnapshot&f){
  bridge::AccountFlowAccount out{};out.revision=f.revision;out.notification.authenticated=f.authenticated;out.notification.busy=f.busy;
  out.notification.party=f.party.has_value();out.notification.message_empty=f.party_message.empty();
  if(f.party)for(const auto&m:f.party->members)out.notification.members.push_back({account_bytes(m.handle),m.local_players.size()});
  for(const auto&i:f.party_invitations)out.notification.invitations.push_back({account_bytes(i.id)});
  for(const auto&p:f.friends)out.friends.push_back({account_bytes(p.id)});
  out.clan=f.clan.has_value();if(f.clan)for(const auto&p:f.clan->members)out.clan_members.push_back({account_bytes(p.id)});
  out.friend_search=f.friend_search.has_value();out.command_serial=f.party_command_serial;
  out.successful={f.party_command_successful.has_value(),f.party_command_successful.value_or(false)};
  out.party_message=account_bytes(f.party_message);return out;
}
}
// Worker snapshots and primitive frame/device operations. Rust owns revision
// admission, membership transitions, voice intent and identity-based focus.
struct FrontendAccountFlowNativeIo final:bridge::NativeFrontendAccountFlowIo {
  MenuState&m;mutable std::optional<OnlineAccountSnapshot>next;mutable OnlineVoiceSnapshot voice;
  mutable std::exception_ptr exception;
  explicit FrontendAccountFlowNativeIo(MenuState&value):m(value){}
  template<class F>auto capture(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){exception=std::current_exception();throw std::runtime_error("Native account flow IO failed");}}
  template<class F>auto invoke(F&&f)const->std::invoke_result_t<F>{try{return f();}catch(...){if(exception)std::rethrow_exception(exception);throw;}}
  bridge::AccountFlowFacts account_flow_facts()const override{return capture([&]{
    bridge::AccountFlowFacts out{};out.account_client=bool(m.online_account_);out.login_client=bool(m.discord_login_);out.voice_client=bool(m.online_voice_);out.queue_client=bool(m.online_matchmaking_);
    out.signed_in=account_bytes(m.signed_in_);for(const auto&[key,value]:m.profile_service_tags_)out.tags.push_back({account_bytes(key),account_bytes(value)});
    out.account=account_facts(m.account_);out.voice_revision=m.voice_.revision;out.launch=m.launch.has_value();out.mode=m.mode_;out.sounds=m.pending_ui_sounds_.size();
    for(const auto&f:m.stack_){bridge::AccountFlowFrame frame{};frame.screen=f.screen;frame.context=f.context;frame.selected=f.selected;frame.pcr=bool(f.vista_pcr);
      for(const auto&e:f.entries)frame.entries.push_back({account_bytes(e.label),e.destination,e.value});out.frames.push_back(std::move(frame));}return out;
  });}
  bridge::LobbyViewBytes account_flow_identity()const override{return capture([&]{bridge::LobbyViewBytes out{};const auto identity=m.discord_login_->snapshot().identity;
    out.present=identity.has_value();if(identity)out.value=account_bytes(identity->username);return out;});}
  void account_flow_profile(rust::Slice<const std::uint8_t>name,rust::Slice<const std::uint8_t>tag)const override{capture([&]{m.online_account_->profile(m.appearance_for_profile(account_text(name)),account_text(tag));});}
  void account_flow_connect(rust::Slice<const std::uint8_t>name)const override{capture([&]{m.online_account_->connect(account_text(name));});}
  void account_flow_sign_out()const override{capture([&]{m.online_account_->sign_out();});}
  bridge::AccountFlowAccount account_flow_snapshot()const override{return capture([&]{next=m.online_account_->snapshot();return account_facts(*next);});}
  std::uint64_t account_flow_stage_voice(bool read)const override{return capture([&]{voice=read?m.online_voice_->snapshot():OnlineVoiceSnapshot{};return voice.revision;});}
  bool account_flow_native_friends()const override{return capture([&]{return m.native_friends();});}
  void account_flow_effect(const bridge::AccountFlowEffect&e)const override{capture([&]{
    switch(e.kind){
    case 0:m.pending_ui_sounds_.push_back(static_cast<FrontendUiSound>(e.a));break;
    case 1:m.account_=std::move(*next);next.reset();break;
    case 2:m.voice_=std::move(voice);break;
    case 3:m.online_matchmaking_->cancel_queue();break;
    case 4:m.matchmaking_search_.stop();break;
    case 5:m.return_online();break;
    case 6:m.open(e.a,e.b);break;
    case 7:m.online_voice_->leave();break;
    case 8:m.online_voice_.reset();break;
    case 9:m.online_voice_=std::make_shared<OnlineVoice>(m.online_account_,true);break;
    case 10:m.online_voice_->join();break;
    case 11:m.voice_=m.online_voice_->snapshot();break;
    case 12:m.stack_.erase(m.stack_.begin()+e.index,m.stack_.end());break;
    case 13:m.stack_.at(e.index).entries.clear();break;
    case 14:{auto&f=m.stack_.at(e.index);switch(e.a){case 0:m.populate_online_friends(f);break;case 1:m.populate_online_party(f);break;case 2:m.populate_online_clans(f);break;default:throw std::logic_error("Invalid native account frame kind");}break;}
    case 15:m.stack_.at(e.index).selected=e.a;break;
    case 16:++m.revision;break;
    case 17:m.unavailable(account_text(e.text));break;
    default:throw std::logic_error("Invalid native account flow effect");
    }
  });}
};
void MenuState::account_flow_drive(bool continuation){FrontendAccountFlowNativeIo io(*this);io.invoke([&]{party_flow_.observe_account(io,continuation);});}
}
