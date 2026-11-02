#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
namespace halo2::online_protocol_bridge {
struct FrontendLobbyFacts;
struct FrontendLobbyObservation;
struct FrontendLobbyInputEffects;
}
namespace halo2 {
// Value-semantics native handle for the Rust pregame controller. Native UI
// applies returned effects after Rust returns, including reentrant open().
class FrontendMatchmakingLobby {
public:
  FrontendMatchmakingLobby();
  ~FrontendMatchmakingLobby();
  FrontendMatchmakingLobby(const FrontendMatchmakingLobby &);
  FrontendMatchmakingLobby &operator=(const FrontendMatchmakingLobby &);
  FrontendMatchmakingLobby(FrontendMatchmakingLobby &&) noexcept;
  FrontendMatchmakingLobby &operator=(FrontendMatchmakingLobby &&) noexcept;
  std::int32_t seconds() const;
  bool matchmaking() const;
  bool leave_pending() const;
  void cancel();
  void exit();
  void enter(const online_protocol_bridge::FrontendLobbyFacts &);
  online_protocol_bridge::FrontendLobbyObservation observe(const online_protocol_bridge::FrontendLobbyFacts &);
  std::string start_error(const online_protocol_bridge::FrontendLobbyFacts &) const;
  using Submit = std::function<bool(unsigned, const std::string &, const std::string &)>;
  online_protocol_bridge::FrontendLobbyInputEffects input(
      const online_protocol_bridge::FrontendLobbyFacts &, unsigned, int, const Submit &);
  bool observe_leave(const online_protocol_bridge::FrontendLobbyFacts &);
  void left();
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
