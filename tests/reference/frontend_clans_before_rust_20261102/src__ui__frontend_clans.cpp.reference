#include "halo2/frontend.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr unsigned clan_context = 41;
constexpr const char *roles[]{"Peon","Member","Staff","Overlord"};
const OnlineClanMember *target_member(const OnlineAccountSnapshot &s, const std::string &id) {
  if (s.clan) for (const auto &m : s.clan->members) if (m.id == id) return &m;
  return nullptr;
}
}
void MenuState::set_online_account(std::shared_ptr<OnlineAccount> client) {
  if(online_account_==client) {
    // The account survives renderer teardown; its SDL voice adapter does not.
    if(client && !online_voice_) {
      online_voice_=std::make_shared<OnlineVoice>(client);
      voice_={};voice_.revision=~std::uint64_t{};
    }
    return;
  }
  for(auto &account:local_profile_accounts_){if(account)account->sign_out();}
  local_profile_accounts_={};
  if (online_voice_) online_voice_->leave();
  if (online_account_) online_account_->sign_out();
  online_account_ = std::move(client); account_ = {}; party_flow_.account_name({});
  party_flow_.friend_join_serial(0);
  pending_ui_sounds_.clear();
  party_flow_.clear_dismissed();
  if(discord_social_)discord_social_->bind_account(online_account_);
  online_voice_ = online_account_ ? std::make_shared<OnlineVoice>(online_account_) : nullptr;
  party_flow_.voice_party(false);
  voice_ = {};
}
void MenuState::set_online_voice(std::shared_ptr<OnlineVoice> client) {
  if (online_voice_) online_voice_->leave();
  online_voice_=std::move(client);party_flow_.voice_party(false);voice_={};
}
void MenuState::voice_push_to_talk(bool held) {
  if (online_voice_) online_voice_->push_to_talk(held && frame().screen!=167);
}
bool MenuState::online_clan_screen(const MenuFrame &f) const {
  return (f.screen == 26 && f.context == 1) || f.screen == 217 || f.screen == 236 ||
      ((f.screen == 31 || f.screen == 211 || f.screen == 33) && f.context == clan_context);
}
void MenuState::update_online_account(){account_flow_drive(false);}
void MenuState::populate_online_clans(MenuFrame &f) const {
  const auto *m = target_member(account_,clan_target_);
  if (f.screen == 26) {
    f.title = "ONLINE"; f.description.clear();
    if (account_.clan) {
      for (const auto &member : account_.clan->members)
        f.entries.push_back({member.name + (member.self ? " (you)" : ""),31,int(f.entries.size())});
      for (unsigned i=0;i<account_.clan->outgoing.size();++i)
        f.entries.push_back({"Invited: " + account_.clan->outgoing[i].name,31,int(2000+i)});
    } else {
      f.entries.push_back({"Create Clan",167,4});
      for (unsigned i=0;i<account_.invitations.size();++i)
        f.entries.push_back({"Invitation: " + account_.invitations[i].name,31,int(1000+i)});
    }
  } else if (f.screen == 217) {
    f.title = "CLAN OPTIONS"; f.description.clear();
    if (!account_.clan) { f.entries.push_back({"Create Clan",167,4}); return; }
    if (account_.clan->role >= 1) f.entries.push_back({"Send Clan Invite",167,7});
    if(account_.clan->role>=1 && account_.party && account_.party->leader)
      f.entries.push_back({"Invite Clan To Party",-33,1});
    f.entries.push_back({"Read Clan Messages",211,int(clan_context)});
    if (account_.clan->role >= 1) f.entries.push_back({"Message Clan",167,8});
    if (online_voice_) {
      f.entries.push_back({!party_flow_.voice_party() && (voice_.joined || voice_.connecting) ? "Leave Clan Voice" : "Join Clan Voice",-32,0});
      if (!party_flow_.voice_party() && voice_.joined) f.entries.push_back({voice_.microphone_muted ? "Unmute Microphone" : "Mute Microphone",-32,1});
    }
    f.entries.push_back({"Leave Clan",-30,3});
  } else if (f.screen == 31) {
    f.title = "CLAN"; f.description.clear();
    if (clan_target_kind_ == 1) {
      const auto it = std::find_if(account_.invitations.begin(),account_.invitations.end(),[&](const auto &i){return i.id==clan_target_;});
      if (it != account_.invitations.end()) { f.title = it->name; f.entries={{"Accept Clan Invite",-30,0},{"Decline Clan Invite",-30,1},{"Block Future Invites",-30,2}}; }
    } else if (clan_target_kind_ == 2) {
      if (account_.clan) {
        const auto it = std::find_if(account_.clan->outgoing.begin(),account_.clan->outgoing.end(),[&](const auto &i){return i.id==clan_target_;});
        if (it != account_.clan->outgoing.end()) { f.title = it->name; f.entries={{"Cancel Clan Invite",-30,4}}; }
      }
    } else if (m && account_.clan) {
      f.title=m->name;
      if (m->self) f.entries.push_back({"Leave Clan",-30,3});
      else if (account_.clan->role >= 2 && account_.clan->role > m->role) {
        f.entries.push_back({"Change Clan Role",236,0}); f.entries.push_back({"Remove From Clan",-30,5});
      }
      if (!m->self && !party_flow_.voice_party() && voice_.joined) {
        const bool muted=std::find(voice_.muted.begin(),voice_.muted.end(),m->id)!=voice_.muted.end();
        f.entries.push_back({muted ? "Unmute Voice" : "Mute Voice",-32,2});
      }
    }
    if (f.entries.empty()) f.entries.push_back({"Back",-5,0});
  } else if (f.screen == 236) {
    f.title = "CHANGE CLAN ROLE"; f.description.clear();
    for (unsigned i=0;i<4;++i) f.entries.push_back({roles[i],-30,int(10+i)});
  } else if (f.screen == 211) {
    f.title = "CLAN MESSAGES"; f.description.clear();
    if (account_.clan) for (unsigned i=0;i<account_.clan->messages.size();++i)
      f.entries.push_back({account_.clan->messages[i].name,-1,int(i)});
    if (f.entries.empty()) f.entries.push_back({"No clan messages",-1,0});
  }
}
std::string MenuState::online_clan_column(const MenuFrame &f, unsigned row, unsigned column) const {
  if (f.screen != 26 || f.context != 1 || !account_.clan) return {};
  if (row >= account_.clan->members.size()) return column == 1 ? "Invited" : "";
  if (column != 1) return {};
  const auto &member=account_.clan->members[row];
  std::string label=roles[member.role];
  switch(member.presence) {
  case OnlineClanPresence::Online:label+=" - ONLINE";break;
  case OnlineClanPresence::Offline:label+=" - Offline";break;
  case OnlineClanPresence::Playing:label+=" - Playing Halo 2";break;
  default:break;
  }
  if (!party_flow_.voice_party() && voice_.joined) {
    if(std::find(voice_.muted.begin(),voice_.muted.end(),member.id)!=voice_.muted.end()) label+=" - muted";
    else if((member.self && voice_.transmitting) || std::find(voice_.talking.begin(),voice_.talking.end(),member.id)!=voice_.talking.end()) label+=" - talking";
  }
  return label;
}
std::string MenuState::online_clan_text(const MenuFrame &f, unsigned i) const {
  if (f.screen == 33) return i == 0 ? f.description : "";
  if (f.screen == 26) {
    const auto &screen=content_.screens.at(26);
    const auto &pane=screen.panes.at(1);
    if (i == 0) return "CLAN";
    if (i == 1 || i == 3) return content_.localized(screen,pane.texts.at(i).string);
    if (i == 2) return account_.clan ? account_.clan->name : "No clan";
    if (i == 4) return account_.clan ? std::to_string(account_.clan->members.size()) +
        (account_.clan->members.size() == 1 ? " member" : " members") : "";
    if (i == 6) return !account_.clan ? "" : !party_flow_.voice_party() && voice_.joined
        ? (voice_.microphone_muted ? "MIC MUTED" : "MIC ON") : "VOICE OFF";
    if (i == 8) return !party_flow_.voice_party() && online_voice_ && voice_.revision>1 ? (voice_.joined && voice_.microphone_muted ? "Clan voice - microphone muted." : voice_.message) : account_.message;
    return {};
  }
  if (f.screen == 236) {
    const auto *m = target_member(account_,clan_target_);
    if (i == 0) return m ? m->name + "\nCurrent role: " + roles[m->role] : "Member unavailable";
    if (i == 1) return roles[std::min(f.selected,3u)];
    if (i == 2) return content_.localized(content_.screens.at(236), content_.screens.at(236).panes.at(std::min(f.selected,3u)).texts.at(i).string);
    return {};
  }
  if (f.screen == 211) {
    if (!account_.clan || f.selected >= account_.clan->messages.size()) return i == 1 ? "No clan messages." : "";
    const auto &body=account_.clan->messages[f.selected].body;
    std::vector<std::string> pages;
    for (std::size_t begin=0;begin<body.size();) {
      auto end=std::min(begin+64,body.size());
      while(end<body.size() && (static_cast<unsigned char>(body[end])&0xc0)==0x80) --end;
      pages.push_back(body.substr(begin,end-begin)); begin=end;
    }
    if(i==1) return pages[clan_message_page_%pages.size()];
    if(i==2) return pages.size()>1 ? "Message " + std::to_string(clan_message_page_%pages.size()+1) + "/" + std::to_string(pages.size()) + " - X next" : "Clan message";
    return {};
  }
  return i == 0 ? account_.message : "";
}
void MenuState::clan_command(OnlineClanCommand command) {
  if (command.action=="leave" && !party_flow_.voice_party() && online_voice_) online_voice_->leave();
  if (!online_account_ || !online_account_->command(std::move(command))) {
    unavailable("Wait for the ONLINE account connection or current clan operation to finish."); return;
  }
  while (stack_.size()>1 && !(frame().screen==26 && frame().context==1)) stack_.pop_back();
  if (frame().screen!=26) open(26,1);
  update_online_account(); ++revision;
}
void MenuState::confirm_clan(OnlineClanCommand command, std::string message) {
  clan_confirmation_ = std::move(command); open(33,clan_context);
  auto &f=stack_.back(); f.title="CLAN CONFIRMATION"; f.description=std::move(message);
  f.entries={{"Confirm",-31,0},{"Cancel",-5,0}}; f.selected=1;
}
bool MenuState::accept_online_clans() {
  const auto f=frame();
  if (!online_clan_screen(f)) return false;
  if (f.screen==33) {
    if (f.selected==0 && clan_confirmation_) { auto command=std::move(*clan_confirmation_); clan_confirmation_.reset(); clan_command(std::move(command)); }
    else { clan_confirmation_.reset(); back(); }
    return true;
  }
  if (f.entries.empty() || f.selected>=f.entries.size()) return true;
  auto entry=f.entries[f.selected];
  if(entry.destination==-33){party_command({"invite_clan"});return true;}
  if (f.screen==26) {
    if (entry.destination==167) { keyboard_text.clear(); open(167,4); return true; }
    auto index=unsigned(entry.value);
    if (index>=2000 && account_.clan && index-2000<account_.clan->outgoing.size()) { clan_target_=account_.clan->outgoing[index-2000].id; clan_target_kind_=2; }
    else if (index>=1000 && index-1000<account_.invitations.size()) { clan_target_=account_.invitations[index-1000].id; clan_target_kind_=1; }
    else if (account_.clan && index<account_.clan->members.size()) { clan_target_=account_.clan->members[index].id; clan_target_kind_=0; }
    else return true;
    open(31,clan_context); return true;
  }
  if (entry.destination==167) { keyboard_text.clear(); open(167,unsigned(entry.value)); return true; }
  if (entry.destination==211) { open(211,clan_context); return true; }
  if (entry.destination==236) { open(236); const auto *m=target_member(account_,clan_target_); if(m) select(m->role); return true; }
  if (entry.destination==-5) { back(); return true; }
  if (entry.destination==-32 && online_voice_) {
    if(entry.value==0) {
      if(party_flow_.voice_party()) {
        online_voice_->leave();online_voice_.reset();party_flow_.voice_wanted(false);party_flow_.voice_party(false);
        online_voice_=std::make_shared<OnlineVoice>(online_account_);online_voice_->join();
      } else if(voice_.joined || voice_.connecting)online_voice_->leave();else online_voice_->join();
    }
    else if(entry.value==1) online_voice_->mute_microphone(!voice_.microphone_muted);
    else if(entry.value==2) online_voice_->mute(clan_target_,std::find(voice_.muted.begin(),voice_.muted.end(),clan_target_)==voice_.muted.end());
    back(); update_online_account(); return true;
  }
  if (entry.destination!=-30) return true;
  if (entry.value>=10) {
    const auto rank=unsigned(entry.value-10);
    OnlineClanCommand c{"role",clan_target_,rank,true};
    if(rank==3) confirm_clan(std::move(c),"Promote this member to Overlord? You won't be able to undo this.");
    else clan_command(std::move(c));
  } else {
    constexpr const char *actions[]{"accept","decline","block","leave","cancel","remove"};
    OnlineClanCommand c{actions[entry.value],clan_target_};
    if(entry.value==3) confirm_clan(std::move(c),account_.clan && account_.clan->members.size()==1 ? "Leave and permanently delete your clan?" : "Leave your clan?");
    else if(entry.value==5) confirm_clan(std::move(c),"Remove this member from your clan?");
    else clan_command(std::move(c));
  }
  return true;
}
} // namespace halo2
