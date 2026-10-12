#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace halo2 {
class OnlineAccount;
// Private transport data. This is never a menu row or a social snapshot.
struct DiscordPartyLink {
  std::string party_id, join_secret;
  std::uint64_t version{};
  unsigned current_size{}, max_size{};
  std::chrono::steady_clock::time_point expires{};
};
struct DiscordPartyActivityStatus {
  bool busy{};
  std::string message;
  std::uint64_t revision{};
};
// A user action starts exactly one server-invite -> presence -> SDK-invite
// sequence. SDK completion callbacks cannot start another sequence on their own.
// The SDK adapter supplies the two asynchronous operations, allowing the real
// account transport and cancellation policy to be checked without Discord DMs.
class DiscordPartyActivity {
public:
  using Complete = std::function<void(bool)>;
  using Publish = std::function<void(const DiscordPartyLink &, Complete)>;
  using Send = std::function<void(const std::string &, Complete)>;
  DiscordPartyActivity(std::shared_ptr<OnlineAccount>, Publish, Send);
  ~DiscordPartyActivity();
  DiscordPartyActivity(const DiscordPartyActivity &) = delete;
  DiscordPartyActivity &operator=(const DiscordPartyActivity &) = delete;
  bool invite(const std::string &verified_friend_subject);
  // Called only after the user accepts an invite in Discord or the game.
  bool join(std::string secret);
  void pump(bool playing);
  void reset();
  DiscordPartyActivityStatus status() const;
  std::optional<DiscordPartyLink> link() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
