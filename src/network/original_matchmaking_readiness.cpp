#include "halo2/original_matchmaking_readiness.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <vector>

namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
std::vector<bridge::MatchmakingSeat> encode(std::span<const OriginalMatchmakingSeat> seats) {
  std::vector<bridge::MatchmakingSeat> rows;rows.reserve(seats.size());
  for(const auto &seat:seats)rows.push_back({seat.occupied,seat.peer,seat.team});
  return rows;
}
}
bool original_matchmaking_connected_teams(std::span<const OriginalMatchmakingPeer> peers,
    unsigned current_peer,std::span<const OriginalMatchmakingSeat> seats) {
  std::vector<std::uint32_t> masks;masks.reserve(peers.size());
  for(const auto &peer:peers)masks.push_back(peer.connections);
  const auto rows=encode(seats);
  return bridge::matchmaking_connected_teams({masks.data(),masks.size()},current_peer,{rows.data(),rows.size()});
}
OriginalMatchmakingEligibility original_matchmaking_eligibility(bool require_teams,bool connected_teams,
    bool check_original_local_policy,bool original_live_logon_succeeded,bool original_local_bit31,
    bool disable_teams,bool disable_local_policy) {
  const auto result=bridge::matchmaking_eligibility(require_teams,connected_teams,check_original_local_policy,
    original_live_logon_succeeded,original_local_bit31,disable_teams,disable_local_policy);
  return {result.ready,result.defer};
}
std::optional<unsigned> original_matchmaking_error_dialog(std::int32_t state,bool local_cancel) {
  const auto result=bridge::matchmaking_error_dialog(state,local_cancel);
  if(result.present)return result.id;
  return {};
}
bool original_matchmaking_error_clear(bool local_host,std::int32_t state,std::uint32_t shown,std::uint32_t now) {
  return bridge::matchmaking_error_clear(local_host,state,shown,now);
}
bool native_matchmaking_launch_ready(const NativeMatchmakingLaunchObservation &observation,
    std::span<const OriginalMatchmakingSeat> seats) {
  const bridge::MatchmakingLaunchObservation input{observation.session_present,observation.transferred,
    observation.loading,observation.start_supported,observation.start_ms.has_value(),observation.start_ms.value_or(0),
    observation.team_mode.has_value(),observation.team_mode.value_or(false),observation.current_peer};
  const auto rows=encode(seats);
  return bridge::matchmaking_native_launch_ready(input,{rows.data(),rows.size()});
}
} // namespace halo2
