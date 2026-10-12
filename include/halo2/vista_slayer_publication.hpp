#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace halo2 {
inline constexpr std::string_view vista_slayer_publication_source_sha256 =
    "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109";
inline constexpr std::uint32_t vista_slayer_publication_source_begin = 0x510c2b;
inline constexpr std::uint32_t vista_slayer_publication_source_end = 0x510c5f;
inline constexpr std::string_view vista_slayer_publication_source_code_sha256 =
    "90a2d4d7a375b306558b045a0fe6273d7334587a7a8d4aa48957faf94fcf9196";
inline constexpr std::size_t vista_slayer_activation_storage_size = 0x24;
inline constexpr std::size_t vista_slayer_activation_defined_bytes = 0x22;

// Original510C2B calls45B05D with type2/subtype0, replaces recipient+8,
// then requests45DC77 publication. This is a source-qualified Slayer engine
// callback, not a timer or a sound request. The actual4498B7 result is supplied
// as its consumed world+0C byte: exactly4 suppresses the whole callback.
// Caller-owned bytes+22/+23 remain opaque; only the first34 decimal bytes are
// defined. A suppressed callback leaves every supplied storage byte untouched.
// Returns publication intent only. No event delivery, audience/duplicate
// selection, voice request or live engine/session owner is implemented here.
bool vista_slayer_activation_publication(std::uint8_t world_phase_byte,
    std::uint32_t recipient,
    std::span<std::byte,vista_slayer_activation_storage_size> caller_storage) noexcept;

// Original45339B checks the phase and player+2 bit1 before querying5AE0CB.
// The actual query implementation and player-pool lifetime remain caller-owned.
bool vista_player_activation_query_admitted(std::uint8_t world_phase_byte,
                                           std::uint16_t flags_before_query) noexcept;

enum class VistaPlayerActivationService { engine_activation, player_notification };
struct VistaPlayerActivationUpdate {
  std::uint16_t flags{};
  std::array<VistaPlayerActivationService,2> calls{};
  unsigned call_count{};
  std::span<const VistaPlayerActivationService> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
// Bounded4533E2..45341A/453434..45343F continuation, called only after an
// admitted query. Pass the flags actually reread after that query, not a cached
// pre-query value. AL is compared literally against bit0; it is not normalized
// to bool. Flags are written before the listed call intents. A nonzero changed
// byte requests46A29E first; either changed case requests5B6662(recipient,0x200).
// The engine wrapper's two queries, virtual dispatch and notification/delivery
// side effects remain external. No reentrant mutation is fabricated here.
VistaPlayerActivationUpdate vista_player_activation_after_query(
    std::uint16_t flags_after_query,std::uint8_t query_al) noexcept;
} // namespace halo2
