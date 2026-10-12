#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace halo2 {
inline constexpr std::string_view vista_session_countdown_source_sha256 =
    "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109";

enum class VistaSessionCountdownService {
  ui_state, controller_user, host_permission, active_stage,
  session_stage, session_flag, clock, elapsed, write
};
struct VistaSessionCountdownCall {
  VistaSessionCountdownService service{};
  std::array<std::uint32_t,5> arguments{};
  unsigned argument_count{};
  bool operator==(const VistaSessionCountdownCall&) const = default;
};
struct VistaSessionCountdownResult {
  // Both routines consume only AL. The timer always returns false.
  bool succeeded{};
  std::array<VistaSessionCountdownCall,6> calls{};
  unsigned call_count{};
  std::span<const VistaSessionCountdownCall> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};

struct VistaSessionCountdownStartRequest {
  std::int32_t controller{};
  std::int32_t requested_stage{};
  std::int32_t minimum_stage{};
};
struct VistaSessionCountdownStartQueries {
  std::int32_t ui_state{}; //616490(NULL), not an inferred readiness state.
  std::int32_t controller_user=-1; //60687F; only literal NONE is rejected here.
  bool host_permission{}; //5AD940 AL truth value.
  std::int32_t active_stage{}; //616B7A EAX.
  bool write_succeeded{}; //5B11E2 AL truth value.
};
// Literal617328..6173A5 service-intent adapter. The actual lobby643A40 requests
// minimum3 and615A4B returns10, but this interface keeps its source arguments.
// It owns no controller table, readiness producer, network write, or live UI.
VistaSessionCountdownResult vista_session_countdown_start(
    const VistaSessionCountdownStartRequest&,
    const VistaSessionCountdownStartQueries&);

struct VistaSessionCountdownTaskState {
  std::int32_t last_stage=-1; //Task+14, initialized by original5D9B62.
  std::uint32_t last_clock{}; //Task+10, initialized by original5D9B62.
};
struct VistaSessionCountdownTimerQueries {
  std::int32_t session_stage{}; //5C25DF EAX.
  std::uint8_t session_flag{}; //5C261F AL; preserve the byte, do not normalize.
  // Actual network clock samples at the corresponding original service calls.
  // Unconsumed samples are ignored. These are not fabricated frame timestamps.
  std::uint32_t stage_change_clock{};
  std::uint32_t elapsed_clock{};
  std::uint32_t decrement_clock{};
};
// Literal5D9BA7..5D9C1A. The caller's task/session admission is NOT implemented.
// 5B3CB4 subtracts last_clock with uint32 wrap; originalJL compares its signed
// result with1000. A positive stage decrements at most once per admitted call.
// Original5C96FB arguments1/3 consume bytes. The original scratch argument3
// has unowned upper24 bits; intents represent only its consumed flag byte.
// External/reentrant callback effects and network delivery remain unowned.
VistaSessionCountdownResult vista_session_countdown_timer(
    VistaSessionCountdownTaskState&,const VistaSessionCountdownTimerQueries&);
} // namespace halo2
