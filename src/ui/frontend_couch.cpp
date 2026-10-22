#include "halo2/frontend.hpp"
#include <algorithm>

namespace halo2 {
namespace { constexpr unsigned couch_context=0x434f5543; }
void MenuState::set_local_discord_login_factory(std::function<std::shared_ptr<DiscordLogin>()> factory) {
  local_discord_factory_=std::move(factory);
}
bool MenuState::online_couch_screen(const MenuFrame &f) const {
  return f.screen==31 && f.context==couch_context;
}
bool MenuState::online_couch_mutable() const {
  if(!online_identity())return false;
  switch(matchmaking_.status) {
  case OnlineMatchmakingStatus::Joining:case OnlineMatchmakingStatus::Searching:
  case OnlineMatchmakingStatus::CheckingConnections:case OnlineMatchmakingStatus::AwaitingTransport:
  case OnlineMatchmakingStatus::Loading:return false;
  default:return true;
  }
}
void MenuState::open_online_local_player(unsigned controller) {
  if(!controller || controller>=4 || local_profiles_[controller].empty() || !online_couch_mutable())return;
  online_local_options_=controller;open(31,couch_context);
}
void MenuState::populate_online_couch(MenuFrame &f) const {
  f.description.clear();f.title="LOCAL PLAYER";
  if(!online_local_options_){f.entries={{"Back",-5,0}};return;}
  const auto controller=*online_local_options_;
  f.title="LOCAL PLAYER "+std::to_string(controller+1);
  const auto login=local_discord_logins_[controller];
  const auto status=login?login->snapshot().status:DiscordLoginStatus::SignedOut;
  const bool busy=status==DiscordLoginStatus::Waiting || status==DiscordLoginStatus::Verifying;
  const std::string provider=discord_login_ && discord_login_->uses_steam()?"Steam":"Discord";
  f.entries={{busy?"Cancel "+provider+" Sign-In":"Play as Guest",-80,0}};
  if(!busy)f.entries.push_back({login && login->authorization()?"Continue with "+provider:"Sign in with "+provider,-80,1});
  f.entries.push_back({"Remove Local Player",-80,2});
}
std::string MenuState::online_couch_text(const MenuFrame &,unsigned i) const {
  if(!online_local_options_ || i>1)return {};
  const auto controller=*online_local_options_;
  if(i==0)return local_profiles_[controller];
  const auto login=local_discord_logins_[controller];
  if(login) {
    const auto status=login->snapshot();
    if(status.status==DiscordLoginStatus::Waiting || status.status==DiscordLoginStatus::Verifying)
      return login->uses_steam()?"Finish Steam in your browser.":"Finish Discord in your browser.";
    if(login->authorization())return login->uses_steam()?"Steam linked; stats saved.":"Discord linked; stats saved.";
    if(status.status==DiscordLoginStatus::Failed)return "Sign-in failed. Try again.";
  }
  return "Casual only; stats not saved.";
}
bool MenuState::accept_online_couch() {
  if(frame().screen==10 && joining_online_ && joining_controller_ &&
      !frame().entries.empty() && frame().entries[frame().selected].destination==-80) {
    const auto controller=*joining_controller_;
    local_profiles_[controller]="Guest "+std::to_string(controller+1);
    joining_controller_.reset();joining_online_=false;profile_=host_profile_;
    stack_.pop_back();++couch_revision_;++revision;open_online_local_player(controller);return true;
  }
  if(!online_couch_screen(frame()))return false;
  if(!online_local_options_ || frame().entries.empty())return true;
  const auto controller=*online_local_options_;
  auto &login=local_discord_logins_[controller];
  const auto action=frame().entries[frame().selected].value;
  if(action==0) {
    if(local_profile_accounts_[controller])local_profile_accounts_[controller]->sign_out();
    local_profile_accounts_[controller].reset();
    if(login)login->cancel();
    login.reset();local_discord_subjects_[controller].clear();
    ++couch_revision_;back();return true;
  }
  if(action==2) {back();leave_local_controller(controller);return true;}
  if(login && login->authorization()){back();return true;}
  if(!local_discord_factory_) {unavailable("Account sign-in is unavailable for this local player.");return true;}
  const auto pending=[](const auto &login) {
    if(!login)return false;
    const auto s=login->snapshot().status;
    return s==DiscordLoginStatus::Waiting || s==DiscordLoginStatus::Verifying;
  };
  if(pending(discord_login_) || std::any_of(local_discord_logins_.begin(),local_discord_logins_.end(),pending)) {
    unavailable("Finish the current account sign-in first.");return true;
  }
  if(!login)login=local_discord_factory_();
  login->start();++revision;return true;
}
void MenuState::update_online_couch() {
  if(online_identity())for(auto &f:stack_)if(f.screen==186) {
    const auto description=build(186,0).description;
    if(f.description!=description){f.description=description;++revision;}
  }
  for(unsigned i=1;i<4;++i) {
    const auto identity=local_discord_logins_[i] && local_discord_logins_[i]->authorization()
        ?local_discord_logins_[i]->snapshot().identity:std::nullopt;
    const auto subject=identity?identity->id:std::string{};
    if(subject!=local_discord_subjects_[i]){local_discord_subjects_[i]=subject;++couch_revision_;++revision;}
    const auto primary=discord_login_?discord_login_->snapshot().identity:std::nullopt;
    const bool duplicate=identity && ((primary && primary->id==identity->id) ||
        std::find(local_discord_subjects_.begin()+1,local_discord_subjects_.begin()+i,subject)!=local_discord_subjects_.begin()+i);
    auto &account=local_profile_accounts_[i];
    if(!online_identity() || !online_account_ || !identity || duplicate || local_profiles_[i].empty()) {
      if(account)account->sign_out();
      account.reset();
    } else {
      if(!account)account=online_account_->profile_worker(local_discord_logins_[i]);
      const auto tag=profile_service_tags_.find(local_profiles_[i]);
      account->profile(appearance_for_profile(local_profiles_[i]),tag==profile_service_tags_.end()?std::string{}:tag->second);
      account->connect(local_profiles_[i]);
    }
  }
  if(online_couch_screen(frame())) {
    auto updated=build(31,couch_context);
    if(updated.entries.size()!=frame().entries.size() ||
       !std::equal(updated.entries.begin(),updated.entries.end(),frame().entries.begin(),[](const auto &a,const auto &b) {
         return a.label==b.label && a.destination==b.destination && a.value==b.value;
       })) {
      updated.instance=frame().instance;updated.selected=std::min(frame().selected,unsigned(updated.entries.size()-1));
      stack_.back()=std::move(updated);++revision;
    }
  }
}
void MenuState::clear_online_couch() {
  for(auto &account:local_profile_accounts_){if(account)account->sign_out();}
  local_profile_accounts_={};
  for(auto &login:local_discord_logins_){if(login)login->cancel();login.reset();}
  local_discord_subjects_={};online_local_options_.reset();joining_online_=false;++couch_revision_;
}
std::vector<OnlineLocalPlayer> MenuState::online_local_players() const {
  std::vector<OnlineLocalPlayer> result;
  for(const auto &player:local_players())if(player.controller)
    result.push_back({player.name,local_discord_logins_[player.controller]});
  return result;
}
} // namespace halo2
