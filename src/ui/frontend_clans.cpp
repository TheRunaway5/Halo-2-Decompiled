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
  for(auto &account:local_profile_accounts_){if(account)account->sign_out();}
  local_profile_accounts_={};
  if (online_voice_) online_voice_->leave();
  if (online_account_) online_account_->sign_out();
  online_account_ = std::move(client); account_ = {}; account_name_.clear();
  if(discord_social_)discord_social_->bind_account(online_account_);
  online_voice_ = online_account_ ? std::make_shared<OnlineVoice>(online_account_) : nullptr;
  voice_party_=false;
  voice_ = {};
}
void MenuState::set_online_voice(std::shared_ptr<OnlineVoice> client) {
  if (online_voice_) online_voice_->leave();
  online_voice_=std::move(client);voice_party_=false;voice_={};
}
void MenuState::voice_push_to_talk(bool held) {
  if (online_voice_) online_voice_->push_to_talk(held && frame().screen!=167);
}
bool MenuState::online_clan_screen(const MenuFrame &f) const {
  return (f.screen == 26 && f.context == 1) || f.screen == 217 || f.screen == 236 ||
      ((f.screen == 31 || f.screen == 211 || f.screen == 33) && f.context == clan_context);
}
void MenuState::update_online_account() {
  if (!online_account_) return;
  const auto identity = discord_login_ ? discord_login_->snapshot().identity : std::nullopt;
  if (identity) {
    auto name = signed_in_;
    if(name.empty())name=identity->username;
    else {
      const auto tag=profile_service_tags_.find(name);
      online_account_->profile(appearance_for_profile(name),tag==profile_service_tags_.end()?std::string{}:tag->second);
    }
    online_account_->connect(name); account_name_ = name;
  } else { online_account_->sign_out(); account_name_.clear(); }
  auto next = online_account_->snapshot();
  auto voice = online_voice_ ? online_voice_->snapshot() : OnlineVoiceSnapshot{};
  if (next.revision == account_.revision && voice.revision == voice_.revision) return;
  const bool departed_party=account_.party.has_value() && !next.party && next.authenticated;
  const bool joined_party=!account_.party && next.party && next.authenticated;
  auto previous_account=std::move(account_);
  account_ = std::move(next);
  voice_ = std::move(voice);
  // Discord acceptance uses the account worker outside the menu's command
  // wrapper. Enter the actual party lobby only after authoritative membership,
  // preserving an already open lobby or retained postgame report.
  if(joined_party && !frame().vista_pcr && !launch &&
     std::none_of(stack_.begin(),stack_.end(),[](const auto &f){return f.screen==14;})) {
    if(online_matchmaking_)online_matchmaking_->cancel_queue();
    matchmaking_active_=quickmatch_pending_=false;
    return_online();open(14);
  }
  if(party_voice_wanted_ && account_.party && account_.authenticated && !voice_party_ && !frame().vista_pcr) {
    if(online_voice_)online_voice_->leave();
    online_voice_.reset(); // Join the old device worker before switching groups.
    online_voice_=std::make_shared<OnlineVoice>(online_account_,true);voice_party_=true;
    online_voice_->join();voice_=online_voice_->snapshot();
  }
  if(departed_party && voice_party_) {
    online_voice_->leave();party_voice_wanted_=false;
  }
  if(departed_party) {
    const auto lobby=std::find_if(stack_.begin(),stack_.end(),[](const auto &f){return f.screen==14;});
    if(lobby!=stack_.end()) {
      stack_.erase(lobby,stack_.end());
      open(26,2); // Leave/removal disposes the party lobby, retaining ONLINE.
    }
  }
  for (auto &f : stack_) {
    if ((!online_clan_screen(f) && !online_party_screen(f)) || f.screen == 33) continue;
    const auto selected = f.selected; std::string focus;
    if (selected < f.entries.size()) focus = f.entries[selected].label;
    std::string member_focus;
    if(f.screen==26 && f.context==1 && previous_account.clan && selected<f.entries.size()) {
      const auto index=unsigned(f.entries[selected].value);
      if(index<previous_account.clan->members.size())member_focus=previous_account.clan->members[index].id;
    }
    f.entries.clear();if(online_party_screen(f))populate_online_party(f);else populate_online_clans(f);
    f.selected = f.entries.empty() ? 0 : std::min(selected, unsigned(f.entries.size()-1));
    // Party members may deliberately use identical screen names. Their
    // server order is stable; display text cannot identify the selected row.
    if(!member_focus.empty() && account_.clan) {
      for(unsigned i=0;i<f.entries.size();++i) {
        const auto index=unsigned(f.entries[i].value);
        if(index<account_.clan->members.size() && account_.clan->members[index].id==member_focus){f.selected=i;break;}
      }
    } else if(!online_party_screen(f)) {
      const auto it = std::find_if(f.entries.begin(),f.entries.end(),[&](const auto &e){return e.label == focus;});
      if (it != f.entries.end()) f.selected = unsigned(it-f.entries.begin());
    }
  }
  ++revision;
}
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
      f.entries.push_back({!voice_party_ && (voice_.joined || voice_.connecting) ? "Leave Clan Voice" : "Join Clan Voice",-32,0});
      if (!voice_party_ && voice_.joined) f.entries.push_back({voice_.microphone_muted ? "Unmute Microphone" : "Mute Microphone",-32,1});
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
      if (!m->self && !voice_party_ && voice_.joined) {
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
  if (!voice_party_ && voice_.joined) {
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
    if (i == 6) return !account_.clan ? "" : !voice_party_ && voice_.joined
        ? (voice_.microphone_muted ? "MIC MUTED" : "MIC ON") : "VOICE OFF";
    if (i == 8) return !voice_party_ && online_voice_ && voice_.revision>1 ? (voice_.joined && voice_.microphone_muted ? "Clan voice - microphone muted." : voice_.message) : account_.message;
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
  if (command.action=="leave" && !voice_party_ && online_voice_) online_voice_->leave();
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
      if(voice_party_) {
        online_voice_->leave();online_voice_.reset();party_voice_wanted_=voice_party_=false;
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
