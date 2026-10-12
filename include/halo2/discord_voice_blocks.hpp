#pragma once
#include "halo2/discord_login.hpp"
#include <functional>
#include <vector>

namespace halo2 {
struct DiscordVoiceBlockOptions {
  std::string endpoint;
  bool allow_loopback_http{};
  std::chrono::milliseconds retry_interval{std::chrono::seconds(5)};
};
DiscordVoiceBlockOptions discord_voice_block_environment();
struct DiscordVoiceBlockRequest { std::string url, bearer, body; };
struct DiscordVoiceBlockResponse { long status{}; std::string body; };
// Memory-only, private SDK-to-service handoff. This owner survives world/menu
// teardown, coalesces full block snapshots and retries an uncertain PUT.
// Disconnect forgets credentials/work; it does not unblock stored preferences.
class DiscordVoiceBlocks {
public:
  using Http=std::function<DiscordVoiceBlockResponse(const DiscordVoiceBlockRequest &,std::stop_token)>;
  DiscordVoiceBlocks(std::shared_ptr<DiscordLogin>,DiscordVoiceBlockOptions,Http={});
  ~DiscordVoiceBlocks();
  bool update(std::vector<std::string> subjects);
  void reset();
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
