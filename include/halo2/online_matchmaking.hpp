#pragma once
#include "halo2/discord_login.hpp"
#include "halo2/online_match_session.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

namespace halo2 {
struct OnlinePlaylist {
  std::string id, revision, name, description;
  bool ranked{}, quickmatch{}, eligible{};
  unsigned minimum_players{}, maximum_players{};
  std::optional<unsigned> level;
  bool clan_match{};
};
struct OnlineMatchPlayer { std::string name; unsigned team{}; bool self{}; };
struct OnlineMatch { std::string map, variant; std::vector<OnlineMatchPlayer> players; };
struct OnlineGatheringPlayer { std::string name; bool self{}; };
struct OnlineGatheringProgress {
  std::vector<OnlineGatheringPlayer> players;
  unsigned minimum_players{}, maximum_players{};
  // Gathering growth wait, never the subsequent game-start countdown.
  std::optional<unsigned> wait_ms;
};
enum class OnlineMatchmakingStatus {
  Unconfigured, Idle, Connecting, Ready, Joining, Searching,
  CheckingConnections, AwaitingTransport, Loading, Failed
};
struct OnlineMatchmakingSnapshot {
  OnlineMatchmakingStatus status = OnlineMatchmakingStatus::Unconfigured;
  std::vector<OnlinePlaylist> playlists;
  std::string playlist_id, message;
  std::optional<OnlineMatch> match;
  std::optional<OnlineGatheringProgress> gathering;
  unsigned queued_players{};
  std::uint64_t revision{};
};
struct OnlineMatchmakingOptions {
  std::string endpoint, compatibility = "halo2-native-queue-v8";
  std::vector<std::string> maps;
  bool allow_loopback_http{};
  std::chrono::milliseconds poll_interval = std::chrono::seconds(1);
  unsigned language = 0; // Current English frontend. Retail title index, not OS/Discord locale.
};
// One pinned queue owner. Directory failover must never duplicate live queues.
// H2_MATCHMAKING_URL; H2_MATCHMAKING_ALLOW_LOOPBACK_HTTP=1 for development only.
// If absent, use the Halo To Forever master; an explicit empty value disables it.
OnlineMatchmakingOptions online_matchmaking_environment(const std::filesystem::path &maps);
struct OnlineMatchmakingRequest { std::string method, url, body, bearer; };
struct OnlineMatchmakingResponse { long status{}; std::string body; };
class OnlineMatchmaking {
public:
  using Http = std::function<OnlineMatchmakingResponse(const OnlineMatchmakingRequest &, std::stop_token)>;
  OnlineMatchmaking(std::shared_ptr<DiscordLogin>, OnlineMatchmakingOptions = {}, Http = {});
  ~OnlineMatchmaking();
  OnlineMatchmaking(const OnlineMatchmaking &) = delete;
  OnlineMatchmaking &operator=(const OnlineMatchmaking &) = delete;
  void refresh(const std::string &screen_name);
  void join(const std::string &playlist_id);
  void cancel_queue();
  void sign_out();
  OnlineMatchmakingSnapshot snapshot() const;
  std::optional<OnlineMatchmakingSnapshot> snapshot_after(std::uint64_t) const;
  // Transfer the authenticated connection across frontend teardown. No bearer
  // or account subject is ever copied into a presentation snapshot.
  std::shared_ptr<OnlineMatchSession> take_launch();
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
