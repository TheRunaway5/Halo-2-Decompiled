#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Supplied PC image SHA256
// 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// Lifecycle slot2 is c_life_cycle_state_handler_start_game, not an invented
// loading-screen timer. Its handler owns only these two bytes (+10/+11).
struct VistaSessionLoadingHandlerState {
  std::uint8_t flag10{}, flag11{};
  bool operator==(const VistaSessionLoadingHandlerState&) const = default;
};
enum class VistaSessionLoadingService {
  base_enter, seed_query, random_bytes, identity_write, seed_write,
  clear_parameter_byte, clear_prepared_banks, preflight, admitted, authority,
  prepare_players, network_query, refresh_parameters, clear_countdown,
  phase_write, request
};
struct VistaSessionLoadingCall {
  VistaSessionLoadingService service{};
  std::array<std::uint32_t,3> arguments{};
  unsigned argument_count{};
  bool operator==(const VistaSessionLoadingCall&) const = default;
};
struct VistaSessionLoadingUpdate {
  std::uint8_t repeat{}; // Literal original AL, including nonnormalized bytes.
  std::array<VistaSessionLoadingCall,16> calls{};
  unsigned call_count{};
  std::span<const VistaSessionLoadingCall> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
struct VistaSessionLoadingEnterValues {
  std::uint32_t seed{}; // Actual45906F result; no native RNG is selected here.
  std::array<std::uint32_t,2> identity{}; // Actual5B5A84 eight-byte output.
};
struct VistaSessionLoadingEnterQueries {
  std::uint32_t session_state{},phase{}; // Actual+73A0/+4C68 after base enter.
  std::optional<VistaSessionLoadingEnterValues> values;
};
// Literal5D98ED joined with5D9746. old_handler is a caller-owned opaque token
// for the actual old handler, never a guest pointer manufactured by this API.
// The original base-enter argument length/payload are always0/NULL. Parameter
// setters/bank clearing remain ordered intents owned by the real session.
VistaSessionLoadingUpdate vista_session_loading_enter(
    VistaSessionLoadingHandlerState&,std::uint32_t old_handler,
    const VistaSessionLoadingEnterQueries&);

struct VistaSessionLoadingGateQueries {
  std::uint32_t session_state{}; // Actual first+73A0 read, after preparation.
  std::uint8_t network_query{}; // Actual5E0D4F AL, not inferred bandwidth.
  std::uint8_t local_flag{}; // Actual+4C74, read after5E0D4F.
  //5E0D4F is an explicit service boundary; preserve the later source reads
  // independently rather than asserting that callbacks cannot change them.
  std::uint32_t session_state_after_query{};
  std::uint8_t transition_flag{}; // Actual+73A4 when later state equals7.
  std::uint8_t refresh_accepted{}; // Actual5C5BC5 AL if refresh is reached.
};
struct VistaSessionLoadingUpdateQueries {
  std::uint8_t preflight{},pending_after_preflight{},admitted{};
  std::uint32_t phase{}; // Actual+4C68 after preflight/admission.
  std::uint8_t authority{},players_prepared{}; //5AD85D /5D991C actual AL.
  // Required only after the original preparation service succeeds.
  std::optional<VistaSessionLoadingGateQueries> gate;
};
// Literal5D9A9F joined with5D97BB. Actual5D991C player preparation is a
// supplied service result; its membership/player/variant banks are not built
// by this API.5C5BC5 refresh,5C969F clear,5C596E phase write, and5D75DA request
// remain ordered intents. No world load/session/readiness is fabricated.
// Phase2 may return1; the next same-pump phase4 update requests literal kind3.
// Missing consumed gate data rejects atomically before handler mutation.
VistaSessionLoadingUpdate vista_session_loading_update(
    VistaSessionLoadingHandlerState&,const VistaSessionLoadingUpdateQueries&);
} // namespace halo2
