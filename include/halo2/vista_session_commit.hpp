#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Every field below is supplied by its owner. No peer, readiness, initial
// session, or clock is constructed here. Source: pinned Vista PC executable.
struct VistaSessionCountdownCommitState {
  std::uint8_t running{};             // Session +4C94.
  std::int32_t stage{};               // +4C98.
  std::uint8_t flag{};                // +4C9C.
  std::uint32_t reason{};             // +4CA0.
  std::int32_t reason_stage{};        // +4CA4.
  std::array<std::uint32_t,2> identity{}; // +4CA8/+4CAC.
  std::uint32_t revision{};           // Shared session revision +4C60.
  bool operator==(const VistaSessionCountdownCommitState&) const = default;
};
struct VistaSessionCommitContext {
  std::uint32_t session_state{}; // +73A0, not lifecycle/UI state.
  std::uint32_t local_peer{};    // +74.
  std::uint32_t host_peer{};     // +71C0.
  std::uint8_t local_flag{};     // +4C74; keep the source byte.
};
struct VistaSessionCountdownWrite {
  std::uint8_t running{};
  std::int32_t stage{};
  std::uint8_t flag{};
  std::uint32_t reason{};
  // Required only if the actual reason-1 branch dereferences it.
  std::optional<std::array<std::uint32_t,2>> identity;
};
// Literal 5C6853..5C6998. Sender identity is explicit; no network resolution.
// Returns original AL changed truth. Missing consumed identity rejects
// atomically with FormatError instead of emulating an invalid guest pointer.
bool vista_session_countdown_commit(VistaSessionCountdownCommitState&,
    const VistaSessionCommitContext&,std::uint32_t sender_peer,
    const VistaSessionCountdownWrite&);
struct VistaSessionLocalWriteResult { bool accepted{},changed{}; };
// Bounded local 5C96FB path: states 5..8 and actual host==local are required.
// Other contexts reject with FormatError, not a fabricated remote delivery.
// Original wrapper returns AL true even when its commit leaf changes nothing.
VistaSessionLocalWriteResult vista_session_countdown_local_write(
    VistaSessionCountdownCommitState&,const VistaSessionCommitContext&,
    const VistaSessionCountdownWrite&);

struct VistaSessionReadiness {
  bool ready{};
  std::uint32_t blocked_mask{};
  bool operator==(const VistaSessionReadiness&) const = default;
};
// Literal 5D76C5, after its exact 5C20C8 view selection. Count is session +84;
// status i is session +110+i*10C. Nonpositive count consumes no statuses.
// The x86 SHL bit index wraps modulo 32. Truncated positive spans reject.
VistaSessionReadiness vista_session_peer_readiness(std::int32_t count,
    std::span<const std::int32_t> statuses);

struct VistaSessionPhasePeer {
  std::uint8_t present{}; // +71C5+i*1C.
  std::uint8_t marked{};  // +71C6+i*1C; retained unless present is nonzero.
  bool operator==(const VistaSessionPhasePeer&) const = default;
};
struct VistaSessionPhaseState {
  std::uint32_t phase{};            // +4C68.
  std::uint32_t phase_revision{};   // +4C64.
  std::uint32_t phase_clock{};      // +4C6C.
  std::uint8_t notification{};      // +796C.
  std::uint32_t previous_phase{};   // +7970.
  std::uint32_t previous_clock{};   // +7974.
  bool operator==(const VistaSessionPhaseState&) const = default;
};
// Literal 5C596E: preserves peer marks, both revisions, previous phase and
// both independently supplied clock samples. Returns consumed clock count
// (0 for unchanged phase; otherwise 2). Shared revision is +4C60 above.
unsigned vista_session_phase_write(VistaSessionPhaseState&,
    std::uint32_t& shared_revision,std::uint32_t requested_phase,
    std::int32_t peer_count,std::span<VistaSessionPhasePeer> peers,
    std::array<std::uint32_t,2> clock_samples);

enum class VistaSessionZeroService { stage,preflight,readiness,phase,write,clock };
struct VistaSessionZeroCall {
  VistaSessionZeroService service{};
  std::array<std::uint32_t,5> arguments{};
  unsigned argument_count{};
  bool operator==(const VistaSessionZeroCall&) const = default;
};
enum class VistaSessionZeroAction { none,advance_phase,retry_stage };
struct VistaSessionZeroResult {
  VistaSessionZeroAction action{};
  std::array<VistaSessionZeroCall,6> calls{};
  unsigned call_count{};
  std::span<const VistaSessionZeroCall> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
// Literal 5D9C1A with its actual getters/commit/phase/readiness leaves joined.
// Caller must already be in the proved local-host states 5..8. Preflight is
// actual session +4C90. No load completion, task selection, peer construction,
// callbacks, external/reentrant writes, or gameplay activation is modeled.
VistaSessionZeroResult vista_session_countdown_zero(
    VistaSessionCountdownCommitState&,VistaSessionPhaseState&,
    const VistaSessionCommitContext&,std::uint32_t preflight,
    std::int32_t peer_count,std::span<const std::int32_t> statuses,
    std::span<VistaSessionPhasePeer> peers,
    std::array<std::uint32_t,2> phase_clock_samples);
} // namespace halo2
