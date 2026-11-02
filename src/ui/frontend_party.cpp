#include "halo2/frontend.hpp"
#include "halo2/frontend_matchmaking_party.hpp"
#include <algorithm>

namespace halo2 {

std::vector<FrontendUiSound> MenuState::take_ui_sounds() {
  auto sounds=std::move(pending_ui_sounds_);pending_ui_sounds_.clear();return sounds;
}
std::optional<FrontendInviteNotice> MenuState::invite_notice() const {
  return frontend_party_notice_rust(account_,signed_in_,party_flow_.dismissed(),launch.has_value(),unsigned(matchmaking_.status));
}
bool MenuState::act_invite_notice(bool accept) {
  return party_flow_drive(FrontendPartyFlowAction::Notice,accept?1:0);
}
std::vector<OnlinePartyMember> MenuState::online_party_members() const {
  FrontendPartyIo io;
  io.primary_appearance=[&]{return appearance_for_profile(signed_in_);};
  io.local_players=[&]{
    std::vector<FrontendPartySeat> out;
    for(const auto &p:local_players())out.push_back({p.controller,p.name,p.appearance});
    return out;
  };
  io.authorization=[&](std::uint32_t controller)->std::optional<bool> {
    const auto &login=local_discord_logins_[controller];
    if(!login)return {};
    return login->authorization().has_value();
  };
  return frontend_party_members_rust(account_.party,io);
}
bool MenuState::online_party_screen(const MenuFrame &f) const {
  return frontend_party_flow_screen(mode_,f.screen,f.context);
}
bool MenuState::party_invite_picker() const {return frontend_party_flow_picker(frame().screen,frame().context);}
bool MenuState::switch_invite_tab(int direction) {
  return party_flow_drive(FrontendPartyFlowAction::SwitchTab,direction);
}
void MenuState::populate_online_party(MenuFrame &f) const {party_view_populate(f);}
std::string MenuState::online_party_text(const MenuFrame &f,unsigned i) const {return party_view_text(f,i);}
void MenuState::update_online_privacy() {
  party_flow_drive(FrontendPartyFlowAction::ObservePrivacy);
}
std::string MenuState::online_party_column(const MenuFrame &f,unsigned row,unsigned column) const {
  // Preserve the source caller's profile/local-seat reads before cell guards.
  const auto members=online_party_members();
  return frontend_party_column_rust(members,voice_,party_flow_.voice_party(),f.screen,row,column);
}



bool MenuState::accept_online_party() {
  return party_flow_drive(FrontendPartyFlowAction::Accept);
}
} // namespace halo2
