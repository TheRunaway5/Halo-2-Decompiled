#include "halo2/frontend.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr unsigned friend_context=43;
constexpr unsigned activity_context=44;
constexpr unsigned search_context=46;
bool party_join_blocked(OnlineMatchmakingStatus status) {
  return status==OnlineMatchmakingStatus::Joining || status==OnlineMatchmakingStatus::Searching ||
      status==OnlineMatchmakingStatus::CheckingConnections || status==OnlineMatchmakingStatus::AwaitingTransport ||
      status==OnlineMatchmakingStatus::Loading;
}
const char *presence(DiscordFriendPresence value) {
  switch(value) {
  case DiscordFriendPresence::Playing:return "Playing Halo 2";
  case DiscordFriendPresence::Online:return "ONLINE";
  case DiscordFriendPresence::Idle:return "Away";
  case DiscordFriendPresence::DoNotDisturb:return "Do not disturb";
  default:return "Offline";
  }
}
}
void MenuState::set_discord_social(std::shared_ptr<DiscordFriends> social) {
  discord_social_=std::move(social);friends_=discord_social_?discord_social_->snapshot():DiscordSocialSnapshot{};
  friend_target_.clear();activity_target_.clear();
  if(discord_social_)discord_social_->bind_account(online_account_);
}
bool MenuState::online_friends_screen(const MenuFrame &f) const {
  return (f.screen==26 && f.context==0) || ((f.screen==216 || f.screen==31) && f.context==friend_context) ||
      (f.screen==31 && (f.context==activity_context || f.context==search_context));
}
bool MenuState::native_friends() const {
  return account_.friends_available || (discord_login_ && discord_login_->uses_steam());
}
bool MenuState::online_friends_available() const {
  return online_identity().has_value() &&
      ((account_.authenticated && account_.friends_available) ||
       (friends_.available && friends_.enabled && friends_.ready));
}
void MenuState::update_online_friend_join() {
  if(!friend_join_serial_)return;
  if(!account_.authenticated || frame().screen!=31 || frame().context!=friend_context ||
     account_.party_command_serial!=friend_join_serial_) {friend_join_serial_=0;return;}
  if(account_.busy || !account_.party_command_successful.has_value())return;
  const bool accepted=account_.party_command_successful==true && account_.party.has_value();
  friend_join_serial_=0;
  if(!accepted)unavailable(account_.party_message.empty()?"The party join was not accepted. Refresh Friends and try again.":account_.party_message);
}
void MenuState::populate_native_friends(MenuFrame &f) const {
  f.title=f.screen==26?"ONLINE":f.screen==31?"FRIEND OPTIONS":"FRIENDS OPTIONS";
  f.description=account_.friend_tag.empty()?account_.friends_message:"Your friend tag: "+account_.friend_tag;
  if(!account_.friends_message.empty() && !account_.friend_tag.empty())f.description+="\n"+account_.friends_message;
  if(f.screen==26 || f.screen==216) {
    f.entries.push_back({"Find Player by Friend Tag",167,11});
    for(unsigned i=0;i<account_.friends.size();++i) {
      const auto &row=account_.friends[i];
      if((row.state=="blocked")!=(f.screen==216))continue;
      f.entries.push_back({(row.state=="incoming"?"Request: ":row.state=="outgoing"?"Pending: ":row.state=="blocked"?"Blocked: ":"")+row.name,31,int(i)});
    }
    if(f.screen==26 && f.entries.size()==1)f.entries.push_back({account_.friends_available?"No friends added yet":"Friends are unavailable",-82,0});
    if(f.screen==216){f.entries.push_back({"Party Members",26,2});f.entries.push_back({"Back",-5,0});}
    return;
  }
  const OnlineFriend *row=nullptr;
  if(f.context==search_context && account_.friend_search)row=&*account_.friend_search;
  else for(const auto &r:account_.friends)if(r.id==friend_target_){row=&r;break;}
  if(row) {
    f.description=row->tag+"\n"+(row->presence=="playing"?"Playing Halo 2":row->presence=="online"?"ONLINE":"Offline");
    if(!account_.friends_message.empty())f.description+="\n"+account_.friends_message;
    const auto relation=std::find_if(account_.friends.begin(),account_.friends.end(),[&](const auto &r){return r.id==row->id;});
    const auto state=relation==account_.friends.end()?std::string{}:relation->state;
    if(state.empty())f.entries.push_back({"Send Friend Request",-83,0});
    if(state=="incoming"){f.entries.push_back({"Accept Friend Request",-83,1});f.entries.push_back({"Decline Friend Request",-83,2});}
    if(state=="outgoing")f.entries.push_back({"Cancel Friend Request",-83,3});
    if(state=="friend") {
      if(account_.party && account_.party->leader)f.entries.push_back({"Invite To Party",-83,7});
      // Original2B55AE/19A250: this game-owned friend list supplies same-title
      // identity and observed online/joinable state. Do not offer joining in
      // the source joining/search/transfer interval or with an accepted party
      // that has not completed its existing explicit Leave action.
      if(f.context==friend_context && !account_.busy && !account_.party && !launch && !party_join_blocked(matchmaking_.status) &&
         row->presence!="offline" && row->party_joinable==true && !row->party_context.empty())
        f.entries.push_back({"Join Party",-83,8}); // Actual31 UNIC,0A0002A8.
      f.entries.push_back({"Remove Friend",-83,4});
    }
    f.entries.push_back({state=="blocked"?"Unblock Player":"Block Player",-83,state=="blocked"?6:5});
  }
  f.entries.push_back({"Back",-5,0});
}
bool MenuState::accept_native_friends() {
  const auto f=frame();if(f.entries.empty() || f.selected>=f.entries.size())return true;
  const auto entry=f.entries[f.selected];
  if(entry.destination==167){keyboard_text.clear();open(167,11);return true;}
  if(entry.destination==-5){back();return true;}
  if(entry.destination==26){open(26,unsigned(entry.value));return true;}
  if(entry.destination==31) {
    const auto i=unsigned(entry.value);if(i>=account_.friends.size())return true;
    friend_target_=account_.friends[i].id;open(31,friend_context);return true;
  }
  if(entry.destination!=-83 || !online_account_ || account_.busy)return true;
  const auto target=f.context==search_context && account_.friend_search?account_.friend_search->id:friend_target_;
  if(target.empty())return true;
  if(entry.value==7){party_command({"invite_friend",target});return true;}
  if(entry.value==8) {
    const auto row=std::find_if(account_.friends.begin(),account_.friends.end(),[&](const auto &friend_){return friend_.id==target;});
    if(f.context==friend_context && row!=account_.friends.end() && !account_.party && !launch &&
       !party_join_blocked(matchmaking_.status) && row->presence!="offline" && row->party_joinable==true) {
      OnlinePartyCommand command;command.action="join_friend";command.value=target;command.join_context=row->party_context;
      if(online_account_->party_command(std::move(command))) {
        // update_online_account's existing joined_party observer opens14 only
        // after the real accepted membership arrives, including typed reply
        // validation of the selected target context. Failure stays here.
        friend_join_serial_=online_account_->snapshot().party_command_serial;
        update_online_account();++revision;
      } else unavailable("Refresh Friends before joining this party.");
    }
    return true;
  }
  constexpr const char *actions[]{"request","accept","decline","cancel","remove","block","unblock"};
  if(entry.value>=0 && entry.value<=6 && online_account_->friend_command(actions[entry.value],target)) {
    back();update_online_account();++revision;
  }
  return true;
}
void MenuState::populate_online_friends(MenuFrame &f) const {
  if(native_friends() && f.context!=activity_context){populate_native_friends(f);return;}
  f.title=f.screen==26?"ONLINE":f.screen==31?"FRIEND OPTIONS":"FRIENDS OPTIONS";
  f.description=friends_.message;
  const std::string enable=discord_login_ && discord_login_->uses_steam()?"Link Discord Friends":"Enable Discord Friends";
  if(f.screen==31 && f.context==activity_context) {
    f.title="GAME INVITATION";
    const auto it=std::find_if(friends_.invitations.begin(),friends_.invitations.end(),[&](const auto &i){return i.handle==activity_target_;});
    if(it!=friends_.invitations.end()) {
      f.description=it->name+"\nHalo 2 party invitation";
      f.entries={{friends_.busy?"Accepting Invitation...":"Accept Game Invite",-35,friends_.busy?0:7},
                 {"Dismiss Invitation",-35,8}};
    }
    f.entries.push_back({"Back",-5,0});return;
  }
  if(f.screen==26) {
    if(!friends_.available)f.entries={{"Discord Friends unavailable",-35,0}};
    else if(!friends_.enabled)f.entries={{enable,-35,1}};
    else if(!friends_.ready)f.entries={{friends_.connecting?"Connecting Discord Friends...":"Reconnect Discord Friends",-35,friends_.connecting?0:1}};
    else {
      for(unsigned i=0;i<friends_.invitations.size();++i)
        f.entries.push_back({friends_.invitations[i].name,31,-int(i)-1});
      for(unsigned i=0;i<friends_.friends.size();++i) {
        const auto &row=friends_.friends[i];
        if(row.kind==DiscordFriendKind::Blocked)continue;
        std::string name=row.name;
        if(row.kind==DiscordFriendKind::Incoming)name="Request: "+name;
        if(row.kind==DiscordFriendKind::Outgoing)name="Pending: "+name;
        f.entries.push_back({std::move(name),31,int(i)});
      }
      if(f.entries.empty())f.entries={{"No Discord friends",-35,0}};
    }
  } else if(f.screen==216) {
    if(friends_.available)f.entries.push_back({friends_.enabled?"Reconnect Discord Friends":enable,-35,1});
    if(friends_.enabled)f.entries.push_back({"Disconnect Discord Friends",-35,2});
    f.entries.push_back({"Party Members",26,2});
    f.entries.push_back({"Back",-5,0});
  } else {
    const auto it=std::find_if(friends_.friends.begin(),friends_.friends.end(),[&](const auto &r){return r.handle==friend_target_;});
    if(it==friends_.friends.end()) {f.entries={{"Back",-5,0}};return;}
    f.description=it->name+"\n"+presence(it->presence);
    if(it->kind==DiscordFriendKind::Friend && account_.party && account_.party->leader)
      f.entries.push_back({"Invite To Party",-35,3});
    if(it->kind==DiscordFriendKind::Incoming) {
      f.entries.push_back({"Accept Discord Friend Request",-35,4});
      f.entries.push_back({"Decline Discord Friend Request",-35,5});
    }
    if(it->kind==DiscordFriendKind::Outgoing)f.entries.push_back({"Cancel Discord Friend Request",-35,6});
    f.entries.push_back({"Back",-5,0});
  }
}
void MenuState::update_online_friends() {
  if(!discord_social_)return;
  discord_social_->presence(profile_name(),false);discord_social_->pump();
  if(native_friends())return;
  const auto value=discord_social_->snapshot();
  if(value.revision==friends_.revision)return;
  const auto previous_friends=std::move(friends_);friends_=value;
  for(auto &f:stack_)if(online_friends_screen(f)) {
    std::string selected;
    if(f.screen==26 && f.selected<f.entries.size() && f.entries[f.selected].destination==31) {
      const auto i=unsigned(f.entries[f.selected].value);
      // The stored row handle, rather than its duplicate display name, owns focus.
      const auto value=f.entries[f.selected].value;
      if(value<0) {
        const auto invitation=unsigned(-value-1);
        if(invitation<previous_friends.invitations.size())selected=previous_friends.invitations[invitation].handle;
      } else if(i<previous_friends.friends.size())selected=previous_friends.friends[i].handle;
    }
    const auto previous=f.selected;f.entries.clear();populate_online_friends(f);
    f.selected=f.entries.empty()?0:std::min(previous,unsigned(f.entries.size()-1));
    if(!selected.empty())for(unsigned i=0;i<f.entries.size();++i) {
      const auto index=unsigned(f.entries[i].value);
      const auto value=f.entries[i].value;
      if(f.entries[i].destination!=31)continue;
      if(value<0) {
        const auto invitation=unsigned(-value-1);
        if(invitation<friends_.invitations.size() && friends_.invitations[invitation].handle==selected)f.selected=i;
      } else if(index<friends_.friends.size() && friends_.friends[index].handle==selected)f.selected=i;
    }
  }
  ++revision;
}
bool MenuState::accept_online_friends() {
  const auto f=frame();if(!online_friends_screen(f))return false;
  if(native_friends() && f.context!=activity_context)return accept_native_friends();
  if(f.entries.empty() || f.selected>=f.entries.size())return true;
  const auto entry=f.entries[f.selected];
  if(entry.destination==-5){back();return true;}
  if(entry.destination==26){open(26,unsigned(entry.value));return true;}
  if(entry.destination==31) {
    if(entry.value<0) {
      const auto i=unsigned(-entry.value-1);if(i>=friends_.invitations.size())return true;
      activity_target_=friends_.invitations[i].handle;open(31,activity_context);return true;
    }
    const auto i=unsigned(entry.value);if(i>=friends_.friends.size())return true;
    friend_target_=friends_.friends[i].handle;open(31,friend_context);return true;
  }
  if(entry.destination!=-35 || !discord_social_)return true;
  if(f.screen==31 && f.context==activity_context) {
    if(entry.value==7)discord_social_->accept_activity(activity_target_);
    else if(entry.value==8){discord_social_->dismiss_activity(activity_target_);back();}
    update_online_friends();return true;
  }
  if(entry.value==1)discord_social_->enable();
  else if(entry.value==2)discord_social_->disconnect();
  else if(entry.value>=3) {
    const auto row=std::find_if(friends_.friends.begin(),friends_.friends.end(),[&](const auto &r){return r.handle==friend_target_;});
    if(row==friends_.friends.end())return true;
    if(entry.value==3 && row->kind==DiscordFriendKind::Friend && account_.party && account_.party->leader) {
      if(discord_social_->invite_to_party(row->handle)) {
        update_online_account();
        while(stack_.size()>1 && !(frame().screen==26 && frame().context==2) && frame().screen!=14 && frame().screen!=186)stack_.pop_back();
        if(frame().screen!=26)open(26,2);
        ++revision;
      } else unavailable("Refresh Discord Friends and your ONLINE party before inviting this player.");
      return true;
    }
    constexpr const char *actions[]{"accept","decline","cancel"};
    if(entry.value>=4 && entry.value<=6)discord_social_->command(row->handle,actions[entry.value-4]);
    back();
  }
  update_online_friends();return true;
}
std::string MenuState::online_friends_text(const MenuFrame &f,unsigned index) const {
  if(native_friends() && f.context!=activity_context) {
    if(f.screen==26) {
      const auto &s=content_.screens.at(26);const auto &pane=s.panes.at(0);
      if(index==0 || index==1 || index==3)return content_.localized(s,pane.texts.at(index).string);
      return index==2?(account_.friend_tag.empty()?"Halo friends":account_.friend_tag):"";
    }
    const OnlineFriend *row=nullptr;
    if(f.context==search_context && account_.friend_search)row=&*account_.friend_search;
    else for(const auto &r:account_.friends)if(r.id==friend_target_){row=&r;break;}
    return row?(index==0?row->name:index==1?row->tag:""):index==0?account_.friends_message:"";
  }
  if(f.screen==26) {
    const auto &s=content_.screens.at(26);const auto &pane=s.panes.at(0);
    if(index==0 || index==1 || index==3)return content_.localized(s,pane.texts.at(index).string);
    if(index==2)return "Discord friends";
    return {};
  }
  if(f.screen==31) {
    if(f.context==activity_context) {
      const auto it=std::find_if(friends_.invitations.begin(),friends_.invitations.end(),[&](const auto &i){return i.handle==activity_target_;});
      if(it==friends_.invitations.end())return index==0?(friends_.busy?"Joining ONLINE party...":"Invitation unavailable"):"";
      if(index==2 && account_.party)return "Leave current party first.";
      return index==0?it->name:index==1?it->status:"";
    }
    const auto row=std::find_if(friends_.friends.begin(),friends_.friends.end(),[&](const auto &r){return r.handle==friend_target_;});
    if(row==friends_.friends.end())return index==0?"Friend unavailable":"";
    return index==0?row->name:index==1?presence(row->presence):"";
  }
  return index==0?friends_.message:std::string{};
}
std::string MenuState::online_friends_column(const MenuFrame &f,unsigned row,unsigned column) const {
  if(f.screen!=26 || row>=f.entries.size() || f.entries[row].destination!=31 || column!=1)return {};
  if(native_friends()) {
    const auto i=unsigned(f.entries[row].value);if(i>=account_.friends.size())return {};
    const auto &r=account_.friends[i];
    return r.state=="incoming"?"Friend request":r.state=="outgoing"?"Pending":r.presence=="playing"?"Playing Halo 2":r.presence=="online"?"ONLINE":"Offline";
  }
  const auto index=unsigned(f.entries[row].value);
  if(f.entries[row].value<0) {
    const auto invitation=unsigned(-f.entries[row].value-1);
    return invitation<friends_.invitations.size()?friends_.invitations[invitation].status:"";
  }
  return index<friends_.friends.size()?presence(friends_.friends[index].presence):"";
}
} // namespace halo2
