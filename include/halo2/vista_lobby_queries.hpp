#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include "halo2/vista_loading.hpp"

namespace halo2 {
// Supplied halo2.exe SHA33119398...109. The source selected-map service
// receives three output pointers. Only its third output is consumed here;
// it is initialized to0 and the service's AL return is ignored by61678C.
struct VistaLobbyStatusStringServices {
  std::function<std::uint8_t(std::uint32_t&)> selected_map_third_output;
};
std::uint32_t vista_lobby_status_string(std::int32_t readiness,
    const VistaLobbyStatusStringServices&);

using VistaLobbyQuerySession = std::optional<std::span<const std::byte>>;
// Literal5AD736 admission. Disabled returns before examining the bank.
VistaLobbyQuerySession vista_lobby_query_current_session(
    std::span<const std::byte> source_bank,std::uint8_t lifecycle_enabled);
enum class VistaLobbyQueryCall { membership_view,membership_generation,maximum_consumer };
struct VistaLobbyQueryServices {
  // One actual5AD736 query per wrapper. Absence represents its false return.
  std::function<VistaLobbyQuerySession()> current_session;
  // Optional trace of unchanged internal call order. The argument is the
  //5C20C8 output-pointer mask, generation, or5E275C initial peer respectively.
  std::function<void(VistaLobbyQueryCall,std::uint32_t)> observed;
};
// Complete61525A ->5B1BDE ->5B1A37, for the single non-null count output.
// Reads membership+11E4; it does not recompute/popcount or clamp that count.
std::int32_t vista_lobby_player_count(const VistaLobbyQueryServices&);

// Actual retained globals used by5E275C, not session max-player parameters.
// No initializer/default values are inferred by this interface.
struct VistaLobbyMaximumParameters {
  std::array<std::int32_t,17> thresholds{}; //8F8158 +4*index; index0 unused.
  std::int32_t minimum_capacity{}; //8F81E8.
  std::int32_t minimum_field_ac{}; //8F81EC.
  std::int32_t minimum_field_a8{}; //8F81F0.
  std::int32_t field_ac_margin{}; //8F81F4, signed comparison after32-bit addition.
};
// The supplied span begins at actual global8F8158 and includes through8F81F7.
VistaLobbyMaximumParameters vista_lobby_maximum_parameters(std::span<const std::byte>);
// Consumed5AB400 initializer prefix: literal SSE then x87 FISTP order,
// both18-entry retained tables and the four maximum parameters. Other
// initializer globals are outside this helper; no live owner birth is implied.
std::array<std::byte,0xa0> vista_lobby_maximum_parameters_initialize(
    const VistaLoadingFloatingPoint&);
// Complete615214 ->5B0828 ->5E275C with only the final capacity output.
// Unlike player_count, maximum has no3..8 state table after5AD736 admission.
std::int32_t vista_lobby_maximum(const VistaLobbyQueryServices&,
    const VistaLobbyMaximumParameters&);
} // namespace halo2
