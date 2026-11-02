#pragma once
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Original Xbox TU1.5 image 21b207e7...409c4fd: complete data decisions
// 0x62DE0/0x721B0, post-host-leave eligibility 0x72150..0x721A6, and
// lobby error selector 0x256AF7. Policy is owned by Rust; these declarations
// expose typed native observations. Platform transactions remain owner inputs.
struct OriginalMatchmakingPeer { std::uint32_t connections{}; };
struct OriginalMatchmakingSeat { bool occupied{}; unsigned peer{}; std::int8_t team=-1; };
bool original_matchmaking_connected_teams(std::span<const OriginalMatchmakingPeer> peers,
    unsigned current_peer,std::span<const OriginalMatchmakingSeat> seats);
struct OriginalMatchmakingEligibility { bool ready{}, defer{}; };
// The source's original Live policy bit is deliberately explicit. Replacement
// Discord/EOS authentication supplies no equivalent bit and must not set it.
OriginalMatchmakingEligibility original_matchmaking_eligibility(bool require_teams,bool connected_teams,
    bool check_original_local_policy,bool original_live_logon_succeeded,bool original_local_bit31,
    bool disable_teams=false,bool disable_local_policy=false);
std::optional<unsigned> original_matchmaking_error_dialog(std::int32_t state,bool local_cancel=false);
// Original unsigned UI clock subtraction, with state-clear after 3000 ms.
bool original_matchmaking_error_clear(bool local_host,std::int32_t state,
    std::uint32_t shown_time,std::uint32_t now);

// The admitted native transport supplies loading and roster facts. The Rust
// owner applies the separate native handoff policy and original team gate.
struct NativeMatchmakingLaunchObservation {
  bool session_present{},transferred{},loading{},start_supported{};
  std::optional<unsigned> start_ms;
  std::optional<bool> team_mode;
  unsigned current_peer{};
};
bool native_matchmaking_launch_ready(const NativeMatchmakingLaunchObservation &,
    std::span<const OriginalMatchmakingSeat>);
} // namespace halo2
