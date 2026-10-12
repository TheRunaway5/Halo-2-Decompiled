#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace halo2 {
struct DiscordGameInvite {
  std::string handle, name, status="Game invitation";
  bool operator==(const DiscordGameInvite &) const = default;
};
struct DiscordActivityInboxSnapshot {
  bool busy{};
  std::vector<DiscordGameInvite> invitations;
  std::string message;
  std::uint64_t revision{};
};
// Owns private SDK invitation records. Only opaque handles and display names
// leave this owner; accepting requires a separate explicit user action.
class DiscordActivityInbox {
public:
  using Complete=std::function<void(bool,std::string)>;
  using Accept=std::function<void(Complete)>;
  using Join=std::function<bool(std::string)>;
  // Empty permits acceptance; otherwise return a fixed, user-facing reason.
  using CanAccept=std::function<std::string()>;
  using Now=std::function<std::chrono::steady_clock::time_point()>;
  DiscordActivityInbox(Join,CanAccept,Now={});
  ~DiscordActivityInbox();
  DiscordActivityInbox(const DiscordActivityInbox &)=delete;
  DiscordActivityInbox &operator=(const DiscordActivityInbox &)=delete;
  void received(std::string private_key,std::string name,bool valid,Accept);
  bool accept(const std::string &handle);
  bool dismiss(const std::string &handle);
  void pump();
  void reset();
  DiscordActivityInboxSnapshot snapshot() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
