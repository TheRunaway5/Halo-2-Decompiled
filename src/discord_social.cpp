#include "halo2/discord_social.hpp"
#include "halo2/discord_voice_blocks.hpp"
#include "halo2/discord_party_activity.hpp"
#include "halo2/online_account.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <set>
#ifdef HALO2_DISCORD_SOCIAL_SDK
#define DISCORDPP_IMPLEMENTATION
#include <discordpp.h>
#endif

namespace halo2 {
struct DiscordSocial::Impl {
  std::shared_ptr<DiscordLogin> identity;
  DiscordLogin consent;
  DiscordVoiceBlocks voice_blocks;
  DiscordSocialSnapshot state;
  std::string subject;
  std::string activity_name;
  bool activity_playing{};
  bool using_identity{};
  bool relationships_dirty{true}, activity_valid{};
  bool relationship_busy{}, presence_pending{}, launch_registered{};
  std::uint64_t application_id{}, presence_serial{}, party_revision{};
  std::string published_secret;
  std::uint64_t published_version{};
  unsigned published_size{};
  std::optional<std::string> pending_join;
  std::unique_ptr<DiscordPartyActivity> party_activity;
  std::unique_ptr<DiscordActivityInbox> inbox;
  std::weak_ptr<OnlineAccount> bound_account;
  std::uint64_t inbox_revision{};
  std::optional<std::pair<DiscordPartyLink,DiscordPartyActivity::Complete>> queued_activity;
  std::chrono::steady_clock::time_point next_refresh{}, next_activity{};
  unsigned generation{}, next_handle{};
  std::shared_ptr<bool> alive=std::make_shared<bool>(true);
#ifdef HALO2_DISCORD_SOCIAL_SDK
  std::unique_ptr<discordpp::Client> client;
  std::map<std::uint64_t,std::string> handles;
  std::map<std::string,std::uint64_t> targets;
  std::vector<discordpp::ActivityInvite> deferred_invites;
#endif
  Impl(std::shared_ptr<DiscordLogin> login, DiscordLogin::OpenBrowser browser,
       DiscordLoginOptions options, DiscordLogin::Http http)
    : identity(std::move(login)), consent(std::move(browser),[&]{options.social_presence=true;options.social_communication=true;return options;}(),std::move(http)),
      voice_blocks(identity,discord_voice_block_environment()) {
    inbox=std::make_unique<DiscordActivityInbox>([this](std::string secret) {
      return party_activity && party_activity->join(std::move(secret));
    },[this]() -> std::string {
      if(!state.ready)return "Reconnect Discord Friends before accepting this invitation.";
      if(activity_playing)return "Return to ONLINE before accepting this invitation.";
      if(relationship_busy || (party_activity && party_activity->status().busy))return "Wait for the current ONLINE operation to finish.";
      const auto account=bound_account.lock();
      if(!account)return "Sign into ONLINE before accepting this invitation.";
      if(account->snapshot().party)return "Leave your current party before accepting this invitation.";
      return {};
    });
    const auto parsed=std::from_chars(options.client_id.data(),options.client_id.data()+options.client_id.size(),application_id);
    if(parsed.ec!=std::errc{} || parsed.ptr!=options.client_id.data()+options.client_id.size())application_id=0;
#ifdef HALO2_DISCORD_SOCIAL_SDK
    state.available=true;
#endif
  }
  void changed() { ++state.revision; }
  void stop() {
    voice_blocks.reset();
    *alive=false;alive=std::make_shared<bool>(true);++generation;next_handle=0;
    if(party_activity)party_activity->reset();
    inbox->reset();inbox_revision=0;
    queued_activity.reset();pending_join.reset();published_secret.clear();++presence_serial;
    relationship_busy=presence_pending=launch_registered=false;party_revision=0;
#ifdef HALO2_DISCORD_SOCIAL_SDK
    if(client){client->ClearRichPresence();client->Disconnect();}
    client.reset();handles.clear();targets.clear();deferred_invites.clear();
#endif
    consent.cancel();subject.clear();activity_name.clear();using_identity=false;
    relationships_dirty=true;activity_valid=false;next_refresh={};next_activity={};
    const auto available=state.available;const auto revision=state.revision+1;
    state={};state.available=available;state.revision=revision;
  }
#ifdef HALO2_DISCORD_SOCIAL_SDK
  void write_presence(std::optional<DiscordPartyLink> link,DiscordPartyActivity::Complete done={}) {
    if(!client || !state.ready){if(done)done(false);return;}
    discordpp::Activity activity;
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetDetails(activity_playing?"Playing Halo 2":"ONLINE menus");
    activity.SetState(activity_name.empty()?std::optional<std::string>{}:std::optional<std::string>{activity_name.substr(0,128)});
    if(link && !activity_playing) {
      discordpp::ActivityParty party;party.SetId(link->party_id);
      party.SetCurrentSize(int(link->current_size));party.SetMaxSize(int(link->max_size));activity.SetParty(party);
      discordpp::ActivitySecrets secrets;secrets.SetJoin(link->join_secret);activity.SetSecrets(secrets);
      activity.SetSupportedPlatforms(discordpp::ActivityGamePlatforms::Desktop);
    }
    const auto weak=std::weak_ptr<bool>(alive);const auto serial=++presence_serial;
    presence_pending=true;activity_valid=false;next_activity=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    client->UpdateRichPresence(std::move(activity),[this,weak,serial,name=activity_name,playing=activity_playing,link=std::move(link),done=std::move(done)](auto result) {
      const auto live=weak.lock();if(!live || !*live || serial!=presence_serial)return;
      presence_pending=false;activity_valid=result.Successful() && name==activity_name && playing==activity_playing;
      published_secret=result.Successful() && link?link->join_secret:std::string{};
      published_version=result.Successful() && link?link->version:0;
      published_size=result.Successful() && link?link->current_size:0;
      if(done)done(result.Successful());
    });
  }
  void pump_activity() {
    inbox->pump();const auto incoming=inbox->snapshot();
    if(incoming.revision!=inbox_revision) {
      inbox_revision=incoming.revision;state.invitations=incoming.invitations;
      if(!incoming.message.empty())state.message=incoming.message;
      changed();
    }
    if(!party_activity)return;
    if(state.ready && pending_join) {
      auto secret=std::move(*pending_join);pending_join.reset();party_activity->join(std::move(secret));
    }
    party_activity->pump(activity_playing);
    const auto status=party_activity->status();
    state.busy=relationship_busy || status.busy || incoming.busy;
    if(status.revision!=party_revision) {
      party_revision=status.revision;
      if(!status.message.empty())state.message=status.message;
      changed();
    }
    if(state.ready && !presence_pending && queued_activity) {
      auto queued=std::move(*queued_activity);queued_activity.reset();
      const auto current=party_activity->link();
      if(current && current->join_secret==queued.first.join_secret)write_presence(*current,std::move(queued.second));
      else queued.second(false);
    }
  }
  void received_invite(discordpp::ActivityInvite invite) {
    if(!invite.MessageId() || !invite.ChannelId())return;
    const auto key=std::to_string(invite.ChannelId())+":"+std::to_string(invite.MessageId());
    bool permitted=invite.ApplicationId()==application_id && invite.Type()==discordpp::ActivityActionTypes::Join &&
        invite.SenderId() && std::to_string(invite.SenderId())!=subject && invite.IsValid();
    for(const auto &relation:client->GetRelationships())
      if(relation.Id()==invite.SenderId() && relation.DiscordRelationshipType()==discordpp::RelationshipType::Blocked)permitted=false;
    const auto user=client->GetUser(invite.SenderId());
    const auto name=user?user->DisplayName():std::string{"Discord player"};
    const auto weak=std::weak_ptr<bool>(alive);
    inbox->received(key,name,permitted,[this,weak,invite=std::move(invite)](DiscordActivityInbox::Complete done) {
      const auto live=weak.lock();if(!live || !*live || !client || !state.ready){done(false,{});return;}
      for(const auto &relation:client->GetRelationships())
        if(relation.Id()==invite.SenderId() && relation.DiscordRelationshipType()==discordpp::RelationshipType::Blocked){done(false,{});return;}
      client->AcceptActivityInvite(invite,[weak,done=std::move(done)](auto result,std::string secret) {
        const auto live=weak.lock();if(!live || !*live)return;
        done(result.Successful(),std::move(secret));
      });
    });
  }
  void refresh() {
    if(!client || !state.ready)return;
    const auto now=std::chrono::steady_clock::now();
    if(!relationships_dirty && now<next_refresh)return;
    relationships_dirty=false;next_refresh=now+std::chrono::seconds(1);
    std::vector<DiscordFriend> rows;
    std::set<std::uint64_t> seen;
    std::map<std::string,std::uint64_t> current;
    std::vector<std::string> blocked;
    for(const auto &relation:client->GetRelationships()) {
      const auto id=relation.Id();const auto user=relation.User();
      if(!id || !seen.insert(id).second)continue;
      const auto kind=relation.DiscordRelationshipType();
      if(kind==discordpp::RelationshipType::Blocked)blocked.push_back(std::to_string(id));
      if(!user || rows.size()>=2000)continue;
      if(kind!=discordpp::RelationshipType::Friend && kind!=discordpp::RelationshipType::PendingIncoming &&
         kind!=discordpp::RelationshipType::PendingOutgoing && kind!=discordpp::RelationshipType::Blocked)continue;
      auto &handle=handles[id];
      if(handle.empty())handle="friend-"+std::to_string(generation)+"-"+std::to_string(++next_handle);
      current.emplace(handle,id);
      DiscordFriend row;row.handle=handle;row.name=user->DisplayName();row.username=user->Username();
      // Names are untrusted display data, never markup or menu commands.
      for(auto *name:{&row.name,&row.username}) {
        if(name->size()>128)name->resize(128);
        std::erase_if(*name,[](unsigned char c){return c<32 || c==127;});
      }
      if(row.name.empty())row.name="Discord friend";
      row.kind=kind==discordpp::RelationshipType::Friend?DiscordFriendKind::Friend:
          kind==discordpp::RelationshipType::PendingIncoming?DiscordFriendKind::Incoming:
          kind==discordpp::RelationshipType::PendingOutgoing?DiscordFriendKind::Outgoing:DiscordFriendKind::Blocked;
      const auto status=user->Status();
      row.presence=status==discordpp::StatusType::Online || status==discordpp::StatusType::Streaming?DiscordFriendPresence::Online:
          status==discordpp::StatusType::Idle?DiscordFriendPresence::Idle:
          status==discordpp::StatusType::Dnd?DiscordFriendPresence::DoNotDisturb:DiscordFriendPresence::Offline;
      if(row.kind==DiscordFriendKind::Friend && row.presence!=DiscordFriendPresence::Offline && user->GameActivity())
        row.presence=DiscordFriendPresence::Playing;
      rows.push_back(std::move(row));
    }
    targets=std::move(current);
    voice_blocks.update(std::move(blocked));
    std::erase_if(handles,[&](const auto &entry){return !seen.contains(entry.first);});
    std::stable_sort(rows.begin(),rows.end(),[](const auto &a,const auto &b) {
      if(a.kind!=b.kind)return a.kind<b.kind;
      if(a.presence!=b.presence)return a.presence>b.presence;
      if(a.name!=b.name)return a.name<b.name;
      return a.handle<b.handle;
    });
    if(rows!=state.friends){state.friends=std::move(rows);changed();}
  }
  void connect(const std::string &token) {
    client=std::make_unique<discordpp::Client>();
    client->SetLogDir("",discordpp::LoggingSeverity::None);
    if(!application_id) {
      state.connecting=false;state.message="Discord application configuration is invalid.";changed();client.reset();return;
    }
    client->SetApplicationId(application_id);
    // A raw native executable omits the launcher's asset/configuration setup.
    // Register only the actual launcher supplied by that launcher process.
#if defined(_WIN32) || defined(__linux__)
    if(const auto *launcher=std::getenv("H2_DISCORD_LAUNCHER")) {
      const std::string path=launcher;std::error_code error;
      if(std::filesystem::path(path).is_absolute() && std::filesystem::is_regular_file(path,error) &&
         path.find('"')==std::string::npos && std::none_of(path.begin(),path.end(),[](unsigned char c){return c<32;}))
        launch_registered=client->RegisterLaunchCommand(application_id,"\""+path+"\" --skip-intro");
    }
#endif
    const auto weak=std::weak_ptr<bool>(alive);
    auto received=[this,weak](discordpp::ActivityInvite invite) {
      const auto live=weak.lock();if(!live || !*live || !client)return;
      if(state.ready)received_invite(std::move(invite));
      else if(state.connecting) {
        if(deferred_invites.size()==100)deferred_invites.erase(deferred_invites.begin());
        deferred_invites.push_back(std::move(invite));
      }
    };
    client->SetActivityInviteCreatedCallback(received);
    client->SetActivityInviteUpdatedCallback(received);
    client->SetActivityJoinCallback([this,weak](std::string secret) {
      const auto live=weak.lock();if(!live || !*live || !party_activity || secret.size()!=48)return;
      // AcceptActivityInvite supplies its own completion; an SDK join event
      // for that same user action must not race a second membership handoff.
      if(inbox->snapshot().busy)return;
      if(state.ready)party_activity->join(std::move(secret));
      else if(state.connecting && !pending_join)pending_join=std::move(secret);
    });
    auto dirty=[this,weak](auto...) {
      const auto live=weak.lock();if(live && *live)relationships_dirty=true;
    };
    client->SetRelationshipCreatedCallback(dirty);
    client->SetRelationshipDeletedCallback(dirty);
    client->SetRelationshipGroupsUpdatedCallback(dirty);
    client->SetUserUpdatedCallback(dirty);
    client->SetStatusChangedCallback([this,weak](auto status,auto,int) {
      const auto live=weak.lock();if(!live || !*live)return;
      state.ready=false;state.connecting=true;
      if(status==discordpp::Client::Status::Ready) {
        const auto user=client->GetCurrentUserV2();
        if(!user || std::to_string(user->Id())!=subject) {
          state.connecting=false;state.message="Use the same Discord account as your ONLINE identity.";
          state.friends.clear();state.invitations.clear();inbox->reset();deferred_invites.clear();
          if(party_activity)party_activity->reset();
          client->Disconnect();changed();return;
        }
        state.ready=true;state.connecting=false;state.message=launch_registered?"Discord friends connected.":
            "Discord friends connected. Open Halo 2 before accepting Discord invites on this device.";
        relationships_dirty=true;activity_valid=false;refresh();
        auto received=std::move(deferred_invites);deferred_invites.clear();
        for(auto &invite:received)received_invite(std::move(invite));
      } else if(status==discordpp::Client::Status::Disconnected) {
        state.connecting=false;state.friends.clear();targets.clear();
        state.message="Discord friends disconnected. Reconnect to try again.";
        if(party_activity)party_activity->reset();
        inbox->reset();state.invitations.clear();inbox_revision=0;
        deferred_invites.clear();
        queued_activity.reset();pending_join.reset();published_secret.clear();++presence_serial;
        presence_pending=false;activity_valid=false;state.busy=relationship_busy=false;
      }
      changed();
    });
    client->UpdateToken(discordpp::AuthorizationTokenType::Bearer,token,[this,weak](auto result) {
      const auto live=weak.lock();if(!live || !*live)return;
      if(result.Successful())client->Connect();
      else {state.connecting=false;state.message="Discord could not enable friends. Reconnect to try again.";changed();}
    });
  }
#endif
};
DiscordSocial::DiscordSocial(std::shared_ptr<DiscordLogin> identity,DiscordLogin::OpenBrowser browser,
    DiscordLoginOptions options,DiscordLogin::Http http)
  :impl_(std::make_unique<Impl>(std::move(identity),std::move(browser),std::move(options),std::move(http))) {}
DiscordSocial::~DiscordSocial(){impl_->stop();*impl_->alive=false;}
void DiscordSocial::disconnect(){impl_->stop();}
void DiscordSocial::enable() {
  auto &p=*impl_;p.stop();
  if(!p.state.available){p.state.message="Discord Social SDK is unavailable in this build.";p.changed();return;}
  const auto identity=p.identity?p.identity->snapshot().identity:std::nullopt;
  if(!identity){p.state.message="Sign into ONLINE before enabling Discord friends.";p.changed();return;}
  p.subject=identity->id;p.state.enabled=p.state.connecting=true;
#ifdef HALO2_DISCORD_SOCIAL_SDK
  if(const auto token=p.identity->social_authorization()) {
    p.using_identity=true;p.state.message="Connecting Discord friends...";p.changed();p.connect(*token);return;
  }
#endif
  p.state.message="Approve Discord friends access in your browser.";p.changed();p.consent.start();
}
void DiscordSocial::pump() {
  auto &p=*impl_;
  if(!p.state.enabled)return;
  const auto identity=p.identity?p.identity->snapshot().identity:std::nullopt;
  if(!identity || identity->id!=p.subject){p.stop();return;}
#ifdef HALO2_DISCORD_SOCIAL_SDK
  if(p.using_identity) {discordpp::RunCallbacks();p.refresh();p.pump_activity();return;}
#endif
  const auto consent=p.consent.snapshot();
  if(consent.status==DiscordLoginStatus::Failed || consent.status==DiscordLoginStatus::SignedOut) {
    const auto message=consent.message.empty()?"Discord friends authorization expired. Reconnect to try again.":consent.message;
    p.stop();p.state.message=message;p.changed();
    return;
  }
  if(consent.identity && consent.identity->id!=p.subject) {
    p.stop();p.state.message="Use the same Discord account as your ONLINE identity.";p.changed();return;
  }
#ifdef HALO2_DISCORD_SOCIAL_SDK
  if(consent.identity && !p.client)if(const auto token=p.consent.authorization())p.connect(*token);
  discordpp::RunCallbacks();p.refresh();p.pump_activity();
#endif
}
DiscordSocialSnapshot DiscordSocial::snapshot() const {return impl_->state;}
void DiscordSocial::presence(const std::string &name,bool playing) {
  auto &p=*impl_;
  if(p.activity_name!=name || p.activity_playing!=playing)p.activity_valid=false;
  p.activity_name=name;p.activity_playing=playing;
#ifdef HALO2_DISCORD_SOCIAL_SDK
  const auto link=p.party_activity?p.party_activity->link():std::nullopt;
  if(link && (link->version!=p.published_version || link->current_size!=p.published_size))p.activity_valid=false;
  if(!p.published_secret.empty() && (!link || link->join_secret!=p.published_secret)) {
    p.client->ClearRichPresence();p.published_secret.clear();p.activity_valid=false;
    ++p.presence_serial;p.presence_pending=false;p.queued_activity.reset();p.next_activity={};
  }
  if(!p.state.ready || p.presence_pending || p.queued_activity || std::chrono::steady_clock::now()<p.next_activity || p.activity_valid)return;
  p.write_presence(link);
#endif
}
void DiscordSocial::bind_account(std::shared_ptr<OnlineAccount> account) {
  auto &p=*impl_;
  if(p.party_activity)p.party_activity->reset();
  p.inbox->reset();p.inbox_revision=0;p.bound_account=account;
  p.state.invitations.clear();p.state.busy=p.relationship_busy;p.changed();
  p.queued_activity.reset();p.pending_join.reset();p.published_secret.clear();
  ++p.presence_serial;p.presence_pending=false;p.activity_valid=false;p.party_revision=0;
#ifdef HALO2_DISCORD_SOCIAL_SDK
  if(p.client)p.client->ClearRichPresence();
#endif
  p.party_activity=std::make_unique<DiscordPartyActivity>(std::move(account),
    [&p](const DiscordPartyLink &link,DiscordPartyActivity::Complete done) {
#ifdef HALO2_DISCORD_SOCIAL_SDK
      if(!p.state.ready || p.activity_playing){done(false);return;}
      if(p.presence_pending)p.queued_activity=std::make_pair(link,std::move(done));
      else p.write_presence(link,std::move(done));
#else
      (void)p;(void)link;done(false);
#endif
    },[&p](const std::string &subject,DiscordPartyActivity::Complete done) {
#ifdef HALO2_DISCORD_SOCIAL_SDK
      std::uint64_t id{};const auto parsed=std::from_chars(subject.data(),subject.data()+subject.size(),id);
      if(!p.state.ready || p.activity_playing || parsed.ec!=std::errc{} || parsed.ptr!=subject.data()+subject.size() || !id) {done(false);return;}
      bool friend_now{};
      for(const auto &relation:p.client->GetRelationships())
        if(relation.Id()==id && relation.DiscordRelationshipType()==discordpp::RelationshipType::Friend)friend_now=true;
      if(!friend_now){done(false);return;}
      p.client->SendActivityInvite(id,"",[done=std::move(done)](auto result){done(result.Successful());});
#else
      (void)p;(void)subject;done(false);
#endif
    });
}
bool DiscordSocial::invite_to_party(const std::string &handle) {
  auto &p=*impl_;if(p.state.busy || !p.party_activity)return false;
  const auto subject=party_recipient(handle);
  const bool accepted=subject && p.party_activity->invite(*subject);
#ifdef HALO2_DISCORD_SOCIAL_SDK
  p.pump_activity();
#endif
  return accepted;
}
bool DiscordSocial::accept_activity(const std::string &handle) {
  auto &p=*impl_;if(!p.state.ready)return false;
  const bool accepted=p.inbox->accept(handle);
#ifdef HALO2_DISCORD_SOCIAL_SDK
  p.pump_activity();
#endif
  return accepted;
}
bool DiscordSocial::dismiss_activity(const std::string &handle) {
  auto &p=*impl_;const bool dismissed=p.inbox->dismiss(handle);
#ifdef HALO2_DISCORD_SOCIAL_SDK
  p.pump_activity();
#endif
  return dismissed;
}
bool DiscordSocial::command(const std::string &handle,const std::string &action) {
#ifdef HALO2_DISCORD_SOCIAL_SDK
  auto &p=*impl_;if(!p.state.ready || p.state.busy || !p.targets.contains(handle))return false;
  const auto row=std::find_if(p.state.friends.begin(),p.state.friends.end(),[&](const auto &v){return v.handle==handle;});
  if(row==p.state.friends.end())return false;
  if((action=="accept" || action=="decline") && row->kind!=DiscordFriendKind::Incoming)return false;
  if(action=="cancel" && row->kind!=DiscordFriendKind::Outgoing)return false;
  if(action!="accept" && action!="decline" && action!="cancel")return false;
  const auto weak=std::weak_ptr<bool>(p.alive);
  auto done=[&p,weak](discordpp::ClientResult result) {
    const auto live=weak.lock();if(!live || !*live)return;
    p.relationship_busy=false;p.state.busy=(p.party_activity && p.party_activity->status().busy) || p.inbox->snapshot().busy;
    p.state.message=result.Successful()?"Discord friends updated.":"Discord could not update this friend request.";
    p.relationships_dirty=true;p.changed();p.refresh();
  };
  p.relationship_busy=p.state.busy=true;p.changed();const auto id=p.targets.at(handle);
  if(action=="accept")p.client->AcceptDiscordFriendRequest(id,done);
  else if(action=="decline")p.client->RejectDiscordFriendRequest(id,done);
  else p.client->CancelDiscordFriendRequest(id,done);
  return true;
#else
  (void)handle;(void)action;return false;
#endif
}
std::optional<std::string> DiscordSocial::party_recipient(const std::string &handle) const {
#ifdef HALO2_DISCORD_SOCIAL_SDK
  const auto &p=*impl_;if(!p.state.ready)return std::nullopt;
  const auto row=std::find_if(p.state.friends.begin(),p.state.friends.end(),[&](const auto &r){return r.handle==handle;});
  const auto target=p.targets.find(handle);
  if(row!=p.state.friends.end() && row->kind==DiscordFriendKind::Friend && target!=p.targets.end())
    return std::to_string(target->second);
#else
  (void)handle;
#endif
  return std::nullopt;
}
} // namespace halo2
