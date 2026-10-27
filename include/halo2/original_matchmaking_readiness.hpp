#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Original Xbox TU1.5 image 21b207e7...409c4fd: complete data decisions
// 0x62DE0/0x721B0, post-host-leave eligibility 0x72150..0x721A6, and
// lobby error selector 0x256AF7. Platform transactions remain owner inputs.
struct OriginalMatchmakingPeer { std::uint32_t connections{}; };
struct OriginalMatchmakingSeat { bool occupied{}; unsigned peer{}; std::int8_t team=-1; };
inline bool original_matchmaking_connected_teams(std::span<const OriginalMatchmakingPeer> peers,
    unsigned current_peer,std::span<const OriginalMatchmakingSeat> seats) {
  if(peers.empty() || peers.size()>16 || current_peer>=peers.size())return false;
  const auto complete=(1u<<peers.size())-1;
  std::uint32_t connected=1u<<current_peer;
  for(unsigned i=0;i<peers.size();++i)if(peers[i].connections==complete)connected|=1u<<i;
  std::array<std::uint16_t,16> represented{};
  for(const auto &seat:seats)if(seat.occupied && seat.peer<16 && (connected&(1u<<seat.peer)) && seat.team>=0 && seat.team<16)
    represented[unsigned(seat.team)]|=std::uint16_t(1u<<seat.peer);
  unsigned count=0;for(const auto team:represented)count+=team!=0;
  return count>1;
}
struct OriginalMatchmakingEligibility { bool ready{}, defer{}; };
// The source's original Live policy bit is deliberately explicit. Replacement
// Discord/EOS authentication supplies no equivalent bit and must not set it.
inline OriginalMatchmakingEligibility original_matchmaking_eligibility(bool require_teams,bool connected_teams,
    bool check_original_local_policy,bool original_live_logon_succeeded,bool original_local_bit31,
    bool disable_teams=false,bool disable_local_policy=false) {
  const bool defer=(require_teams && !disable_teams && !connected_teams) ||
      (check_original_local_policy && !disable_local_policy && original_live_logon_succeeded && original_local_bit31);
  return {!defer,defer};
}
inline std::optional<unsigned> original_matchmaking_error_dialog(std::int32_t state,bool local_cancel=false) {
  if(state==0 || state==2 || state==3 || (state==4 && local_cancel))return {};
  if(state==5)return 0xac;
  if(state==4)return 0xad;
  if(state==6 || state==14 || state==19)return 0xae;
  return 0xab;
}
// Original unsigned UI clock subtraction, with state-clear after 3000 ms.
inline bool original_matchmaking_error_clear(bool local_host,std::int32_t state,
    std::uint32_t shown_time,std::uint32_t now) {
  return shown_time!=0 && local_host && state!=0 && now-shown_time>=3000;
}
} // namespace halo2
