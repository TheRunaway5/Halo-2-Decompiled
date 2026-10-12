#include "halo2/frontend.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr unsigned party_context=42;
}
bool MenuState::online_party_screen(const MenuFrame &f) const {
  return (f.screen==14 && mode_==3) || (f.screen==26 && f.context==2) || (f.screen==216 && f.context==0) ||
         (f.screen==31 && f.context==party_context);
}
void MenuState::populate_online_party(MenuFrame &f) const {
  f.description.clear();
  if(f.screen==14) {
    f.title="ONLINE PARTY";
    if(account_.party)f.entries={{"Playlists",219,0},{"Party Options",216,0}};
    else f.entries={{"Create Party",-33,0},{"Party Invitations",26,2}};
  } else if(f.screen==26) {
    f.title="ONLINE";
    if(account_.party) {
      for(unsigned i=0;i<account_.party->members.size();++i)
        f.entries.push_back({account_.party->members[i].name,31,int(i)});
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
      f.entries.push_back({"Invite Player",167,9});
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
  if(f.screen==14) {
    if(i==0)return account_.busy?"Updating party...":!account_.party_message.empty()?account_.party_message:
        !account_.party?account_.message:voice_party_ && voice_.revision>1?voice_.message:"Choose a playlist together.";
    if(i==2)return "ONLINE PARTY";
    if(i==3) {
      const auto p=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),[&](const auto &p){return p.id==selected_playlist_;});
      return p==matchmaking_.playlists.end()?"Select a playlist":p->name;
    }
    if(i==43 && account_.party)for(const auto &m:account_.party->members)if(m.leader)return m.name;
    if(i==44)return account_.party?std::to_string(account_.party->members.size())+" / 16":"";
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
    if(i==4)return account_.party?std::to_string(account_.party->members.size()):"";
    if(i==6)return voice_party_ && voice_.joined?(voice_.microphone_muted?"MUTE":"VOICE"):"";
    return {};
  }
  return i==0?account_.party_message.empty()?account_.message:account_.party_message:"";
}
std::string MenuState::online_party_column(const MenuFrame &f,unsigned row,unsigned column) const {
  if(f.screen!=26 || !account_.party || row>=account_.party->members.size() || column!=1)return {};
  const auto &m=account_.party->members[row];
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
  if(f.screen==14 && entry.destination>0){if(!account_.party || account_.party->leader)open(unsigned(entry.destination),unsigned(entry.value));return true;}
  if(f.screen==26 && entry.destination==31) {
    const auto i=unsigned(entry.value);party_target_version_=0;
    if(i>=2000 && account_.party && i-2000<account_.party->outgoing.size()){party_target_=account_.party->outgoing[i-2000].id;party_target_kind_=2;}
    else if(i>=1000 && i-1000<account_.party_invitations.size()){const auto &v=account_.party_invitations[i-1000];party_target_=v.id;party_target_version_=v.version;party_target_kind_=1;}
    else if(account_.party && i<account_.party->members.size()){party_target_=account_.party->members[i].handle;party_target_kind_=0;}
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
