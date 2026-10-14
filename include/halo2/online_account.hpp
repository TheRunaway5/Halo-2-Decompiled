#pragma once
#include "halo2/online_matchmaking.hpp"
#include "halo2/online_voice.hpp"
#include "halo2/discord_party_activity.hpp"
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
  struct Local { std::string name; bool guest{}; };
  std::vector<Local> local_players;
  int local_controller=-1; // Native presentation only; never serialized.
  bool guest{};
};
struct OnlinePartyInvite { std::string id, name; std::uint64_t version{}; };
struct OnlineParty {
  std::uint64_t version{};
  bool leader{};
  std::vector<OnlinePartyMember> members;
  std::vector<OnlineClanInvite> outgoing;
};
struct OnlineAccountSnapshot {
  bool authenticated{}, busy{};
  std::string message;
  std::optional<OnlineClan> clan;
  std::vector<OnlineClanInvite> invitations;
  std::uint64_t revision{};
  std::optional<OnlineParty> party;
  std::vector<OnlinePartyInvite> party_invitations;
  std::string party_message;
};
struct OnlineAccountOptions {
  std::string endpoint;
  bool allow_loopback_http{};
  std::chrono::milliseconds poll_interval = std::chrono::seconds(5);
  bool enable_parties{};
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
};
// Credentials stay in the worker; snapshots contain only social display data.
class OnlineAccount {
public:
  using Http = OnlineMatchmaking::Http;
  OnlineAccount(std::shared_ptr<DiscordLogin>, OnlineAccountOptions, Http = {});
  ~OnlineAccount();
  void connect(const std::string &screen_name);
  void sign_out();
  bool command(OnlineClanCommand);
  bool party_command(OnlinePartyCommand);
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
