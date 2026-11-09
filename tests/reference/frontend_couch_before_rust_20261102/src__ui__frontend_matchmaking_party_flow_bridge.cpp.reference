#include "halo2/frontend_matchmaking_party_flow.hpp"
#include "halo2/online_account.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> bytes(const std::string &s){rust::Vec<std::uint8_t> out;out.reserve(s.size());for(unsigned char c:s)out.push_back(c);return out;}
std::string flow_text(const rust::Vec<std::uint8_t>&s){return {reinterpret_cast<const char*>(s.data()),s.size()};}
bridge::PartyFlowCommand encoded(const OnlinePartyCommand &c) {
  bridge::PartyFlowCommand out{};out.action=bytes(c.action);out.value=bytes(c.value);out.playlist=bytes(c.playlist);out.revision=bytes(c.revision);
  out.version=c.version;out.privacy_present=c.privacy.has_value();out.privacy=c.privacy.value_or(0);
  out.privacy_revision=c.privacy_revision;out.privacy_context=bytes(c.privacy_context);out.join_context=bytes(c.join_context);return out;
}
}
struct FrontendMatchmakingPartyFlow::Impl {
  rust::Box<bridge::FrontendPartyFlow> state;
  Impl():state(bridge::new_frontend_party_flow()){}
  explicit Impl(const bridge::FrontendPartyFlow&other):state(bridge::clone_frontend_party_flow(other)){}
};
FrontendMatchmakingPartyFlow::FrontendMatchmakingPartyFlow():impl_(std::make_unique<Impl>()){}
FrontendMatchmakingPartyFlow::~FrontendMatchmakingPartyFlow()=default;
FrontendMatchmakingPartyFlow::FrontendMatchmakingPartyFlow(const FrontendMatchmakingPartyFlow&o):impl_(std::make_unique<Impl>(*o.impl_->state)){}
FrontendMatchmakingPartyFlow&FrontendMatchmakingPartyFlow::operator=(const FrontendMatchmakingPartyFlow&o){if(this!=&o){auto value=std::make_unique<Impl>(*o.impl_->state);impl_.swap(value);}return *this;}
FrontendMatchmakingPartyFlow::FrontendMatchmakingPartyFlow(FrontendMatchmakingPartyFlow&&)noexcept=default;
FrontendMatchmakingPartyFlow&FrontendMatchmakingPartyFlow::operator=(FrontendMatchmakingPartyFlow&&)noexcept=default;
bridge::PartyFlowState FrontendMatchmakingPartyFlow::state()const{return bridge::frontend_party_flow_state(*impl_->state);}
std::uint64_t FrontendMatchmakingPartyFlow::friend_join_serial()const{return state().friend_join_serial;}
void FrontendMatchmakingPartyFlow::friend_join_serial(std::uint64_t value)const{bridge::frontend_party_flow_join_serial(*impl_->state,value);}
std::string FrontendMatchmakingPartyFlow::account_name()const{return flow_text(state().account_name);}
void FrontendMatchmakingPartyFlow::account_name(const std::string&value)const{const auto data=bytes(value);bridge::frontend_party_flow_account_name(*impl_->state,{data.data(),data.size()});}
bool FrontendMatchmakingPartyFlow::voice_party()const{return state().voice_party;}
void FrontendMatchmakingPartyFlow::voice_party(bool value)const{bridge::frontend_party_flow_voice_party(*impl_->state,value);}
bool FrontendMatchmakingPartyFlow::voice_wanted()const{return state().voice_wanted;}
void FrontendMatchmakingPartyFlow::voice_wanted(bool value)const{bridge::frontend_party_flow_voice_wanted(*impl_->state,value);}
bool FrontendMatchmakingPartyFlow::match_voice(bool selected,bool joined)const{return bridge::frontend_party_flow_match_voice(*impl_->state,selected,joined);}
void FrontendMatchmakingPartyFlow::observe_account(const bridge::NativeFrontendAccountFlowIo&io,bool continuation)const{bridge::frontend_account_flow(*impl_->state,continuation,io);}
bridge::FriendsFlowResult FrontendMatchmakingPartyFlow::friends(unsigned action,unsigned a,unsigned b,const bridge::NativeFrontendFriendsIo&io)const{return bridge::frontend_friends_flow(*impl_->state,action,a,b,io);}
bridge::ClansFlowResult FrontendMatchmakingPartyFlow::clans(unsigned action,const bridge::ClansFlowArgument&arg,const bridge::NativeFrontendClansIo&io)const{return bridge::frontend_clans_flow(*impl_->state,action,arg,io);}
unsigned FrontendMatchmakingPartyFlow::tab()const{return state().tab;}
std::string FrontendMatchmakingPartyFlow::target()const{return flow_text(state().target);}
unsigned FrontendMatchmakingPartyFlow::kind()const{return state().kind;}
std::vector<std::string> FrontendMatchmakingPartyFlow::dismissed()const{std::vector<std::string> out;for(const auto &id:state().dismissed)out.push_back(flow_text(id.value));return out;}
void FrontendMatchmakingPartyFlow::clear_dismissed()const{bridge::frontend_party_flow_clear_dismissed(*impl_->state);}
void FrontendMatchmakingPartyFlow::prune(const std::vector<std::string>&current)const{rust::Vec<bridge::PartyBytesData> ids;for(const auto &id:current)ids.push_back({bytes(id)});bridge::frontend_party_flow_prune(*impl_->state,{ids.data(),ids.size()});}
bool FrontendMatchmakingPartyFlow::run(FrontendPartyFlowAction action,const bridge::NativeFrontendPartyFlowIo&io,int value)const{return bridge::frontend_party_flow_run(*impl_->state,unsigned(action),value,io);}
void FrontendMatchmakingPartyFlow::command(const OnlinePartyCommand&command,const bridge::NativeFrontendPartyFlowIo&io)const{bridge::frontend_party_flow_command(*impl_->state,encoded(command),io);}
void FrontendMatchmakingPartyFlow::populate(const bridge::NativeFrontendPartyViewIo&io)const{bridge::frontend_party_view_populate(*impl_->state,io);}
std::string FrontendMatchmakingPartyFlow::text(unsigned index,const bridge::NativeFrontendPartyViewIo&io)const{return flow_text(bridge::frontend_party_view_text(index,io));}
bool frontend_party_flow_screen(int mode,unsigned screen,unsigned context){return bridge::frontend_party_flow_screen(mode,screen,context);}
bool frontend_party_flow_picker(unsigned screen,unsigned context){return bridge::frontend_party_flow_picker(screen,context);}
bool frontend_friends_flow_screen(unsigned screen,unsigned context){return bridge::frontend_friends_screen(screen,context);}
bool frontend_clans_flow_screen(unsigned screen,unsigned context){return bridge::frontend_clans_screen(screen,context);}
}
