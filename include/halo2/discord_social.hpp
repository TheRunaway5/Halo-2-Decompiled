#pragma once
#include "halo2/discord_login.hpp"
#include "halo2/discord_activity_inbox.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace halo2 {
class OnlineAccount;
enum class DiscordFriendKind { Friend, Incoming, Outgoing, Blocked };
enum class DiscordFriendPresence { Offline, Online, Idle, DoNotDisturb, Playing };
struct DiscordFriend {
  std::string handle, name, username;
  DiscordFriendKind kind{};
  DiscordFriendPresence presence{};
  bool operator==(const DiscordFriend &) const = default;
};
struct DiscordSocialSnapshot {
  bool available{}, enabled{}, connecting{}, ready{}, busy{};
  std::vector<DiscordFriend> friends;
  std::string message;
  std::uint64_t revision{};
  std::vector<DiscordGameInvite> invitations;
};
// Optional user-consented Social SDK owner. Main-thread pump/commands; no
// IDs, tokens, SDK handles or arbitrary SDK errors enter public snapshots.
class DiscordFriends {
public:
  virtual ~DiscordFriends() = default;
  virtual void enable() = 0;
  virtual void disconnect() = 0;
  virtual void pump() = 0;
  virtual void presence(const std::string &screen_name, bool playing) = 0;
  virtual DiscordSocialSnapshot snapshot() const = 0;
  virtual bool command(const std::string &handle, const std::string &action) = 0;
  virtual void bind_account(std::shared_ptr<OnlineAccount>) {}
  virtual bool invite_to_party(const std::string &) {return false;}
  virtual bool accept_activity(const std::string &) {return false;}
  virtual bool dismiss_activity(const std::string &) {return false;}
  // Private invitation handoff only; never put the subject in UI snapshots.
  virtual std::optional<std::string> party_recipient(const std::string &handle) const = 0;
};
class DiscordSocial final : public DiscordFriends {
public:
  DiscordSocial(std::shared_ptr<DiscordLogin>, DiscordLogin::OpenBrowser,
                DiscordLoginOptions = {}, DiscordLogin::Http = {});
  ~DiscordSocial() override;
  DiscordSocial(const DiscordSocial &) = delete;
  DiscordSocial &operator=(const DiscordSocial &) = delete;
  void enable() override;
  void disconnect() override;
  void pump() override;
  void presence(const std::string &screen_name, bool playing) override;
  DiscordSocialSnapshot snapshot() const override;
  bool command(const std::string &handle, const std::string &action) override;
  void bind_account(std::shared_ptr<OnlineAccount>) override;
  bool invite_to_party(const std::string &handle) override;
  bool accept_activity(const std::string &handle) override;
  bool dismiss_activity(const std::string &handle) override;
  std::optional<std::string> party_recipient(const std::string &handle) const override;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
