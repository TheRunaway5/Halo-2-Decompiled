#include "halo2/multiplayer_outcome.hpp"
#include "halo2/cache.hpp"
#include <cstring>
#include <type_traits>

namespace halo2 {
namespace {
template<class T> void retain_bank(T& destination,const T& source) {
  static_assert(std::is_trivially_copyable_v<T>);
  // Preserve the complete source-owned representation, including opaque
  // fields. These banks have no native pointer/container ownership.
  std::memcpy(&destination,&source,sizeof(T));
}
}
MultiplayerRuntimeOutcome retain_multiplayer_outcome(MultiplayerExitReason reason,
    MultiplayerOutcomeAuthority authority,const MultiplayerState* state,
    MultiplayerOutcomeBanksView banks) {
  switch(reason) {
    case MultiplayerExitReason::headless_limit:
    case MultiplayerExitReason::frame_limit:
    case MultiplayerExitReason::captured_tick:
    case MultiplayerExitReason::window_quit:
    case MultiplayerExitReason::escape: break;
    case MultiplayerExitReason::session_world_close: break;
    case MultiplayerExitReason::online_postgame:
    case MultiplayerExitReason::online_disconnected: break;
    default: throw FormatError("Unknown native multiplayer exit reason");
  }
  switch(authority) {
    case MultiplayerOutcomeAuthority::local:
    case MultiplayerOutcomeAuthority::host:
      if(!state)
        throw FormatError("Authoritative multiplayer outcome requires its actual state");
      if(!banks.match_statistics || !banks.result_recording)
        throw FormatError("Authoritative multiplayer outcome requires its actual match and recording banks");
      break;
    case MultiplayerOutcomeAuthority::client_snapshot:
      if(banks.match_statistics || banks.recorded_statistics || banks.result_recording || banks.recorded_events)
        throw FormatError("Client multiplayer outcome cannot retain local authoritative banks");
      break;
    default: throw FormatError("Unknown native multiplayer outcome authority");
  }
  MultiplayerRuntimeOutcome result;
  result.reason=reason;
  result.authority=authority;
  if(state)result.state.emplace(*state);
  if(authority!=MultiplayerOutcomeAuthority::client_snapshot) {
    auto& retained=result.authoritative_banks.emplace();
    retain_bank(retained.match_statistics,*banks.match_statistics);
    retain_bank(retained.result_recording,*banks.result_recording);
    if(banks.recorded_statistics)
      retain_bank(retained.recorded_statistics.emplace(),*banks.recorded_statistics);
    if(banks.recorded_events)
      retain_bank(retained.recorded_events.emplace(),*banks.recorded_events);
  }
  return result;
}
} // namespace halo2
