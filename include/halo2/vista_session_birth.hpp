#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Supplied Vista PE SHA256
// 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// This is the existing source owner's field bank, not a fabricated session.
// The normal Network slice is 5AD8FC -> 5AD7D0(1,0) -> 5CAF2E, with no
// optional player array. Source pointers/unknown bytes remain caller-owned.
inline constexpr std::size_t vista_session_source_size=0x7b88;
struct VistaSessionBirthValues {
  std::array<std::byte,36> local_machine; // Actual 5B5836 output.
  std::array<std::byte,8> session_key_id; // Actual 5B56B9 XNKID first output.
  std::array<std::byte,16> session_key;   // Actual 5B56B9 XNKEY second output.
  std::array<std::uint32_t,3> network_clocks; // Actual ordered 5B3CA0 results.
  std::uint32_t language;                // Actual 4381FD result.
  std::array<std::byte,8> random;         // Actual 5B5A84 output.
  std::uint32_t process_clock;           // Actual 437E5E result.
};
struct VistaSessionBirthQueries {
  bool local_machine_available{}; // Actual 5B5836 AL.
  bool network_created{};         // Actual 5B63C7 AL.
  bool session_key_available{};      // Actual 5B56B9 AL.
  // Required only after all three source queries succeed. No session key ID,
  // session key, clock, language, or random value is constructed by this API.
  std::optional<VistaSessionBirthValues> values;
};
enum class VistaSessionBirthService {
  local_machine,network_create,network_key,network_register,
  network_qos_listen,network_clock,language,random_bytes,
  process_clock,publish_peer_payload
};
struct VistaSessionBirthCall {
  VistaSessionBirthService service{};
  // Actual source owner word only for the network_register method. Native
  // bindings resolve their own owner/handles; this is not a PC pointer cast.
  std::uint32_t owner{};
  // Consumed scalar arguments / pointer-presence flags only. Actual local
  // machine/session-key/output banks remain caller-owned; no guest stack or
  // native pointer is manufactured. network_create consumes only byte arg3.
  std::array<std::uint32_t,3> arguments{};
  unsigned argument_count{};
  bool operator==(const VistaSessionBirthCall&) const = default;
};
struct VistaSessionBirthResult {
  bool accepted{}; // Original wrapper AL; does not imply a world is ready.
  std::array<VistaSessionBirthCall,12> calls{};
  unsigned call_count{};
  std::span<const VistaSessionBirthCall> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
// Writes every exact constructor field/clear in the admitted normal slice,
// preserving all other bytes. Already nonzero +73A0 returns AL false untouched.
// Requires source network slot +5C==0 and quiescent transport byte +48==0 if
// the constructor is reached. Unsupported/truncated/missing inputs reject
// atomically; supported failed queries retain the source's name-clear write.
// The final actual 5B333D payload publication is an intent, not a guessed
// payload or ready status. External/reentrant mutations and remote birth are
// outside this pure supplied-result boundary. No live runtime is activated.
VistaSessionBirthResult vista_session_local_birth(std::span<std::byte>,
    const VistaSessionBirthQueries&);

// Literal 5D7591, not the complete manager/network initialization. Five actual
// owner words are copied in source argument order. Handler slots/current kind
// and bytes +34..+36 are cleared; requested kind/length/payload/padding retain
// their original bytes. Actual 5AD494's subsequent registrations are owned by
// the caller and must bind its real seven handlers, not invented callbacks.
void vista_session_lifecycle_initialize(std::span<std::byte>,
    const std::array<std::uint32_t,5>& actual_owner_words);
} // namespace halo2
