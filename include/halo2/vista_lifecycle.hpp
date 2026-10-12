#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Supplied PC halo2.exe, SHA256
// 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// These are projections of existing owner fields, not session constructors.
struct VistaLifecycleManager {
  std::uint32_t current_kind{}; // +0
  std::uint8_t transitioning{},updating{},pending{}; // +34/+35/+36
  std::uint32_t requested_kind{}; // +38
  std::int32_t payload_length{}; // +3C
  // Caller-owned stable storage corresponding to +40. The original capacity
  // is not guessed; each admitted request must fit the supplied storage.
  std::span<std::byte> payload;
};
// Literal5D75DA. Always clears the first payload word, including nonpositive
// lengths; positive lengths then copy exactly the supplied bytes.
void vista_lifecycle_request(VistaLifecycleManager&,std::uint32_t kind,
    std::int32_t length,std::span<const std::byte> payload);

struct VistaLifecycleHandler {
  // Return the actual AL byte. Any nonzero byte repeats the original pump.
  std::function<std::uint8_t(VistaLifecycleManager&)> update;
  std::function<void(VistaLifecycleManager&,std::uint32_t old_kind,
      std::int32_t length,std::span<const std::byte> payload)> enter;
  std::function<void(VistaLifecycleManager&,std::uint32_t new_kind)> exit;
};
// Literal5D7738. The callback is actual5EA803, before request consumption.
// Exit/enter/update can queue another request. The original retains a payload
// pointer, not a byte snapshot: exit mutations remain visible to enter. Caller
// must keep payload storage and the handler table stable throughout the pump.
// There is no invented frame gap or iteration limit. Invalid callback/record
// boundaries reject with FormatError; callback failures are not guest faults.
void vista_lifecycle_pump(VistaLifecycleManager&,
    std::span<const VistaLifecycleHandler>,
    const std::function<void(VistaLifecycleManager&)>& network_update);

enum class VistaLifecycleService {
  preflight,admitted,closing,authority,game_over_complete,phase_write,
  remote_closing,force_end_query,force_end,world_ready,world_identity,
  selected_map,world_map,map_change,request,postgame_external,base_enter
};
struct VistaLifecycleCall {
  VistaLifecycleService service{};
  std::array<std::uint32_t,3> arguments{};
  unsigned argument_count{};
  bool operator==(const VistaLifecycleCall&) const = default;
};
struct VistaLifecycleUpdate {
  std::uint8_t repeat{};
  std::array<VistaLifecycleCall,16> calls{};
  unsigned call_count{};
  std::span<const VistaLifecycleCall> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
struct VistaLifecycleInGameQueries {
  std::uint8_t preflight{},pending_after_preflight{},admitted{},closing{},authority{};
  std::uint32_t phase{};
  std::uint8_t game_over_complete{},force_end{},world_ready{};
  std::array<std::uint32_t,2> retained_identity{},world_identity{};
  std::uint8_t selected_map_valid{};
  std::array<std::uint32_t,2> selected_map{},world_map{};
};
// Literal5D95A7, with external services represented by ordered intents. These
// inputs are their actual observed query results in call order. In particular
// world_ready/identity/map queries occur after a possible phase-write service;
// callers must not replace reentrant service results with stale preflight data.
// This leaf does not itself apply phase writes, requests, or a world transition.
VistaLifecycleUpdate vista_lifecycle_in_game_update(const VistaLifecycleInGameQueries&);
struct VistaLifecyclePostGameQueries {
  std::uint8_t preflight{},pending_after_preflight{};
  std::uint32_t phase{};
};
// Literal5D903F. A requested kind1 returns1, unlike the in-game request branch.
VistaLifecycleUpdate vista_lifecycle_post_game_update(const VistaLifecyclePostGameQueries&);
// Literal5D9070 and this image's404541 (xor al,al): service5B07A0(5), then
// base enter with old handler and zero length/null payload. No UI/map-load
// meaning is assigned to5B07A0's external service.
VistaLifecycleUpdate vista_lifecycle_post_game_enter(std::uint32_t old_kind);
} // namespace halo2
