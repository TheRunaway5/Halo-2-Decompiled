#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Source: supplied Vista halo2.exe SHA256
// 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// These are bounded raw code-unit banks consumed by 68B052(count=0x104).
// No locale folding, path canonicalization, or captured ready value is used.
using VistaSessionMapPath = std::array<std::uint16_t,260>;
struct VistaSessionSelectedMap {
  std::uint32_t type{};  // Campaign ID: session +4CB8 / loaded options +80, not cache/world kind.
  std::int32_t id{};     // Session +4DD8 / loaded options +84.
  VistaSessionMapPath path{}; // Session +4CBC / loaded options +88.
};
struct VistaSessionMapReadinessSession {
  std::uint32_t state{}; // Actual session +73A0.
  VistaSessionSelectedMap selected;
};
struct VistaSessionMapReadinessState {
  std::int32_t status{};          // 91A620.
  std::int32_t progress{};        // 91A624.
  std::uint8_t map_query_flag{};  // 91A588; preserve on unconsumed branches.
  std::int32_t selected_id{};     // 822424; preserve on unconsumed branches.
  bool operator==(const VistaSessionMapReadinessState&) const = default;
};
struct VistaSessionMapReadinessQueries {
  std::uint32_t lifecycle_slot{}; // Actual 5AD660 result.
  // Actual 5AD736 result/field view. Missing means no current session.
  std::optional<VistaSessionMapReadinessSession> session;
  bool loaded_world_valid{}; // Actual 4497EA AL truth value.
  // Required only if the actual loaded-options query is reached.
  std::optional<VistaSessionSelectedMap> loaded_options; // 449801 view.
  std::int32_t map_query{}; // Actual 44B633 result; no file/loader invented.
  // Actual 4317D2 result AFTER the original caller's FSTP binary32 spill.
  std::uint32_t map_progress_bits{};
  // Actual caller SSE environment for MULSS/CVTTSS2SI. Normal thread setup
  // is not asserted; masked exceptions and supported reserved bits required.
  std::uint32_t mxcsr{};
};
enum class VistaSessionMapReadinessService {
  lifecycle, current_session, session_admission, selection_kind, selected_map,
  loaded_world, loaded_options, path_compare, map_query, path_conversion,
  map_progress, map_request
};
struct VistaSessionMapReadinessCall {
  VistaSessionMapReadinessService service{};
  // Pointer arguments remain owner-held paths, not synthetic native addresses.
  // This stores consumed scalar arguments: compare count, request flag,
  // progress mode, and conversion codepage/flags/count/capacity as applicable.
  std::array<std::uint32_t,4> arguments{};
  unsigned argument_count{};
  bool operator==(const VistaSessionMapReadinessCall&) const = default;
};
struct VistaSessionMapReadinessResult {
  std::array<VistaSessionMapReadinessCall,12> calls{};
  unsigned call_count{};
  std::span<const VistaSessionMapReadinessCall> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
bool vista_session_map_paths_equal(const VistaSessionMapPath&,
    const VistaSessionMapPath&) noexcept;
// Literal normal multiplayer branch of 5B2920, including getter admission,
// query order, raw status/flag, and exact integer progress conversion.
// Alternate/campaign selection, missing consumed loaded metadata, unsupported
// FP contexts reject atomically. No file query, loader, or live session starts.
VistaSessionMapReadinessResult vista_session_map_readiness(
    VistaSessionMapReadinessState&,const VistaSessionMapReadinessQueries&);

using VistaSessionPeerMapPayload = std::array<std::uint8_t,0xC8>;
// Source 5B2D62..5B2D69 writes payload +60/+64. All other bytes come from
// the actual payload owner and remain unchanged; this is not full 5B2C51.
void vista_session_map_readiness_payload(VistaSessionPeerMapPayload&,
    const VistaSessionMapReadinessState&) noexcept;
struct VistaSessionPeerMapRecord {
  std::array<std::uint8_t,4> presence{}; // Peer +24, including retained padding.
  VistaSessionPeerMapPayload payload{}; // Peer +28.
  bool operator==(const VistaSessionPeerMapRecord&) const = default;
};
// Bounded local 5C83F3 path (states 5..8), selecting actual host +71C0.
// Copies all C8 bytes, writes only presence[0]=1, increments +70/+7930 with
// wrap, and returns original AL true. Remote delivery/invalid host rejects
// atomically. No peer, unused payload field, or readiness status is fabricated.
bool vista_session_publish_local_map_readiness(std::uint32_t session_state,
    std::uint32_t host_peer,std::span<VistaSessionPeerMapRecord>,
    std::uint32_t& membership_revision,std::uint32_t& change_revision,
    const VistaSessionPeerMapPayload&);
} // namespace halo2
