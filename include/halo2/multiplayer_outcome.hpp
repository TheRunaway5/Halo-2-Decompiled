#pragma once
#include "halo2/multiplayer.hpp"
#include <optional>

namespace halo2 {
struct SourceAuthorityRetainedWorld;
// Native runtime termination observations, not original session/lifecycle
// states. Finishing a match does not itself terminate the current runtime.
enum class MultiplayerExitReason {
  headless_limit, frame_limit, captured_tick, window_quit, escape, session_world_close,
  online_postgame, online_disconnected
};
enum class MultiplayerOutcomeAuthority { local, host, client_snapshot };

// Views are borrowed only for the duration of retain_multiplayer_outcome.
// Local/host callers supply their actual match and recording banks. The
// recorded bank may be absent. A client snapshot supplies no local banks.
struct MultiplayerOutcomeBanksView {
  const SourceGameEndStats* match_statistics{};
  const SourceResultStatistics* recorded_statistics{};
  const SourceResultRecording* result_recording{};
  const SourceResultEventState* recorded_events{};
};
struct MultiplayerOutcomeBanks {
  SourceGameEndStats match_statistics;
  std::optional<SourceResultStatistics> recorded_statistics;
  SourceResultRecording result_recording;
  std::optional<SourceResultEventState> recorded_events;
};
struct MultiplayerRuntimeOutcome {
  MultiplayerExitReason reason{};
  MultiplayerOutcomeAuthority authority{};
  // A client can terminate before receiving any host snapshot. Preserve that
  // absence rather than copying the runtime's local placeholder as host data.
  // Actual local/host outcomes always contain their authoritative state.
  std::optional<MultiplayerState> state;
  // Present only for actual authoritative local/host matches. A client's
  // snapshot results_finalized flag does not create an authoritative bank.
  std::optional<MultiplayerOutcomeBanks> authoritative_banks;
};

// Owns deep copies without interpreting, completing, remapping or publishing
// any source bank. The caller supplies the last actual exit reason, including
// capture/event ordering. Local/host require a nonnull state; a client may
// supply nullptr only when no host snapshot was received. Unsupported
// owner/reason or bank combinations throw FormatError. No clock, readiness,
// identity, phase, PCR or navigation is
// manufactured; retaining an outcome performs no runtime transition.
MultiplayerRuntimeOutcome retain_multiplayer_outcome(MultiplayerExitReason,
    MultiplayerOutcomeAuthority,const MultiplayerState*,
    MultiplayerOutcomeBanksView = {});
// Optional application-owned handoff. A bound real session lifecycle can
// request teardown through MultiplayerOptions::session_world_update. The
// runtime does not select a report or interpret game-over as permission.
int run_multiplayer(const MultiplayerOptions&,MultiplayerRuntimeOutcome&);
// Application-owned recovery handoff on an ONLINE client disconnect. Drain
// the already admitted native replay before teardown, keeping this complete
// validated world separate from the client's display/PCR outcome. No host is
// elected or promoted. The output is cleared before starting a new runtime.
int run_multiplayer(const MultiplayerOptions&,MultiplayerRuntimeOutcome&,
    std::shared_ptr<const SourceAuthorityRetainedWorld>&);
} // namespace halo2
