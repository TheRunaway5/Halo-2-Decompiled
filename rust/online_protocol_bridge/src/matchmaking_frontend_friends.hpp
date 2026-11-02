#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct FriendsFlowFacts;struct FriendsFlowSocial;struct FriendsFlowEffect;struct PartyFlowCommand;
class NativeFrontendFriendsIo {
public:
 virtual ~NativeFrontendFriendsIo()=default;
 virtual FriendsFlowFacts friends_flow_facts()const=0;
 virtual bool friends_flow_steam()const=0;
 virtual bool friends_flow_identity()const=0;
 virtual void friends_flow_pane()const=0;
 virtual rust::Vec<std::uint8_t> friends_flow_localized(std::uint32_t)const=0;
 virtual rust::Vec<std::uint8_t> friends_flow_profile_name()const=0;
 virtual FriendsFlowSocial friends_flow_social_snapshot()const=0;
 virtual bool friends_flow_friend_command(rust::Slice<const std::uint8_t>,rust::Slice<const std::uint8_t>)const=0;
 virtual bool friends_flow_party_command(const PartyFlowCommand&)const=0;
 virtual void friends_flow_wrapped_party_command(const PartyFlowCommand&)const=0;
 virtual std::uint64_t friends_flow_serial()const=0;
 virtual void friends_flow_update_account()const=0;
 virtual bool friends_flow_social_command(std::uint32_t,rust::Slice<const std::uint8_t>,rust::Slice<const std::uint8_t>)const=0;
 virtual void friends_flow_effect(const FriendsFlowEffect&)const=0;
};
}
