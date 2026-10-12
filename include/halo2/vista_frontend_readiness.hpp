#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Supplied Vista halo2.exe, SHA256
// 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// This is the complete 616490 decision producer. Its service results come
// from actual owners; it never accepts a desired UI readiness state.
struct VistaFrontendVariantView {
  std::int32_t engine_type{}; // Actual variant +44.
  std::uint8_t flags{};       // Actual variant +48, consumed bit0 only.
};
struct VistaFrontendMapView {
  std::int32_t map_id{};       // 61564E's second output.
  std::uint32_t custom_map_id{}; // 61564E's third output.
};
// Actual ten-byte capacity bank: built-in metadata +C54, custom output +C48.
// Engine types1..9 index the bank; its byte0 is not consumed by616490.
using VistaFrontendMapCapacities = std::array<std::uint8_t,10>;
struct VistaFrontendPlayerQueries {
  std::uint8_t present{};  // Actual 6156E1 AL.
  std::uint8_t eligible{}; // Actual 615820 AL; consumed only when present.
  // Actual 6157CA pointer and its signed byte +7C. Missing is a null pointer.
  std::optional<std::int8_t> team;
};
struct VistaFrontendReadinessQueries {
  std::uint8_t lifecycle_active{}; // Actual 5B5038 AL, used by 6154D8.
  std::uint32_t lifecycle_slot{};  // Actual 5AD660, only when active.
  std::uint8_t current_session{}; // Actual 5AD736 AL.
  std::optional<std::uint32_t> session_state; // Actual session +73A0.
  std::int32_t mode{}; // Actual 615EA9 result, not an inferred UI mode.
  std::uint32_t map_status{}; // Actual 5B1929 EAX.
  std::int32_t map_progress{}; // Actual 5B1929 first output.
  std::int32_t local_map_status{}; // Actual 5B1913 EAX.
  std::array<VistaFrontendPlayerQueries,16> players{};
  std::optional<VistaFrontendVariantView> variant; // Actual 5B1313 pointer.
  std::optional<VistaFrontendMapView> map;
  std::optional<VistaFrontendMapCapacities> builtin_metadata; // 459A1A pointer.
  std::uint8_t custom_metadata_loaded{}; // Actual 459369 AL.
  std::optional<VistaFrontendMapCapacities> custom_metadata;
  std::int32_t countdown{}; // Actual 5B13C7, with signed negative admission.
  std::uint8_t host_permission{}; // Actual 5AD940 AL.
  std::uint32_t mxcsr{}; // Actual caller SSE context, masked exceptions required.
};
enum class VistaFrontendReadinessService {
  lifecycle_context, lifecycle_active, lifecycle_slot, current_session,
  session_admission, mode, map_readiness, local_map_readiness, player_present,
  player_eligible, variant, map_selection, builtin_map_metadata,
  custom_metadata_clear, custom_map_metadata, player_configuration,
  countdown, host_permission
};
struct VistaFrontendReadinessCall {
  VistaFrontendReadinessService service{};
  std::uint32_t argument{};
  bool operator==(const VistaFrontendReadinessCall&) const = default;
};
struct VistaFrontendReadinessResult {
  std::int32_t state{};
  // The nullable original float output is initially zero, and is replaced
  // only on the original status2/3/4 branches. Exact binary32 bits retained.
  std::optional<std::uint32_t> progress_bits;
  std::array<VistaFrontendReadinessCall,64> calls{};
  unsigned call_count{};
  std::span<const VistaFrontendReadinessCall> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
VistaFrontendReadinessResult vista_frontend_readiness(
    const VistaFrontendReadinessQueries&,bool progress_output);

// Literal 6156E1 / 615820 / 6157CA consumed-field helpers. The caller owns
// the actual successful 5B1BDE query, mask, local peer and record banks.
// Missing consumed records reject instead of inventing empty membership.
bool vista_frontend_player_present(std::uint8_t query_al,std::uint32_t mask,
    std::uint32_t datum) noexcept;
bool vista_frontend_player_eligible(std::uint8_t query_al,std::uint32_t mask,
    std::uint32_t datum,std::int32_t local_peer,
    std::span<const std::int32_t> player_peer_indices);
std::optional<std::int8_t> vista_frontend_player_team(std::uint8_t query_al,
    std::uint32_t mask,std::uint32_t datum,
    std::span<const std::int8_t> configuration_teams);
} // namespace halo2
