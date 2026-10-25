#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

namespace halo2 {
// Discovery metadata only. No account identifiers or admission credentials.
struct OnlineServer {
  // id is the origin/server pair used by browser focus and selection caches.
  std::string id, name, host, map, gametype, game_version, region, server_type;
  unsigned port{}, players{}, max_players{};
  std::int64_t expires_at{};
  std::string origin_id, server_id;
};
enum class OnlineDirectoryStatus { Unconfigured, Idle, Loading, Ready, Failed };
struct OnlineDirectorySnapshot {
  OnlineDirectoryStatus status = OnlineDirectoryStatus::Unconfigured;
  std::vector<OnlineServer> servers;
  std::string message;
  std::uint64_t revision{};
};
struct OnlineDirectoryOptions {
  std::vector<std::string> masters;
  bool allow_loopback_http{};
};
// H2_MASTER_URLS: comma/whitespace-separated HTTPS origins, in failover order.
// If absent, use the Halo To Forever master; an explicit empty value disables it.
// H2_DIRECTORY_ALLOW_LOOPBACK_HTTP=1 admits 127.0.0.1 / ::1 development masters.
OnlineDirectoryOptions online_directory_environment();
struct OnlineDirectoryResponse { long status{}; std::string body; };
class OnlineDirectory {
public:
  using Http = std::function<OnlineDirectoryResponse(const std::string &, std::stop_token)>;
  using Now = std::function<std::int64_t()>;
  explicit OnlineDirectory(OnlineDirectoryOptions = {}, Http = {}, Now = {});
  ~OnlineDirectory();
  OnlineDirectory(const OnlineDirectory &) = delete;
  OnlineDirectory &operator=(const OnlineDirectory &) = delete;
  void refresh(); // Nonblocking; coalesces refreshes while a request is active.
  void cancel();  // Discards cached results and invalidates outstanding replies.
  OnlineDirectorySnapshot snapshot() const; // Expired listings are removed here too.
  std::optional<OnlineDirectorySnapshot> snapshot_after(std::uint64_t revision) const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
