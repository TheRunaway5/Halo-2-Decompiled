#pragma once
#include "halo2/frontend.hpp"
#include <functional>
namespace halo2 {
struct FrontendPartySeat {
  std::uint32_t controller{};
  std::string name;
  FrontendAppearance appearance;
};
struct FrontendPartyIo {
  std::function<FrontendAppearance()> primary_appearance;
  std::function<std::vector<FrontendPartySeat>()> local_players;
  std::function<std::optional<bool>(std::uint32_t)> authorization;
};
std::vector<OnlinePartyMember> frontend_party_members_rust(
    const std::optional<OnlineParty> &,const FrontendPartyIo &);
std::string frontend_party_column_rust(const std::vector<OnlinePartyMember> &,
    const OnlineVoiceSnapshot &,bool party_voice,unsigned screen,unsigned row,
    unsigned column);
std::optional<FrontendInviteNotice> frontend_party_notice_rust(
    const OnlineAccountSnapshot &,const std::string &signed_in,
    const std::vector<std::string> &dismissed,bool launch,unsigned status);
} // namespace halo2
