#include "halo2/frontend.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <stdexcept>
namespace halo2 {
namespace bridge = online_protocol_bridge;
bridge::FrontendLobbyFacts MenuState::online_lobby_facts() const {
  bridge::FrontendLobbyFacts f{};
  f.mode=std::int32_t(mode_);
  for(const auto &frame:stack_)f.screens.push_back(frame.screen);
  f.account_client=bool(online_account_);f.authenticated=account_.authenticated;
  f.busy=account_.busy;f.party_message_present=!account_.party_message.empty();
  f.queue_client=bool(online_matchmaking_);f.selected_playlist=matchmaking_search_.selected();
  for(const auto &p:matchmaking_.playlists)
    f.playlists.push_back({p.id,p.revision,p.eligible});
  f.party_present=account_.party.has_value();
  if(account_.party) {
    const auto &p=*account_.party;f.leader=p.leader;f.countdown_supported=p.countdown_supported;
    for(const auto &m:p.members)f.members.push_back({m.handle,m.ready_playlist});
    f.countdown_present=p.countdown.has_value();
    if(p.countdown) {
      const auto &c=*p.countdown;f.serial=c.serial;f.complete=c.complete;
      f.seconds_present=c.seconds.has_value();f.seconds=c.seconds.value_or(0);
    }
  }
  return f;
}
void MenuState::enter_online_matchmaking_lobby() {
  online_lobby_.enter(online_lobby_facts());
  if(frame().screen==14 && mode_==3){stack_.back().entries.clear();populate_online_party(stack_.back());++revision;}
}
void MenuState::cancel_online_lobby_countdown() { online_lobby_.cancel(); }
bool MenuState::update_online_lobby_countdown() {
  const auto effect=online_lobby_.observe(online_lobby_facts());
  if(effect.departure) {
    matchmaking_search_.quick(false);
    if(online_matchmaking_)online_matchmaking_->cancel_queue();
    reset_online_matchmaking_progress();
    while(stack_.size()>1 && frame().screen!=14)stack_.pop_back();
    dialog(0xae,-5,true);++revision;return true;
  }
  if(effect.revision)++revision;
  if(effect.beep && pending_ui_sounds_.size()<16)pending_ui_sounds_.push_back(FrontendUiSound::PregameCountdown);
  if(effect.search) {
    while(stack_.size()>1 && frame().screen!=14)stack_.pop_back();
    open(210);return true;
  }
  return false;
}
std::string MenuState::online_lobby_start_error() const { return online_lobby_.start_error(online_lobby_facts()); }
bool MenuState::online_lobby_input(unsigned event,int destination) {
  const auto effect=online_lobby_.input(online_lobby_facts(),event,destination,
      [&](unsigned command,const std::string &playlist,const std::string &version) {
        if(!online_account_)throw std::logic_error("Native lobby command has no account owner");
        switch(command) {
        case 1:return online_account_->party_command({"countdown_start",{},playlist,version});
        case 2:return online_account_->party_command({"countdown_cancel"});
        case 3:return online_account_->party_command({"countdown_request"});
        case 4:return online_account_->party_command({"leave_lobby"});
        default:throw std::logic_error("Invalid native lobby command");
        }
      });
  if(!effect.handled)return false;
  if(effect.sound && (!effect.bounded_sound || pending_ui_sounds_.size()<16))pending_ui_sounds_.push_back(FrontendUiSound::Error);
  if(!effect.error.empty())unavailable(std::string(effect.error));
  if(effect.refresh_account)update_online_account();
  if(effect.confirm_leave)dialog(0x8b,-11);
  if(effect.friends)open(216);
  if(effect.begin_leave) {
    if(online_matchmaking_)online_matchmaking_->cancel_queue();
    matchmaking_search_.quick(false);update_online_account();update_online_lobby_leave();
  }
  return true;
}
void MenuState::update_online_lobby_leave() {
  if(!online_lobby_.observe_leave(online_lobby_facts()))return;
  if(online_matchmaking_)online_matchmaking_->sign_out();
  online_lobby_.left();
  matchmaking_search_.stop();matchmaking_={};
  matchmaking_search_.left();
  reset_online_matchmaking_progress();launch.reset();
  while(stack_.size()>1 && frame().screen!=6)stack_.pop_back();
  ++revision;
}
}
