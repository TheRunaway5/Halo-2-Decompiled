#include "halo2/frontend.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr unsigned party_context=42;
constexpr unsigned invite_context=45;
}
std::vector<FrontendUiSound> frontend_party_notification_sounds(
    const OnlineAccountSnapshot &before,const OnlineAccountSnapshot &after) {
  // The first authenticated snapshot is a baseline. Polls, name/profile
  // edits, readiness, leadership and voice activity never imply membership.
  if(!before.authenticated || !after.authenticated)return {};
  std::vector<FrontendUiSound> sounds;
  if(!before.party && after.party)sounds.push_back(FrontendUiSound::Advance);
  else if(before.party && !after.party)sounds.push_back(FrontendUiSound::Back);
  else if(before.party && after.party) {
    const auto added=[](const OnlineParty &a,const OnlineParty &b) {
      return std::any_of(b.members.begin(),b.members.end(),[&](const auto &m) {
        const auto old=std::find_if(a.members.begin(),a.members.end(),[&](const auto &p){return p.handle==m.handle;});
        return old==a.members.end() || old->local_players.size()<m.local_players.size();
      });
    };
    if(added(*after.party,*before.party))sounds.push_back(FrontendUiSound::Back);
    if(added(*before.party,*after.party))sounds.push_back(FrontendUiSound::Advance);
  }
  if(std::any_of(after.party_invitations.begin(),after.party_invitations.end(),[&](const auto &invite) {
    return std::none_of(before.party_invitations.begin(),before.party_invitations.end(),[&](const auto &old){return old.id==invite.id;});
  }))sounds.push_back(FrontendUiSound::ReceiveMessage);
  if(before.busy && !after.busy && !after.party_message.empty())sounds.push_back(FrontendUiSound::Error);
  return sounds;
}
std::vector<FrontendUiSound> MenuState::take_ui_sounds() {
  auto sounds=std::move(pending_ui_sounds_);pending_ui_sounds_.clear();return sounds;
}
std::optional<FrontendInviteNotice> MenuState::invite_notice() const {
  if(!account_.authenticated || signed_in_.empty())return {};
  for(const auto &invite:account_.party_invitations)
    if(std::find(dismissed_party_invites_.begin(),dismissed_party_invites_.end(),invite.id)==dismissed_party_invites_.end())
      return FrontendInviteNotice{invite.id,invite.name,account_.busy,!account_.busy && !account_.party && !launch &&
        matchmaking_.status!=OnlineMatchmakingStatus::CheckingConnections &&
        matchmaking_.status!=OnlineMatchmakingStatus::AwaitingTransport && matchmaking_.status!=OnlineMatchmakingStatus::Loading};
  return {};
}
bool MenuState::act_invite_notice(bool accept) {
  const auto notice=invite_notice();if(!notice)return false;
  const auto invite=std::find_if(account_.party_invitations.begin(),account_.party_invitations.end(),[&](const auto &i){return i.id==notice->id;});
  if(invite==account_.party_invitations.end())return false;
  if(!accept){dismissed_party_invites_.push_back(invite->id);++revision;return true;}
  if(!notice->can_accept || !online_account_)return false;
  // Remain on this screen until the worker confirms actual membership.
  if(!online_account_->party_command({"accept",invite->id,{},{},invite->version}))return false;
  if(online_matchmaking_)online_matchmaking_->cancel_queue();
  update_online_account();++revision;return true;
}
std::vector<OnlinePartyMember> MenuState::online_party_members() const {
  std::vector<OnlinePartyMember> result;
  if(!account_.party)return result;
  for(const auto &member:account_.party->members) {
    auto primary=member;
    if(member.self)primary.appearance=appearance_for_profile(signed_in_);
    result.push_back(std::move(primary));
    if(member.self) {
      for(const auto &p:local_players())if(p.controller) {
        auto local=member;local.name=p.name;local.leader=false;local.local_players.clear();
        local.local_controller=int(p.controller);
        local.appearance=p.appearance;
        local.guest=!local_discord_logins_[p.controller] || !local_discord_logins_[p.controller]->authorization();
        result.push_back(std::move(local));
      }
    } else for(const auto &p:member.local_players) {
      auto local=member;local.name=p.name;local.leader=false;local.local_players.clear();local.guest=p.guest;local.appearance=p.appearance;
      // Remote couch seats belong to the primary's connection. They cannot
      // transfer leadership or leave the accepted party independently.
      local.local_controller=-2;result.push_back(std::move(local));
    }
  }
  return result;
}
bool MenuState::online_party_screen(const MenuFrame &f) const {
  return (f.screen==14 && mode_==3) || (f.screen==26 && f.context==2) || (f.screen==216 && f.context==0) ||
         (f.screen==31 && (f.context==party_context || f.context==invite_context));
}
bool MenuState::party_invite_picker() const {return frame().screen==31 && frame().context==invite_context;}
bool MenuState::switch_invite_tab(int direction) {
  if(!party_invite_picker())return false;
  if(direction)party_invite_tab_=1-party_invite_tab_;
  auto &f=stack_.back();f.entries.clear();f.selected=0;populate_online_party(f);++revision;return true;
}
void MenuState::populate_online_party(MenuFrame &f) const {
  f.description.clear();
  if(f.screen==31 && f.context==invite_context) {
    f.title="INVITE TO PARTY";
    if(party_invite_tab_==0) {
      f.description=account_.steam_friends_message;
      for(unsigned i=0;i<account_.steam_friends.size();++i) {
        const auto &p=account_.steam_friends[i];
        f.entries.push_back({p.name+(p.halo_online?" - Halo ONLINE":!p.registered?" - Not signed into Halo":" - Steam Online"),-36,int(i)});
      }
      if(account_.steam_friends.empty())f.entries.push_back({account_.steam_friends_state=="loading"?"Loading Steam friends...":"No visible online Steam friends",-82,0});
      f.entries.push_back({"Refresh Steam Friends",-36,-1});
    } else {
      f.description=account_.clan?"Select an online clan member to invite.":"Join a clan to invite its members.";
      if(account_.clan)for(unsigned i=0;i<account_.clan->members.size();++i) {
        const auto &p=account_.clan->members[i];
        if(!p.self && (p.presence==OnlineClanPresence::Online || p.presence==OnlineClanPresence::Playing))
          f.entries.push_back({p.name+(p.presence==OnlineClanPresence::Playing?" - In match":""),-37,int(i)});
      }
      if(f.entries.empty())f.entries.push_back({"No available online clan members",-82,0});
    }
    f.entries.push_back({"Search Discord Username",167,9});
    f.entries.push_back({"Back",-5,0});return;
  }
  if(f.screen==14) {
    f.title="ONLINE PARTY";
    if(account_.party)f.entries={{"Playlists",219,0},{"Party Options",216,0}};
    else f.entries={{"Create Party",-33,0},{"Party Invitations",26,2}};
  } else if(f.screen==26) {
    f.title="ONLINE";
    if(account_.party) {
      const auto members=online_party_members();
      for(unsigned i=0;i<members.size();++i)
        f.entries.push_back({members[i].name,members[i].local_controller==-2?-82:31,int(i)});
      for(unsigned i=0;i<account_.party->outgoing.size();++i)
        f.entries.push_back({"Invited: "+account_.party->outgoing[i].name,31,int(2000+i)});
    } else f.entries.push_back({"Create Party",-33,0});
    for(unsigned i=0;i<account_.party_invitations.size();++i)
      f.entries.push_back({"Invite: "+account_.party_invitations[i].name,31,int(1000+i)});
  } else if(f.screen==216) {
    f.title="PARTY OPTIONS";
    if(!account_.party){f.entries={{"Create Party",-33,0},{"Party Invitations",26,0}};return;}
    f.entries.push_back({"Playlists",219,0});
    if(account_.party->leader) {
      f.entries.push_back({"Invite Player",31,int(invite_context)});
      if(account_.clan && account_.clan->role>=1)f.entries.push_back({"Invite Clan To Party",-33,1});
    }
    f.entries.push_back({"Party Members",26,0});
    if(online_voice_) {
      f.entries.push_back({voice_party_ && (voice_.joined || voice_.connecting)?"Leave Party Voice":"Join Party Voice",-34,0});
      if(voice_party_ && voice_.joined)f.entries.push_back({voice_.microphone_muted?"Unmute Microphone":"Mute Microphone",-34,1});
    }
    f.entries.push_back({"Stop Searching",-33,6});
    f.entries.push_back({"Leave Party",-33,5});
  } else if(f.screen==31) {
    f.title="PARTY";
    if(party_target_kind_==1) {
      const auto it=std::find_if(account_.party_invitations.begin(),account_.party_invitations.end(),[&](const auto &i){return i.id==party_target_;});
      if(it!=account_.party_invitations.end()){f.title=it->name;f.entries={{"Accept Party Invite",-33,2},{"Decline Party Invite",-33,3},{"Block Future Invites",-33,4}};}
    } else if(party_target_kind_==2 && account_.party) {
      const auto it=std::find_if(account_.party->outgoing.begin(),account_.party->outgoing.end(),[&](const auto &i){return i.id==party_target_;});
      if(it!=account_.party->outgoing.end()){f.title=it->name;f.entries={{"Cancel Party Invite",-33,7}};}
    } else if(account_.party) {
      const auto it=std::find_if(account_.party->members.begin(),account_.party->members.end(),[&](const auto &m){return m.handle==party_target_;});
      if(it!=account_.party->members.end()) {
        f.title=it->name;
        if(it->self)f.entries={{"Playlists",219,0},{"Leave Party",-33,5}};
        else if(account_.party->leader)f.entries={{"Make Party Leader",-33,8},{"Remove From Party",-33,9}};
        if(!it->self && voice_party_ && voice_.joined) {
          const bool muted=std::find(voice_.muted.begin(),voice_.muted.end(),it->handle)!=voice_.muted.end();
          f.entries.push_back({muted?"Unmute Voice":"Mute Voice",-34,2});
        }
      }
    }
    if(f.entries.empty())f.entries.push_back({"Back",-5,0});
  }
}
std::string MenuState::online_party_text(const MenuFrame &f,unsigned i) const {
  if(f.screen==31 && f.context==invite_context)return i==0?f.description:std::string{};
  if(f.screen==14) {
    if(i==0)return account_.busy?"Updating party...":!account_.party_message.empty()?account_.party_message:
        !account_.party?account_.message:voice_party_ && voice_.revision>1?voice_.message:"Choose a playlist together.";
    if(i==2)return "ONLINE PARTY";
    if(i==3) {
      const auto p=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),[&](const auto &p){return p.id==selected_playlist_;});
      return p==matchmaking_.playlists.end()?"Select a playlist":p->name;
    }
    if(i==43 && account_.party)for(const auto &m:account_.party->members)if(m.leader)return m.name;
    if(i==44)return account_.party?std::to_string(online_party_members().size())+" / 16":"";
    // Use the authored empty detail fields in the left panel. The original
    // help field47 occupies the right roster and collides with a full party.
    if(i==4)return "\xee\x84\x83 PARTY MEMBERS";
    if(i==5)return "\xee\x84\x82 PARTY OPTIONS";
    if(i==6)return "V: hold to talk";
    return {};
  }
  if(f.screen==26) {
    const auto &s=content_.screens.at(26);const auto &pane=s.panes.at(2);
    if(i==0)return "PARTY";
    if(i==1 || i==3)return content_.localized(s,pane.texts.at(i).string);
    if(i==2)return account_.party?"Party members":"Party invitations";
    if(i==4)return account_.party?std::to_string(online_party_members().size()):"";
    if(i==6)return voice_party_ && voice_.joined?(voice_.microphone_muted?"MUTE":"VOICE"):"";
    return {};
  }
  return i==0?account_.party_message.empty()?account_.message:account_.party_message:"";
}
std::string MenuState::online_party_column(const MenuFrame &f,unsigned row,unsigned column) const {
  const auto members=online_party_members();
  if(f.screen!=26 || row>=members.size() || column!=1)return {};
  const auto &m=members[row];
  if(m.guest)return "Guest";
  if(voice_party_ && voice_.joined) {
    if(std::find(voice_.muted.begin(),voice_.muted.end(),m.handle)!=voice_.muted.end())return "Muted";
    if((m.self && voice_.transmitting) || std::find(voice_.talking.begin(),voice_.talking.end(),m.handle)!=voice_.talking.end())return "Talking";
  }
  return !m.online?"Offline":!m.ready_playlist.empty()?"Ready":m.leader?"Leader":"ONLINE";
}
void MenuState::party_command(OnlinePartyCommand command) {
  const bool enter=command.action=="create" || command.action=="accept";
  if(!online_account_ || !online_account_->party_command(std::move(command))){unavailable("Wait for the current ONLINE operation to finish.");return;}
  update_online_account();
  if(enter){while(stack_.size()>1 && frame().screen!=186)stack_.pop_back();open(14);}
  else {
    while(stack_.size()>1 && !(frame().screen==26 && frame().context==2) && frame().screen!=14 && frame().screen!=186)stack_.pop_back();
    if(frame().screen!=26)open(26,2);
  }
  ++revision;
}
bool MenuState::accept_online_party() {
  const auto f=frame();if(!online_party_screen(f))return false;
  if(f.entries.empty() || f.selected>=f.entries.size())return true;
  const auto entry=f.entries[f.selected];
  if(f.screen==216 && entry.destination==31 && entry.value==int(invite_context)) {
    party_invite_tab_=0;
    if(online_account_)online_account_->refresh_steam_friends();
    update_online_account();open(31,invite_context);return true;
  }
  if(f.context==invite_context && (entry.destination==-36 || entry.destination==-37)) {
    if(entry.value==-1){if(online_account_)online_account_->refresh_steam_friends();update_online_account();return true;}
    const auto i=unsigned(entry.value);
    if(!account_.party || !account_.party->leader || account_.busy)return true;
    if(entry.destination==-36 && i<account_.steam_friends.size()) {
      const auto &p=account_.steam_friends[i];
      if(!p.registered){unavailable("This Steam friend needs to sign into Halo ONLINE before receiving an invitation.");return true;}
      party_command({"invite_steam",p.id});
    } else if(entry.destination==-37 && account_.clan && i<account_.clan->members.size())
      party_command({"invite_member",account_.clan->members[i].id});
    return true;
  }
  if(f.screen==14 && entry.destination>0){if(!account_.party || account_.party->leader)open(unsigned(entry.destination),unsigned(entry.value));return true;}
  if(f.screen==26 && entry.destination==31) {
    const auto i=unsigned(entry.value);party_target_version_=0;
    if(i>=2000 && account_.party && i-2000<account_.party->outgoing.size()){party_target_=account_.party->outgoing[i-2000].id;party_target_kind_=2;}
    else if(i>=1000 && i-1000<account_.party_invitations.size()){const auto &v=account_.party_invitations[i-1000];party_target_=v.id;party_target_version_=v.version;party_target_kind_=1;}
    else if(account_.party && i<online_party_members().size()) {
      const auto member=online_party_members()[i];
      if(member.local_controller>0){open_online_local_player(unsigned(member.local_controller));return true;}
      party_target_=member.handle;party_target_kind_=0;
    }
    else return true;
    open(31,party_context);return true;
  }
  if(entry.destination==167){keyboard_text.clear();open(167,9);return true;}
  if(entry.destination>0){open(unsigned(entry.destination),entry.destination==26?2u:unsigned(entry.value));return true;}
  if(entry.destination==-5){back();return true;}
  if(entry.destination==-34 && online_voice_) {
    if(entry.value==0) {
      if(voice_party_ && (voice_.joined || voice_.connecting)){online_voice_->leave();party_voice_wanted_=false;}
      else {
        online_voice_->leave();online_voice_.reset();
        online_voice_=std::make_shared<OnlineVoice>(online_account_,true);voice_party_=party_voice_wanted_=true;online_voice_->join();
      }
    } else if(entry.value==1)online_voice_->mute_microphone(!voice_.microphone_muted);
    else if(entry.value==2)online_voice_->mute(party_target_,std::find(voice_.muted.begin(),voice_.muted.end(),party_target_)==voice_.muted.end());
    voice_=online_voice_->snapshot();back();update_online_account();return true;
  }
  if(entry.destination==-33) {
    constexpr const char *actions[]{"create","invite_clan","accept","decline","block","leave","unready","cancel","transfer","remove"};
    if(entry.value<0 || entry.value>=10)return true;
    OnlinePartyCommand command;command.action=actions[entry.value];command.value=party_target_;command.version=party_target_version_;
    if(entry.value!=2)command.version=0;
    if(entry.value==6 && online_matchmaking_)online_matchmaking_->cancel_queue();
    party_command(std::move(command));return true;
  }
  return true;
}
} // namespace halo2
