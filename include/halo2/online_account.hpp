#pragma once
#include "halo2/online_matchmaking.hpp"
#include "halo2/online_voice.hpp"
#include "halo2/discord_party_activity.hpp"
#include "halo2/frontend_appearance.hpp"
namespace halo2 {
enum class OnlineClanPresence { Unknown, Offline, Online, Playing };
struct OnlineClanMember {
  std::string id, name;
  unsigned role{};
  bool self{};
  OnlineClanPresence presence = OnlineClanPresence::Unknown;
};
struct OnlineClanInvite { std::string id, name; };
struct OnlineClanMessage { std::string id, name, body; };
struct OnlineClan {
  std::string id, name;
  unsigned role{};
  std::vector<OnlineClanMember> members;
  std::vector<OnlineClanInvite> outgoing;
  std::vector<OnlineClanMessage> messages;
};
struct OnlinePartyMember {
  std::string handle, name, ready_playlist;
  bool self{}, leader{}, online{};
  struct Local { std::string name; bool guest{}; std::optional<FrontendAppearance> appearance; };
  std::vector<Local> local_players;
  int local_controller=-1; // Native presentation only; never serialized.
  bool guest{};
  std::optional<FrontendAppearance> appearance;
  // Server presence: an open connection to a completed match's post-game
  // room. Absent when the service does not report it.
  std::optional<bool> postgame;
};
struct OnlinePartyInvite { std::string id, name; std::uint64_t version{}; };
struct OnlineSteamFriend { std::string id,name; bool registered{},halo_online{}; };
struct OnlineFriend {
  std::string id,name,tag,state,presence;
  std::optional<bool> party_joinable;
  std::string party_context;
};
struct OnlineParty {
  struct Privacy {
    unsigned mode{};
    std::uint64_t revision{};
    std::string context;
    bool operator==(const Privacy&) const = default;
  };
  struct Countdown {
    std::uint64_t serial{};
    std::optional<std::int32_t> seconds;
    unsigned mode{};
    bool complete{};
    std::string requester;
    bool operator==(const Countdown&) const = default;
  };
  std::uint64_t version{};
  bool leader{};
  std::vector<OnlinePartyMember> members;
  std::vector<OnlineClanInvite> outgoing;
  bool countdown_supported{};
  std::optional<Countdown> countdown;
  // Absent on legacy services: no public admission policy is inferred.
  std::optional<Privacy> privacy;
};
// Retail party members follow the host's PCR Done: 5C884A(session,1) moves
// the whole session to pregame and every machine's observer opens lobby14.
// A member who is not the leader is that follower here. The service reports
// who still holds the completed match's room open; the leader has left it
// once this member is there and the leader is not.
inline bool online_party_follower(const OnlineParty &p) { return !p.leader && p.members.size()>1; }
inline bool online_party_leader_left_postgame(const OnlineParty &p) {
  if(!online_party_follower(p))return false;
  bool self{},leader{};
  for(const auto &m:p.members) {
    if(m.self)self=m.postgame==true;
    if(m.leader)leader=m.postgame==false;
  }
  return self && leader;
}
struct OnlineAccountSnapshot {
  bool authenticated{}, busy{};
  std::string message;
  std::optional<OnlineClan> clan;
  std::vector<OnlineClanInvite> invitations;
  std::uint64_t revision{};
  std::optional<OnlineParty> party;
  std::vector<OnlinePartyInvite> party_invitations;
  std::string party_message;
  std::uint64_t party_command_serial{};
  std::optional<bool> party_command_successful;
  std::string profile_message;
  std::vector<OnlineSteamFriend> steam_friends;
  std::string steam_friends_state,steam_friends_message;
  std::string friend_tag,friends_message;
  bool friends_available{};
  std::vector<OnlineFriend> friends;
  std::optional<OnlineFriend> friend_search;
};
struct OnlineAccountOptions {
  std::string endpoint;
  bool allow_loopback_http{};
  std::chrono::milliseconds poll_interval = std::chrono::seconds(5);
  bool enable_parties{};
  bool profile_only{};
};
OnlineAccountOptions online_account_environment();
struct OnlineClanCommand {
  std::string action, value;
  unsigned role{};
  bool confirmed{};
};
struct OnlinePartyCommand {
  // join_activity carries a private SDK join secret in value. It never uses
  // a client-selected party version/roster or exposes the secret in snapshots.
  std::string action, value, playlist, revision;
  std::uint64_t version{};
  std::optional<unsigned> privacy;
  std::uint64_t privacy_revision{};
  std::string privacy_context;
  std::string join_context;
};
// Credentials stay in the worker; snapshots contain only social display data.
class OnlineAccount {
public:
  using Http = OnlineMatchmaking::Http;
  OnlineAccount(std::shared_ptr<DiscordLogin>, OnlineAccountOptions, Http = {});
  ~OnlineAccount();
  void connect(const std::string &screen_name);
  void profile(const FrontendAppearance &, const std::string &service_tag);
  std::shared_ptr<OnlineAccount> profile_worker(std::shared_ptr<DiscordLogin>) const;
  void sign_out();
  bool command(OnlineClanCommand);
  bool party_command(OnlinePartyCommand);
  void refresh_steam_friends();
  bool friend_command(const std::string &action,const std::string &value);
  OnlineAccountSnapshot snapshot() const;
private:
  friend class OnlineVoice;
  friend class DiscordPartyActivity;
  struct DiscordOperation {
    std::uint64_t serial{};
    bool pending{}, successful{};
    std::optional<DiscordPartyLink> link;
  };
  std::uint64_t begin_discord_invite(const std::string &subject);
  std::uint64_t begin_discord_join(const std::string &secret);
  std::optional<DiscordOperation> discord_operation(std::uint64_t serial) const;
  std::optional<DiscordPartyLink> discord_party_link() const;
  std::optional<VoiceAuthorization> voice_authorization(bool party) const;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
