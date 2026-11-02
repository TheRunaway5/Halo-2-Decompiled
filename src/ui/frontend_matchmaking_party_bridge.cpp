#include "halo2/frontend_matchmaking_party.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <bit>
#include <exception>
#include <stdexcept>
#include <type_traits>
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
rust::Vec<std::uint8_t> bytes(const std::string &value) {
  rust::Vec<std::uint8_t> out;out.reserve(value.size());for(unsigned char c:value)out.push_back(c);return out;
}
std::string string(const rust::Vec<std::uint8_t> &value) {return {reinterpret_cast<const char*>(value.data()),value.size()};}
std::array<std::uint8_t,9> appearance(const FrontendAppearance &value) {return std::bit_cast<std::array<std::uint8_t,9>>(value);}
std::optional<FrontendAppearance> appearance(bool present,const std::array<std::uint8_t,9> &value) {
  if(!present)return {};
  return std::bit_cast<FrontendAppearance>(value);
}
bridge::PartyMemberData encode(const OnlinePartyMember &p) {
  bridge::PartyMemberData out{};out.handle=bytes(p.handle);out.name=bytes(p.name);out.ready=bytes(p.ready_playlist);
  out.is_self=p.self;out.leader=p.leader;out.online=p.online;out.controller=p.local_controller;out.guest=p.guest;
  out.appearance_present=p.appearance.has_value();if(p.appearance)out.appearance=appearance(*p.appearance);
  out.postgame_present=p.postgame.has_value();out.postgame=p.postgame.value_or(false);
  for(const auto &s:p.local_players) {bridge::PartyLocalData seat{};seat.name=bytes(s.name);seat.guest=s.guest;
    seat.appearance_present=s.appearance.has_value();if(s.appearance)seat.appearance=appearance(*s.appearance);out.locals.push_back(std::move(seat));}
  return out;
}
OnlinePartyMember decode(const bridge::PartyMemberData &p) {
  OnlinePartyMember out;out.handle=string(p.handle);out.name=string(p.name);out.ready_playlist=string(p.ready);
  out.self=p.is_self;out.leader=p.leader;out.online=p.online;out.local_controller=p.controller;out.guest=p.guest;
  out.appearance=appearance(p.appearance_present,p.appearance);if(p.postgame_present)out.postgame=p.postgame;
  for(const auto &s:p.locals)out.local_players.push_back({string(s.name),s.guest,appearance(s.appearance_present,s.appearance)});
  return out;
}
rust::Vec<bridge::PartyMemberData> members(const std::vector<OnlinePartyMember> &p) {
  rust::Vec<bridge::PartyMemberData> out;out.reserve(p.size());for(const auto &m:p)out.push_back(encode(m));return out;
}
bridge::PartyNotificationData notification(const OnlineAccountSnapshot &f) {
  bridge::PartyNotificationData out{};out.authenticated=f.authenticated;out.busy=f.busy;out.party=f.party.has_value();out.message_empty=f.party_message.empty();
  if(f.party)for(const auto &m:f.party->members)out.members.push_back({bytes(m.handle),m.local_players.size()});
  for(const auto &i:f.party_invitations)out.invitations.push_back({bytes(i.id)});
  return out;
}
struct Io final:bridge::NativeFrontendPartyIo {
  const FrontendPartyIo &native;mutable std::exception_ptr error;
  explicit Io(const FrontendPartyIo &value):native(value){}
  template<class F>auto capture(F&&f)const->std::invoke_result_t<F> {
    try{return f();}catch(...){error=std::current_exception();throw std::runtime_error("Native party projection IO failed");}
  }
  template<class F>auto invoke(F&&f)const->std::invoke_result_t<F> {
    try{return f();}catch(...){if(error)std::rethrow_exception(error);throw;}
  }
  bridge::PartyAppearanceData party_primary_appearance()const override{return capture([&]{return bridge::PartyAppearanceData{appearance(native.primary_appearance())};});}
  rust::Vec<bridge::PartySeatData> party_local_players()const override{return capture([&]{rust::Vec<bridge::PartySeatData> out;
    for(const auto &s:native.local_players())out.push_back({s.controller,bytes(s.name),appearance(s.appearance)});
    return out;});}
  bridge::PartyAuthData party_authorization(std::uint32_t controller)const override{return capture([&]{const auto a=native.authorization(controller);return bridge::PartyAuthData{a.has_value(),a.value_or(false)};});}
};
} // namespace
std::vector<OnlinePartyMember> frontend_party_members_rust(const std::optional<OnlineParty> &party,const FrontendPartyIo &native) {
  const auto rows=party?members(party->members):rust::Vec<bridge::PartyMemberData>{};Io io(native);
  return io.invoke([&]{const auto result=bridge::frontend_party_members(party.has_value(),{rows.data(),rows.size()},io);
    std::vector<OnlinePartyMember> out;out.reserve(result.size());for(const auto &m:result)out.push_back(decode(m));return out;});
}
std::string frontend_party_column_rust(const std::vector<OnlinePartyMember> &party,
    const OnlineVoiceSnapshot &voice,bool party_voice,unsigned screen,unsigned row,unsigned column) {
  const auto rows=members(party);bridge::PartyVoiceData v{};v.party=party_voice;v.joined=voice.joined;v.transmitting=voice.transmitting;
  for(const auto &s:voice.muted)v.muted.push_back({bytes(s)});
  for(const auto &s:voice.talking)v.talking.push_back({bytes(s)});
  return string(bridge::frontend_party_column({rows.data(),rows.size()},v,screen,row,column));
}
std::vector<FrontendUiSound> frontend_party_notification_sounds(const OnlineAccountSnapshot &before,const OnlineAccountSnapshot &after) {
  static_assert(unsigned(FrontendUiSound::Advance)==3 && unsigned(FrontendUiSound::Back)==4 &&
    unsigned(FrontendUiSound::ReceiveMessage)==8 && unsigned(FrontendUiSound::Error)==2);
  const auto values=bridge::frontend_party_sounds(notification(before),notification(after));std::vector<FrontendUiSound> out;
  for(auto value:values)out.push_back(static_cast<FrontendUiSound>(value));
  return out;
}
std::optional<FrontendInviteNotice> frontend_party_notice_rust(const OnlineAccountSnapshot &account,
    const std::string &signed_in,const std::vector<std::string> &dismissed,bool launch,unsigned status) {
  bridge::PartyNoticeFactsData f{};f.authenticated=account.authenticated;f.signed_in=bytes(signed_in);
  f.busy=account.busy;f.party=account.party.has_value();f.launch=launch;f.status=status;
  for(const auto &i:account.party_invitations)f.invitations.push_back({bytes(i.id),bytes(i.name)});
  for(const auto &id:dismissed)f.dismissed.push_back({bytes(id)});
  const auto n=bridge::frontend_party_notice(f);if(!n.present)return {};
  return FrontendInviteNotice{string(n.id),string(n.name),n.busy,n.can_accept};
}
} // namespace halo2
